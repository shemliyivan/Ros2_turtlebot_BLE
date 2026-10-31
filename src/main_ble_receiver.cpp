#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "turtle_ble/infrastructure/bluez_gatt_server.hpp"
#include "turtle_ble/nodes/ble_receiver_node.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto gatt_server = std::make_unique<turtle_ble::infrastructure::BluezGattServer>();
  auto node = std::make_shared<turtle_ble::nodes::BleReceiverNode>(std::move(gatt_server));
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}