#ifndef TURTLE_BLE__DOMAIN__KINEMATICS_HPP_
#define TURTLE_BLE__DOMAIN__KINEMATICS_HPP_

#include <cmath>
#include <string_view>

namespace turtle_ble::domain
{

struct VelocityCommand
{
  double linear_x{0.0};
  double angular_z{0.0};
};

enum class MotionState
{
  Stopped,
  Forward,
  Backward,
  TurnLeft,
  TurnRight,
  Maneuver
};

class KinematicsClassifier
{
public:
  static constexpr double kDefaultDeadzone = 0.01;
  static constexpr double kSimLinearScale = 5.0;

  static MotionState classify(
    const VelocityCommand & cmd,
    double deadzone = kDefaultDeadzone) noexcept
  {
    const bool is_moving_linear = std::fabs(cmd.linear_x) > deadzone;
    const bool is_turning = std::fabs(cmd.angular_z) > deadzone;

    if (is_moving_linear && is_turning) {
      return MotionState::Maneuver;
    }
    if (is_moving_linear) {
      return (cmd.linear_x > 0.0) ? MotionState::Forward : MotionState::Backward;
    }
    if (is_turning) {
      return (cmd.angular_z > 0.0) ? MotionState::TurnLeft : MotionState::TurnRight;
    }
    return MotionState::Stopped;
  }

  static std::string_view to_string(MotionState state) noexcept
  {
    switch (state) {
      case MotionState::Maneuver:  return "MANEUVER";
      case MotionState::Forward:   return "FORWARD";
      case MotionState::Backward:  return "BACKWARD";
      case MotionState::TurnLeft:  return "TURN_LEFT";
      case MotionState::TurnRight: return "TURN_RIGHT";
      case MotionState::Stopped:
      default:                     return "STOPPED";
    }
  }
};

}  // namespace turtle_ble::domain

#endif  // TURTLE_BLE__DOMAIN__KINEMATICS_HPP_