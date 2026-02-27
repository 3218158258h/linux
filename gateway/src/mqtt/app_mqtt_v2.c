/**
 * @file app_mqtt_v2.c
 * @brief MQTT客户端改进版实现
 */

#include "app_mqtt_v2.h"
#include "../thirdparty/log.c/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* 使用paho-mqtt库 */
#include <MQTTClient.h>

/* 内部结构 */
typedef struct {
    MQTTClient mqtt_client;
    pthread_t thread;
    int running;
    pthread_mutex_t lock;
    
    /* 重连状态 */
    int reconnect_delay;
    time_t last_reconnect_time;
} MqttInternal;

/* 连接丢失回调 */
static void connection_lost(void *context, char *cause)
{
    MqttClient *client = (MqttClient *)context;
    if (!client) return;
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    pthread_mutex_lock(&internal->lock);
    
    client->state = MQTT_STATE_DISCONNECTED;
    internal->reconnect_delay = client->config.reconnect_min_interval;
    
    pthread_mutex_unlock(&internal->lock);
    
    log_warn("MQTT connection lost: %s", cause ? cause : "unknown");
    
    /* 触发回调 */
    if (client->on_disconnected) {
        client->on_disconnected(client);
    }
    if (client->on_state_changed) {
        client->on_state_changed(client, MQTT_STATE_DISCONNECTED);
    }
}

/* 消息到达回调 */
static int message_arrived(void *context, char *topicName, int topicLen,
                           MQTTClient_message *message)
{
    MqttClient *client = (MqttClient *)context;
    if (!client) {
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }
    
    log_debug("MQTT message received on topic: %s", topicName);
    
    if (client->on_message) {
        client->on_message(client, topicName, message->payload, message->payloadlen);
    }
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

/* 交付完成回调 */
static void delivery_complete(void *context, MQTTClient_deliveryToken dt)
{
    (void)context;
    (void)dt;
}

/* 计算重连延迟(指数退避) */
static int calculate_reconnect_delay(MqttClient *client)
{
    MqttInternal *internal = (MqttInternal *)client->internal;
    int delay = internal->reconnect_delay;
    
    /* 指数退避 */
    internal->reconnect_delay *= 2;
    if (internal->reconnect_delay > client->config.reconnect_max_interval) {
        internal->reconnect_delay = client->config.reconnect_max_interval;
    }
    
    return delay;
}

/* 执行重连 */
static int do_reconnect(MqttClient *client)
{
    MqttInternal *internal = (MqttInternal *)client->internal;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    
    opts.keepAliveInterval = client->config.keepalive_interval;
    opts.cleansession = client->config.clean_session;
    
    if (client->config.username[0]) {
        opts.username = client->config.username;
    }
    if (client->config.password[0]) {
        opts.password = client->config.password;
    }
    
    opts.reliable = 1;
    opts.connectTimeout = 10;
    
    int rc = MQTTClient_connect(internal->mqtt_client, &opts);
    
    if (rc == MQTTCLIENT_SUCCESS) {
        client->state = MQTT_STATE_CONNECTED;
        client->reconnect_attempts = 0;
        internal->reconnect_delay = client->config.reconnect_min_interval;
        
        log_info("MQTT reconnected successfully");
        
        if (client->on_connected) {
            client->on_connected(client);
        }
        if (client->on_state_changed) {
            client->on_state_changed(client, MQTT_STATE_CONNECTED);
        }
        
        return 0;
    }
    
    log_warn("MQTT reconnect failed: %d", rc);
    return -1;
}

/* MQTT主循环 */
void mqtt_loop(MqttClient *client)
{
    if (!client) return;
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    while (internal->running) {
        /* 检查连接状态 */
        pthread_mutex_lock(&internal->lock);
        MqttState state = client->state;
        pthread_mutex_unlock(&internal->lock);
        
        if (state == MQTT_STATE_DISCONNECTED && client->config.auto_reconnect) {
            /* 检查是否达到最大重连次数 */
            if (client->config.reconnect_max_attempts > 0 &&
                client->reconnect_attempts >= client->config.reconnect_max_attempts) {
                
                pthread_mutex_lock(&internal->lock);
                client->state = MQTT_STATE_FAILED;
                pthread_mutex_unlock(&internal->lock);
                
                log_error("MQTT max reconnect attempts reached");
                
                if (client->on_state_changed) {
                    client->on_state_changed(client, MQTT_STATE_FAILED);
                }
                break;
            }
            
            /* 检查重连间隔 */
            time_t now = time(NULL);
            int delay = calculate_reconnect_delay(client);
            
            if (now - internal->last_reconnect_time >= delay) {
                pthread_mutex_lock(&internal->lock);
                client->state = MQTT_STATE_RECONNECTING;
                pthread_mutex_unlock(&internal->lock);
                
                if (client->on_state_changed) {
                    client->on_state_changed(client, MQTT_STATE_RECONNECTING);
                }
                
                client->reconnect_attempts++;
                internal->last_reconnect_time = now;
                
                log_info("MQTT reconnecting (attempt %d)...", client->reconnect_attempts);
                
                do_reconnect(client);
            }
        }
        
        /* 等待消息 */
        usleep(100000); /* 100ms */
    }
}

/* 后台线程函数 */
static void *mqtt_thread_func(void *arg)
{
    MqttClient *client = (MqttClient *)arg;
    mqtt_loop(client);
    return NULL;
}

int mqtt_init(MqttClient *client, const MqttConfig *config)
{
    if (!client) return -1;
    
    memset(client, 0, sizeof(MqttClient));
    
    if (config) {
        memcpy(&client->config, config, sizeof(MqttConfig));
    } else {
        /* 默认配置 */
        strcpy(client->config.broker_url, "tcp://localhost:1883");
        client->config.keepalive_interval = 60;
        client->config.clean_session = 1;
        client->config.default_qos = MQTT_QOS_0;
        client->config.auto_reconnect = 1;
        client->config.reconnect_min_interval = 1;
        client->config.reconnect_max_interval = 60;
        client->config.reconnect_max_attempts = 0; /* 无限重试 */
    }
    
    /* 分配内部结构 */
    MqttInternal *internal = calloc(1, sizeof(MqttInternal));
    if (!internal) return -1;
    
    pthread_mutex_init(&internal->lock, NULL);
    internal->reconnect_delay = client->config.reconnect_min_interval;
    
    client->internal = internal;
    client->state = MQTT_STATE_DISCONNECTED;
    
    /* 创建MQTT客户端 */
    int rc = MQTTClient_create(&internal->mqtt_client,
                               client->config.broker_url,
                               client->config.client_id,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        log_error("Failed to create MQTT client: %d", rc);
        pthread_mutex_destroy(&internal->lock);
        free(internal);
        return -1;
    }
    
    /* 设置回调 */
    MQTTClient_setCallbacks(internal->mqtt_client, client,
                           connection_lost, message_arrived, delivery_complete);
    
    log_debug("MQTT client initialized: %s", client->config.broker_url);
    return 0;
}

int mqtt_init_default(MqttClient *client, const char *broker_url, const char *client_id)
{
    MqttConfig config = {0};
    
    if (broker_url) {
        strncpy(config.broker_url, broker_url, sizeof(config.broker_url) - 1);
    } else {
        strcpy(config.broker_url, "tcp://localhost:1883");
    }
    
    if (client_id) {
        strncpy(config.client_id, client_id, sizeof(config.client_id) - 1);
    }
    
    config.keepalive_interval = 60;
    config.clean_session = 1;
    config.default_qos = MQTT_QOS_1;
    config.auto_reconnect = 1;
    config.reconnect_min_interval = 1;
    config.reconnect_max_interval = 60;
    config.reconnect_max_attempts = 0;
    
    return mqtt_init(client, &config);
}

void mqtt_destroy(MqttClient *client)
{
    if (!client) return;
    
    mqtt_stop(client);
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    if (internal) {
        MQTTClient_destroy(&internal->mqtt_client);
        pthread_mutex_destroy(&internal->lock);
        free(internal);
        client->internal = NULL;
    }
    
    memset(client, 0, sizeof(MqttClient));
}

int mqtt_connect(MqttClient *client)
{
    if (!client || !client->internal) return -1;
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    pthread_mutex_lock(&internal->lock);
    client->state = MQTT_STATE_CONNECTING;
    pthread_mutex_unlock(&internal->lock);
    
    if (client->on_state_changed) {
        client->on_state_changed(client, MQTT_STATE_CONNECTING);
    }
    
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    opts.keepAliveInterval = client->config.keepalive_interval;
    opts.cleansession = client->config.clean_session;
    
    if (client->config.username[0]) {
        opts.username = client->config.username;
    }
    if (client->config.password[0]) {
        opts.password = client->config.password;
    }
    
    opts.reliable = 1;
    opts.connectTimeout = 10;
    
    int rc = MQTTClient_connect(internal->mqtt_client, &opts);
    
    if (rc == MQTTCLIENT_SUCCESS) {
        pthread_mutex_lock(&internal->lock);
        client->state = MQTT_STATE_CONNECTED;
        internal->reconnect_delay = client->config.reconnect_min_interval;
        pthread_mutex_unlock(&internal->lock);
        
        log_info("MQTT connected to %s", client->config.broker_url);
        
        if (client->on_connected) {
            client->on_connected(client);
        }
        if (client->on_state_changed) {
            client->on_state_changed(client, MQTT_STATE_CONNECTED);
        }
        
        return 0;
    }
    
    pthread_mutex_lock(&internal->lock);
    client->state = MQTT_STATE_FAILED;
    pthread_mutex_unlock(&internal->lock);
    
    log_error("MQTT connection failed: %d", rc);
    
    if (client->on_state_changed) {
        client->on_state_changed(client, MQTT_STATE_FAILED);
    }
    
    return -1;
}

void mqtt_disconnect(MqttClient *client)
{
    if (!client || !client->internal) return;
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    /* 只在已连接或正在连接时才调用 disconnect */
    if (client->state == MQTT_STATE_CONNECTED || 
        client->state == MQTT_STATE_CONNECTING ||
        client->state == MQTT_STATE_RECONNECTING) {
        MQTTClient_disconnect(internal->mqtt_client, 5000);
    }
    
    pthread_mutex_lock(&internal->lock);
    client->state = MQTT_STATE_DISCONNECTED;
    pthread_mutex_unlock(&internal->lock);
    
    log_info("MQTT disconnected");
    
    if (client->on_disconnected) {
        client->on_disconnected(client);
    }
    if (client->on_state_changed) {
        client->on_state_changed(client, MQTT_STATE_DISCONNECTED);
    }
}

MqttState mqtt_get_state(MqttClient *client)
{
    if (!client) return MQTT_STATE_DISCONNECTED;
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    if (!internal) return MQTT_STATE_DISCONNECTED;
    
    MqttState state;
    pthread_mutex_lock(&internal->lock);
    state = client->state;
    pthread_mutex_unlock(&internal->lock);
    
    return state;
}

int mqtt_is_connected(MqttClient *client)
{
    return mqtt_get_state(client) == MQTT_STATE_CONNECTED;
}

int mqtt_publish(MqttClient *client, const char *topic, 
                 const void *payload, size_t len, MqttQos qos)
{
    if (!client || !client->internal || !topic || !payload) return -1;
    
    if (!mqtt_is_connected(client)) {
        log_warn("MQTT not connected, cannot publish");
        return -1;
    }
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = (void *)payload;
    pubmsg.payloadlen = len;
    pubmsg.qos = qos;
    pubmsg.retained = 0;
    
    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(internal->mqtt_client, topic, &pubmsg, &token);
    
    if (rc == MQTTCLIENT_SUCCESS) {
        /* 等待交付完成(QoS > 0时) */
        if (qos > MQTT_QOS_0) {
            MQTTClient_waitForCompletion(internal->mqtt_client, token, 5000);
        }
        log_debug("MQTT message published to %s", topic);
        return 0;
    }
    
    log_error("MQTT publish failed: %d", rc);
    return -1;
}

int mqtt_publish_string(MqttClient *client, const char *topic, 
                        const char *message, MqttQos qos)
{
    if (!message) return -1;
    return mqtt_publish(client, topic, message, strlen(message), qos);
}

int mqtt_subscribe(MqttClient *client, const char *topic, MqttQos qos)
{
    if (!client || !client->internal || !topic) return -1;
    
    if (!mqtt_is_connected(client)) {
        log_warn("MQTT not connected, cannot subscribe");
        return -1;
    }
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    int rc = MQTTClient_subscribe(internal->mqtt_client, topic, qos);
    
    if (rc == MQTTCLIENT_SUCCESS) {
        log_info("MQTT subscribed to %s", topic);
        return 0;
    }
    
    log_error("MQTT subscribe failed: %d", rc);
    return -1;
}

int mqtt_unsubscribe(MqttClient *client, const char *topic)
{
    if (!client || !client->internal || !topic) return -1;
    
    if (!mqtt_is_connected(client)) {
        return -1;
    }
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    int rc = MQTTClient_unsubscribe(internal->mqtt_client, topic);
    return (rc == MQTTCLIENT_SUCCESS) ? 0 : -1;
}

void mqtt_on_connected(MqttClient *client, void (*callback)(MqttClient *))
{
    if (client) client->on_connected = callback;
}

void mqtt_on_disconnected(MqttClient *client, void (*callback)(MqttClient *))
{
    if (client) client->on_disconnected = callback;
}

void mqtt_on_message(MqttClient *client, 
                     void (*callback)(MqttClient *, const char *, const void *, size_t))
{
    if (client) client->on_message = callback;
}

void mqtt_on_state_changed(MqttClient *client, void (*callback)(MqttClient *, MqttState))
{
    if (client) client->on_state_changed = callback;
}

int mqtt_start(MqttClient *client)
{
    if (!client || !client->internal) return -1;
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    if (internal->running) return 0; /* 已启动 */
    
    /* 先连接 */
    if (mqtt_connect(client) != 0) {
        /* 连接失败，但启动后台线程进行重连 */
        log_warn("MQTT initial connection failed, will retry in background");
    }
    
    internal->running = 1;
    internal->last_reconnect_time = time(NULL);
    
    /* 创建后台线程 */
    if (pthread_create(&internal->thread, NULL, mqtt_thread_func, client) != 0) {
        log_error("Failed to create MQTT thread");
        internal->running = 0;
        return -1;
    }
    
    log_info("MQTT client started");
    return 0;
}

void mqtt_stop(MqttClient *client)
{
    if (!client || !client->internal) return;
    
    MqttInternal *internal = (MqttInternal *)client->internal;
    
    if (!internal->running) {
        /* 即使没有运行，也需要断开连接 */
        mqtt_disconnect(client);
        return;
    }
    
    log_info("Stopping MQTT client...");
    
    internal->running = 0;
    pthread_join(internal->thread, NULL);
    
    mqtt_disconnect(client);
    
    log_info("MQTT client stopped");
}

