#include "ynx_hardware_interface/ynx_hardware_interface.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace ynx_hardware_interface
{

  hardware_interface::CallbackReturn YnxHardwareInterface::on_init(const hardware_interface::HardwareComponentInterfaceParams & params) {
    if (hardware_interface::SystemInterface::on_init(params) != hardware_interface::CallbackReturn::SUCCESS) {
      return hardware_interface::CallbackReturn::ERROR;
    }

    ip_= info_.hardware_parameters["ip"];
    port_= info_.hardware_parameters["port"];
    if (ip_.empty()) {
      RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), "[INIT] Ip address not provided!");
      return hardware_interface::CallbackReturn::ERROR;
    }
    if (port_.empty()) {
      RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), "[INIT] Port not provided!");
      return hardware_interface::CallbackReturn::ERROR;
    }

    // Initialize to 0.0 for the mock interface to avoid NaN errors in controllers
    position_states_.resize(info_.joints.size(), 0.0);
    previous_position_states_.resize(info_.joints.size(), 0.0);
    velocity_states_.resize(info_.joints.size(), 0.0);
    position_commands_.resize(info_.joints.size(), 0.0);
    previous_position_commands_.resize(info_.joints.size(), 0.0);

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn YnxHardwareInterface::on_configure(const rclcpp_lifecycle::State & /*previous_state*/) {
    // Create gRPC Channel
    RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[CONFIG] Connecting to %s:%s ...", ip_.c_str(), port_.c_str());
    grpc_channel_ = grpc::CreateChannel(ip_ + ":" + port_, grpc::InsecureChannelCredentials());
    // Create gRPC stubs
    monitor_stub_ = rcs::v1::RealtimeMonitorService::NewStub(grpc_channel_);
    motion_stub_ = rcs::v1::IncrementMoveBufferService::NewStub(grpc_channel_);
    servo_stub_ = rcs::v1::ServoPowerControlService::NewStub(grpc_channel_);
    alarm_stub_ = rcs::v1::AlarmControlService::NewStub(grpc_channel_);
    system_stub_ = rcs::v1::SystemInfoService::NewStub(grpc_channel_);
    io_stub_ = rcs::v1::IOService::NewStub(grpc_channel_);

    // 2. Perform Connection Check (Handshake)
    grpc::ClientContext context;
    // Set a 2-second deadline for the connection check
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
    context.set_deadline(deadline);

    rcs::v1::GetFirmwareVersionRequest version_req;
    rcs::v1::GetFirmwareVersionResponse version_res;
    grpc::Status status = system_stub_->GetFirmwareVersion(&context, version_req, &version_res);

    if (status.ok() && version_res.status() == rcs::v1::GetFirmwareVersionResponse::STATUS_SUCCESS) {
      RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[CONFIG] Connection established!");
    } else {
      RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), "[CONFIG] Failed to connect to controller! gRPC Error: %s (%d)", status.error_message().c_str(), status.error_code());
      return hardware_interface::CallbackReturn::FAILURE;
    }

    RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[CONFIG] Connection established!");

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn YnxHardwareInterface::on_activate(const rclcpp_lifecycle::State & /*previous_state*/) {
    grpc::ClientContext alarm_context;
    rcs::v1::ClearAlarmErrorRequest alarm_req;
    rcs::v1::ClearAlarmErrorResponse alarm_res;
    grpc::Status alarm_status = alarm_stub_->ClearAlarmError(&alarm_context, alarm_req, &alarm_res);
    if (alarm_status.ok() && alarm_res.status() == rcs::v1::ClearAlarmErrorResponse::STATUS_SUCCESS) {
      RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] Alarms cleared successfully!");
    } else {
      RCLCPP_WARN(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] Clear alarm command failed or returned non-success. Status: %d", alarm_res.status());
    }

    // Power on servos
    grpc::ClientContext servo_context;
    rcs::v1::PowerOnServosRequest servo_req;
    rcs::v1::PowerOnServosResponse servo_res;

    grpc::Status servo_status = servo_stub_->PowerOnServos(&servo_context, servo_req, &servo_res);

    if (servo_status.ok() && servo_res.status() == rcs::v1::PowerOnServosResponse::STATUS_SUCCESS) {
      RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] Servos powered on successfully!");
    } else {
      RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] Failed to turn on servos. Status: %d", servo_res.status());
      return hardware_interface::CallbackReturn::ERROR; 
    }

    // Sync States
    int max_retries = 5;
    bool read_success = false;

    for (int i = 0; i < max_retries; i++) {
      if (read(rclcpp::Time(), rclcpp::Duration::from_seconds(0.0)) == hardware_interface::return_type::OK) {
        read_success = true;
        break;
      }
      RCLCPP_WARN(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] Failed to read initial state. Retrying...");
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!read_success) {
      RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] Could not read starting position. Aborting activation.");
      return hardware_interface::CallbackReturn::ERROR;
    }
    for (uint i = 0; i < position_states_.size(); i++) {
      position_commands_[i] = position_states_[i];
      previous_position_commands_[i] = position_states_[i];
    }
    RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] States synced succesfully!");

    // Start Increment Move Buffer
    grpc::ClientContext motion_context;
    rcs::v1::StartIncrementMoveBufferRequest motion_req;
    rcs::v1::StartIncrementMoveBufferResponse motion_res;

    motion_req.set_control_group_bit(control_group_bit_);
    motion_req.set_trigger(0); // 0 starts motion immediately without waiting for queued buffer threshold

    grpc::Status status = motion_stub_->StartIncrementMoveBuffer(&motion_context, motion_req, &motion_res);

    if (status.ok() && motion_res.status() == rcs::v1::StartIncrementMoveBufferResponse::STATUS_SUCCESS) {
      task_no_ = motion_res.task_no();
      RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] Increment Motion Buffer started successfully!");
    } else {
      RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), "[ACTIVATION] Failed to start Increment Move Buffer. Status: %d", motion_res.status());
      return hardware_interface::CallbackReturn::ERROR;
    }

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn YnxHardwareInterface::on_deactivate(const rclcpp_lifecycle::State & /*previous_state*/) {
    if (task_no_ >= 0) {
      grpc::ClientContext context;
      rcs::v1::StopIncrementMoveBufferRequest req;
      rcs::v1::StopIncrementMoveBufferResponse res;

      req.set_task_no(task_no_);

      motion_stub_->StopIncrementMoveBuffer(&context, req, &res);
      task_no_ = -1;
    }

    grpc::ClientContext servo_context;
    rcs::v1::PowerOffServosRequest servo_req;
    rcs::v1::PowerOffServosResponse servo_res;

    grpc::Status servo_status = servo_stub_->PowerOffServos(&servo_context, servo_req, &servo_res);

    if (servo_status.ok() && servo_res.status() == rcs::v1::PowerOffServosResponse::STATUS_SUCCESS) {
      RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[DEACTIVATION] Servos powered off successfully.");
    } else {
      RCLCPP_WARN(rclcpp::get_logger("YnxHardwareInterface"), "[DEACTIVATION] Failed to turn off servos. Status: %d", servo_res.status());
    }

    RCLCPP_INFO(rclcpp::get_logger("YnxHardwareInterface"), "[DEACTIVATION] Succesfully deactivated!");

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::return_type YnxHardwareInterface::read(const rclcpp::Time & /*time*/, const rclcpp::Duration & period) {
    // Monitor Controller Status
    grpc::ClientContext status_context;
    rcs::v1::GetAlarmErrorRequest status_req;
    rcs::v1::GetAlarmErrorResponse status_res;
    grpc::Status status_grpc = alarm_stub_->GetAlarmError(&status_context, status_req, &status_res);

    if (status_grpc.ok() && status_res.status() == rcs::v1::GetAlarmErrorResponse::STATUS_SUCCESS) {
      bool has_active_faults = (status_res.alarms_size() > 0 || status_res.errors_size() > 0);
      if (has_active_faults) {
        RCLCPP_ERROR_THROTTLE(rclcpp::get_logger("YnxHardwareInterface"), *this->get_clock(), 1000, "[READ] Robot has %d active alarms and %d active errors! Halting.", status_res.alarms_size(), status_res.errors_size());
        return hardware_interface::return_type::ERROR; 
      }
    }

    // Read Joint Positions
    grpc::ClientContext feedback_context;
    rcs::v1::GetFeedbackAxesPosRequest request;
    rcs::v1::GetFeedbackAxesPosResponse response;
    request.set_group_no(group_no_); 
    grpc::Status status = monitor_stub_->GetFeedbackAxesPos(&feedback_context, request, &response);

    if (status.ok()) {
      if (response.status() == rcs::v1::GetFeedbackAxesPosResponse::STATUS_SUCCESS) {
        for (uint i = 0; i < info_.joints.size(); i++) {
          // Extract Position
          previous_position_states_[i] = position_states_[i];
          double joint_pos_degrees = response.axes_pos().pos(i); 
          position_states_[i] = joint_pos_degrees * (M_PI / 180.0);

          // Calculate Velocity
          if (period.seconds() > 0.0) {
            velocity_states_[i] = (position_states_[i] - previous_position_states_[i]) / period.seconds();
          }
        }
      } else {
        RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), 
            "[READ] Robot internal error. Status code: %d", response.status());
        return hardware_interface::return_type::ERROR;
      }
    } else {
      RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), 
          "[READ] gRPC Call failed: %s", status.error_message().c_str());
      return hardware_interface::return_type::ERROR;
    }

    // Prepare I/O Request for inputs 1-10 (10-19) and outputs 1-10 (10010-10019)
    grpc::ClientContext io_context;
    rcs::v1::GetIOStatusRequest io_req;
    rcs::v1::GetIOStatusResponse io_res;

    for (uint32_t addr = 10; addr < 20; ++addr) {
      io_req.add_addresses(addr); // General Inputs
    }
    for (uint32_t addr = 10010; addr < 10020; ++addr) {
      io_req.add_addresses(addr); // General Outputs
    }
    // Execute gRPC Call
    grpc::Status io_status = io_stub_->GetIOStatus(&io_context, io_req, &io_res);
    if (!io_status.ok()) {
      // Handle gRPC transport level failure
      RCLCPP_WARN_THROTTLE(
          rclcpp::get_logger("YnxHardwareInterface"), *this->get_clock(), 2000,
          "[READ] I/O gRPC call failed. Error Code: %d, Message: %s",
          io_status.error_code(), io_status.error_message().c_str());
    } else if (io_res.status() != rcs::v1::GetIOStatusResponse::STATUS_SUCCESS) {
      // Handle controller API level status error
      RCLCPP_WARN_THROTTLE(
          rclcpp::get_logger("YnxHardwareInterface"), *this->get_clock(), 2000,
          "[READ] GetIOStatus returned non-success response status: %d",
          static_cast<int>(io_res.status()));
    } else {
      // Successfully update internal buffers
      for (int i = 0; i < 10; ++i) {
        gpio_input_states_[i] = static_cast<double>(io_res.io_response(i).value());
          gpio_output_states_[i] = static_cast<double>(io_res.io_response(i + 10).value());
      }
    }

    return hardware_interface::return_type::OK;
  }

  hardware_interface::return_type YnxHardwareInterface::write(const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/) {
    // Safety check to ensure we have a valid task handle
    if (task_no_ < 0) {
      return hardware_interface::return_type::ERROR;
    }

    // Create incremental motion buffer request
    grpc::ClientContext context;
    rcs::v1::SendIncrementMoveBufferRequest req;
    rcs::v1::SendIncrementMoveBufferResponse res;

    req.set_task_no(task_no_);
    req.set_timeout(100); 
    req.set_group_num(1);     // Number of control groups in this request
    req.set_position_num(1);  // Number of positions per group in this request

    rcs::v1::IncrementMoveGroupRequest* group_req = req.add_requests();
    group_req->set_group_no(group_no_);
    rcs::v1::AxesPos* angle_pos = group_req->mutable_angle();

    // Calculate the angle position delta
    for (uint i = 0; i < info_.joints.size(); i++) {
      double delta_rad = position_commands_[i] - previous_position_commands_[i];
      previous_position_commands_[i] = position_commands_[i];
      double delta_deg = delta_rad * (180.0 / M_PI);
      angle_pos->add_pos(delta_deg);
    }

    // Send the incremental movement buffer
    grpc::Status status = motion_stub_->SendIncrementMoveBuffer(&context, req, &res);

    if (!status.ok() || res.status() != rcs::v1::SendIncrementMoveBufferResponse::STATUS_SUCCESS) {
      RCLCPP_ERROR(rclcpp::get_logger("YnxHardwareInterface"), 
          "[WRITE] Failed to send Increment Move Buffer. gRPC ok: %d, Response status: %d", 
          status.ok(), res.status());
      return hardware_interface::return_type::ERROR;
    }

    // Write IO ports
    rcs::v1::SetIOStatusRequest set_io_req;
    for (size_t i = 0; i < 10; ++i) {
      if (gpio_output_commands_[i] != gpio_output_states_[i]) {
        auto* io_req = set_io_req.add_io_request();
        io_req->set_address(10018);
        io_req->set_value(static_cast<uint32_t>(1));
      }
    }
    if (set_io_req.io_request_size() > 0) {
      grpc::ClientContext set_io_context;
      rcs::v1::SetIOStatusResponse set_io_res;
      grpc::Status status = io_stub_->SetIOStatus(&set_io_context, set_io_req, &set_io_res);
      if (!status.ok() || set_io_res.status() != rcs::v1::SetIOStatusResponse::STATUS_SUCCESS) {
        RCLCPP_ERROR(
            rclcpp::get_logger("YnxHardwareInterface"), 
            "[READ] SetIOStatus returned non-success response status: %d",
            static_cast<int>(set_io_res.status()));
      }
    }

    return hardware_interface::return_type::OK;
  }

  std::vector<hardware_interface::StateInterface> YnxHardwareInterface::export_state_interfaces() {
    std::vector<hardware_interface::StateInterface> state_interfaces;
    // Export joint interfaces
    for (uint i = 0; i < info_.joints.size(); i++) {
      state_interfaces.emplace_back(hardware_interface::StateInterface(
            info_.joints[i].name, hardware_interface::HW_IF_POSITION, &position_states_[i]));
      state_interfaces.emplace_back(hardware_interface::StateInterface(
            info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &velocity_states_[i]));
    }
    // Export GPIO interfaces for ports 1-10
    for (size_t i = 0; i < 10; ++i) {
      state_interfaces.emplace_back("gpio_io", "digital_input_" + std::to_string(i + 1), &gpio_input_states_[i]);
      state_interfaces.emplace_back("gpio_io", "digital_output_" + std::to_string(i + 1), &gpio_output_states_[i]);
    }
    return state_interfaces;
  }

  std::vector<hardware_interface::CommandInterface> YnxHardwareInterface::export_command_interfaces() {
    std::vector<hardware_interface::CommandInterface> command_interfaces;
    // Export joint interfaces
    for (uint i = 0; i < info_.joints.size(); i++) {
      command_interfaces.emplace_back(hardware_interface::CommandInterface(
            info_.joints[i].name, hardware_interface::HW_IF_POSITION, &position_commands_[i]));
    }
    // Export GPIO command interfaces for outputs 1-10
    for (size_t i = 0; i < 10; ++i) {
      command_interfaces.emplace_back("gpio_io", "digital_output_" + std::to_string(i + 1), &gpio_output_commands_[i]);
    }
    return command_interfaces;
  }

}  // namespace ynx_hardware_interface

// Export the class to pluginlib so it can be dynamically loaded
PLUGINLIB_EXPORT_CLASS(
    ynx_hardware_interface::YnxHardwareInterface, hardware_interface::SystemInterface)
