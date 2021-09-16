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
#include "msgqueue.h"
#include "print.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;
using namespace httplib;

#define VERSION   "V1.00r0"

/* -- Types --------------------------------------------------------------- */


/* -- Module Global Function Prototypes ----------------------------------- */
static void m_on_get(const Request &req, Response &res);
static void m_on_post_labels(const Request &req, Response &res);
static void m_on_options_labels(const Request &req, Response &res);
static void m_print_thread();
static void m_print_labels(cJSON * labels);

/* -- (Module) Global Variables ------------------------------------------- */
static MessageQueue<cJSON *, 32> m_LabelQueue;
extern const unsigned char __index_html[];
extern const unsigned int __index_html_len;


/* -- Implementation ------------------------------------------------------ */

int main(int argc, char * argv[])
{
   thread printThread(&m_print_thread);
   uint16_t port = 8092;
   Server svr;

   //get port number from command  line argument
   if (argc >= 2)
   {
      port = atoi(argv[1]);
      if (port < 1024)
      {
         port = 8092; //back to default port
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
         "ip":"127.0.0.1",
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
   static uint32_t session = 1;
   cJSON * label;
   void * prt;

   //start label printing
   //use the IP given in the first label to connect to the LabelWriter
   prt = nullptr;
   cJSON_ArrayForEach(label, labels)
   {
      prt = print_json_start(label, session);
      break;
   }

   //start printing the labels (all labels will be printed by the LabelWriter given by the IP of the first label)
   if (prt != nullptr) //connected to LabelWriter?
   {
      session++;
      cJSON_ArrayForEach(label, labels)
      {
         //print requested label
         int status = print_json_do(label, prt);
         if (label->next != nullptr)
         {
            this_thread::sleep_for(chrono::microseconds(500)); //delay between the single labels
         }
      }

      //finalize printing process (make form-feed and close TCP connection to LabelWriter)
      print_json_end(prt);
   }
}

