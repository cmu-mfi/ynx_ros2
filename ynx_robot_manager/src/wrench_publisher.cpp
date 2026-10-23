#include <cmath>
#include "ynx_robot_manager/ynx_robot_manager.hpp"

namespace ynx_robot_manager {
  // --- Wrench Publisher - Setup --- ///
  void YnxRobotManager::wrench_publisher_setup() {
    // YNX Wrench Subscription
    input_wrench_subscriber_ = this->create_subscription<WrenchStamped>(
        "force_torque_sensor_broadcaster/wrench", 
         rclcpp::QoS(10), 
         std::bind(&YnxRobotManager::input_wrench_subscription_callback_, this, std::placeholders::_1)
        );
    // Wrench Publisher
    wrench_publisher_ = this->create_publisher<WrenchStamped>("wrench", rclcpp::QoS(10));
  }

  // --- Wrench Publisher - Subscription Callback --- ///
  void YnxRobotManager::input_wrench_subscription_callback_(const WrenchStamped::SharedPtr msg)
  {
    const auto & f = msg->wrench.force;
    const auto & t = msg->wrench.torque;
    // Check if any force or torque component is NaN
    bool has_nan = std::isnan(f.x) || std::isnan(f.y) || std::isnan(f.z) ||
                   std::isnan(t.x) || std::isnan(t.y) || std::isnan(t.z);
    if (has_nan) {
      // Skip saving (and publishing) invalid data
      return; 
    }
    WrenchStamped output_msg;
    output_msg.header = msg->header;
    output_msg.wrench.force.x =  -msg->wrench.force.x;
    output_msg.wrench.force.y =  -msg->wrench.force.y;
    output_msg.wrench.force.z =   msg->wrench.force.z;
    output_msg.wrench.torque.x = -msg->wrench.torque.x;
    output_msg.wrench.torque.y = -msg->wrench.torque.y;
    output_msg.wrench.torque.z =  msg->wrench.torque.z;
    {
      std::lock_guard<std::mutex> lock(wrench_mutex_);
      current_wrench_ = output_msg;
    }
    wrench_publisher_->publish(output_msg);
  }

}  // namespace ynx_robot_manager
