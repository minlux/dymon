//-----------------------------------------------------------------------------
/*!
   \file
   \brief A HTTP webserver that serves as print server to print labes on a DYMO LabelWriter wireless.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#ifdef _WIN32
   //see https://www.heise.de/ct/hotline/IPv6-Programme-mit-MinGW-1100748.html
   #define _WIN32_WINNT 0x501
   #define IPV6_V6ONLY 27
#endif


#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <thread>
#include "httplib.h"
#include "msg_queue.h"
#include "print_json.h"
#include "usbprint.h"
#include "argtable3.h"
#include "vt100.h"


/* -- Defines ------------------------------------------------------------- */
#define APP_NAME           "dymon_srv"
#define APP_VERSION        "2.1.0"

/* -- Types --------------------------------------------------------------- */
using namespace std;
using namespace httplib;


/* -- Module Global Function Prototypes ----------------------------------- */
static void m_on_get(const Request &req, Response &res);
static void m_on_post_labels(const Request &req, Response &res);
static void m_on_options_labels(const Request &req, Response &res);
static void m_print_thread();
static void m_print_labels(cJSON * labels);

/* -- (Module) Global Variables ------------------------------------------- */
extern const unsigned char __index_html[];
extern const unsigned int __index_html_len;
static MessageQueue<cJSON *, 32> m_LabelQueue;
static PrintJson * printJson;


/* -- Implementation ------------------------------------------------------ */


static void print_usage(void ** argtable)
{
   puts(VT100_BOLD_UNDERLINED "Usage:" VT100_RESET " " VT100_BOLD APP_NAME VT100_RESET);
   arg_print_syntax(stdout, argtable, "\n\n");
}

static void print_help(void ** argtable)
{
   //Description
   puts("Printserver for DYMO LabelWriter.\n");
   //Usage
   print_usage(argtable);
   //Options
   puts(VT100_BOLD_UNDERLINED "Options:" VT100_RESET);
   arg_print_glossary(stdout, argtable,"  %-25s %s\n");
}


static void usage(void)
{
   cout << "Usage:\n";
   cout << " dymon_srv [<usb>|<usb450>] [-p <port>]\n\n";

   cout << "Examples:\n";
   cout << " dymon_srv\n";
   cout << " dymon_srv -p 9000\n";
   cout << " dymon_srv usb:/dev/usb/lp0\n";
   cout << " dymon_srv usb450:/dev/usb/lp1\n";
   cout << " dymon_srv usb:vid_0922 -p 8093\n";
   cout << endl;
}



int main(int argc, char * argv[])
{
   struct arg_lit * argHelp;
   struct arg_lit * argVersion;
   struct arg_str * argUsb;
   struct arg_str * argNet;
   struct arg_int * argModel;
   struct arg_int * argPort;
   struct arg_end * argEnd;
   void * argtable[] =
   {
      argHelp = arg_lit0(NULL, "help", "Print help and exit"),
      argVersion = arg_lit0(NULL, "version", "Print version and exit"),
   #ifdef _WIN32
      argUsb = arg_str0(NULL, "usb", "<ID>", "Use USB printer with vid/pid ID (e.g. 'vid_0922')"),
   #else
      argUsb = arg_str0(NULL, "usb", "<DEVICE>", "Use USB printer device (e.g. '/dev/usb/lp1')"),
   #endif
      argNet = arg_str0(NULL, "net", "<IP>", "Force use of network printer with IP (e.g. '192.168.178.23')"),
      argModel = arg_int0(NULL, "model", "<NUMBER>", "Model number of DYMO LabelWriter (e.g. '450')"),
      argPort = arg_int0("p", "port", "<NUMBER>", "TCP port number of server [default: 8092]"),
      argEnd = arg_end(3),
   };

   // Parse command line arguments.
   arg_parse(argc, argv, argtable);

   // Help
   if (argHelp->count)
   {
      print_help(argtable);
      return 0;
   }

   // Version
   if (argVersion->count)
   {
      puts(APP_VERSION);
      return 0;
   }

   // Ensure that there is at most one interface specified
   if (argUsb->count && argNet->count)
   {
      puts(VT100_RED "Error:" VT100_RESET " Only one interface allowed (one of '--usb' or '--net')\n");
      print_usage(argtable);
      return -1;
   }


   thread printThread(&m_print_thread);
   Server svr;


   //get interfaze and path
   if (argUsb->count)
   {
      const char * const dev = argUsb->sval[0];
      const int model = (argModel->count ? argModel->ival[0] : 0);
      const bool lw450 = (model == 450);
      const char * path;
   #ifdef _WIN32
      //get the device name, to be used on windows
      //
      static char devname[256];
      int err = usbprint_get_devicename(devname, sizeof(devname), &argv[1][4]); //input something like "vid_0922" and get device name like "\\?\usb#vid_0922&pid_0028#04133046018600#{28d78fad-5a12-11d1-ae5b-0000f803a8c2}"
      if (err != 0)
      {
         cout << "Can't find any USB connected DYMO" << endl;
         return -2;
      }
      path = devname;
   #else
      path = dev;
   #endif
      printJson = new PrintJson(new DymonUsb(1, lw450), path);
   }
   else //net
   {
      const char * const ip = argNet->sval[0];
      cJSON * json = cJSON_CreateObject();
      cJSON_AddItemToObject(json, "ip", cJSON_CreateString(ip)); //wrap the ip address into a json object
      printJson = new PrintJson(new DymonNet, (char *)json); //pass this to DymonNet::start, which expects a json object
   }


   //greetings
   const uint16_t port = (uint16_t)(argPort->count ? argPort->ival[0] : 8092);
   cout << "Printserver for DYMO LabelWriter Wireless, by minlux. V" APP_VERSION "\n" << endl;
   cout << "\nStart listening to port " << port << endl;
   cout << "Press [Ctrl + C] to quit!" << endl;

   svr.Get("/", &m_on_get);
   svr.Post("/labelprinter/labels", &m_on_post_labels);
   svr.Options("/labelprinter/labels", &m_on_options_labels);
   svr.listen("0.0.0.0", port);
   return 0;
}


//serve index.html
static void m_on_get(const Request & /*req*/, Response &res)
{
   res.set_header("Access-Control-Allow-Origin", "*"); //CORS
   res.set_content((const char *)__index_html, __index_html_len, "text/html");
}


//typically before a POST request, a browser first sends the OPTION request...
//this resposne is needed to allow cross origin requests.
//it is needed to let the browser continue with the actual POST request.
static void m_on_options_labels(const Request &req, Response &res)
{
   //CORS
   res.set_header("Access-Control-Allow-Origin", "*");
   res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Origin, Authorization");
   res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
   res.set_header("Keep-Alive", "timeout=5, max=100");
   res.set_header("Connection", "Keep-Alive");
   res.set_header("Content-Type", "application/json");
   res.status = 200; //HTTP status code
}


/*
   POST data like this expexted (Array of JSON objects):
   [
      {
         "ip":"127.0.0.1", //don't care for USB printers
         "width":272,
         "height":252,
         "orientation":0,
         "text":"\\24cTitle\n\\3_\nHello\n\\rWorld\n\n\\100#214",
         "count":1
      },
      ...
   ]

   Example, sending data with CURL:
   ----
   curl -d '{"ip":"192.168.178.49", "width":272, "height":252, "orientation":0,
     "text":"Manuel Heiß\nminlux.de\n\\_\nhttps://github.com/minlux", "count":1 }'
     -H "Content-Type: application/json"
     -X POST http://localhost:8092/labelprinter/labels
*/
static void m_on_post_labels(const Request &req, Response &res)
{
   //request body is expected to contain json array of objects
   const char * requestBody = req.body.c_str();
   cJSON * const labels = cJSON_Parse(requestBody);
   if (labels != NULL)
   {
      //push the "print labels request" into the queue - a "m_print_thread" will pop the label out of the queue and process it!
      m_LabelQueue.push_nowait(labels); //the actual printing of the label is done in function m_print_labels!!!
   }

   //set response
   res.set_header("Access-Control-Allow-Origin", "*");
   res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Origin, Authorization");
   res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
   res.set_content("{\"status\":\"OK\"}", "application/json");
   res.status = 200; //HTTP status code
}




static void m_print_thread()
{
   cJSON * labels;

   while (true)
   {
      //get next print request out of the queue. wait until one become available
      m_LabelQueue.pop(labels);
      //print the labels
      m_print_labels(labels);
      //cleanup JSON
      cJSON_Delete(labels);
   }
}


static void m_print_labels(cJSON * label)
{
   //start label printing
   //if set, use the IP given in the label to connect to the LabelWriter, otherwise the one given with '--net' will be used
   cJSON * ip = cJSON_GetObjectItemCaseSensitive((cJSON *)label, "ip"); //get IP
   const char * value = cJSON_GetStringValue(ip);
   int err = printJson->start((value && (strlen(value) > 0)) ? ip : nullptr);
   if (err == 0)
   {
      //print requested labels
      printJson->print(label);
   }
   //finalize printing process (make form-feed and close TCP connection to LabelWriter)
   printJson->end();
}

