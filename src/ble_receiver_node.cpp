#include <gio/gio.h>
#include <glib.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

static const char * kAppPath      = "/org/bluez/turtle_ble";
static const char * kServicePath  = "/org/bluez/turtle_ble/service0";
static const char * kTxCharPath   = "/org/bluez/turtle_ble/service0/char0";
static const char * kRxCharPath   = "/org/bluez/turtle_ble/service0/char1";
static const char * kAdvPath      = "/org/bluez/turtle_ble/adv0";

static const char * kServiceUuid  = "0000ffe0-0000-1000-8000-00805f9b34fb";
static const char * kTxCharUuid   = "0000ffe1-0000-1000-8000-00805f9b34fb";
static const char * kRxCharUuid   = "0000ffe2-0000-1000-8000-00805f9b34fb";

static const char * kIntrospectionXml =
  "<node>"
  "  <interface name='org.freedesktop.DBus.ObjectManager'>"
  "    <method name='GetManagedObjects'>"
  "      <arg name='objects' type='a{oa{sa{sv}}}' direction='out'/>"
  "    </method>"
  "  </interface>"
  "  <interface name='org.bluez.GattService1'>"
  "    <property name='UUID' type='s' access='read'/>"
  "    <property name='Primary' type='b' access='read'/>"
  "  </interface>"
  "  <interface name='org.bluez.GattCharacteristic1'>"
  "    <method name='ReadValue'>"
  "      <arg name='options' type='a{sv}' direction='in'/>"
  "      <arg name='value' type='ay' direction='out'/>"
  "    </method>"
  "    <method name='WriteValue'>"
  "      <arg name='value' type='ay' direction='in'/>"
  "      <arg name='options' type='a{sv}' direction='in'/>"
  "    </method>"
  "    <method name='StartNotify'/>"
  "    <method name='StopNotify'/>"
  "    <property name='UUID' type='s' access='read'/>"
  "    <property name='Service' type='o' access='read'/>"
  "    <property name='Flags' type='as' access='read'/>"
  "  </interface>"
  "  <interface name='org.bluez.LEAdvertisement1'>"
  "    <method name='Release'/>"
  "    <property name='Type' type='s' access='read'/>"
  "    <property name='LocalName' type='s' access='read'/>"
  "    <property name='ServiceUUIDs' type='as' access='read'/>"
  "  </interface>"
  "</node>";

class BleReceiverNode : public rclcpp::Node
{
public:
  BleReceiverNode() : Node("ble_receiver_node")
  {
    raw_pub_ = this->create_publisher<std_msgs::msg::String>("/ble_raw_packets", 10);

    // Підписуємось на фідбек від 3-го вузла, щоб відправляти його на телефон через BLE Notify
    feedback_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/ble_feedback", 10,
      std::bind(&BleReceiverNode::on_feedback_msg, this, std::placeholders::_1));

    init_bluez_gatt_server();

    // Прокручуємо події GLib/D-Bus кожні 10 мс синхронно з ROS 2
    timer_ = this->create_wall_timer(10ms, []() {
      while (g_main_context_pending(nullptr)) {
        g_main_context_iteration(nullptr, FALSE);
      }
    });
  }

  ~BleReceiverNode() override
  {
    if (node_info_ != nullptr) {
      g_dbus_node_info_unref(node_info_);
    }
    if (dbus_conn_ != nullptr) {
      g_object_unref(dbus_conn_);
    }
  }

  void handle_ble_write(const std::string & packet)
  {
    std_msgs::msg::String msg;
    msg.data = packet;
    while (!msg.data.empty() && (msg.data.back() == '\n' || msg.data.back() == '\r')) {
      msg.data.pop_back();
    }
    RCLCPP_INFO(this->get_logger(), "BLE RX [0xFFE1] -> '%s'", msg.data.c_str());
    raw_pub_->publish(msg);
  }

  void set_notify_state(bool enabled)
  {
    notify_enabled_ = enabled;
    RCLCPP_INFO(
      this->get_logger(), "BLE Notify [0xFFE2] підписка: %s",
      enabled ? "УВІМКНЕНО" : "ВИМКНЕНО");
  }

  std::string get_last_feedback() const { return last_feedback_; }

private:
  void on_feedback_msg(const std_msgs::msg::String::SharedPtr msg)
  {
    last_feedback_ = msg->data;
    if (!notify_enabled_ || dbus_conn_ == nullptr) {
      return;
    }

    // Пушимо оновлене значення характеристики 0xFFE2 через сигнал PropertiesChanged
    GVariantBuilder changed_builder;
    g_variant_builder_init(&changed_builder, G_VARIANT_TYPE("a{sv}"));
    GVariant * byte_array = g_variant_new_fixed_array(
      G_VARIANT_TYPE_BYTE, last_feedback_.data(), last_feedback_.size(), sizeof(guint8));
    g_variant_builder_add(&changed_builder, "{sv}", "Value", byte_array);

    GVariantBuilder invalidated_builder;
    g_variant_builder_init(&invalidated_builder, G_VARIANT_TYPE("as"));

    g_dbus_connection_emit_signal(
      dbus_conn_, nullptr, kRxCharPath,
      "org.freedesktop.DBus.Properties", "PropertiesChanged",
      g_variant_new(
        "(sa{sv}as)", "org.bluez.GattCharacteristic1",
        &changed_builder, &invalidated_builder),
      nullptr);
  }

  static void on_method_call(
    GDBusConnection *, const gchar *, const gchar * object_path,
    const gchar *, const gchar * method_name,
    GVariant * parameters, GDBusMethodInvocation * invocation, gpointer user_data)
  {
    auto * self = static_cast<BleReceiverNode *>(user_data);

    if (g_strcmp0(method_name, "GetManagedObjects") == 0) {
      GVariantBuilder root;
      g_variant_builder_init(&root, G_VARIANT_TYPE("a{oa{sa{sv}}}"));

      // 1. Опис GATT Service (0xFFE0)
      {
        GVariantBuilder ifaces, props;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
        g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(kServiceUuid));
        g_variant_builder_add(&props, "{sv}", "Primary", g_variant_new_boolean(TRUE));
        g_variant_builder_add(&ifaces, "{sa{sv}}", "org.bluez.GattService1", &props);
        g_variant_builder_add(&root, "{oa{sa{sv}}}", kServicePath, &ifaces);
      }

      // 2. Опис TX Characteristic (0xFFE1 - Write)
      {
        const gchar * flags[] = {"write", "write-without-response", nullptr};
        GVariantBuilder ifaces, props;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
        g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(kTxCharUuid));
        g_variant_builder_add(&props, "{sv}", "Service", g_variant_new_object_path(kServicePath));
        g_variant_builder_add(&props, "{sv}", "Flags", g_variant_new_strv(flags, -1));
        g_variant_builder_add(&ifaces, "{sa{sv}}", "org.bluez.GattCharacteristic1", &props);
        g_variant_builder_add(&root, "{oa{sa{sv}}}", kTxCharPath, &ifaces);
      }

      // 3. Опис RX Characteristic (0xFFE2 - Notify)
      {
        const gchar * flags[] = {"notify", "read", nullptr};
        GVariantBuilder ifaces, props;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
        g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(kRxCharUuid));
        g_variant_builder_add(&props, "{sv}", "Service", g_variant_new_object_path(kServicePath));
        g_variant_builder_add(&props, "{sv}", "Flags", g_variant_new_strv(flags, -1));
        g_variant_builder_add(&ifaces, "{sa{sv}}", "org.bluez.GattCharacteristic1", &props);
        g_variant_builder_add(&root, "{oa{sa{sv}}}", kRxCharPath, &ifaces);
      }

      g_dbus_method_invocation_return_value(
        invocation, g_variant_new("(a{oa{sa{sv}}})", &root));
      return;
    }

    if (g_strcmp0(method_name, "WriteValue") == 0) {
      GVariant * val = g_variant_get_child_value(parameters, 0);
      gsize len = 0;
      const auto * bytes = static_cast<const char *>(
        g_variant_get_fixed_array(val, &len, sizeof(guint8)));
      if (bytes != nullptr && len > 0) {
        self->handle_ble_write(std::string(bytes, len));
      }
      g_variant_unref(val);
      g_dbus_method_invocation_return_value(invocation, nullptr);
      return;
    }

    if (g_strcmp0(method_name, "ReadValue") == 0) {
      std::string fb = self->get_last_feedback();
      GVariant * arr = g_variant_new_fixed_array(
        G_VARIANT_TYPE_BYTE, fb.data(), fb.size(), sizeof(guint8));
      g_dbus_method_invocation_return_value(invocation, g_variant_new("(@ay)", arr));
      return;
    }

    if (g_strcmp0(method_name, "StartNotify") == 0) {
      self->set_notify_state(true);
      g_dbus_method_invocation_return_value(invocation, nullptr);
      return;
    }

    if (g_strcmp0(method_name, "StopNotify") == 0) {
      self->set_notify_state(false);
      g_dbus_method_invocation_return_value(invocation, nullptr);
      return;
    }

    if (g_strcmp0(method_name, "Release") == 0 && g_strcmp0(object_path, kAdvPath) == 0) {
      g_dbus_method_invocation_return_value(invocation, nullptr);
      return;
    }
  }

  static GVariant * on_get_property(
    GDBusConnection *, const gchar *, const gchar * object_path,
    const gchar *, const gchar * property_name, GError **, gpointer)
  {
    if (g_strcmp0(object_path, kServicePath) == 0) {
      if (g_strcmp0(property_name, "UUID") == 0) return g_variant_new_string(kServiceUuid);
      if (g_strcmp0(property_name, "Primary") == 0) return g_variant_new_boolean(TRUE);
    }
    if (g_strcmp0(object_path, kTxCharPath) == 0) {
      const gchar * flags[] = {"write", "write-without-response", nullptr};
      if (g_strcmp0(property_name, "UUID") == 0) return g_variant_new_string(kTxCharUuid);
      if (g_strcmp0(property_name, "Service") == 0) return g_variant_new_object_path(kServicePath);
      if (g_strcmp0(property_name, "Flags") == 0) return g_variant_new_strv(flags, -1);
    }
    if (g_strcmp0(object_path, kRxCharPath) == 0) {
      const gchar * flags[] = {"notify", "read", nullptr};
      if (g_strcmp0(property_name, "UUID") == 0) return g_variant_new_string(kRxCharUuid);
      if (g_strcmp0(property_name, "Service") == 0) return g_variant_new_object_path(kServicePath);
      if (g_strcmp0(property_name, "Flags") == 0) return g_variant_new_strv(flags, -1);
    }
    if (g_strcmp0(object_path, kAdvPath) == 0) {
      const gchar * uuids[] = {kServiceUuid, nullptr};
      if (g_strcmp0(property_name, "Type") == 0) return g_variant_new_string("peripheral");
      if (g_strcmp0(property_name, "LocalName") == 0) return g_variant_new_string("TurtleBot-BLE");
      if (g_strcmp0(property_name, "ServiceUUIDs") == 0) return g_variant_new_strv(uuids, -1);
    }
    return nullptr;
  }

  void init_bluez_gatt_server()
  {
    GError * err = nullptr;
    dbus_conn_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &err);
    if (dbus_conn_ == nullptr) {
      RCLCPP_ERROR(this->get_logger(), "Помилка підключення до System D-Bus: %s", err->message);
      g_error_free(err);
      return;
    }

    node_info_ = g_dbus_node_info_new_for_xml(kIntrospectionXml, &err);
    static const GDBusInterfaceVTable vtable = {on_method_call, on_get_property, nullptr, {0}};

    g_dbus_connection_register_object(
      dbus_conn_, kAppPath, node_info_->interfaces[0], &vtable, this, nullptr, nullptr);
    g_dbus_connection_register_object(
      dbus_conn_, kServicePath, node_info_->interfaces[1], &vtable, this, nullptr, nullptr);
    g_dbus_connection_register_object(
      dbus_conn_, kTxCharPath, node_info_->interfaces[2], &vtable, this, nullptr, nullptr);
    g_dbus_connection_register_object(
      dbus_conn_, kRxCharPath, node_info_->interfaces[2], &vtable, this, nullptr, nullptr);
    g_dbus_connection_register_object(
      dbus_conn_, kAdvPath, node_info_->interfaces[3], &vtable, this, nullptr, nullptr);

    // 1. Реєструємо GATT-таблицю в BlueZ на адаптері hci0
    g_dbus_connection_call(
      dbus_conn_, "org.bluez", "/org/bluez/hci0", "org.bluez.GattManager1",
      "RegisterApplication", g_variant_new("(oa{sv})", kAppPath, nullptr),
      nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
      [](GObject * src, GAsyncResult * res, gpointer user_data) {
        auto * self = static_cast<BleReceiverNode *>(user_data);
        GError * e = nullptr;
        GVariant * ret = g_dbus_connection_call_finish(G_DBUS_CONNECTION(src), res, &e);
        if (e != nullptr) {
          RCLCPP_ERROR(self->get_logger(), "GattManager1 помилка: %s", e->message);
          g_error_free(e);
        } else {
          RCLCPP_INFO(self->get_logger(), "GATT-сервер (0xFFE0) успішно зареєстровано в BlueZ!");
          g_variant_unref(ret);
        }
      },
      this);

    // 2. Вмикаємо BLE Advertising (ім'я пристрою в ефірі: "TurtleBot-BLE")
    g_dbus_connection_call(
      dbus_conn_, "org.bluez", "/org/bluez/hci0", "org.bluez.LEAdvertisingManager1",
      "RegisterAdvertisement", g_variant_new("(oa{sv})", kAdvPath, nullptr),
      nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
      [](GObject * src, GAsyncResult * res, gpointer user_data) {
        auto * self = static_cast<BleReceiverNode *>(user_data);
        GError * e = nullptr;
        GVariant * ret = g_dbus_connection_call_finish(G_DBUS_CONNECTION(src), res, &e);
        if (e != nullptr) {
          RCLCPP_ERROR(self->get_logger(), "LEAdvertising помилка: %s", e->message);
          g_error_free(e);
        } else {
          RCLCPP_INFO(self->get_logger(), "BLE Advertising активний! Ім'я в ефірі: 'TurtleBot-BLE'");
          g_variant_unref(ret);
        }
      },
      this);
  }

  GDBusConnection * dbus_conn_{nullptr};
  GDBusNodeInfo * node_info_{nullptr};
  bool notify_enabled_{false};
  std::string last_feedback_{"ACK:READY;V:0.00;W:0.00"};

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr raw_pub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr feedback_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<BleReceiverNode>());
  rclcpp::shutdown();
  return 0;
}