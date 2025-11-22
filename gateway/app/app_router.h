#if !defined(__APP_ROUTER_H__)
#define __APP_ROUTER_H__

#include "app_device.h"

int app_router_init();

int app_router_registerDevice(Device *device);

void app_router_close();

#endif // __APP_ROUTER_H__
