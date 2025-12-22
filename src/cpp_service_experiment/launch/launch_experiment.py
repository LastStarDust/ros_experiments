# -*- coding: utf-8 -*-

from launch import LaunchDescription
from launch_ros.actions import LifecycleNode, Node

def generate_launch_description():
    test_server_node = LifecycleNode(
        package='cpp_service_experiment',
        executable='test_server',
        name='test_server',
        namespace='',
        output='screen',
        respawn=True
    )

    test_client_node = Node(
        package='cpp_service_experiment',
        executable='test_client',
        name='test_client',
        output='screen'
    )

    return LaunchDescription([
        test_server_node,
        test_client_node
    ])