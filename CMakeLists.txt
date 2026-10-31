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
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)

include_directories(
  include
  ${GLIB_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}_core STATIC
  src/domain/protocol_codec.cpp
  src/infrastructure/bluez_gatt_server.cpp
  src/nodes/ble_receiver_node.cpp
  src/nodes/motion_controller_node.cpp
  src/nodes/ble_feedback_node.cpp
)
ament_target_dependencies(${PROJECT_NAME}_core
  rclcpp
  std_msgs
  geometry_msgs
)
target_link_libraries(${PROJECT_NAME}_core
  ${GLIB_LIBRARIES}
)

add_executable(ble_receiver_node src/main_ble_receiver.cpp)
target_link_libraries(ble_receiver_node ${PROJECT_NAME}_core)

add_executable(motion_controller_node src/main_motion_controller.cpp)
target_link_libraries(motion_controller_node ${PROJECT_NAME}_core)

add_executable(ble_feedback_node src/main_ble_feedback.cpp)
target_link_libraries(ble_feedback_node ${PROJECT_NAME}_core)

install(TARGETS
  ble_receiver_node
  motion_controller_node
  ble_feedback_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/
)

ament_package()