#ifndef TURTLE_BLE__NODES__BLE_RECEIVER_NODE_HPP_
#define TURTLE_BLE__NODES__BLE_RECEIVER_NODE_HPP_

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "turtle_ble/interfaces/i_ble_gatt_server.hpp"

namespace turtle_ble::nodes
{

class BleReceiverNode final : public rclcpp::Node
{
public:
  explicit BleReceiverNode(std::unique_ptr<interfaces::IBleGattServer> gatt_server);

private:
  void on_ble_packet_received(const std::string & raw_packet);
  void on_feedback_received(const std_msgs::msg::String::SharedPtr msg);

  std::unique_ptr<interfaces::IBleGattServer> gatt_server_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr raw_pub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr feedback_sub_;
  rclcpp::TimerBase::SharedPtr event_loop_timer_;
};

}  // namespace turtle_ble::nodes

#endif  // TURTLE_BLE__NODES__BLE_RECEIVER_NODE_HPP_