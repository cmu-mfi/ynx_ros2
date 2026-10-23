cmake_minimum_required(VERSION 3.8)
project(ynx_robot_manager)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(DEPENDENCIES
  ament_cmake
  moveit_core
  moveit_ros_planning_interface
  moveit_servo
  rclcpp
  rclcpp_action
  tf2_ros
  robot_manager_interfaces
  std_srvs
  geometry_msgs
  control_msgs
)

# Find dependencies
foreach(Dependency IN ITEMS ${DEPENDENCIES})
  find_package(${Dependency} REQUIRED)
endforeach()

# Define the executable
add_executable(
  ynx_robot_manager 
  src/ynx_robot_manager.cpp 
  src/moveit.cpp
  src/servo.cpp
  src/joint_goal.cpp
  src/pose_goal.cpp
  src/home_service.cpp
  src/park_service.cpp
  src/set_payload_service.cpp
  src/set_io_service.cpp
  src/wrench_publisher.cpp
  src/pose_servo.cpp
  src/twist_servo.cpp
  src/ee_state_publisher.cpp
  src/admittance_pose_servo.cpp
  src/tf.cpp
)

# Add include directories for the executable
target_include_directories(ynx_robot_manager PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Link the libraries
ament_target_dependencies(ynx_robot_manager ${DEPENDENCIES})
ament_export_include_directories(include)
ament_export_dependencies(${DEPENDENCIES})

install(TARGETS ynx_robot_manager
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include
)

ament_package()
