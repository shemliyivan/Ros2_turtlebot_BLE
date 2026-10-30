#ifndef TURTLE_BLE__INFRASTRUCTURE__BLUEZ_GATT_SERVER_HPP_
#define TURTLE_BLE__INFRASTRUCTURE__BLUEZ_GATT_SERVER_HPP_

#include <gio/gio.h>
#include <glib.h>

#include <string>

#include "turtle_ble/interfaces/i_ble_gatt_server.hpp"

namespace turtle_ble::infrastructure
{

class BluezGattServer final : public interfaces::IBleGattServer
{
public:
  BluezGattServer();
  ~BluezGattServer() override;

  BluezGattServer(const BluezGattServer &) = delete;
  BluezGattServer & operator=(const BluezGattServer &) = delete;

  bool start() override;
  void process_events() override;
  void notify_feedback(const std::string & payload) override;

  void set_on_write_callback(WriteCallback cb) override;
  void set_logger_callback(LogCallback cb) override;

private:
  void log_info(const std::string & msg) const;
  void log_error(const std::string & msg) const;
  void register_dbus_objects();
  void register_gatt_application();
  void register_advertisement();

  static void on_method_call(
    GDBusConnection * conn, const gchar * sender, const gchar * object_path,
    const gchar * interface_name, const gchar * method_name,
    GVariant * parameters, GDBusMethodInvocation * invocation, gpointer user_data);

  static GVariant * on_get_property(
    GDBusConnection * conn, const gchar * sender, const gchar * object_path,
    const gchar * interface_name, const gchar * property_name,
    GError ** error, gpointer user_data);

  GDBusConnection * dbus_conn_{nullptr};
  GDBusNodeInfo * node_info_{nullptr};
  bool notify_enabled_{false};
  std::string last_feedback_{"ACK:READY;V:0.00;W:0.00"};

  WriteCallback on_write_cb_;
  LogCallback logger_cb_;
};

}  // namespace turtle_ble::infrastructure

#endif  // TURTLE_BLE__INFRASTRUCTURE__BLUEZ_GATT_SERVER_HPP_