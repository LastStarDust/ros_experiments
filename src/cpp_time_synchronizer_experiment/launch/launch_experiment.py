# -*- coding: utf-8 -*-

from launch import LaunchDescription
from launch_ros.actions import LifecycleNode

def generate_launch_description():
    test_time_synchronizer_node = LifecycleNode(
        package='cpp_time_synchronizer_experiment',
        executable='test_time_synchronizer',
        name='test_time_synchronizer',
        namespace='',
        output='screen',
        autostart=True,
    )

    return LaunchDescription([test_time_synchronizer_node])