
#include <globals.h>
#if OTA_APP // OTA_APP build
// OTA_APP: entry point for OTA applications (OTA = Over The Air - 3rd party installed apps) 
// If building a 3rd party app, set your build environment to OTA_APP in PlatformIO/VSCode
void APP_INIT() {
}

void processKB_APP() {
}
void einkHandler_APP() {
}
#endif
