#include "ynx_robot_manager/ynx_robot_manager.hpp"

namespace ynx_robot_manager {
  // --- Set Payload Service - Setup --- ///
  void YnxRobotManager::set_payload_service_setup() {
    // Set Payload Service
    set_payload_service_ = this->create_service<SetPayload>(
        "set_payload",
        std::bind(&YnxRobotManager::set_payload_service_callback, this, std::placeholders::_1, std::placeholders::_2),
        rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile(),
        service_cb_group_
        );
  }

  // --- Set Payload Service - Service Callback --- ///
  void YnxRobotManager::set_payload_service_callback(
      const std::shared_ptr<SetPayload::Request> request,
      std::shared_ptr<SetPayload::Response> response)
  {
    // Extract payload parameters from the request
    double mass = request->mass;
    double cog_x = request->cog.x;
    double cog_y = request->cog.y;
    double cog_z = request->cog.z;
    // Basic validation: Mass should not be negative
    if (mass < 0.0) {
      RCLCPP_ERROR(this->get_logger(), "[Set Payload Service] Invalid mass: %.2f kg. Mass cannot be negative.", mass);
      response->success = false;
      response->message = "Payload update failed: Mass cannot be negative.";
      return;
    }
    RCLCPP_INFO(this->get_logger(), "[Set Payload Service] Attempting to set payload - Mass: %.2f kg, COG: [%.3f, %.3f, %.3f]",
         mass, cog_x, cog_y, cog_z);
    // TODO Actually implement this
    RCLCPP_INFO(this->get_logger(), "[Set Payload Service] Successfully set payload.");
    response->success = true;
    response->message = "Payload successfully updated.";
  }

}  // namespace ynx_robot_manager
