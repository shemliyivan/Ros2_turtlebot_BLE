#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "turtle_ble/nodes/motion_controller_node.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<turtle_ble::nodes::MotionControllerNode>());
  rclcpp::shutdown();
  return 0;
}