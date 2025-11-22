#include "app_mqtt.h"
#include <MQTTClient.h>
#include <assert.h>
#include <stdlib.h>
#include "thirdparty/log.c/log.h"

static MQTTClient client;
static MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
static MQTTClient_deliveryToken token;
static int (*recv_callback)(char *, int);

/**
 * @brief 连接丢失时回调函数
 *
 * @param context
 * @param cause
 */
void app_mqtt_connectionLost(void *context, char *cause)
{
    assert(context == NULL);
    log_fatal("Connection lost: %s", cause);
    exit(EXIT_FAILURE);
}

int app_mqtt_messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    assert(context == NULL);
    log_trace("Message arrived\n\ttopic: %.*s\n\tpayload: %.*s", topicLen, topicName, (int)message->payloadlen, (char *)message->payload);
    // 调用回调函数将数据写入缓存

    int result = recv_callback((char *)message->payload, message->payloadlen);
    return result == 0 ? 1 : 0;
}

void app_mqtt_deliveryComplete(void *context, MQTTClient_deliveryToken dt)
{
    assert(context == NULL);
    log_trace("Message with token %d delivered", dt);
}

int app_mqtt_init()
{
    // 初始化客户端
    int rc;
    rc = MQTTClient_create(&client, MQTT_SERVER, CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTTClient_create error: %d", rc);
        goto EXIT;
    }

    // 设置回调函数
    rc = MQTTClient_setCallbacks(client, NULL, app_mqtt_connectionLost, app_mqtt_messageArrived, app_mqtt_deliveryComplete);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTTClient_setCallbacks error, rc=%d", rc);
        goto DESTROY_EXIT;
    }

    // 连接服务器
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTTClient_connect error, rc=%d", rc);
        goto DESTROY_EXIT;
    }

    // 订阅话题
    rc = MQTTClient_subscribe(client, PULL_TOPIC, 0);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTTClient_subscribe error, rc=%d", rc);
        goto DISCONNECT_EXIT;
    }

    log_info("MQTTClient initialize success");
    return 0;
DISCONNECT_EXIT:
    MQTTClient_disconnect(client, 1000);
DESTROY_EXIT:
    MQTTClient_destroy(&client);
EXIT:
    return -1;
}

void app_mqtt_registerRecvCallback(int (*callback)(char *, int))
{
    recv_callback = callback;
}

int app_mqtt_send(char *json_str, int len)
{
    MQTTClient_message message = MQTTClient_message_initializer;
    message.payload = json_str;
    message.payloadlen = len;
    message.qos = 0;
    message.retained = 0;
    int rc = MQTTClient_publishMessage(client, PUSH_TOPIC, &message, &token);
    if (rc == MQTTCLIENT_SUCCESS)
    {
        log_trace("MQTTClient_publishMessage success");
        return 0;
    }
    else
    {
        log_error("MQTTClient_publishMessage failed, rc=%d", rc);
        return -1;
    }
}

void app_mqtt_close()
{
    log_info("MQTT Client closing...");
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
}
