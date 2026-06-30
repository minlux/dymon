// Adapted from https://blog.peter.skarpetis.com/archives/2005/04/07/getting-a-handle-on-usbprintsys/comment-page-2/
/* Code to find the device path for a usbprint.sys controlled
 * usb printer and print to it
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <setupapi.h>

static const GUID GUID_DEVINTERFACE_USBPRINT = {
   0x28d78fad, 0x5a12, 0x11D1,
   { 0xae, 0x5b, 0x00, 0x00, 0xf8, 0x03, 0xa8, 0xc2 }
};

typedef struct {
   char *path;      /* heap-allocated device path, caller must free */
   char  name[256]; /* friendly name from registry, or "" */
} UsbPrinterInfo;

/* Fills info for the device at index. Returns 1 on success, 0 if no more devices. */
static int get_printer_info(HDEVINFO devs, DWORD index, UsbPrinterInfo *info)
{
   SP_DEVICE_INTERFACE_DATA iface;
   iface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
   if (!SetupDiEnumDeviceInterfaces(devs, NULL, &GUID_DEVINTERFACE_USBPRINT, index, &iface))
   {
      return 0;
   }

   DWORD size = 0;
   SetupDiGetDeviceInterfaceDetailA(devs, &iface, NULL, 0, &size, NULL);

   SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail =
      (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)malloc(size);
   if (!detail)
   {
      return 0;
   }
   detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

   SP_DEVINFO_DATA devinfo;
   devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
   if (!SetupDiGetDeviceInterfaceDetailA(devs, &iface, detail, size, NULL, &devinfo))
   {
      free(detail);
      return 0;
   }

   info->path = _strdup(detail->DevicePath);
   free(detail);

   info->name[0] = '\0';
   SetupDiGetDeviceRegistryPropertyA(devs, &devinfo,
      SPDRP_FRIENDLYNAME, NULL, (PBYTE)info->name, (DWORD)sizeof(info->name) - 1, NULL);

   return 1;
}


void usbprint_discover(void)
{
   HDEVINFO devs = SetupDiGetClassDevsA(
      &GUID_DEVINTERFACE_USBPRINT, NULL, NULL,
      DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (devs == INVALID_HANDLE_VALUE)
   {
      fprintf(stderr, "SetupDiGetClassDevs failed (error %lu)\n", GetLastError());
      return;
   }

   printf("USB printers (usbprint.sys):\n\n");
   int found = 0;
   for (DWORD i = 0; ; i++)
   {
      UsbPrinterInfo info = { 0 };
      if (!get_printer_info(devs, i, &info))
      {
         break;
      }

      char vid[32] = { 0 };
      const char *vid_start = strstr(info.path, "vid_");
      if (vid_start)
      {
         const char *vid_end = strchr(vid_start, '&');
         if (!vid_end)
         {
            vid_end = strchr(vid_start, '#');
         }
         if (vid_end)
         {
            size_t len = (size_t)(vid_end - vid_start);
            if (len < sizeof(vid))
            {
               memcpy(vid, vid_start, len);
               vid[len] = '\0';
            }
         }
      }

      printf("  Name : %s\n", info.name[0] ? info.name : "(unknown)");
      printf("  Path : %s\n", info.path);
      if (vid[0])
      {
         printf("  Use  : --usb %s\n", vid);
      }
      printf("\n");

      free(info.path);
      found++;
   }

   if (found == 0)
   {
      printf("  (none found - is the printer plugged in and powered on?)\n\n");
   }

   SetupDiDestroyDeviceInfoList(devs);
}


// Get the obscure device-name Windows assigns to a USB device (e.g. "\\?\usb#vid_0922&pid_0028#...#{28d78fad-...}")
// Returns 0 on success, -1 on API error, -3 if no device matches deviceIdentifier.
int usbprint_get_devicename(char buffer[], unsigned int bufsize, const char *deviceIdentifier)
{
   buffer[0] = '\0';

   HDEVINFO devs = SetupDiGetClassDevsA(
      &GUID_DEVINTERFACE_USBPRINT, NULL, NULL,
      DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (devs == INVALID_HANDLE_VALUE)
   {
      return -1;
   }

   int result = -3;
   for (DWORD i = 0; ; i++)
   {
      UsbPrinterInfo info = { 0 };
      if (!get_printer_info(devs, i, &info))
      {
         break;
      }

      if (strstr(info.path, deviceIdentifier))
      {
         strncpy(buffer, info.path, bufsize);
         buffer[bufsize - 1] = '\0';
         free(info.path);
         result = 0;
         break;
      }

      free(info.path);
   }

   SetupDiDestroyDeviceInfoList(devs);
   return result;
}
