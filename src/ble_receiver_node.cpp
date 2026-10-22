#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class BleReceiverNode : public rclcpp::Node
{
public:
  BleReceiverNode() : Node("ble_receiver_node")
  {
    publisher_ = this->create_publisher<std_msgs::msg::String>("/ble_raw_packets", 10);

    // Неблокуючий UDP-сокет на порту 8888 для прийому пакетів з додатка
    sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    int flags = fcntl(sock_fd_, F_GETFL, 0);
    fcntl(sock_fd_, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8888);

    if (bind(sock_fd_, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
      RCLCPP_ERROR(this->get_logger(), "Не вдалося відкрити порт 8888!");
    } else {
      RCLCPP_INFO(this->get_logger(), "Вузол 1 (BLE Receiver) запущено. Слухаю пакети...");
    }

    // Опитуємо буфер кожні 20 мс (50 Гц)
    timer_ = this->create_wall_timer(20ms, std::bind(&BleReceiverNode::poll_packets, this));
  }

  ~BleReceiverNode() override
  {
    if (sock_fd_ >= 0) {
      close(sock_fd_);
    }
  }

private:
  void poll_packets()
  {
    char buffer[128];
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    ssize_t len = recvfrom(
      sock_fd_, buffer, sizeof(buffer) - 1, 0,
      reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);

    if (len > 0) {
      buffer[len] = '\0';
      auto msg = std_msgs::msg::String();
      msg.data = std::string(buffer);

      // Прибираємо символ переносу рядка для чистого логу
      if (!msg.data.empty() && msg.data.back() == '\n') {
        msg.data.pop_back();
      }

      RCLCPP_INFO(this->get_logger(), "Отримано пакет -> публікую в /ble_raw_packets: '%s'", msg.data.c_str());
      publisher_->publish(msg);
    }
  }

  int sock_fd_{-1};
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<BleReceiverNode>());
  rclcpp::shutdown();
  return 0;
}
