#include "ynx_robot_manager/ynx_robot_manager.hpp"
#include <algorithm>
#include <cstdlib>

namespace ynx_robot_manager {

  // --- Readiness - Setup --- ///
  void YnxRobotManager::readiness_setup() {
    this->get_parameter_or<std::string>("hardware_component_name", hardware_component_name_, "nex10");
    this->get_parameter_or<std::vector<std::string>>(
        "required_controllers", required_controllers_,
        {"joint_state_broadcaster", "joint_trajectory_controller", "gpio_command_controller"});
    readiness_cb_group_ = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    rclcpp::SubscriptionOptions options;
    options.callback_group = readiness_cb_group_;
    auto status_qos = rclcpp::QoS(1).reliable().transient_local();
    controller_activity_subscriber_ = this->create_subscription<ControllerManagerActivity>(
        "controller_manager/activity", status_qos,
        std::bind(&YnxRobotManager::readiness_activity_callback_, this, std::placeholders::_1), options);
  }

  // --- Readiness - Activity Callback --- ///
  void YnxRobotManager::readiness_activity_callback_(const ControllerManagerActivity::SharedPtr msg) {
    hardware_ready_ = false;
    for (const auto& component : msg->hardware_components) {
      if (component.name == hardware_component_name_) {
        hardware_ready_ = component.state.id == lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE;
        break;
      }
    }
    controllers_ready_ = !required_controllers_.empty();
    for (const auto& name : required_controllers_) {
      auto controller = std::find_if(msg->controllers.begin(), msg->controllers.end(),
          [&name](const auto& state) { return state.name == name; });
      if (controller == msg->controllers.end() ||
          controller->state.id != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
        controllers_ready_ = false;
        break;
      }
    }
    readiness_update();
  }

  // --- Readiness - Update --- ///
  void YnxRobotManager::readiness_update() {
    std::string reason;
    if (!hardware_ready_) {
      reason = "Robot hardware is not active or its state is unavailable.";
    } else if (!controllers_ready_) {
      reason = "Required controllers are not active or their state is unavailable.";
    }

    bool readiness_lost;
    {
      std::lock_guard<std::mutex> lock(control_mode_mutex_);
      bool ready = reason.empty();
      readiness_lost = control_mode_ != ControlMode::NOT_READY && !ready;
      if (control_mode_ == ControlMode::NOT_READY && ready) {
        control_mode_ = ControlMode::IDLE;
      }
    }
    if (readiness_lost) {
      RCLCPP_FATAL(this->get_logger(), "[Readiness] %s Exiting robot manager.", reason.c_str());
      std::exit(EXIT_FAILURE);
    }
    if (reason != readiness_reason_) {
      if (reason.empty()) {
        RCLCPP_INFO(this->get_logger(), "[Readiness] Robot is ready.");
      } else {
        RCLCPP_WARN(this->get_logger(), "[Readiness] %s", reason.c_str());
      }
      readiness_reason_ = reason;
    }
  }

  // --- Readiness - Command Check --- ///
  bool YnxRobotManager::is_robot_ready() {
    std::lock_guard<std::mutex> lock(control_mode_mutex_);
    return control_mode_ != ControlMode::NOT_READY;
  }

}  // namespace ynx_robot_manager
