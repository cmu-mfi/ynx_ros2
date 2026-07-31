# Dockerfile for ynx_ros2 — compiles all ynx ROS2 packages and launches the bringup.
# Tested on Ubuntu 24.04 / ROS 2 Jazzy.
#
# Build:
#   docker build -t ynx_ros2 .
#
# Run (mock hardware, no real robot needed):
#   docker run --rm --env-file .env ynx_ros2
#
# Drop into a shell (workspace already sourced):
#   docker run --rm -it ynx_ros2 bash
#
# Override individual launch args (appended to the bringup command):
#   docker run --rm --env-file .env ynx_ros2 bringup use_mock_hardware:=false ip:=192.168.19.201

FROM osrf/ros:jazzy-desktop

# Avoid interactive tzdata/geo prompts during apt installs
ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=jazzy

# --- 1. System + ROS build dependencies -------------------------------------------
# gRPC / protobuf are required by ynx_hardware_interface (not shipped in the base image).
# python3-colcon-common-extensions provides colcon for the build step.
RUN apt-get update && apt-get install -y --no-install-recommends \
        protobuf-compiler-grpc \
        protobuf-compiler \
        libprotobuf-dev \
        libgrpc-dev \
        libgrpc++-dev \
        libabsl-dev \
        python3-colcon-common-extensions \
        python3-rosdep \
        git \
        bash-completion \
    && rm -rf /var/lib/apt/lists/*

# Initialise rosdep if not already done (base image may have it initialised)
RUN if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then \
        rosdep init || true; \
    fi \
    && rosdep update

# --- 2. Workspace setup -----------------------------------------------------------
WORKDIR /ws
RUN mkdir -p /ws/src

# --- 3. External interface package (required by ynx_robot_manager + ynx_examples) --
# robot_manager_interfaces provides the JointGoal/PoseGoal/Home/Park/SetPayload/SetIo
# action/srv types. Not available via apt, so cloned from source.
RUN git clone --depth 1 https://github.com/cmu-mfi/robot_manager_ros2.git /ws/src/robot_manager_ros2

# --- 4. Copy this repo into the workspace ----------------------------------------
COPY . /ws/src/ynx_ros2

# --- 5. Install remaining ROS dependencies via rosdep ----------------------------
# `--ignore-src` skips packages already in src; `-r` resolves repeatedly; `-y` non-interactive.
# This pulls moveit2, moveit_servo, pilz_industrial_motion_planner, ros2_control,
# ros2_controllers, ros2controlcli, ur (-> ur_msgs), tf_transformations, etc.
RUN apt-get update \
    && bash -c "source /opt/ros/${ROS_DISTRO}/setup.bash \
        && rosdep install --from-paths /ws/src --ignore-src -r -y" \
    && rm -rf /var/lib/apt/lists/*

# --- 6. Build all packages --------------------------------------------------------
# --symlink-install keeps python packages editable and saves image size.
# Build robot_manager_interfaces first (ynx_robot_manager/ynx_examples depend on it),
# then the rest. colcon resolves the order via package.xml deps, so a single build works.
RUN bash -c "source /opt/ros/${ROS_DISTRO}/setup.bash \
    && cd /ws \
    && colcon build --symlink-install --event-handlers console_cohesion+"

# --- 7. Runtime configuration -----------------------------------------------------
# Default ENV values mirror .env.example so the image runs out-of-the-box with mock hardware
# and no FT sensor (netft_hardware_interface is not included in this image).
ENV LOG_LEVEL=error
ENV ROBOT_NS=
ENV ROBOT_MODEL=nex10
ENV ROBOT_IP=192.168.19.201
ENV ROBOT_PORT=50300
ENV USE_MOCK_HARDWARE=true
ENV LAUNCH_SERVO=true

# Entrypoint sources the workspaces; `bringup` arg launches the default bringup.
RUN chmod +x /ws/src/ynx_ros2/entrypoint.sh
ENTRYPOINT ["/ws/src/ynx_ros2/entrypoint.sh"]
CMD ["bringup"]
