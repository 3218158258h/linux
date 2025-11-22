#if !defined(__APP_BLUETOOTH_H__)
#define __APP_BLUETOOTH_H__

#include "app_message.h"
#include "app_serial.h"

int app_bluetooth_setConnectionType(SerialDevice *serial_device);

int app_bluetooth_status(SerialDevice *serial_device);
int app_bluetooth_setBaudRate(SerialDevice *serial_device, SerialBaudRate baud_rate);
int app_bluetooth_reset(SerialDevice *serial_device);
int app_bluetooth_setNetID(SerialDevice *serial_device, char *net_id);
int app_bluetooth_setMAddr(SerialDevice *serial_device, char *m_addr);

int app_bluetooth_postRead(Device *device, void *ptr, int *len);
int app_bluetooth_preWrite(Device *device, void *ptr, int *len);

#endif // __APP_BLUETOOTH_H__
