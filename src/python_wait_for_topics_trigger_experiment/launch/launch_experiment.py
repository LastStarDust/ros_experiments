from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='python_wait_for_topics_trigger_experiment',
            executable='repeater',
            name='wait_for_topics_repeater',
            output='screen',
        ),
    ])
