#include "app_dds.h"
#include "../thirdparty/log.c/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



#ifdef USE_CYCLONE_DDS
#include <dds/dds.h>
#define DDS_ENABLED 1
#include "GatewayData.h"
#define MAX_DATA_LEN 4096 
#else
#define DDS_ENABLED 0
#endif

typedef struct DdsTopicInfo {
    char name[128];
    char type_name[64];
    DdsQosPolicy qos;
#ifdef USE_CYCLONE_DDS
    dds_entity_t topic;
    dds_entity_t writer;
    dds_entity_t reader;
#else
    void *topic;
    void *writer;
    void *reader;
#endif
    int is_registered;
} DdsTopicInfo;

#define MAX_TOPICS 32

typedef struct DdsInternal {
    DdsTopicInfo topics[MAX_TOPICS];
    int topic_count;
#ifdef USE_CYCLONE_DDS
    dds_entity_t participant;
    dds_entity_t publisher;
    dds_entity_t subscriber;
#endif
} DdsInternal;

DdsQosPolicy dds_qos_default(void)
{
    DdsQosPolicy qos = {
        .reliability = 0,
        .durability = 0,
        .history_depth = 1,
        .deadline_ms = 0,
        .lifespan_ms = 0
    };
    return qos;
}

DdsQosPolicy dds_qos_reliable(void)
{
    DdsQosPolicy qos = {
        .reliability = 1,
        .durability = 1,
        .history_depth = 10,
        .deadline_ms = 1000,
        .lifespan_ms = 60000
    };
    return qos;
}

static DdsTopicInfo *find_topic(DdsManager *manager, const char *name)
{
    DdsInternal *internal = (DdsInternal *)manager->participant;
    if (!internal) return NULL;
    
    for (int i = 0; i < internal->topic_count; i++) {
        if (strcmp(internal->topics[i].name, name) == 0) {
            return &internal->topics[i];
        }
    }
    return NULL;
}

int dds_unsubscribe(DdsManager *manager, const char *topic_name)
{
    if (!manager || !topic_name) return -1;
    
#if DDS_ENABLED
    DdsInternal *internal = (DdsInternal *)manager->participant;
    DdsTopicInfo *info = find_topic(manager, topic_name);
    
    if (!info) {
        log_warn("Topic not registered: %s", topic_name);
        return -1;
    }
    
    if (info->reader) {
        dds_delete(info->reader);
        info->reader = NULL;
    }
#endif
    
    log_info("DDS unsubscribed from: %s", topic_name);
    return 0;
}


int dds_init(DdsManager *manager, const DdsConfig *config)
{
    if (!manager) return -1;
    
    memset(manager, 0, sizeof(DdsManager));
    
    if (config) {
        memcpy(&manager->config, config, sizeof(DdsConfig));
    } else {
        manager->config.domain_id = 0;
        strcpy(manager->config.participant_name, "gateway");
        manager->config.default_qos = dds_qos_default();
        manager->config.auto_discovery = 1;
    }
    
#if DDS_ENABLED
    DdsInternal *internal = calloc(1, sizeof(DdsInternal));
    if (!internal) {
        log_error("Failed to allocate DDS internal structure");
        return -1;
    }
    
    internal->participant = dds_create_participant(
        manager->config.domain_id, NULL, NULL);
    
    if (internal->participant < 0) {
        log_error("Failed to create DDS participant");
        free(internal);
        return -1;
    }
    
    internal->publisher = dds_create_publisher(internal->participant, NULL, NULL);
    internal->subscriber = dds_create_subscriber(internal->participant, NULL, NULL);
    
    manager->participant = internal;
    manager->publisher = &internal->publisher;
    manager->subscriber = &internal->subscriber;
    
    log_info("DDS initialized: domain=%d", manager->config.domain_id);
#else
    log_warn("DDS not enabled");
    manager->participant = NULL;
#endif
    
    manager->state = DDS_STATE_ENABLED;
    manager->is_initialized = 1;
    
    return 0;
}

int dds_init_default(DdsManager *manager, int domain_id)
{
    DdsConfig config = {
        .domain_id = domain_id,
        .auto_discovery = 1
    };
    strcpy(config.participant_name, "gateway");
    config.default_qos = dds_qos_default();
    
    return dds_init(manager, &config);
}

int dds_register_topic(DdsManager *manager, const DdsTopic *topic)
{
    if (!manager || !topic) return -1;
    
    DdsInternal *internal = (DdsInternal *)manager->participant;
    if (!internal) return -1;
    
    if (internal->topic_count >= MAX_TOPICS) {
        log_error("Max topics reached");
        return -1;
    }
    
    DdsTopicInfo *info = &internal->topics[internal->topic_count];
    strncpy(info->name, topic->name, sizeof(info->name) - 1);
    strncpy(info->type_name, topic->type_name, sizeof(info->type_name) - 1);
    info->qos = topic->qos;
    info->is_registered = 1;
    info->writer = 0;
    info->reader = 0;
    
#if DDS_ENABLED
    dds_topic_descriptor_t *desc = NULL;
    if (strcmp(topic->name, "GatewayData") == 0) {
        desc = (dds_topic_descriptor_t*)&Gateway_DataType_desc;
    } else if (strcmp(topic->name, "GatewayCommand") == 0) {
        desc = (dds_topic_descriptor_t*)&Gateway_CommandType_desc;
    }
    
    if (desc) {
        info->topic = dds_create_topic(internal->participant, desc, topic->name, NULL, NULL);
        if (info->topic < 0) {
            log_error("Failed to create DDS topic: %s", topic->name);
            info->topic = 0;
        } else {
            log_debug("DDS topic created: %s", topic->name);
        }
    } else {
        log_warn("Unknown topic type: %s", topic->name);
        info->topic = 0;
    }
#else
    info->topic = NULL;
#endif
    
    internal->topic_count++;
    
    log_info("DDS topic registered: %s", topic->name);
    return 0;
}

int dds_unregister_topic(DdsManager *manager, const char *topic_name)
{
    if (!manager || !topic_name) return -1;
    
    DdsInternal *internal = (DdsInternal *)manager->participant;
    if (!internal) return -1;
    
    for (int i = 0; i < internal->topic_count; i++) {
        if (strcmp(internal->topics[i].name, topic_name) == 0) {
#if DDS_ENABLED
            if (internal->topics[i].writer) {
                dds_delete(internal->topics[i].writer);
            }
            if (internal->topics[i].reader) {
                dds_delete(internal->topics[i].reader);
            }
            if (internal->topics[i].topic) {
                dds_delete(internal->topics[i].topic);
            }
#endif
            // 移动后面的元素
            for (int j = i; j < internal->topic_count - 1; j++) {
                internal->topics[j] = internal->topics[j + 1];
            }
            internal->topic_count--;
            log_info("DDS topic unregistered: %s", topic_name);
            return 0;
        }
    }
    return -1;
}

void dds_on_data_available(DdsManager *manager,
                           void (*callback)(DdsManager *, const char *, const void *, size_t))
{
    if (manager) {
        manager->on_data_available = callback;
    }
}

void dds_on_subscriber_matched(DdsManager *manager,
                               void (*callback)(DdsManager *, const char *))
{
    if (manager) {
        manager->on_subscriber_matched = callback;
    }
}

void dds_on_state_changed(DdsManager *manager,
                          void (*callback)(DdsManager *, DdsState))
{
    if (manager) {
        manager->on_state_changed = callback;
    }
}

void dds_close(DdsManager *manager)
{
    if (!manager || !manager->is_initialized) return;
    
#if DDS_ENABLED
    DdsInternal *internal = (DdsInternal *)manager->participant;
    if (internal) {
        if (internal->participant > 0) {
            dds_delete(internal->participant);
        }
        free(internal);
    }
#endif
    
    manager->is_initialized = 0;
    manager->state = DDS_STATE_DISABLED;
    
    log_info("DDS closed");
}

int dds_publish(DdsManager *manager, const char *topic_name,
                const void *data, size_t len)
{
    if (!manager || !topic_name || !data) return -1;
    
    if (!manager->is_initialized || manager->state != DDS_STATE_ENABLED) {
        log_warn("DDS not enabled");
        return -1;
    }
    
#if DDS_ENABLED
    DdsInternal *internal = (DdsInternal *)manager->participant;
    DdsTopicInfo *info = find_topic(manager, topic_name);
    
    if (!info) {
        log_error("Topic not registered: %s", topic_name);
        return -1;
    }
    
    if (!info->writer) {
        info->writer = dds_create_writer(internal->publisher, info->topic, NULL, NULL);
        if (info->writer < 0) {
            log_error("Failed to create writer");
            return -1;
        }
    }
    
    /* 封装数据到结构体 */
    Gateway_DataType msg;
    memset(&msg, 0, sizeof(msg));
    msg.length = (len > MAX_DATA_LEN) ? MAX_DATA_LEN : (uint32_t)len; 
    memcpy(msg.data, data, msg.length);
    
    /* 发送结构体 */
    int ret = dds_write(info->writer, &msg);
    if (ret != DDS_RETCODE_OK) {
        log_error("Failed to write to topic: %s", topic_name);
        return -1;
    }
    
    log_debug("DDS published to %s: %zu bytes", topic_name, len);
    return 0;
#else
    log_debug("DDS publish (stub): topic=%s, len=%zu", topic_name, len);
    return 0;
#endif
}

#if DDS_ENABLED
/* 根据 reader 查找话题名 */
static const char* find_topic_name_by_reader(DdsManager *manager, dds_entity_t reader)
{
    DdsInternal *internal = (DdsInternal *)manager->participant;
    
    for (int i = 0; i < internal->topic_count; i++) {
        if (internal->topics[i].reader == reader) {
            return internal->topics[i].name;
        }
    }
    return "unknown";
}

/* 数据到达回调 - DDS 内部线程调用 */
static void on_dds_data_available(dds_entity_t reader, void *arg)
{
    DdsManager *manager = (DdsManager *)arg;
    Gateway_DataType msg;
    void *samples[1] = { &msg };
    dds_sample_info_t infos[1];
    
    int n = dds_take(reader, samples, infos, 1, 1);
    if (n > 0 && infos[0].valid_data) {
        const char *topic_name = find_topic_name_by_reader(manager, reader);
        
        log_debug("DDS received from %s: %u bytes", topic_name, msg.length);
        
        /* 调用用户回调 */
        if (manager->on_data_available) {
            manager->on_data_available(manager, topic_name, msg.data, msg.length);
        }
    }
}
#endif

/* ========== 修改 dds_subscribe ========== */

int dds_subscribe(DdsManager *manager, const char *topic_name)
{
    if (!manager || !topic_name) return -1;
    
#if DDS_ENABLED
    DdsInternal *internal = (DdsInternal *)manager->participant;
    DdsTopicInfo *info = find_topic(manager, topic_name);
    
    if (!info) {
        log_error("Topic not registered: %s", topic_name);
        return -1;
    }
    
    if (!info->reader) {
        /* 创建 listener，设置数据到达回调 */
        dds_listener_t *listener = dds_create_listener(manager);
        dds_lset_data_available(listener, on_dds_data_available);
        
        /* 创建 reader，绑定 listener */
        info->reader = dds_create_reader(internal->subscriber, info->topic, NULL, listener);
        
        if (info->reader < 0) {
            dds_delete_listener(listener);
            log_error("Failed to create reader: %s", dds_strretcode(info->reader));
            return -1;
        }
        
        /* listener 被 reader 接管，不需要手动删除 */
        log_debug("DDS reader created with listener for: %s", topic_name);
    }
#endif
    
    log_info("DDS subscribed to: %s", topic_name);
    return 0;
}

DdsState dds_get_state(DdsManager *manager)
{
    return manager ? manager->state : DDS_STATE_DISABLED;
}

int dds_is_enabled(DdsManager *manager)
{
    return manager && manager->is_initialized && manager->state == DDS_STATE_ENABLED;
}

/**
 * @brief 注册默认话题（发布和订阅）
 */
int dds_register_default_topics(DdsManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    DdsConfig *cfg = &manager->config;
    
    /* 注册发布话题 */
    if (cfg->default_publish_topic[0]) {
        DdsTopic topic = {0};
        strncpy(topic.name, cfg->default_publish_topic, sizeof(topic.name) - 1);
        strncpy(topic.type_name, cfg->default_publish_type[0] ? 
                cfg->default_publish_type : "GatewayData", sizeof(topic.type_name) - 1);
        topic.qos = dds_qos_reliable();  /* 使用可靠QoS */
        
        if (dds_register_topic(manager, &topic) != 0) {
            log_warn("Failed to register DDS publish topic: %s", topic.name);
        } else {
            log_info("DDS publish topic registered: %s (type: %s)", 
                     topic.name, topic.type_name);
        }
    }
    
    /* 注册订阅话题 */
    if (cfg->default_subscribe_topic[0]) {
        DdsTopic topic = {0};
        strncpy(topic.name, cfg->default_subscribe_topic, sizeof(topic.name) - 1);
        strncpy(topic.type_name, cfg->default_subscribe_type[0] ? 
                cfg->default_subscribe_type : "GatewayCommand", sizeof(topic.type_name) - 1);
        topic.qos = dds_qos_reliable();  /* 使用可靠QoS */
        
        if (dds_register_topic(manager, &topic) != 0) {
            log_warn("Failed to register DDS subscribe topic: %s", topic.name);
        } else {
            log_info("DDS subscribe topic registered: %s (type: %s)", 
                     topic.name, topic.type_name);
        }
    }
    
    return 0;
}

/**
 * @brief 订阅默认话题
 */
int dds_subscribe_default(DdsManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    const char *topic = manager->config.default_subscribe_topic;
    if (!topic[0]) {
        log_warn("DDS subscribe topic not configured");
        return -1;
    }
    
    log_info("DDS subscribing to: %s", topic);
    return dds_subscribe(manager, topic);
}

/**
 * @brief 发布到默认话题
 */
int dds_publish_default(DdsManager *manager, const void *data, size_t len)
{
    if (!manager || !data) return -1;
    
    const char *topic = manager->config.default_publish_topic;
    if (!topic[0]) {
        log_warn("DDS publish topic not configured");
        return -1;
    }
    
    return dds_publish(manager, topic, data, len);
}