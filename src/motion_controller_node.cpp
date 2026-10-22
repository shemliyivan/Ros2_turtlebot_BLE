#include <cstdio>
#include <memory>
#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

class MotionControllerNode : public rclcpp::Node
{
public:
  MotionControllerNode() : Node("motion_controller_node")
  {
    subscription_ = this->create_subscription<std_msgs::msg::String>(
      "/ble_raw_packets", 10,
      std::bind(&MotionControllerNode::packet_callback, this, std::placeholders::_1));

    // Публікація в /cmd_vel (для TurtleBot), і в /turtle1/cmd_vel (для turtlesim)(це test)
    robot_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    sim_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);

    RCLCPP_INFO(this->get_logger(), "Вузол 2 (Motion Controller) запущено. Готовий крутити колеса!");
  }

private:
  void packet_callback(const std_msgs::msg::String::SharedPtr msg)
  {
    double linear_v = 0.0;
    double angular_w = 0.0;

    // Розбираємо рядок формату "V:0.22;W:-1.50"
    if (std::sscanf(msg->data.c_str(), "V:%lf;W:%lf", &linear_v, &angular_w) == 2) {
      geometry_msgs::msg::Twist robot_cmd;
      robot_cmd.linear.x = linear_v;
      robot_cmd.angular.z = angular_w;
      robot_pub_->publish(robot_cmd);

      // Для вікна turtlesim(test)
      geometry_msgs::msg::Twist sim_cmd;
      sim_cmd.linear.x = linear_v * 5.0;
      sim_cmd.angular.z = angular_w;
      sim_pub_->publish(sim_cmd);

      RCLCPP_INFO(
        this->get_logger(),
        "Виконую рух -> linear.x: %.2f м/с, angular.z: %.2f рад/с",
        linear_v, angular_w);
    } else {
      RCLCPP_WARN(this->get_logger(), "Пошкоджений формат пакета: '%s'", msg->data.c_str());
    }
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr robot_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr sim_pub_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MotionControllerNode>());
  rclcpp::shutdown();
  return 0;
}
