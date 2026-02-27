#include "../include/app_router.h"
#include "../include/app_message.h"
#include "../include/app_config.h"
#include "../thirdparty/log.c/log.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define DEFAULT_CONFIG_FILE "/home/nvidia/gateway/gateway.ini"
#define DEFAULT_MAX_DEVICES 32
#define MAX_MESSAGE_SIZE    4096


static RouterManager g_router_instance;
static RouterManager *g_router = NULL;

static Device *find_device_by_type(RouterManager *router, ConnectionType type)
{
    if (!router) return NULL;
    
    for (int i = 0; i < router->device_count; i++) {
        if (router->devices[i] && 
            router->devices[i]->connection_type == type) {
            return router->devices[i];
        }
    }
    return NULL;
}

static int json_to_binary_safe(const char *json_str, int json_len,
                                unsigned char *buf, int buf_size)
{
    if (!json_str || !buf || json_len <= 0 || buf_size <= 0) {
        return -1;
    }
    
    Message message;
    memset(&message, 0, sizeof(Message));
    
    if (app_message_initByJson(&message, (char *)json_str, json_len) < 0) {
        return -1;
    }
    
    int required_size = message.id_len + message.data_len + 3;
    if (required_size > buf_size) {
        app_message_free(&message);
        return -1;
    }
    
    int buf_len = app_message_saveBinary(&message, buf, buf_size);
    app_message_free(&message);
    
    return buf_len;
}

static int binary_to_json_safe(const void *binary, int binary_len,
                                char *buf, int buf_size)
{
    if (!binary || !buf || binary_len <= 0 || buf_size <= 0) {
        return -1;
    }
    
    Message message;
    memset(&message, 0, sizeof(Message));
    
    if (app_message_initByBinary(&message, (void *)binary, binary_len) < 0) {
        return -1;
    }
    
    int result = app_message_saveJson(&message, buf, buf_size);
    app_message_free(&message);
    
    return (result == 0) ? (int)strlen(buf) : -1;
}

static void on_transport_message(TransportManager *transport, const char *topic,
                                  const void *data, size_t len)
{
    RouterManager *router = (RouterManager *)transport;
    if (!router || !data || len == 0) return;
    
    unsigned char *buf = malloc(MAX_MESSAGE_SIZE);
    if (!buf) return;
    
    int buf_len = json_to_binary_safe((const char *)data, (int)len,
                                       buf, MAX_MESSAGE_SIZE);
    if (buf_len < 0) {
        free(buf);
        return;
    }
    
    ConnectionType type = (ConnectionType)buf[0];
    
    pthread_mutex_lock(&router->lock);
    Device *device = find_device_by_type(router, type);
    pthread_mutex_unlock(&router->lock);
    
    if (!device) {
        free(buf);
        return;
    }
    
    app_device_write(device, buf, buf_len);
    free(buf);
    
    if (router->on_cloud_message) {
        router->on_cloud_message(router, topic, data, len);
    }
}

static void on_transport_state_changed(TransportManager *transport,
                                        TransportState state)
{
    RouterManager *router = (RouterManager *)transport;
    if (!router) return;
    
    RouterState new_state;
    switch (state) {
        case TRANSPORT_STATE_CONNECTED:
            new_state = ROUTER_STATE_RUNNING;
            break;
        case TRANSPORT_STATE_ERROR:
            new_state = ROUTER_STATE_ERROR;
            break;
        default:
            new_state = ROUTER_STATE_STOPPED;
            break;
    }
    
    pthread_mutex_lock(&router->lock);
    router->state = new_state;
    pthread_mutex_unlock(&router->lock);
    
    if (router->on_state_changed) {
        router->on_state_changed(router, new_state);
    }
}

static int on_device_message(void *ptr, int len)
{
    if (!g_router || !ptr || len <= 0) return -1;
    
    RouterManager *router = g_router;
    
    char *buf = malloc(MAX_MESSAGE_SIZE);
    if (!buf) return -1;
    
    int json_len = binary_to_json_safe(ptr, len, buf, MAX_MESSAGE_SIZE);
    if (json_len < 0) {
        free(buf);
        return -1;
    }
    
    /* 使用统一的发布接口 - 自动适配MQTT/DDS */
    int result = transport_publish_default(&router->transport, buf, json_len);
    
    free(buf);
    
    if (result < 0) {
        return -1;
    }
    
    /* 更新统计 */
    pthread_mutex_lock(&router->lock);
    router->stats.messages_sent++;
    pthread_mutex_unlock(&router->lock);
    
    if (router->on_device_message) {
        ConnectionType type = ((unsigned char *)ptr)[0];
        pthread_mutex_lock(&router->lock);
        Device *device = find_device_by_type(router, type);
        pthread_mutex_unlock(&router->lock);
        if (device) {
            router->on_device_message(router, device, ptr, len);
        }
    }
    return 0;
}
int app_router_init(RouterManager *router, const char *config_file)
{
    if (!router) return -1;
    
    memset(router, 0, sizeof(RouterManager));
    
    if (pthread_mutex_init(&router->lock, NULL) != 0) {
        return -1;
    }
    
    router->device_count = 0;
    router->state = ROUTER_STATE_STOPPED;
    router->is_initialized = 1;
    
    const char *cfg_file = config_file ? config_file : DEFAULT_CONFIG_FILE;
    
    if (transport_init_from_config(&router->transport, cfg_file) != 0) {
        TransportConfig tconfig;
        memset(&tconfig, 0, sizeof(TransportConfig));
        tconfig.type = TRANSPORT_TYPE_MQTT;
        strncpy(tconfig.mqtt_broker, "tcp://localhost:1883", sizeof(tconfig.mqtt_broker) - 1);
        strncpy(tconfig.mqtt_client_id, "gateway", sizeof(tconfig.mqtt_client_id) - 1);
        tconfig.mqtt_keepalive = 60;
        tconfig.dds_domain_id = 0;
        tconfig.default_qos = 1;
        
        if (transport_init(&router->transport, &tconfig) != 0) {
            pthread_mutex_destroy(&router->lock);
            return -1;
        }
    }
    
    transport_on_message(&router->transport, on_transport_message);
    transport_on_state_changed(&router->transport, on_transport_state_changed);
    
    g_router = router;
    
    return 0;
}

int app_router_init_default(RouterManager *router)
{
    return app_router_init(router, NULL);
}

void app_router_close(RouterManager *router)
{
    if (!router || !router->is_initialized) return;
    
    transport_disconnect(&router->transport);
    
    pthread_mutex_lock(&router->lock);
    for (int i = 0; i < router->device_count; i++) {
        if (router->devices[i]) {
            app_device_stop(router->devices[i]);
            app_device_close(router->devices[i]);
            router->devices[i] = NULL;
        }
    }
    router->device_count = 0;
    pthread_mutex_unlock(&router->lock);
    
    transport_close(&router->transport);
    pthread_mutex_destroy(&router->lock);
    
    router->is_initialized = 0;
    
    if (g_router == router) {
        g_router = NULL;
    }
}

int app_router_register_device(RouterManager *router, Device *device)
{
    if (!router || !device || !router->is_initialized) return -1;
    
    pthread_mutex_lock(&router->lock);
    
    if (router->device_count >= ROUTER_MAX_DEVICES) {
        pthread_mutex_unlock(&router->lock);
        return -1;
    }
    
    for (int i = 0; i < router->device_count; i++) {
        if (router->devices[i] == device) {
            pthread_mutex_unlock(&router->lock);
            return 0;
        }
    }
    
    router->devices[router->device_count++] = device;
    app_device_registerRecvCallback(device, on_device_message);
    
    pthread_mutex_unlock(&router->lock);
    
    return 0;
}

int app_router_start(RouterManager *router)
{
    if (!router || !router->is_initialized) return -1;
    
    if (router->state == ROUTER_STATE_RUNNING) return 0;
    
    /* 连接传输层 */
    if (transport_connect(&router->transport) != 0) {
        router->state = ROUTER_STATE_ERROR;
        return -1;
    }
    
    /* 订阅下发话题（接收云端指令）- 自动适配MQTT/DDS */
    if (transport_subscribe_default(&router->transport) != 0) {
        log_warn("Failed to subscribe to command topic");
        /* 不返回错误，继续运行 */
    } else {
        log_info("Subscribed to command topic: %s", 
                 transport_get_subscribe_topic(&router->transport));
    }
    
    pthread_mutex_lock(&router->lock);
    for (int i = 0; i < router->device_count; i++) {
        if (router->devices[i]) {
            app_device_start(router->devices[i]);
        }
    }
    router->state = ROUTER_STATE_RUNNING;
    pthread_mutex_unlock(&router->lock);
    
    return 0;
}
void app_router_stop(RouterManager *router)
{
    if (!router) return;
    
    pthread_mutex_lock(&router->lock);
    for (int i = 0; i < router->device_count; i++) {
        if (router->devices[i]) {
            app_device_stop(router->devices[i]);
        }
    }
    router->state = ROUTER_STATE_STOPPED;
    pthread_mutex_unlock(&router->lock);
    
    transport_disconnect(&router->transport);
}

int app_router_registerDevice(Device *device)
{
    if (!g_router) {
        if (app_router_init(&g_router_instance, NULL) != 0) {
            return -1;
        }
        g_router = &g_router_instance;
    }
    return app_router_register_device(g_router, device);
}

int app_router_unregister_device(RouterManager *router, Device *device)
{
    if (!router || !device || !router->is_initialized) return -1;
    
    pthread_mutex_lock(&router->lock);
    
    for (int i = 0; i < router->device_count; i++) {
        if (router->devices[i] == device) {
            app_device_stop(device);
            // 移动后面的元素
            for (int j = i; j < router->device_count - 1; j++) {
                router->devices[j] = router->devices[j + 1];
            }
            router->devices[--router->device_count] = NULL;
            pthread_mutex_unlock(&router->lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&router->lock);
    return -1;
}

int app_router_get_device_count(RouterManager *router)
{
    return router ? router->device_count : 0;
}

int app_router_switch_transport(RouterManager *router, TransportType type)
{
    if (!router || !router->is_initialized) return -1;
    return transport_switch_type(&router->transport, type);
}

TransportType app_router_get_transport_type(RouterManager *router)
{
    return router ? transport_get_type(&router->transport) : TRANSPORT_TYPE_MQTT;
}

int app_router_send_to_cloud(RouterManager *router, const char *topic,
                              const void *data, size_t len)
{
    if (!router || !data) return -1;
    
    /* 如果没有指定话题，使用默认发布话题 */
    if (!topic) {
        return transport_publish_default(&router->transport, data, len);
    }
    return transport_publish(&router->transport, topic, data, len);
}


int app_router_send_to_device(RouterManager *router, Device *device,
                               const void *data, size_t len)
{
    if (!router || !device || !data) return -1;
    return app_device_write(device, (void *)data, (int)len);
}

RouterState app_router_get_state(RouterManager *router)
{
    return router ? router->state : ROUTER_STATE_STOPPED;
}

void app_router_get_stats(RouterManager *router, RouterStats *stats)
{
    if (router && stats) {
        pthread_mutex_lock(&router->lock);
        memcpy(stats, &router->stats, sizeof(RouterStats));
        pthread_mutex_unlock(&router->lock);
    }
}

void app_router_reset_stats(RouterManager *router)
{
    if (router) {
        pthread_mutex_lock(&router->lock);
        memset(&router->stats, 0, sizeof(RouterStats));
        pthread_mutex_unlock(&router->lock);
    }
}

void app_router_on_device_message(RouterManager *router,
    void (*callback)(RouterManager *, Device *, const void *, size_t))
{
    if (router) router->on_device_message = callback;
}

void app_router_on_cloud_message(RouterManager *router,
    void (*callback)(RouterManager *, const char *, const void *, size_t))
{
    if (router) router->on_cloud_message = callback;
}

void app_router_on_state_changed(RouterManager *router,
    void (*callback)(RouterManager *, RouterState))
{
    if (router) router->on_state_changed = callback;
}

