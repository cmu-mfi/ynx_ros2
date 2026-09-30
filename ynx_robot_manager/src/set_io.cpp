#include "ynx_robot_manager/ynx_robot_manager.hpp"

namespace ynx_robot_manager
{
  void YnxRobotManager::set_io_service_callback(
      const std::shared_ptr<SetIo::Request> request,
      std::shared_ptr<SetIo::Response> response) 
  {
    int pin = request->pin;
    int state = request->state;

    // Validate pin range (1 to 16)
    if (pin < 1 || pin > 16) {
      RCLCPP_ERROR(this->get_logger(), "[Set Io Service] Invalid Pin: %d. Must be between 1 and 10.", pin);
      response->success = false;
      response->message = "Io update failed: Invalid pin range (1-10 allowed).";
      return;
    }
    if (state != 0 && state != 1) {
      RCLCPP_ERROR(this->get_logger(), "[Set Io Service] Invalid State: %d. Needs to be 0 or 1.", state);
      response->success = false;
      response->message = "Io update failed: Invalid state.";
      return;
    }

    RCLCPP_INFO(this->get_logger(), "[Set Io Service] Attempting to set io - Pin: %d, state: %s", 
                pin, state ? "true" : "false");

    // Construct the group command message
    control_msgs::msg::DynamicInterfaceGroupValues msg;
    msg.interface_groups.push_back("gpio_io");

    // Populate the sub-message for interface names and values
    control_msgs::msg::InterfaceValue if_val;
    if_val.interface_names.push_back("digital_output_" + std::to_string(pin));
    if_val.values.push_back(static_cast<double>(state));

    msg.interface_values.push_back(if_val);

    // Publish command
    gpio_command_publisher_->publish(msg);

    RCLCPP_INFO(this->get_logger(), "[Set Io Service] Successfully published IO command for pin %d.", pin);
    response->success = true;
    response->message = "Io successfully updated.";
  }
}  // namespace ynx_robot_manager
