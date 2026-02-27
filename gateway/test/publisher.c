/* dds_publisher.c - 发布到 GatewayCommand 话题 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dds/dds.h>
#include "GatewayData.h"

#define TOPIC_NAME    "GatewayCommand"

int main(int argc, char *argv[])
{
    dds_entity_t participant;
    dds_entity_t topic;
    dds_entity_t writer;
    
    printf("DDS 发布者启动\n");
    printf("发布话题: %s\n", TOPIC_NAME);
    
    /* 创建参与者 */
    participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
    if (participant < 0) {
        fprintf(stderr, "创建参与者失败: %s\n", dds_strretcode(participant));
        return -1;
    }
    
    /* 创建话题 - 使用 GatewayData.h 中的 descriptor */
    topic = dds_create_topic(participant, &Gateway_CommandType_desc, TOPIC_NAME, NULL, NULL);
    if (topic < 0) {
        fprintf(stderr, "创建话题失败: %s\n", dds_strretcode(topic));
        dds_delete(participant);
        return -1;
    }
    
    /* 创建写入者 */
    dds_qos_t *qos = dds_create_qos();
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(10));
    writer = dds_create_writer(participant, topic, qos, NULL);
    dds_delete_qos(qos);
    
    if (writer < 0) {
        fprintf(stderr, "创建写入者失败: %s\n", dds_strretcode(writer));
        dds_delete(participant);
        return -1;
    }
    
    /* 等待订阅者 */
    printf("等待订阅者...\n");
    dds_sleepfor(DDS_SECS(1));
    
    /* 发送测试消息 */
    Gateway_CommandType msg;
    memset(&msg, 0, sizeof(msg));
    
    /* 构造消息: connection_type(1) + id_len(1) + data_len(1) + id(2) + data(N) */
    msg.data[0] = 2;  /* connection_type = BLE_MESH */
    msg.data[1] = 2;  /* id_len = 2 */
    msg.data[2] = 10; /* data_len = 10 */
    msg.data[3] = 0x00;  /* id[0] */
    msg.data[4] = 0x01;  /* id[1] */
    memcpy(&msg.data[5], "HELLO_TEST", 10);
    msg.length = 15;
    
    printf("\n发送消息:\n");
    printf("  connection_type: %u\n", msg.data[0]);
    printf("  id_len: %u\n", msg.data[1]);
    printf("  data_len: %u\n", msg.data[2]);
    printf("  id: %02X%02X\n", msg.data[3], msg.data[4]);
    printf("  data: %s\n", &msg.data[5]);
    printf("  总长度: %u 字节\n", msg.length);
    
    int ret = dds_write(writer, &msg);
    if (ret != DDS_RETCODE_OK) {
        fprintf(stderr, "发送失败: %s\n", dds_strretcode(ret));
    } else {
        printf("消息发送成功!\n");
    }
    
    /* 等待消息被接收 */
    dds_sleepfor(DDS_SECS(2));
    
    /* 清理 */
    dds_delete(participant);
    return 0;
}
