#!/bin/bash
set -e

# Source ROS 2 and the built workspace
source /opt/ros/jazzy/setup.bash
if [ -f /ws/install/setup.bash ]; then
    source /ws/install/setup.bash
fi

# Default: launch the ynx bringup using ENV-configured arguments.
# Override with any other command: `docker run <image> bash`
if [ "$1" = "bringup" ]; then
    shift
    # Build the launch arg list (omit ns:= when empty, ROS2 rejects 'ns:=')
    ARGS=(
        log_level:=${LOG_LEVEL}
        model:=${ROBOT_MODEL}
        ip:=${ROBOT_IP}
        port:=${ROBOT_PORT}
        use_mock_hardware:=${USE_MOCK_HARDWARE}
        use_ft_sensor:=${USE_FT_SENSOR}
        ft_sensor_ip:=${FT_SENSOR_IP}
        launch_servo:=${LAUNCH_SERVO}
        launch_rviz:=${LAUNCH_RVIZ}
    )
    if [ -n "${ROBOT_NS}" ]; then
        ARGS+=(ns:=${ROBOT_NS})
    fi
    exec ros2 launch ynx_bringup bringup.launch.py "${ARGS[@]}" "$@"
else
    exec "$@"
fi
