#include "turtle_ble/domain/protocol_codec.hpp"

#include <cstdio>

namespace turtle_ble::domain
{

std::string ProtocolCodec::sanitize_frame(std::string raw_packet)
{
  while (!raw_packet.empty() && (raw_packet.back() == '\n' || raw_packet.back() == '\r')) {
    raw_packet.pop_back();
  }
  return raw_packet;
}

std::optional<VelocityCommand> ProtocolCodec::decode_command(const std::string & frame)
{
  VelocityCommand cmd{};
  if (std::sscanf(frame.c_str(), "V:%lf;W:%lf", &cmd.linear_x, &cmd.angular_z) == 2) {
    return cmd;
  }
  return std::nullopt;
}

std::string ProtocolCodec::encode_feedback(const VelocityCommand & cmd)
{
  const MotionState state = KinematicsClassifier::classify(cmd);
  const std::string_view state_str = KinematicsClassifier::to_string(state);

  char buffer[64];
  std::snprintf(
    buffer, sizeof(buffer), "ACK:%.*s;V:%.2f;W:%.2f",
    static_cast<int>(state_str.size()), state_str.data(),
    cmd.linear_x, cmd.angular_z);

  return std::string(buffer);
}

}  // namespace turtle_ble::domain