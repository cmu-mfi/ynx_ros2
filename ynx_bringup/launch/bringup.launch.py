from launch.conditions import IfCondition
from ament_index_python.packages import get_package_share_directory
import os
import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, SetLaunchConfiguration, OpaqueFunction, IncludeLaunchDescription 
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterFile
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution, PythonExpression 
from launch_ros.parameter_descriptions import ParameterValue
from launch.launch_description_sources import PythonLaunchDescriptionSource

def launch_setup(context):
    # Load parameters
    log_level = context.launch_configurations['log_level']
    ns = context.launch_configurations['ns']
    model = context.launch_configurations['model']
    ip = context.launch_configurations['ip']
    port = context.launch_configurations['port']
    use_mock_hardware = context.launch_configurations["use_mock_hardware"]
    use_ft_sensor = context.launch_configurations['use_ft_sensor']
    ft_sensor_ip = context.launch_configurations['ft_sensor_ip']
    launch_rviz = context.launch_configurations["launch_rviz"]
    launch_servo = context.launch_configurations["launch_servo"]

    # print parameters
    print("")
    print("Starting bringup with paramaters:")
    print(" log_level:           " + log_level)
    if ns == "":
        print(" ns:                  " + "/")
    else:
        print(" ns:                  " + "/" + ns)
    print(" model:               " + model)
    print(" ip:                  " + ip)
    print(" port:                " + port)
    print(" use_mock_hardware:   " + use_mock_hardware)
    if use_mock_hardware == "false":
        print(" ip:                  " + ip)
        print(" port:                " + port)
    print(" use_ft_sensor:       " + use_ft_sensor)
    if use_ft_sensor == "true":
        print(" ft_sensor_ip:        " + ft_sensor_ip)
    print(" launch_servo:        " + launch_servo)
    print(" launch_rviz:         " + launch_rviz)
    print("")

    # prefix for packages
    pkg_prefix = "ynx_"

    # launch files
    driver_launch_path = PathJoinSubstitution([FindPackageShare(pkg_prefix+'bringup'), 'launch', 'driver.launch.py'])
    moveit_launch_path = PathJoinSubstitution([FindPackageShare(pkg_prefix+'bringup'), 'launch', 'moveit.launch.py'])
    robot_manager_launch_path = PathJoinSubstitution([FindPackageShare(pkg_prefix+'bringup'), 'launch', 'robot_manager.launch.py'])

    driver = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(driver_launch_path),
            launch_arguments={
                'log_level': log_level,
                'ns': ns,
                'model': model,
                'ip': ip,
                'port': port,
                'use_mock_hardware': use_mock_hardware,
                'use_ft_sensor': use_ft_sensor,
                'ft_sensor_ip': ft_sensor_ip,
                }.items()
            )

    moveit = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(moveit_launch_path),
            launch_arguments={
                'log_level': log_level,
                'ns': ns,
                'model': model,
                'launch_servo': launch_servo,
                'launch_rviz': launch_rviz,
                }.items()
            )

    robot_manager = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(robot_manager_launch_path),
            launch_arguments={
                'log_level': log_level,
                'ns': ns,
                'model': model,
                }.items()
            )

    return [driver, moveit, robot_manager]

def generate_launch_description():
    # add launch arguments
    declared_arguments = []
    declared_arguments.append(
            DeclareLaunchArgument(
                'log_level',
                default_value='error',
                description="Log Level to use for all nodes",
                choices=["info", "debug", "error"],
                )
            )
    declared_arguments.append(
            DeclareLaunchArgument(
                'ns',
                default_value='',
                description='namespace of the robot (used as prefix, so needed if running multiple robots)'
                )
            )
    declared_arguments.append(
            SetLaunchConfiguration('tf_prefix', PythonExpression(["'", LaunchConfiguration('ns'), "' + '_' if '", LaunchConfiguration('ns'), "' else ''"]))
            )
    declared_arguments.append(
            DeclareLaunchArgument(
                'model',
                default_value='nex10',
                description="Type/series of used YNX robot used.",
                choices=[
                    "nex10",
                    ],
                )
            )
    declared_arguments.append(
            DeclareLaunchArgument(
                "ip", 
                default_value="192.168.19.201",
                description="IP address by which the robot can be reached."
                )
            )
    declared_arguments.append(
            DeclareLaunchArgument(
                "port", 
                default_value="50300",
                description="Port by which the robot can be reached."
                )
            )
    declared_arguments.append(
            DeclareLaunchArgument(
                "use_mock_hardware",
                default_value="false",
                description="Start robot with mock hardware mirroring command to its states.",
                )
            )
    declared_arguments.append(
            DeclareLaunchArgument(
                "use_ft_sensor", 
                default_value="false",
                description="Use the netft force torque sensor."
                )
            )
    declared_arguments.append(
            DeclareLaunchArgument(
                "ft_sensor_ip", 
                default_value="192.168.19.210",
                description="IP address by which the netft force torque sensor can be reached."
                )
            )
    declared_arguments.append(
            DeclareLaunchArgument("launch_servo", default_value="true", description="Launch Moveit Servo?"),
            )
    declared_arguments.append(
            DeclareLaunchArgument("launch_rviz", default_value="false", description="Launch RViz?"),
            )
    return LaunchDescription(declared_arguments + [OpaqueFunction(function=launch_setup)])
