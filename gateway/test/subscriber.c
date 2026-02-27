/* dds_subscriber.c - 订阅 GatewayData 话题 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dds/dds.h>
#include "GatewayData.h"

#define TOPIC_NAME    "GatewayData"

/* 数据接收回调 */
static void data_available(dds_entity_t reader)
{
    Gateway_DataType msg;
    void *samples[1] = { &msg };
    dds_sample_info_t infos[1];
    
    int ret = dds_take(reader, samples, infos, 1, 1);
    if (ret > 0 && infos[0].valid_data) {
        printf("\n========== 收到网关数据 ==========\n");
        printf("数据长度: %u 字节\n", msg.length);
        printf("数据内容: ");
        for (uint32_t i = 0; i < msg.length && i < 64; i++) {
            printf("%02X ", msg.data[i]);
        }
        printf("\n");
        
        if (msg.length > 3) {
            uint8_t conn_type = msg.data[0];
            uint8_t id_len = msg.data[1];
            uint8_t data_len = msg.data[2];
            printf("连接类型: %u, ID长度: %u, 数据长度: %u\n", 
                   conn_type, id_len, data_len);
        }
        printf("==================================\n");
    }
}

int main(int argc, char *argv[])
{
    dds_entity_t participant;
    dds_entity_t topic;
    dds_entity_t reader;
    dds_entity_t waitset;
    dds_entity_t readcond;
    
    printf("DDS 订阅者启动\n");
    printf("订阅话题: %s\n", TOPIC_NAME);
    
    /* 创建参与者 */
    participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
    if (participant < 0) {
        fprintf(stderr, "创建参与者失败: %s\n", dds_strretcode(participant));
        return -1;
    }
    
    /* 创建话题 - 使用 GatewayData.h 中的 descriptor */
    topic = dds_create_topic(participant, &Gateway_DataType_desc, TOPIC_NAME, NULL, NULL);
    if (topic < 0) {
        fprintf(stderr, "创建话题失败: %s\n", dds_strretcode(topic));
        dds_delete(participant);
        return -1;
    }
    
    /* 创建读取者 */
    dds_qos_t *qos = dds_create_qos();
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(10));
    reader = dds_create_reader(participant, topic, qos, NULL);
    dds_delete_qos(qos);
    
    if (reader < 0) {
        fprintf(stderr, "创建读取者失败: %s\n", dds_strretcode(reader));
        dds_delete(participant);
        return -1;
    }
    
    /* 创建等待集 */
    waitset = dds_create_waitset(participant);
    readcond = dds_create_readcondition(reader, DDS_ANY_STATE);
    dds_waitset_attach(waitset, readcond, reader);
    
    printf("等待网关数据...\n\n");
    
    /* 主循环 */
    while (1) {
        dds_attach_t triggered;
        int ret = dds_waitset_wait(waitset, &triggered, 1, DDS_SECS(10));
        if (ret > 0) {
            data_available(reader);
        }
    }
    
    /* 清理 */
    dds_delete(participant);
    return 0;
}
