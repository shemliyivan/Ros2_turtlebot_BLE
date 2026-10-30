#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

class BleFeedbackNode : public rclcpp::Node
{
public:
  BleFeedbackNode() : Node("ble_feedback_node")
  {
    cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel", 10,
      std::bind(&BleFeedbackNode::cmd_vel_callback, this, std::placeholders::_1));

    feedback_pub_ = this->create_publisher<std_msgs::msg::String>("/ble_feedback", 10);

    RCLCPP_INFO(this->get_logger(), "Вузол 3 (BLE Feedback) запущено.");
  }

private:
  void cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    const double v = msg->linear.x;
    const double w = msg->angular.z;

    std::string op_state = "STOPPED";
    const bool is_moving_linear = std::fabs(v) > 0.01;
    const bool is_turning = std::fabs(w) > 0.01;

    if (is_moving_linear && is_turning) {
      op_state = "MANEUVER";
    } else if (is_moving_linear) {
      op_state = (v > 0.0) ? "FORWARD" : "BACKWARD";
    } else if (is_turning) {
      op_state = (w > 0.0) ? "TURN_LEFT" : "TURN_RIGHT";
    }

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "ACK:%s;V:%.2f;W:%.2f", op_state.c_str(), v, w);

    std_msgs::msg::String feedback_msg;
    feedback_msg.data = std::string(buffer);
    feedback_pub_->publish(feedback_msg);

    RCLCPP_INFO(this->get_logger(), "Фідбек сформовано -> %s", feedback_msg.data.c_str());
  }

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr feedback_pub_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<BleFeedbackNode>());
  rclcpp::shutdown();
  return 0;
}