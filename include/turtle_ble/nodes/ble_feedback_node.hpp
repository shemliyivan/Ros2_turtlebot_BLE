#ifndef TURTLE_BLE__NODES__BLE_FEEDBACK_NODE_HPP_
#define TURTLE_BLE__NODES__BLE_FEEDBACK_NODE_HPP_

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

namespace turtle_ble::nodes
{

class BleFeedbackNode final : public rclcpp::Node
{
public:
  BleFeedbackNode();

private:
  void on_cmd_vel(const geometry_msgs::msg::Twist::SharedPtr msg);

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr feedback_pub_;
};

}  // namespace turtle_ble::nodes

#endif  // TURTLE_BLE__NODES__BLE_FEEDBACK_NODE_HPP_