#include <iostream>
#include <thread>
#include <chrono>
#include <cstring> 
#include "dds/dds.hpp"

// 包含生成的 C++ 消息头文件，使用ros2生成的HelloWorldData_.idl文件生成
#include "HelloWorldData_.hpp"



int main() {
    // --- 1. 创建 Domain Participant ---
    // 使用默认域 (Domain ID 0)
    dds::domain::DomainParticipant participant(dds::core::null);
    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    std::cout << "Created Domain Participant" << std::endl;

    // --- 2. 定义 Topic 名称 ---
    // 必须使用 rt/ 前缀，rt/前缀在ros2中会默认去除
    std::string topic_name = "rt/hello_world_receiver";
    std::cout << "Creating topic '" << topic_name << "'" << std::endl;

    // --- 3. 创建话题 ---
    dds::topic::Topic<unitree_go::msg::dds_::HelloWorldData_> topic(participant, topic_name);

    // --- 4. 创建发布者 ---
    dds::pub::Publisher publisher(participant);

    // --- 5. 创建数据写入者 ---
    dds::pub::DataWriter<unitree_go::msg::dds_::HelloWorldData_> writer(publisher, topic);

	/*
    // --- 6. 等待匹配的 Subscriber ---
    // 获取 DataWriter 的 Status Condition
    dds::core::cond::StatusCondition status_condition(writer);
    // 设置感兴趣的条件 (Publication Matched)
    status_condition.enabled_statuses(dds::core::status::StatusMask::publication_matched());

    // 创建 WaitSet 并附加上状态条件
    dds::core::cond::WaitSet waitset;
    waitset += status_condition;

    // 循环等待直到有匹配的订阅者
    while (true) {
        auto conditions = waitset.wait(dds::core::Duration::from_millisecs(100)); // 等待 100ms
        for (const auto& cond : conditions) {
            if (cond == status_condition) {
                 dds::core::status::PublicationMatchedStatus matches = writer.publication_matched_status();
                 if (matches.current_count() > 0) {
                     std::cout << "Found " << matches.current_count() << " matching subscriber(s). Starting to publish..." << std::endl;
                     goto found_subscriber; // 跳出循环
                 }
            }
        }
    }
    found_subscriber:*/

    int count = 0;
    // --- 7. 发布消息 ---
    unitree_go::msg::dds_::HelloWorldData_ msg_instance; // 实例化生成的消息类型
    while (true) {
        // 填充消息内容
        // 根据 HelloWorldData_.hpp 文件中 HelloWorldData_ 结构的定义来设置字段
        // 字段名为 user_id_ 和 message_ (使用生成的 getter/setter 函数)
        msg_instance.user_id_(count); // 设置 user_id_

        // --- 关键：设置 message_ 字段 ---
        // 生成的类型中，message_ 成员是 std::string
        std::string data_str = "Hello from Cyclone DDS!";
        msg_instance.message_(data_str); // 设置 message_

        std::cout << "Publish message: user_id=" << msg_instance.user_id_() << ", message='" << msg_instance.message_() << "'" << std::endl;

        // 写入消息
        writer.write(msg_instance);

        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // 等待 1.5 秒
    }
    return 0;
}