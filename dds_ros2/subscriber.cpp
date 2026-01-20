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
    dds::domain::DomainParticipant participant(dds::core::null);
    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    std::cout << "Created Domain Participant." << std::endl;

    // --- 2. 创建 Topic ---
    std::string topic = topic_name;
    dds::topic::Topic<unitree_go::msg::dds_::HelloWorldData_> topic(participant, topic);
    std::cout << "Subscribing to topic: " << topic << std::endl;

    // --- 3. 创建 Subscriber ---
    dds::sub::Subscriber subscriber(participant);

    // --- 5. 创建 DataReader ---
    dds::sub::DataReader<unitree_go::msg::dds_::HelloWorldData_> reader(subscriber, topic);
    std::cout << "Created DataReader for topic '" << topic_name << "'. Ready to receive messages..." << std::endl;

    // --- 6. 订阅消息 ---
    // 创建一个 LoanedSamples 容器来接收数据
    dds::sub::LoanedSamples<unitree_go::msg::dds_::HelloWorldData_> samples;

    while (true) {
        try {
            // 从DataReader中获取可用的样本
            // 此调用默认是非阻塞的
            samples = reader.take();

            // 遍历接收到的样本
            for (auto sample_it = samples.begin(); sample_it != samples.end(); ++sample_it) {
                // 检查样本状态（INFO、ALIVE等）
                const dds::sub::SampleInfo& info = sample_it.info();
                if (info.valid()) { // 只处理有效的样本
                    // 获取实际的数据载荷
                    const unitree_go::msg::dds_::HelloWorldData_& msg = sample_it.data();

                    // 使用自动生成的getter方法打印接收到的消息内容
                    std::cout << "Received message: user_id=" << msg.user_id_()
                              << ", message='" << msg.message_() << "'" << std::endl;
                } else {
                    // 处理其他状态，如NOT_ALIVE_DISPOSED、NOT_ALIVE_UNREGISTERED
                    std::cout << "Received sample with invalid info state." << std::endl;
                }
            }// samples在此处超出作用域，会自动归还给中间件
        } catch (const dds::core::Exception& e) {
            std::cerr << "Error reading data: " << e.what() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠10ms
    }
    return 0;
}