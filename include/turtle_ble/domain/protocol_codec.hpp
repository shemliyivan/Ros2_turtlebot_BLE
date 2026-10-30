#ifndef TURTLE_BLE__DOMAIN__PROTOCOL_CODEC_HPP_
#define TURTLE_BLE__DOMAIN__PROTOCOL_CODEC_HPP_

#include <optional>
#include <string>

#include "turtle_ble/domain/kinematics.hpp"

namespace turtle_ble::domain
{

class ProtocolCodec
{
public:
  static std::string sanitize_frame(std::string raw_packet);
  static std::optional<VelocityCommand> decode_command(const std::string & frame);
  static std::string encode_feedback(const VelocityCommand & cmd);
};

}  // namespace turtle_ble::domain

#endif  // TURTLE_BLE__DOMAIN__PROTOCOL_CODEC_HPP_