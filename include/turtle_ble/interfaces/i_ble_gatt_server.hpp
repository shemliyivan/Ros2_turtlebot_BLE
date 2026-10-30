#ifndef TURTLE_BLE__INTERFACES__I_BLE_GATT_SERVER_HPP_
#define TURTLE_BLE__INTERFACES__I_BLE_GATT_SERVER_HPP_

#include <functional>
#include <string>

namespace turtle_ble::interfaces
{

class IBleGattServer
{
public:
  using WriteCallback = std::function<void(const std::string &)>;
  using LogCallback = std::function<void(const std::string &, bool is_error)>;

  virtual ~IBleGattServer() = default;

  virtual bool start() = 0;
  virtual void process_events() = 0;
  virtual void notify_feedback(const std::string & payload) = 0;

  virtual void set_on_write_callback(WriteCallback cb) = 0;
  virtual void set_logger_callback(LogCallback cb) = 0;
};

}  // namespace turtle_ble::interfaces

#endif  // TURTLE_BLE__INTERFACES__I_BLE_GATT_SERVER_HPP_