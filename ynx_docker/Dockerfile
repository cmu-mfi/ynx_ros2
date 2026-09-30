FROM ros:jazzy-ros-base

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=jazzy

# Install system dependencies, gRPC, and ROS2 packages manually
RUN apt-get update && apt-get install -y --no-install-recommends \
    protobuf-compiler-grpc \
    protobuf-compiler \
    libprotobuf-dev \
    libgrpc-dev \
    libgrpc++-dev \
    libabsl-dev \
    python3-colcon-common-extensions \
    git \
    bash-completion \
    net-tools \
    iputils-ping \
    openssh-server \
    ros-${ROS_DISTRO}-moveit \
    ros-${ROS_DISTRO}-moveit-servo \
    ros-${ROS_DISTRO}-pilz-industrial-motion-planner \
    ros-${ROS_DISTRO}-ros2-control \
    ros-${ROS_DISTRO}-ros2-controllers \
    ros-${ROS_DISTRO}-ros2controlcli \
    ros-${ROS_DISTRO}-tf-transformations \
    ros-jazzy-rmw-cyclonedds-cpp \
    vim

# SSH Server Setup
RUN mkdir /var/run/sshd \
    && echo 'root:root' | chpasswd \
    && sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config \
    && sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config
EXPOSE 22

# Setup workspace and copy repository content
WORKDIR /ws
COPY ./robot_manager_ros2 /ws/src/robot_manager_ros2
COPY ./ynx_bringup /ws/src/ynx_bringup
COPY ./ynx_examples /ws/src/ynx_examples
COPY ./ynx_description /ws/src/ynx_description
COPY ./ynx_robot_manager /ws/src/ynx_robot_manager
COPY ./ynx_hardware_interface /ws/src/ynx_hardware_interface

# Build ROS2 workspace
RUN bash -c "source /opt/ros/${ROS_DISTRO}/setup.bash \
    && cd /ws \
    && colcon build --event-handlers console_cohesion+"

# Automatically source ROS2 and workspace setup files for interactive shells
RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> /root/.bashrc \
    && echo "source /ws/install/setup.bash" >> /root/.bashrc

ENV ROS_DOMAIN_ID=19
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

CMD ["/usr/sbin/sshd", "-D"]
