#include "turtle_ble/nodes/ble_receiver_node.hpp"

#include <chrono>
#include <utility>

#include "turtle_ble/domain/protocol_codec.hpp"

using namespace std::chrono_literals;

namespace turtle_ble::nodes
{

BleReceiverNode::BleReceiverNode(std::unique_ptr<interfaces::IBleGattServer> gatt_server)
: Node("ble_receiver_node"), gatt_server_(std::move(gatt_server))
{
  raw_pub_ = this->create_publisher<std_msgs::msg::String>("/ble_raw_packets", 10);

  feedback_sub_ = this->create_subscription<std_msgs::msg::String>(
    "/ble_feedback", 10,
    std::bind(&BleReceiverNode::on_feedback_received, this, std::placeholders::_1));

  gatt_server_->set_logger_callback([this](const std::string & text, bool is_error) {
    if (is_error) {
      RCLCPP_ERROR(this->get_logger(), "%s", text.c_str());
    } else {
      RCLCPP_INFO(this->get_logger(), "%s", text.c_str());
    }
  });

  gatt_server_->set_on_write_callback(
    std::bind(&BleReceiverNode::on_ble_packet_received, this, std::placeholders::_1));

  gatt_server_->start();

  event_loop_timer_ = this->create_wall_timer(10ms, [this]() {
    gatt_server_->process_events();
  });
}

void BleReceiverNode::on_ble_packet_received(const std::string & raw_packet)
{
  std_msgs::msg::String msg;
  msg.data = domain::ProtocolCodec::sanitize_frame(raw_packet);
  RCLCPP_INFO(this->get_logger(), "BLE RX [0xFFE1] -> '%s'", msg.data.c_str());
  raw_pub_->publish(msg);
}

void BleReceiverNode::on_feedback_received(const std_msgs::msg::String::SharedPtr msg)
{
  gatt_server_->notify_feedback(msg->data);
}

}  // namespace turtle_ble::nodes