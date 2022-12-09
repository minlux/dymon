#ifndef USBPRINT_UTILS_H_INCLUDED
#define USBPRINT_UTILS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//https://blog.peter.skarpetis.com/archives/2005/04/07/getting-a-handle-on-usbprintsys/comment-page-2/
//https://community.silabs.com/s/article/windows-usb-device-path?language=en_US to find the device path in the registry
int usbprint_utils_get_devicename(char buffer[], unsigned int size, const char * deviceIdentifier);


#ifdef __cplusplus
}
#endif


#endif
