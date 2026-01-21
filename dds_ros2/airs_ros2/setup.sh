#!/bin/bash
echo "Setup airs ros2 environment"
#加载ros2环境
source /opt/ros/humble/setup.bash
source /home/airs_ros2/cyclonedds_ws/install/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export CYCLONEDDS_URI='<CycloneDDS><Domain><General><Interfaces>
                            <NetworkInterface name="ens33" priority="default" multicast="default" />
                        </Interfaces></General></Domain></CycloneDDS>'
