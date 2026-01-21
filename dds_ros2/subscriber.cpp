#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include "dds/dds.hpp"

// 包含生成的 C++ 消息头文件，使用ros2生成的HelloWorldData_.idl文件生成
#include "HelloWorldData_.hpp" 

#define topic_name "rt/hello_world_receiver"

int main() {

        // --- 1. 创建 Domain Participant ---
        // 使用默认域 (Domain ID 0)
        dds::domain::DomainParticipant participant(org::eclipse::cyclonedds::domain::default_id());
        std::cout << "Created Domain Participant." << std::endl;

        // --- 2. 创建 Topic ---
        // 修复：变量名冲突，将字符串变量改为topic_name_str
        std::string topic_name_str = topic_name;
        dds::topic::Topic<unitree_go::msg::dds_::HelloWorldData_> topic(participant, topic_name_str);
        std::cout << "Subscribing to topic: " << topic.name() << std::endl;

        // --- 3. 创建 Subscriber ---
        dds::sub::Subscriber subscriber(participant);

        // --- 4. 创建 DataReader ---
        // 修复：传入正确的Topic对象，解决重载调用歧义
        dds::sub::DataReader<unitree_go::msg::dds_::HelloWorldData_> reader(subscriber, topic);
        std::cout << "Created DataReader for topic '" << topic_name << "'. Ready to receive messages..." << std::endl;

        // --- 5. 订阅消息 ---
        while (true) {
            // 从DataReader中获取可用的样本（非阻塞调用）
            dds::sub::LoanedSamples<unitree_go::msg::dds_::HelloWorldData_> samples = reader.take();

            // 遍历接收到的样本
            for (auto& sample : samples) {
                // 检查样本状态（INFO、ALIVE等）
                const dds::sub::SampleInfo& info = sample.info();
                if (info.valid()) { // 只处理有效的样本
                    // 获取实际的数据载荷
                    const unitree_go::msg::dds_::HelloWorldData_& msg = sample.data();

                    // 使用自动生成的getter方法打印接收到的消息内容
                    std::cout << "Received message: user_id=" << msg.user_id_()
                            << ", message='" << msg.message_() << "'" << std::endl;
                } else {
                    // 处理其他状态，如NOT_ALIVE_DISPOSED、NOT_ALIVE_UNREGISTERED
                    std::cout << "Received sample with invalid info state." << std::endl;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠10ms
    
}
return 0;
}