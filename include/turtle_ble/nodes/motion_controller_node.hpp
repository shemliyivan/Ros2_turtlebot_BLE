#ifndef TURTLE_BLE__NODES__MOTION_CONTROLLER_NODE_HPP_
#define TURTLE_BLE__NODES__MOTION_CONTROLLER_NODE_HPP_

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

namespace turtle_ble::nodes
{

class MotionControllerNode final : public rclcpp::Node
{
public:
  MotionControllerNode();

private:
  void on_raw_packet(const std_msgs::msg::String::SharedPtr msg);

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr robot_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr sim_pub_;
};

}  // namespace turtle_ble::nodes

#endif  // TURTLE_BLE__NODES__MOTION_CONTROLLER_NODE_HPP_