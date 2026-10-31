#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "turtle_ble/nodes/ble_feedback_node.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<turtle_ble::nodes::BleFeedbackNode>());
  rclcpp::shutdown();
  return 0;
}