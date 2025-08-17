//-----------------------------------------------------------------------------
/*!
   \file
   \brief A HTTP webserver that serves as print server to print labes on a DYMO LabelWriter wireless.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <thread>
#include "httplib.h"
#include "base64.h"
#include "msg_queue.h"
#include "text_label.h"
#include "usbprint.h"
#include "argtable3.h"
#include "vt100.h"

/* -- Defines ------------------------------------------------------------- */
#define APP_NAME "dymon_srv"
#define APP_VERSION "2.2.0"

/* -- Types --------------------------------------------------------------- */
using namespace std;
using namespace httplib;

class DymonPrinter
{
public:
   DymonPrinter(Dymon *dymon, const char *device) : _dymon(dymon), device(device) {}
   // inline Dymon &dymon() { return *_dymon; }

   inline int start(const char *ipAddress) { return _dymon->start(device ? (void *)device : (void *)ipAddress); }

   inline int print(Dymon::Bitmap *bitmap, uint32_t copies = 1, bool more = false)
   {
      for (int i = 0; i < copies; ++i)
      {
         int err = _dymon->print(bitmap, 0, ((i + 1) < copies) || more);
         if (err != 0) return err;
      }
      return 0;
   }

   inline void end(void) { _dymon->end(); }

private:
   Dymon *_dymon;
   const char *device;
};


struct Pbm {
   Pbm() { copies = 0; };
   Pbm(std::string ip, uint32_t copies, const uint8_t * data, uint32_t length) 
      : ip(ip), copies(copies), data(std::vector<uint8_t>(data, data + length)) {}
   std::string ip;
   uint32_t copies;
   std::vector<uint8_t> data;
}; 

/* -- Module Global Function Prototypes ----------------------------------- */
static void m_on_options(const Request &req, Response &res);
static void m_on_get_wpm(const Request &req, Response &res);
static void m_on_get_labels(const Request &req, Response &res);
static void m_on_post_labels(const Request &req, Response &res);
static void m_on_post_pbm(const Request &req, Response &res);
static void m_print_labels_thread();
static void m_print_labels(cJSON *labels);
static void m_print_pbms_thread();

/* -- (Module) Global Variables ------------------------------------------- */
extern const unsigned char __wpm_receiver_html[];
extern const unsigned int __wpm_receiver_html_len;
extern const unsigned char __labelwriter_index_html[];
extern const unsigned int __labelwriter_index_html_len;
static MessageQueue<cJSON *, 64> m_LabelQueue;
static MessageQueue<Pbm, 64> m_PbmQueue;
static DymonPrinter *m_Printer;

/* -- Implementation ------------------------------------------------------ */

static void print_usage(void **argtable)
{
   puts(VT100_BOLD_UNDERLINED "Usage:" VT100_RESET " " VT100_BOLD APP_NAME VT100_RESET);
   arg_print_syntax(stdout, argtable, "\n\n");
}

static void print_help(void **argtable)
{
   // Description
   puts("Printserver for DYMO LabelWriter.\n");
   // Usage
   print_usage(argtable);
   // Options
   puts(VT100_BOLD_UNDERLINED "Options:" VT100_RESET);
   arg_print_glossary(stdout, argtable, "  %-25s %s\n");
}

int main(int argc, char *argv[])
{
   struct arg_lit *argHelp;
   struct arg_lit *argVersion;
   struct arg_str *argUsb;
   struct arg_str *argNet;
   struct arg_int *argModel;
   struct arg_int *argPort;
   struct arg_str *argServe;
   struct arg_lit *argDebug;
   struct arg_end *argEnd;
   void *argtable[] =
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
           argServe = arg_str0(NULL, "serve", "<PATH>", "Path to directory statically served by HTTP server"),
           argDebug = arg_lit0(NULL, "debug", "Enable debug output"),
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

   // Check for debug switch
   if (argDebug->count)
   {
      dymonDebug = 1;
   }

   // Ensure that there is at most one interface specified
   if (argUsb->count && argNet->count)
   {
      puts(VT100_RED "Error:" VT100_RESET " Only one interface allowed (one of '--usb' or '--net')\n");
      print_usage(argtable);
      return -1;
   }

   thread printLabelsThread(&m_print_labels_thread);
   thread printPbmsThread(&m_print_pbms_thread);
   Server svr;

   // get interfaze and path
   if (argUsb->count)
   {
      const char *const dev = argUsb->sval[0];
      const int model = (argModel->count ? argModel->ival[0] : 0);
      const bool lw450 = (model == 450);
      const char *path;
#ifdef _WIN32
      // get the device name, to be used on windows
      //
      static char devname[256];
      int err = usbprint_get_devicename(devname, sizeof(devname), &argv[1][4]); // input something like "vid_0922" and get device name like "\\?\usb#vid_0922&pid_0028#04133046018600#{28d78fad-5a12-11d1-ae5b-0000f803a8c2}"
      if (err != 0)
      {
         cout << "Can't find any USB connected DYMO" << endl;
         return -2;
      }
      path = devname;
#else
      path = dev;
#endif
      m_Printer = new DymonPrinter(new DymonUsb(1, lw450), path);
   }
   else // net
   {
      const char *const ip = argNet->count ? argNet->sval[0] : nullptr;
      m_Printer = new DymonPrinter(new DymonNet, ip); // pass this to DymonNet::start, which expects a ip address string
      // m_Printer = new DymonPrinter(new DymonFile("/tmp/labels"), nullptr); // pass this to DymonNet::start, which expects a ip address string
   }

   // greetings
   const uint16_t port = (uint16_t)(argPort->count ? argPort->ival[0] : 8092);
   cout << "Printserver for DYMO LabelWriter, by minlux. V" APP_VERSION "\n"
        << endl;
   cout << "\nStart listening to port " << port << endl;
   cout << "Press [Ctrl + C] to quit!" << endl;

   // Set the REST endpoints.
   // The respective GET endpoint serves a static site, that acts as a proxy.
   // It is capable to receive data via window.postMessage() and forwards it to the respective REST endpoint.
   // With such a proxy, we can use the REST services even from a HTTPS context!
   // See folder www/_wpm/html for an example
   svr.Get("/wpm", &m_on_get_wpm);
   svr.Get("/labels", &m_on_get_wpm);
   svr.Post("/labels", &m_on_post_labels);
   svr.Get("/pbm", &m_on_get_wpm);
   svr.Post("/pbm", &m_on_post_pbm);
   
   // Add a handler for *all* OPTION requests, that replies, so that CORS is possible
   svr.Options(R"(.*)", &m_on_options);

   // serve static files from given directory, or serve a default home page
   if (argServe->count)
   {
      svr.set_mount_point("/", argServe->sval[0]);
   }
   else
   {
      svr.Get("/", [&](const Request & /*req*/, Response &res) {
         res.set_header("Access-Control-Allow-Origin", "*"); // CORS
         res.set_content((const char *)__labelwriter_index_html, __labelwriter_index_html_len, "text/html");
      });
   }

   // Start server
   svr.listen("0.0.0.0", port);
   return 0;
}


// serve todo
static void m_on_get_wpm(const Request & /*req*/, Response &res)
{
   res.set_header("Access-Control-Allow-Origin", "*"); // CORS
   res.set_content((const char *)__wpm_receiver_html, __wpm_receiver_html_len, "text/html");
}

// typically before a POST request, a browser first sends the OPTION request...
// this resposne is needed to allow cross origin requests.
// it is needed to let the browser continue with the actual POST request.
static void m_on_options(const Request &req, Response &res)
{
   // CORS
   res.set_header("Access-Control-Allow-Origin", "*");
   res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Origin, Authorization");
   res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
   res.set_header("Keep-Alive", "timeout=5, max=100");
   res.set_header("Connection", "Keep-Alive");
   // res.set_header("Content-Type", "application/json");
   res.status = 200; // HTTP status code
}

/*
   POST data like this expexted (Single JSON object or array of objects):
   {
      "ip":"127.0.0.1", //don't care for USB printers
      "width":272,
      "height":252,
      "orientation":0,
      "text":"\\24cTitle\n\\3_\nHello\n\\rWorld\n\n\\100#214",
      "count":1
   }

   Example, sending data with CURL:
   ----
   curl -d '{"ip":"192.168.178.67", "width":272, "height":252, "orientation":0,
     "text":"Manuel Heiß\nminlux.de\n\\_\nhttps://github.com/minlux", "count":1 }'
     -H "Content-Type: application/json"
     -X POST http://localhost:8092/labels

   curl -H "Content-Type: application/json" --data @text-label.json http://localhost:8092/labels
   curl -H "Content-Type: application/json" --data @saturn-labels.json http://localhost:8092/labels
*/
static void m_on_post_labels(const Request &req, Response &res)
{
   // request body is expected to contain json array of objects
   const char *requestBody = req.body.c_str();
   cJSON *const labels = cJSON_Parse(requestBody);
   if (labels != NULL)
   {
      // push the "print labels request" into the queue - a "m_print_thread" will pop the label out of the queue and process it!
      m_LabelQueue.push_nowait(labels); // the actual printing of the label is done in function m_print_labels!!!
   }

   // set response
   res.set_header("Access-Control-Allow-Origin", "*");
   res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Origin, Authorization");
   res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
   res.set_content("{\"status\":\"OK\"}", "application/json");
   res.status = 200; // HTTP status code
}

static inline bool starts_with_p4_header(const uint8_t *data)
{
   bool p4 = true;
   p4 &= (*data++ == 'P');
   p4 &= (*data++ == '4');
   p4 &= (*data++ == '\n');
   return p4;
}


/*
   POST P4 encoded portable bitmap (pbm) image.
   See also https://en.wikipedia.org/wiki/Netpbm

   The payload data of the POST request can be either:
   - binary data bytes of a pbm image
   - one or more base64 encoded pbm images (images are seperated by newlines)

   Example:
   ----
   curl -H "Content-Type: application/octet-stream" --data-binary @eagle_25x25.pbm http://localhost:8092/pbm
   curl -H "Content-Type: application/octet-stream" --data-binary @eagle_25x25.pbm 'http://localhost:8092/pbm?ip=192.168.178.21&copies=2'

   Example
   ----
   base64 -w 0 eagle_25x25.pbm > collage.txt
   echo >> collage.txt
   base64 -w 0 label_25x25.pbm > collage.txt
   curl -i -H "Content-Type: text/plain" --data-binary @collage.txt http://localhost:8092/pbm
*/
static void m_on_post_pbm(const Request &req, Response &res)
{
   // Get query parameters (req.params is a std::multimap<std::string, std::string>)
   // IP
   std::string ip = std::string("0.0.0.0");
   auto it = req.params.find("ip");
   if (it != req.params.end()) {
      ip = it->second;
   }
   uint32_t copies = 1;
   it = req.params.find("copies");
   if (it != req.params.end()) {
      int cpys = std::stoi(it->second);
      if (cpys > 1) copies = (uint32_t)cpys;
   }

   // Body data can be either binary PBM P4 data (of one single image)
   //  or base64 data (of one or more PBM P4 images, seperated by \n)
   const uint8_t *body = (const uint8_t *)req.body.data();
   if ((req.body.size() >= 8) && starts_with_p4_header(body))
   {
      m_PbmQueue.push_nowait(Pbm(ip, copies, body, req.body.size()));
   }
   else
   {
      std::vector<uint8_t> pbm(req.body.size());
      size_t start = 0;
      while (true)
      {
         // Find next newline
         size_t pos = req.body.find('\n', start);
         if (pos == std::string::npos)
         {
            const uint32_t cnt = base64_decode(pbm.data(), &req.body[start], req.body.size() - start);
            if ((cnt >= 8) && starts_with_p4_header(pbm.data()))
            {
               m_PbmQueue.push_nowait(Pbm(ip, copies, pbm.data(), cnt));
            }
            break;
         }
         const uint32_t cnt = base64_decode(pbm.data(), &req.body[start], pos - start);
         if ((cnt >= 8) && starts_with_p4_header(pbm.data()))
         {
            m_PbmQueue.push_nowait(Pbm(ip, copies, pbm.data(), cnt));
         }
         // Move past the newline
         start = pos + 1;
      }
   }

   // Set response
   res.set_header("Access-Control-Allow-Origin", "*");
   res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept, Origin, Authorization");
   res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
   // res.set_content("{\"status\":\"OK\"}", "application/json");
   res.status = 200; // HTTP status code
}

static void m_print_labels_thread()
{
   cJSON *labels;

   while (true)
   {
      // get next print request out of the queue. wait until one become available
      m_LabelQueue.pop(labels);
      // print the labels
      m_print_labels(labels);
      // cleanup JSON
      cJSON_Delete(labels);
   }
}

//labels can be either an array of labels, or one single label
static void m_print_labels(cJSON *labels)
{
   // start label printing
   const bool isArray = cJSON_IsArray(labels);
   cJSON * const first = isArray ? labels->child : labels;
   cJSON * const ip = cJSON_GetObjectItemCaseSensitive(first, "ip"); // get IP
   const char * ipAddress = (cJSON_IsString(ip) && ip->valuestring) ? ip->valuestring : "0.0.0.0";
   int err = m_Printer->start(ipAddress); // IP is only used for network labelwriter, and only if IP isn't forced to a specific address given by command line argument '--net'
   if (err == 0)
   {
      cJSON * label;
      cJSON * next;
      for(label = first; label != NULL; label = next)
      {
         next = isArray ? label->next : nullptr;
         //get number of copies to be printed
         cJSON * const count = cJSON_GetObjectItemCaseSensitive(label, "count");
         const uint32_t copies = count ? count->valueint : 1;

         // generate bitmap from label data
         auto bitmap = TextLabel::fromJson(label);
         m_Printer->print(&bitmap, copies, (next != nullptr));
      }
   }
   // finalize printing process (make form-feed and close TCP connection to LabelWriter)
   m_Printer->end();
}


static void m_print_pbms_thread()
{
   Pbm pbm;
   while (true)
   {
      // get next print request out of the queue. wait until one become available
      m_PbmQueue.pop(pbm);
      if (m_Printer->start(pbm.ip.c_str()) == 0)
      {
         do
         {
            auto bitmap = Dymon::Bitmap::fromBytes(pbm.data);
            m_Printer->print(&bitmap, pbm.copies, (m_PbmQueue.count() != 0));
         } while (m_PbmQueue.pop_nowait(pbm));
         m_Printer->end(); // finalize printing (form-feed) and close socket
      }
   }
}
