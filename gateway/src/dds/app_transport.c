/**
 * @file app_transport.c
 * @brief 通信抽象层实现
 */

#include "app_transport.h"
#include "app_config.h"
#include "../thirdparty/log.c/log.h"
#include "app_mqtt_v2.h"
#include "app_dds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* 传输类型字符串 */
static const char *type_strings[] = {
    "mqtt", "dds", "auto"
};

const char *transport_type_to_string(TransportType type)
{
    if (type >= TRANSPORT_TYPE_MQTT && type <= TRANSPORT_TYPE_AUTO) {
        return type_strings[type];
    }
    return "unknown";
}

TransportType transport_string_to_type(const char *str)
{
    if (!str) return TRANSPORT_TYPE_MQTT;
    
    for (int i = 0; i <= TRANSPORT_TYPE_AUTO; i++) {
        if (strcasecmp(str, type_strings[i]) == 0) {
            return (TransportType)i;
        }
    }
    return TRANSPORT_TYPE_MQTT;
}

/* MQTT回调转发 */
static void mqtt_message_callback(MqttClient *client, const char *topic,
                                  const void *payload, size_t len)
{
    TransportManager *manager = (TransportManager *)client->user_data;
    if (manager && manager->on_message) {
        manager->on_message(manager, topic, payload, len);
    }
}

static void mqtt_state_callback(MqttClient *client, MqttState state)
{
    TransportManager *manager = (TransportManager *)client->user_data;
    if (!manager) return;
    
    TransportState tstate;
    switch (state) {
        case MQTT_STATE_CONNECTED:    tstate = TRANSPORT_STATE_CONNECTED; break;
        case MQTT_STATE_CONNECTING:   tstate = TRANSPORT_STATE_CONNECTING; break;
        case MQTT_STATE_RECONNECTING: tstate = TRANSPORT_STATE_CONNECTING; break;
        default:                      tstate = TRANSPORT_STATE_DISCONNECTED; break;
    }
    
    manager->state = tstate;
    
    if (manager->on_state_changed) {
        manager->on_state_changed(manager, tstate);
    }
}

/* DDS回调转发 */
static void dds_data_callback(DdsManager *dds, const char *topic,
                              const void *data, size_t len)
{
    TransportManager *manager = (TransportManager *)dds->user_data;
    if (manager && manager->on_message) {
        manager->on_message(manager, topic, data, len);
    }
}

int transport_init(TransportManager *manager, const TransportConfig *config)
{
    if (!manager) return -1;
    
    memset(manager, 0, sizeof(TransportManager));
    
    if (config) {
        memcpy(&manager->config, config, sizeof(TransportConfig));
    } else {
        /* 默认配置 */
        manager->config.type = TRANSPORT_TYPE_MQTT;
        strcpy(manager->config.mqtt_broker, "tcp://localhost:1883");
        strcpy(manager->config.mqtt_client_id, "gateway");
        manager->config.mqtt_keepalive = 60;
        manager->config.dds_domain_id = 0;
        manager->config.default_qos = 1;
        
        /* 默认话题配置 - MQTT */
        strcpy(manager->config.publish_topic, "gateway/data");
        strcpy(manager->config.subscribe_topic, "gateway/command");
        
        /* 默认话题配置 - DDS */
        strcpy(manager->config.dds_publish_topic, "GatewayData");
        strcpy(manager->config.dds_publish_type, "GatewayDataType");
        strcpy(manager->config.dds_subscribe_topic, "GatewayCommand");
        strcpy(manager->config.dds_subscribe_type, "GatewayCommandType");
    }
    
    manager->state = TRANSPORT_STATE_DISCONNECTED;
    manager->active_type = manager->config.type;
    /* 根据传输类型初始化 */
    if (manager->config.type == TRANSPORT_TYPE_MQTT || 
        manager->config.type == TRANSPORT_TYPE_AUTO) {
        /* 初始化MQTT */
        MqttConfig mqtt_cfg = {0};
        strncpy(mqtt_cfg.broker_url, manager->config.mqtt_broker, sizeof(mqtt_cfg.broker_url) - 1);
        strncpy(mqtt_cfg.client_id, manager->config.mqtt_client_id, sizeof(mqtt_cfg.client_id) - 1);
        mqtt_cfg.keepalive_interval = manager->config.mqtt_keepalive;
        mqtt_cfg.auto_reconnect = 1;
        mqtt_cfg.reconnect_min_interval = 1;
        mqtt_cfg.reconnect_max_interval = 60;
        mqtt_cfg.default_qos = manager->config.default_qos;
    
        MqttClient *mqtt = malloc(sizeof(MqttClient));
        if (mqtt && mqtt_init(mqtt, &mqtt_cfg) == 0) {
            mqtt->user_data = manager;  /* 用于回调转发 */
            mqtt_on_message(mqtt, mqtt_message_callback);
            mqtt_on_state_changed(mqtt, mqtt_state_callback);
            manager->mqtt_client = mqtt;
           log_debug("MQTT transport initialized");
        } else {
            log_warn("Failed to initialize MQTT transport");
            if (mqtt) free(mqtt);
        }
    }
    
    if (manager->config.type == TRANSPORT_TYPE_DDS) {
        /* 初始化DDS */
        DdsManager *dds = malloc(sizeof(DdsManager));
        if (dds) {
            DdsConfig dds_cfg = {0};
            dds_cfg.domain_id = manager->config.dds_domain_id;
            strncpy(dds_cfg.participant_name, manager->config.dds_participant_name, 
                    sizeof(dds_cfg.participant_name) - 1);
            dds_cfg.default_qos = dds_qos_default();
            dds_cfg.auto_discovery = 1;
        
            /* 配置DDS话题 */
            strncpy(dds_cfg.default_publish_topic, manager->config.dds_publish_topic,
                sizeof(dds_cfg.default_publish_topic) - 1);
            strncpy(dds_cfg.default_publish_type, manager->config.dds_publish_type,
                sizeof(dds_cfg.default_publish_type) - 1);
            strncpy(dds_cfg.default_subscribe_topic, manager->config.dds_subscribe_topic,
                sizeof(dds_cfg.default_subscribe_topic) - 1);
            strncpy(dds_cfg.default_subscribe_type, manager->config.dds_subscribe_type,
                sizeof(dds_cfg.default_subscribe_type) - 1);
        
            if (dds_init(dds, &dds_cfg) == 0) {
                dds->user_data = manager;
                dds_on_data_available(dds, dds_data_callback);
            
                /* 注册DDS默认话题 */
                dds_register_default_topics(dds);
            
                manager->dds_manager = dds;
                log_debug("DDS transport initialized");
            } 
            else {
                log_warn("DDS transport not available");
                free(dds);
                manager->dds_manager = NULL;
            }
        }
    }
    manager->is_initialized = 1;
    
    log_info("Transport initialized: type=%s", transport_type_to_string(manager->active_type));
    return 0;
}

int transport_init_from_config(TransportManager *manager, const char *config_file)
{
    if (!manager) return -1;
    
    ConfigManager config;
    if (config_init(&config, config_file) != 0) {
        log_warn("Failed to load config file, using defaults");
        return transport_init(manager, NULL);
    }
    
    config_load(&config);
    
    TransportConfig tconfig = {0};
    
    /* 读取传输类型 */
    char type_str[32] = "mqtt";
    config_get_string(&config, "transport", "type", "mqtt", type_str, sizeof(type_str));
    tconfig.type = transport_string_to_type(type_str);
    
    /* MQTT配置 */
    config_get_string(&config, "mqtt", "server", "tcp://localhost:1883",
                     tconfig.mqtt_broker, sizeof(tconfig.mqtt_broker));
    config_get_string(&config, "mqtt", "client_id", "gateway",
                     tconfig.mqtt_client_id, sizeof(tconfig.mqtt_client_id));
    tconfig.mqtt_keepalive = config_get_int(&config, "mqtt", "keepalive", 60);
    
    /* DDS配置 */
    tconfig.dds_domain_id = config_get_int(&config, "dds", "domain_id", 0);
    config_get_string(&config, "dds", "participant_name", "gateway",
                     tconfig.dds_participant_name, sizeof(tconfig.dds_participant_name));
    
    /* ========== 话题配置 ========== */
    
    /* MQTT话题 */
    config_get_string(&config, "mqtt", "publish_topic", "gateway/data",
                     tconfig.publish_topic, sizeof(tconfig.publish_topic));
    config_get_string(&config, "mqtt", "subscribe_topic", "gateway/command",
                     tconfig.subscribe_topic, sizeof(tconfig.subscribe_topic));
    
    /* DDS话题 */
    config_get_string(&config, "dds", "publish_topic", "GatewayData",
                     tconfig.dds_publish_topic, sizeof(tconfig.dds_publish_topic));
    config_get_string(&config, "dds", "publish_type", "GatewayDataType",
                     tconfig.dds_publish_type, sizeof(tconfig.dds_publish_type));
    config_get_string(&config, "dds", "subscribe_topic", "GatewayCommand",
                     tconfig.dds_subscribe_topic, sizeof(tconfig.dds_subscribe_topic));
    config_get_string(&config, "dds", "subscribe_type", "GatewayCommandType",
                     tconfig.dds_subscribe_type, sizeof(tconfig.dds_subscribe_type));
    
    tconfig.default_qos = config_get_int(&config, "transport", "default_qos", 1);
    
    config_destroy(&config);
    
    return transport_init(manager, &tconfig);
}
void transport_close(TransportManager *manager)
{
    if (!manager) return;
    
    transport_disconnect(manager);
    
    if (manager->mqtt_client) {
        mqtt_destroy((MqttClient *)manager->mqtt_client);
        free(manager->mqtt_client);
        manager->mqtt_client = NULL;
    }
    
    if (manager->dds_manager) {
        dds_close((DdsManager *)manager->dds_manager);
        free(manager->dds_manager);
        manager->dds_manager = NULL;
    }
    
    manager->is_initialized = 0;
    log_info("Transport closed");
}

int transport_connect(TransportManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    manager->state = TRANSPORT_STATE_CONNECTING;
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT && manager->mqtt_client) {
        return mqtt_start((MqttClient *)manager->mqtt_client);
    } else if (manager->active_type == TRANSPORT_TYPE_DDS && manager->dds_manager) {
        /* DDS不需要显式连接 */
        manager->state = TRANSPORT_STATE_CONNECTED;
        return 0;
    }
    
    return -1;
}

void transport_disconnect(TransportManager *manager)
{
    if (!manager) return;
    
    if (manager->mqtt_client) {
        mqtt_stop((MqttClient *)manager->mqtt_client);
    }
    
    manager->state = TRANSPORT_STATE_DISCONNECTED;
}

TransportState transport_get_state(TransportManager *manager)
{
    return manager ? manager->state : TRANSPORT_STATE_DISCONNECTED;
}

int transport_is_connected(TransportManager *manager)
{
    if (!manager) return 0;
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT && manager->mqtt_client) {
        return mqtt_is_connected((MqttClient *)manager->mqtt_client);
    } else if (manager->active_type == TRANSPORT_TYPE_DDS && manager->dds_manager) {
        return dds_is_enabled((DdsManager *)manager->dds_manager);
    }
    
    return 0;
}

int transport_publish(TransportManager *manager, const char *topic,
                      const void *data, size_t len)
{
    if (!manager || !topic || !data) return -1;
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT && manager->mqtt_client) {
        return mqtt_publish((MqttClient *)manager->mqtt_client, topic, 
                           data, len, manager->config.default_qos);
    } else if (manager->active_type == TRANSPORT_TYPE_DDS && manager->dds_manager) {
        return dds_publish((DdsManager *)manager->dds_manager, topic, data, len);
    }
    
    log_error("No active transport");
    return -1;
}

int transport_publish_string(TransportManager *manager, const char *topic,
                             const char *message)
{
    if (!message) return -1;
    return transport_publish(manager, topic, message, strlen(message));
}

int transport_subscribe(TransportManager *manager, const char *topic)
{
    if (!manager || !topic) return -1;
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT && manager->mqtt_client) {
        return mqtt_subscribe((MqttClient *)manager->mqtt_client, 
                             topic, manager->config.default_qos);
    } else if (manager->active_type == TRANSPORT_TYPE_DDS && manager->dds_manager) {
        return dds_subscribe((DdsManager *)manager->dds_manager, topic);
    }
    
    return -1;
}

int transport_unsubscribe(TransportManager *manager, const char *topic)
{
    if (!manager || !topic) return -1;
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT && manager->mqtt_client) {
        return mqtt_unsubscribe((MqttClient *)manager->mqtt_client, topic);
    } else if (manager->active_type == TRANSPORT_TYPE_DDS && manager->dds_manager) {
        return dds_unsubscribe((DdsManager *)manager->dds_manager, topic);
    }
    
    return -1;
}

int transport_switch_type(TransportManager *manager, TransportType type)
{
    if (!manager || !manager->is_initialized) return -1;
    
    if (type == manager->active_type) {
        return 0;
    }
    
    log_info("Switching transport from %s to %s",
             transport_type_to_string(manager->active_type),
             transport_type_to_string(type));
    
    /* 断开当前连接 */
    transport_disconnect(manager);
    
    /* 切换类型 */
    manager->active_type = type;
    
    /* 重新连接 */
    return transport_connect(manager);
}

TransportType transport_get_type(TransportManager *manager)
{
    return manager ? manager->active_type : TRANSPORT_TYPE_MQTT;
}

void transport_on_message(TransportManager *manager,
                          void (*callback)(TransportManager *, const char *, const void *, size_t))
{
    if (manager) {
        manager->on_message = callback;
    }
}

void transport_on_state_changed(TransportManager *manager,
                                void (*callback)(TransportManager *, TransportState))
{
    if (manager) {
        manager->on_state_changed = callback;
    }
}

int transport_subscribe_default(TransportManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT && manager->mqtt_client) {
        /* MQTT订阅 */
        const char *topic = manager->config.subscribe_topic;
        if (topic[0]) {
            log_info("MQTT subscribing to command topic: %s", topic);
            return mqtt_subscribe((MqttClient *)manager->mqtt_client, 
                                  topic, manager->config.default_qos);
        }
    } else if (manager->active_type == TRANSPORT_TYPE_DDS && manager->dds_manager) {
        /* DDS订阅 */
        return dds_subscribe_default((DdsManager *)manager->dds_manager);
    }
    
    return -1;
}

int transport_publish_default(TransportManager *manager,
                               const void *data, size_t len)
{
    if (!manager || !data) return -1;
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT && manager->mqtt_client) {
        /* MQTT发布 */
        const char *topic = manager->config.publish_topic[0] ? 
                            manager->config.publish_topic : "gateway/data";

        return mqtt_publish((MqttClient *)manager->mqtt_client, topic, 
                           data, len, manager->config.default_qos);
    } else if (manager->active_type == TRANSPORT_TYPE_DDS && manager->dds_manager) {
        /* DDS发布 */
        return dds_publish_default((DdsManager *)manager->dds_manager, data, len);
    }
    
    return -1;
}

const char *transport_get_publish_topic(TransportManager *manager)
{
    if (!manager) return "gateway/data";
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT) {
        return manager->config.publish_topic[0] ? 
               manager->config.publish_topic : "gateway/data";
    } else {
        return manager->config.dds_publish_topic[0] ? 
               manager->config.dds_publish_topic : "GatewayData";
    }
}

const char *transport_get_subscribe_topic(TransportManager *manager)
{
    if (!manager) return "gateway/command";
    
    if (manager->active_type == TRANSPORT_TYPE_MQTT) {
        return manager->config.subscribe_topic[0] ? 
               manager->config.subscribe_topic : "gateway/command";
    } else {
        return manager->config.dds_subscribe_topic[0] ? 
               manager->config.dds_subscribe_topic : "GatewayCommand";
    }
}