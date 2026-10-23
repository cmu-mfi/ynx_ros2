from setuptools import find_packages, setup

package_name = 'ynx_examples'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='alex',
    maintainer_email='alex@todo.todo',
    description='TODO: Package description',
    license='BSD-3-Clause',
    entry_points={
        'console_scripts': [
            'io_example = ynx_examples.io_example:main',
            'joint_goal_example = ynx_examples.joint_goal_example:main',
            'pose_goal_example = ynx_examples.pose_goal_example:main',
            'move_action_example = ynx_examples.move_action_example:main',
            'motion_testing = ynx_examples.motion_testing:main',
            'pose_servo_example = ynx_examples.pose_servo_example:main',
            'twist_servo_example = ynx_examples.twist_servo_example:main',
            'admittance_pose_servo_example = ynx_examples.admittance_pose_servo_example:main',
        ],
    },
)
