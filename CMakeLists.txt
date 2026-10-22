cmake_minimum_required(VERSION 3.5)
project(turtle_ble)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(ble_receiver_node src/ble_receiver_node.cpp)
ament_target_dependencies(ble_receiver_node rclcpp std_msgs)

add_executable(motion_controller_node src/motion_controller_node.cpp)
ament_target_dependencies(motion_controller_node rclcpp std_msgs geometry_msgs)

install(TARGETS
  ble_receiver_node
  motion_controller_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
