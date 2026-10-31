#include "turtle_ble/nodes/ble_feedback_node.hpp"

#include "turtle_ble/domain/kinematics.hpp"
#include "turtle_ble/domain/protocol_codec.hpp"

namespace turtle_ble::nodes
{

BleFeedbackNode::BleFeedbackNode()
: Node("ble_feedback_node")
{
  cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
    "/cmd_vel", 10,
    std::bind(&BleFeedbackNode::on_cmd_vel, this, std::placeholders::_1));

  feedback_pub_ = this->create_publisher<std_msgs::msg::String>("/ble_feedback", 10);

  RCLCPP_INFO(this->get_logger(), "Вузол 3 (BLE Feedback) запущено.");
}

void BleFeedbackNode::on_cmd_vel(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  const domain::VelocityCommand cmd{msg->linear.x, msg->angular.z};

  std_msgs::msg::String feedback_msg;
  feedback_msg.data = domain::ProtocolCodec::encode_feedback(cmd);
  feedback_pub_->publish(feedback_msg);

  RCLCPP_INFO(this->get_logger(), "Фідбек сформовано -> %s", feedback_msg.data.c_str());
}

}  // namespace turtle_ble::nodes