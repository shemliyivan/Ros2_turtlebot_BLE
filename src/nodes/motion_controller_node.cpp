#include "turtle_ble/nodes/motion_controller_node.hpp"

#include "turtle_ble/domain/kinematics.hpp"
#include "turtle_ble/domain/protocol_codec.hpp"

namespace turtle_ble::nodes
{

MotionControllerNode::MotionControllerNode()
: Node("motion_controller_node")
{
  subscription_ = this->create_subscription<std_msgs::msg::String>(
    "/ble_raw_packets", 10,
    std::bind(&MotionControllerNode::on_raw_packet, this, std::placeholders::_1));

  robot_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
  sim_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);

  RCLCPP_INFO(this->get_logger(), "Вузол 2 (Motion Controller) запущено. Готовий крутити колеса!");
}

void MotionControllerNode::on_raw_packet(const std_msgs::msg::String::SharedPtr msg)
{
  const auto parsed = domain::ProtocolCodec::decode_command(msg->data);
  if (!parsed.has_value()) {
    RCLCPP_WARN(this->get_logger(), "Пошкоджений формат пакета: '%s'", msg->data.c_str());
    return;
  }

  const auto & cmd = parsed.value();

  geometry_msgs::msg::Twist robot_cmd;
  robot_cmd.linear.x = cmd.linear_x;
  robot_cmd.angular.z = cmd.angular_z;
  robot_pub_->publish(robot_cmd);

  geometry_msgs::msg::Twist sim_cmd;
  sim_cmd.linear.x = cmd.linear_x * domain::KinematicsClassifier::kSimLinearScale;
  sim_cmd.angular.z = cmd.angular_z;
  sim_pub_->publish(sim_cmd);

  RCLCPP_INFO(
    this->get_logger(),
    "Виконую рух -> linear.x: %.2f м/с, angular.z: %.2f рад/с",
    cmd.linear_x, cmd.angular_z);
}

}  // namespace turtle_ble::nodes