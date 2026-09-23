from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, SetLaunchConfiguration, OpaqueFunction
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterFile
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution, PythonExpression 
from launch_ros.parameter_descriptions import ParameterValue

def launch_setup(context):
    # Load parameters
    log_level = context.launch_configurations['log_level']
    ns = context.launch_configurations['ns']
    tf_prefix = context.launch_configurations['tf_prefix']
    model = context.launch_configurations['model']
    ip = context.launch_configurations['ip']
    port = context.launch_configurations['port']
    use_mock_hardware = context.launch_configurations['use_mock_hardware']
    use_ft_sensor = context.launch_configurations['use_ft_sensor']
    ft_sensor_ip = context.launch_configurations['ft_sensor_ip']

    # print parameters
    print("")
    print("Starting driver with paramaters:")
    print(" log_level:           " + log_level)
    if ns == "":
        print(" ns:                  " + "/")
    else:
        print(" ns:                  " + "/" + ns)
    print(" model:               " + model)
    print(" use_mock_hardware:   " + use_mock_hardware)
    if use_mock_hardware == "false":
        print(" ip:                  " + ip)
        print(" port:                " + port)
    print(" use_ft_sensor:       " + use_ft_sensor)
    if use_ft_sensor == "true":
        print(" ft_sensor_ip:        " + ft_sensor_ip)
    print("")

    # prefix for packages
    pkg_prefix = "ynx_"

    # robot description
    urdf_file_path = PathJoinSubstitution(
            [FindPackageShare(pkg_prefix+"description"), "urdf", model, model+".urdf.xacro"]
            )
    urdf_content = Command(
            [
                PathJoinSubstitution([FindExecutable(name="xacro")]),
                " ",
                urdf_file_path,
                " ",
                "ip:=",
                ip,
                " ",
                "port:=",
                port,
                " ",
                "tf_prefix:=",
                tf_prefix,
                " ",
                "use_mock_hardware:=",
                use_mock_hardware,
                " ",
                "use_ft_sensor:=",
                use_ft_sensor,
                " ",
                "ft_sensor_ip:=",
                ft_sensor_ip,
                ])
    robot_description = {
            "robot_description": ParameterValue(urdf_content, value_type=str)
            }

    # ros2_control config
    ros2_controllers_file = PathJoinSubstitution(
            [FindPackageShare(pkg_prefix+"bringup"), "config", "ros2_controllers.yaml"]
            )

    # nodes
    nodes = []

    nodes.append(Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        namespace=ns,
        arguments=["--ros-args", "--log-level", log_level],
        parameters=[
            robot_description,
            {'publish_frequency': 500.0}
            ]
        ))

    nodes.append(Node(
        package="controller_manager",
        executable="ros2_control_node",
        namespace=ns,
        parameters=[
            ParameterFile(ros2_controllers_file, allow_substs=True),
            robot_description
            ],
        arguments=["--ros-args", "--log-level", log_level],
        output="screen",
        ))

    nodes.append(Node(
        package="controller_manager",
        executable="spawner",
        namespace=ns,
        arguments=[
            "joint_state_broadcaster",
            "--ros-args", "--log-level", log_level,
            ]
        ))

    nodes.append(Node(
        package="controller_manager",
        executable="spawner",
        namespace=ns,
        arguments=[
            "joint_trajectory_controller",
            "--ros-args", "--log-level", log_level,
            ]
        ))

    if use_ft_sensor == "true":
        nodes.append(Node(
            package="controller_manager",
            executable="spawner",
            namespace=ns,
            arguments=[
                "force_torque_sensor_broadcaster",
                "--ros-args", "--log-level", log_level,
                ]
            ))

    nodes.append(Node(
        package="controller_manager",
        executable="spawner",
        namespace=ns,
        arguments=[
            "gpio_command_controller",
            "--ros-args", "--log-level", log_level,
            ]
        ))

    return nodes

def generate_launch_description():
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
                default_value="true",
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
    return LaunchDescription(declared_arguments + [OpaqueFunction(function=launch_setup)])
