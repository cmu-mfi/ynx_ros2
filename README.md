# YNX ROS2
This repository contains packages and examples for integrating and controlling Yaskawa YNX Robots using ROS 2.

## 📂 Repository Structure

This workspace is divided into two primary packages:

  * **`ynx_bringup`**: Contains essential launch files, configurations, and parameter files required to spin up the robot drivers, controllers, MoveIt2 nodes and the robot manager.
  * **`ynx_examples`**: Contains sample Python nodes and scripts demonstrating how to send joint and pose goals, interact with the MoveIt2 API, use moveit_servo and perform basic operations with the robot using ROS 2.
  * **`ynx_robot_manager`**: Contains the ynx_robot_manager node. It abstracts the moveit interface and handles other robot specific tasks like IO, payload configuration and force sensor filtering.

## ⚙️ Prerequisites and Dependencies

This repository has been tested on:

  * **OS:** Ubuntu 24.04 LTS
  * **ROS 2:** Jazzy

### Install dependencies using rosdep
Clone this repository into the `src` directory of your ros2 workspace.

```bash
cd <your-ros2-workspace>
rosdep update
rosdep install --from-paths src --ignore-src -r -y
```

## 🚀 Usage

### 1\. Launching the Bringup

To start the driver and moveit environment, use the bringup launch file:

```bash
ros2 launch ynx_bringup bringup.launch.py
```

**Launch file arguments:**

You can configure the launch file by passing these arguments via the command line:

| Argument | Description | Default Value | Available Choices |
| :--- | :--- | :--- | :--- |
| `log_level` | The ROS logging level to use across all nodes. | `error` | `info`, `debug`, `error` |
| `ns` | Namespace of the robot. Used as a prefix, which is required if running multiple robots on the same network. | `""` (empty) | - |
| `ip` | IP address of the robot. | `192.168.19.201` | - |
| `port` | Port by which the robot can be reached. | `50300` | - |
| `model` | The specific type/series of Yaskawa robot being used. | `nex10` | `nex10` |
| `launch_rviz` | Whether to launch RViz for visualization. | `false` | `true`, `false` |
| `launch_servo` | Whether to launch MoveIt Servo for real-time control. | `true` | `true`, `false` |
| `use_mock_hardware`| Start the robot with mock hardware, mirroring commands directly to its states (useful for testing without physical hardware). | `false` | `true`, `false` |

**INFO:**
Because of a bug in RVIZ2, RVIZ2 will not work when using a namespace!

**Example:**
```bash
ros2 launch ynx_bringup bringup.launch.py log_level:=error ns:=nex10 ip:=192.168.19.201 port:=50300 launch_rviz:=false launch_servo:=true use_mock_hardware:=false model:=nex10
```

### 2\. Run an Example Python Script

The `ynx_examples` package includes several Python nodes designed to demonstrate different ways to interact with the Robot using ROS 2. 

Below is a breakdown of the available example scripts:

| Script Name | Description |
| :--- | :--- |
| `joint_goal_example` | Sends a joint goal to the ynx_robot_manager (WARNING: Will move the robot) |
| `pose_goal_example` | Sends a pose goal to the ynx_robot_manager (WARNING: Will move the robot) |
| `servo_example` | Sends twists and poses to moveit_servo (WARNING: Will move the robot) |
| `io_example` | Write the IO of the robot using the `io_and_status_controller`. |
| `move_action_example` | Use move_action interface of the move_group directly and skip ynx_robot_manager |

**Example:**
```bash
ros2 run ynx_examples joint_goal_example --ros-args -p ns:=nex10
```


## Build ACU docker image

```
sudo ./acu_build_img -i ynx_ros2 -t <tag>  -v
```
