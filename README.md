# Docker Commands
First Terminal:
    xhost +local:root
    bluetoothctl discoverable on
    docker start -ai foxy_bot
    cd /root/ros2_ws
    rm -rf build/ install/ log/
    colcon build --packages-select turtle_ble
    source /opt/ros/foxy/setup.bash
    source install/setup.bash
    ros2 run turtle_ble ble_receiver_node
Second Terminal:
    docker exec -it foxy_bot bash
    source /opt/ros/foxy/setup.bash
    source /root/ros2_ws/install/setup.bash
    ros2 run turtle_ble motion_controller_node
Third Terminal:
    docker exec -it foxy_bot bash
    source /opt/ros/foxy/setup.bash
    source /root/ros2_ws/install/setup.bash
    ros2 run turtle_ble ble_feedback_node

# TurtleBot BLE Bridge (`turtle_ble`)

A Clean Architecture ROS 2 C++ package that implements a custom Bluetooth Low Energy (BLE) GATT Server over Linux BlueZ/D-Bus. It bridges a Flutter mobile controller with a differential-drive robot (TurtleBot3 / `turtlesim`).

## Architecture & Nodes

The package is divided into three layers (`domain`, `infrastructure`, `nodes`) and provides three executables:
* **`ble_receiver_node`** (`main_ble_receiver.cpp`): Registers the BlueZ GATT Server (`0xFFE0`) and LE Advertisement (`TurtleBot-BLE`) via System D-Bus. Publishes incoming commands to `/ble_raw_packets` and pushes `/ble_feedback` notifications to the mobile client.
* **`motion_controller_node`** (`main_motion_controller.cpp`): Subscribes to `/ble_raw_packets`, parses velocity frames, and publishes `geometry_msgs/msg/Twist` messages to `/cmd_vel` (real robot) and `/turtle1/cmd_vel` (`turtlesim`).
* **`ble_feedback_node`** (`main_ble_feedback.cpp`): Subscribes to `/cmd_vel`, classifies the kinematic state (`STOPPED`, `FORWARD`, `BACKWARD`, `TURN_LEFT`, `TURN_RIGHT`, `MANEUVER`), and publishes telemetry to `/ble_feedback`.

## BLE GATT Profile & Protocol

| Entity | UUID | Properties | Format / Example |
| :--- | :--- | :--- | :--- |
| **GATT Service** | `0000ffe0-0000-1000-8000-00805f9b34fb` | Primary | — |
| **TX Characteristic** | `0000ffe1-0000-1000-8000-00805f9b34fb` | `write`, `write-without-response` | `V:<linear>;W:<angular>` (e.g., `V:0.22;W:-1.50`) |
| **RX Characteristic** | `0000ffe2-0000-1000-8000-00805f9b34fb` | `notify`, `read` | `ACK:<STATE>;V:<v>;W:<w>` (e.g., `ACK:FORWARD;V:0.22;W:0.00`) |

---

## 1. Prerequisites & Dependencies

Ensure **Ubuntu 20.04 / 22.04**, **ROS 2 (Foxy or Humble)**, and the required BlueZ / GLib D-Bus development libraries are installed:

```bash
sudo apt-get update
sudo apt-get install -y \
  bluez \
  libglib2.0-dev \
  pkg-config \
  ros-${ROS_DISTRO}-rclcpp \
  ros-${ROS_DISTRO}-std-msgs \
  ros-${ROS_DISTRO}-geometry-msgs