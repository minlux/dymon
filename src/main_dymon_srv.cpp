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


/* -- Defines ------------------------------------------------------------- */
using namespace std;
using namespace httplib;

#define VERSION   "V2.00r0"

/* -- Types --------------------------------------------------------------- */


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


static void usage(void)
{
   cout << "Usage:\n";
   cout << " dymon_srv [<usb>] [-p <port>]\n\n";

   cout << "Examples:\n";
   cout << " dymon_srv\n";
   cout << " dymon_srv -p 9000\n";
   cout << " dymon_srv usb:/dev/usb/lp0 <bitmap-file>\n";
   cout << " dymon_srv usb:vid_0922 <bitmap-file> -p 8093\n";
   cout << endl;
}



int main(int argc, char * argv[])
{
   thread printThread(&m_print_thread);
   Server svr;


   //evaluate command line arguments
   //get interface and path
   if ((argc >= 2) && (strncmp(argv[1], "usb:", 4) == 0))
   {
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
      path = &argv[1][4]; //get substring -> device-path expected (something like "/dev/usb/lp0")
   #endif
      printJson = new PrintJson(new DymonUsb, path);
   }
   else //net
   {
      printJson = new PrintJson(new DymonNet, nullptr);
   }

   //-p <tcp-port>
   uint16_t port = 8092;
   for (int i = 1; i < (argc - 1); ++i)
   {
      if ((argv[i][0] == '-') && (argv[i][1] == 'p'))
      {
         uint16_t p = (uint16_t)atoi(argv[i + 1]);
         if (p != 0)
         {
            port = p;
         }
         break;
      }
   }


   //greetings
   cout << "Printserver for DYMO LabelWriter Wireless, by minlux. " VERSION "\n" << endl;
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
         "format":1,
         "title":"todo"
         "body":[
            "first line text",
            "second line",
            "3rd",
            "4th line of text"
         ],
         "barcode":1234567
      },
      ...
   ]

   Example, sending data with CURL:
   ----
   curl -d '[{"ip":"192.168.178.49", "format":1,
     "title":"Manuel Heiß",
     "body":["www.minlux.de", "https://github.com/minlux", "https://www.youtube.com/channel/UC_P8QKvglG382JnKue-dSkw"],
     "barcode":1234567 }]'
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


static void m_print_labels(cJSON * labels)
{
   cJSON * label;
   int err = -1;

   //start label printing
   //use the IP given in the first label to connect to the LabelWriter
   cJSON_ArrayForEach(label, labels)
   {
      err = printJson->start(label);
      break;
   }

   //start printing the labels (all labels will be printed by the LabelWriter given by the IP of the first label)
   if (err == 0)
   {
      cJSON_ArrayForEach(label, labels)
      {
         //print requested label
         err = printJson->print(label);
         if (err != 0) break;
         if (label->next != nullptr)
         {
            this_thread::sleep_for(chrono::microseconds(500)); //delay between the single labels
         }
      }
   }

   //finalize printing process (make form-feed and close TCP connection to LabelWriter)
   printJson->end();
}

