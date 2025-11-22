#include "app/app_message.h"
#include "thirdparty/log.c/log.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    unsigned char data[] = {0x01, 0x02, 0x04, 0x00, 0x01, 0xEA, 0xAC, 0x22, 0x88};
    unsigned char result[9];
    char json_str[1024];
    Message messsage;
    app_message_initByBinary(&messsage, data, sizeof(data));
    app_message_saveJson(&messsage, json_str, sizeof(json_str));

    log_info("%s", json_str);

    Message messsage2;
    app_message_initByJson(&messsage2, json_str, strlen(json_str));
    app_message_saveBinary(&messsage2, result, sizeof(result));

    assert(memcmp(data, result, 9) == 0);

    app_message_free(&messsage);
    app_message_free(&messsage2);
    
}
