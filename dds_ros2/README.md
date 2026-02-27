# 原生cyclonedds与ROS2_humble通信

- 原生cyclonedds程序与ROS2的通信。ROS2_humble底层基于cyclonedds，因此通过简单适配即可实现二者的通信。
- 核心:提取ROS2生成的消息类型，并通过cyclonedds的`idlc`程序生成含消息类型的.cpp/.hpp文件

| 系统       | ROS2版本 | cyclonedds版本 | cyclonedds-cxx版本 |
| :--------- | :------- | :------------- | :----------------- |
| Ubuntu 22.04 | humble   | 0.10.2         | 0.10.2             |

## 流程

1.  安装ROS2_humble版本并转为cyclonedds
2.  安装cyclonedds、cyclonedds-cxx库
3.  导入消息包到ROS2
4.  提取ROS2生成的`xxx_.idl`文件并通过cyclonedds的`idlc`程序生成含消息类型的.cpp/.hpp文件
5.  基于生成的源文件/头文件实现通信

### 1. 安装ROS2_humble版本并转为cyclonedds

```bash
# 安装依赖
$ sudo apt install ros-humble-rmw-cyclonedds-cpp
$ sudo apt install ros-humble-rosidl-generator-dds-idl

# 添加GPG密钥
$ sudo apt update && sudo apt install curl gnupg lsb-release
$ sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg

# 添加ROS2软件源
$ echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(source /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

# 安装ROS2
$ sudo apt install ros-humble-ros-base

# 配置环境
$ source /opt/ros/humble/setup.bash # 可依照需要添加到环境变量，但和cyclonedds库冲突时需取消

# 将ROS2从FastDDS切换为cyclonedds，推荐写入环境变量 ~/.bashrc
$ export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
```

### 2. 安装cyclonedds、cyclonedds-cxx库

```bash
# 克隆 cyclonedds 并切换到 v0.10.2
$ git clone git@github.com:eclipse-cyclonedds/cyclonedds.git /home/cyclonedds
$ cd /home/cyclonedds
$ git checkout 0.10.2
$ mkdir build; cd build

# 启用IDL编译器
$ cmake -DCMAKE_INSTALL_PREFIX=/home/cyclonedds/install -DBUILD_IDLC=ON -DDDS_SHM_TRANSPORT=OFF ..
$ make -j$(nproc)
$ make install


# 克隆 cyclonedds-cxx 并切换到 v0.10.2
$ git clone git@github.com:eclipse-cyclonedds/cyclonedds-cxx.git /home/cyclonedds-cxx
$ cd /home/cyclonedds-cxx
$ git checkout 0.10.2
$ mkdir build; cd build

# 启用IDL库
$ cmake -DCMAKE_INSTALL_PREFIX=/home/cyclonedds-cxx/install -DCMAKE_PREFIX_PATH="/home/cyclonedds/install" -DBUILD_IDLLIB=ON ..
$ make -j$(nproc)
$ make install
```

### 3.安装iceoryx
```bash
# 根据猜测，只需要用到该仓库的iceoryx_binding_c/inclue/iceoryx_binding_c/的头文件
# 但是为了稳妥还是编译安装该库，记得在项目CMakeLists.txt中添加上述的头文件路径
# 该库疑似因为cyclonedds的shm功能需要，可尝试在安装的时候禁用shm：-DDDS_SHM_TRANSPORT=OFF
# 笔者测试禁用未成功
$ git clone https://github.com/eclipse-iceoryx/iceoryx.git /home/iceoryx #克隆iceoryx
$ cd iceoryx
$ ICEORYX_DIR=$PWD
$ mkdir build && cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
$ make -j$(nproc)
$ sudo make install

$ git clone https://github.com/mirror/ncurses.git #iceoryx依赖的ncurses库
$ cd ncurses
$ git checkout v6.2
$ ./configure  --prefix=$ICEORYX_DIR/build/dependencies/ --exec-prefix=$ICEORYX_DIR/build/dependencies/ --with-termlib
$ make -j$(nproc)
$ make install

```

### 4. 导入消息包到ROS2

```bash
# 编译 cyclone-dds，该库是安装在消息包内部的
$ cd /home/airs_ros2/cyclonedds_ws/src

# 克隆cyclonedds仓库
$ git clone git@github.com:ros2/rmw_cyclonedds.git -b humble
$ git clone git@github.com:eclipse-cyclonedds/cyclonedds.git -b releases/0.10.x
$ cd ..

# 编译前要取消ROS2环境，注释掉 source /opt/ros/humble/setup.bash，建议新建终端进行编译
$ colcon build --packages-select cyclonedds # 编译cyclonedds
```

```bash
# 添加Test消息包，请留意工作空间下的readme
# 在airs_ros2/cyclonedds_ws/src/airs/msg/下新建Test.msg消息并填写消息字段
$ cd /home/airs_ros2/cyclonedds_ws/src/airs/msg/
$ vim Test.msg

# 编辑airs_ros2/cyclonedds_ws/src/airs下的CMakeLists.txt, 加入Test.msg
$ cd ..; vim CMakeLists.txt
```

```bash
# 编译消息包
$ source /opt/ros/humble/setup.bash # source ROS2环境变量
$ colcon build # 编译工作空间下的所有功能包

$ cd ..
$ vim setup.sh # 修改网卡名称、配置ros2环境，请勿忘记修改网卡名称
$ source setup.sh # 刷新环境

##############
# 若发生错误，一般是因为.sh脚本windows和linux的换行符不匹配，使用dos2unix一键修复
$ sudo apt update && sudo apt install dos2unix -y
$ dos2unix setup.sh
$ source setup.sh # 重新刷新
##############

$ ros2 interface list # 查看消息包
...
airs/msg/Test
...

$ ros2 interface show airs/msg/Test # 具体查看消息类型
  int64 user_id
  string message
```

### 5. 提取ROS2生成的`_*.idl`文件并通过cyclonedds的idlc程序生成含消息类型的.cpp/.hpp文件

```bash
# 获取ROS2根据Test消息生成的Test_.idl文件
# 注意是Test_.idl而不是Test.idl
$ ls /home/airs_ros2/cyclonedds_ws/build/airs/rosidl_generator_dds_idl/airs/msg/dds_connext
  HelloWorldData_.idl Test_.idl

# 复制到/home/airs_ros2目录
$ cp /home/airs_ros2/cyclonedds_ws/build/airs/rosidl_generator_dds_idl/airs/msg/dds_connext/Test_.idl /home/airs_ros2

# 临时添加cyclonedds和cyclonedds-cxx库的路径
$ export LD_LIBRARY_PATH=/home/cyclonedds/install/lib:$LD_LIBRARY_PATH
$ export LD_LIBRARY_PATH=/home/cyclonedds-cxx/install/lib:$LD_LIBRARY_PATH

# 使用cyclonedds的idl文件编译
$ /home/cyclonedds/install/bin/idlc -l cxx -o ./ Test_.idl

# 编译项目
$ vim CMakeLists.txt
# 请留意例程中的注释，包括命名空间、话题名称、消息类型
# 请修改例程中的消息命名空间，如果消息包是airs/msg/Test，那么例程中的消息命名空间应该是airs::msg
$ mkdir build; cd build;cmake ..
$ make -j$(nproc)
```

### 6. 基于生成的源文件/头文件实现通信

- 具体参考例程 `publisher.cpp` `subscriber.cpp`
```yaml
ros2监听话题
$ source /home/airs_ros2/setup.sh # 加载消息包
Setup airs ros2 environment

$ ros2 interface show airs/msg/HelloWorldData # 查询消息包
int64 user_id
string message

$ ros2 topic list
/hello_world # 我们的话题
/parameter_events
/rosout

$ ros2 topic echo /hello_world airs/msg/HelloWorldData # 持续监听
user_id: 1
message: 'Hello from Cyclone DDS!'
---
user_id: 2
message: 'Hello from Cyclone DDS!'
---
```

```bash
ros2命令行发布消息
$ ros2 topic pub --once /hello_world_receiver airs/msg/HelloWorldData "{user_id: 10, message: 'Hello ROS2 CLI from command line'}"
publisher: beginning loop
publishing #1: airs.msg.HelloWorldData(user_id=10, message='Hello ROS2 CLI from command line')
```

```bash
接收端
$ ./subscriber
Created Domain Participant.
Subscribing to topic: rt/hello_world_receiver
Created DataReader for topic 'rt/hello_world_receiver'. Ready to receive messages...
Received message: user_id=10, message='Hello ROS2 CLI from command line'
Received sample with invalid info state.
Received message: user_id=1, message='Hello from Cyclone DDS!'
Received message: user_id=2, message='Hello from Cyclone DDS!'
```

```bash
发布端
$ ./publisher
Created Domain Participant
Creating topic 'rt/hello_world_receiver'
Publish message: user_id=0, message='Hello from Cyclone DDS!'
Publish message: user_id=1, message='Hello from Cyclone DDS!'
Publish message: user_id=2, message='Hello from Cyclone DDS!'
```

