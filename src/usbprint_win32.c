// Adapted from https://blog.peter.skarpetis.com/archives/2005/04/07/getting-a-handle-on-usbprintsys/comment-page-2/
/* Code to find the device path for a usbprint.sys controlled
 * usb printer and print to it
 */

#include <stdlib.h>
#include <stdio.h>
#include <wtypesbase.h> //this comes mingw64 and is required to define LPVOID etc.
#include <winreg.h>
#include <setupapi.h>
#include <devguid.h>

/* This define is required so that the GUID_DEVINTERFACE_USBPRINT variable is
 * declared an initialised as a static locally, since windows does not include it in any
 * of its libraries
 */
#define SS_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
   static const GUID name = {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}

SS_DEFINE_GUID(GUID_DEVINTERFACE_USBPRINT, 0x28d78fad, 0x5a12, 0x11D1, 0xae, 0x5b, 0x00, 0x00, 0xf8, 0x03, 0xa8, 0xc2); // according to peters blog, this is a microsoft magic number to identify usb printers


//get the obscure device-name, windows assigns to an usb device (something like "\\?\usb#vid_0922&pid_0028#04133046018600#{28d78fad-5a12-11d1-ae5b-0000f803a8c2}")
//returns
//0 on success
//-1, -2 in case of API errors
//-3 none of the devices matches the given device-identifer
int usbprint_get_devicename(char buffer[], unsigned int bufsize, const char *deviceIdentifier)
{
   buffer[0] = 0; //preset

   // get handle to a device information set that contains requested device information elements
   HDEVINFO devs = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USBPRINT, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (devs == INVALID_HANDLE_VALUE)
   {
      return -1;
   }

   int error = -2; //preset
   //enumerates the device interfaces that are contained in a device information set
   DWORD devcount = 0;
   SP_DEVICE_INTERFACE_DATA devinterface;
   devinterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
   while (SetupDiEnumDeviceInterfaces(devs, 0, &GUID_DEVINTERFACE_USBPRINT, devcount++, &devinterface))
   {
      // get details about th device interface
      DWORD size = 0;
      SetupDiGetDeviceInterfaceDetail(devs, &devinterface, 0, 0, &size, 0); //first i just query the size of the details
      {
         PSP_DEVICE_INTERFACE_DETAIL_DATA interface_detail;
         interface_detail = calloc(1, size); //allocate memory for the details
         if (interface_detail)
         {
            SP_DEVINFO_DATA devinfo;
            devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
            interface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            if (SetupDiGetDeviceInterfaceDetail(devs, &devinterface, interface_detail, size, 0, &devinfo))
            {
               error = -3; //preset
               // puts(interface_detail->DevicePath); //test only; something like "\\?\usb#vid_0922&pid_0028#04133046018600#{28d78fad-5a12-11d1-ae5b-0000f803a8c2}" expected

               //now check if the interface path matches my identifyer
               if (strstr(interface_detail->DevicePath, deviceIdentifier))
               {
                  strncpy(buffer, interface_detail->DevicePath, bufsize);
                  buffer[bufsize - 1] = 0; //ensure zero termination
                  free(interface_detail);
                  return 0;
               }
            }
            free(interface_detail);
         }
      }
   }
   return error;
}

