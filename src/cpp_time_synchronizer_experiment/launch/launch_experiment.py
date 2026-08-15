from launch import LaunchDescription
from launch.actions import ExecuteProcess, TimerAction
from launch_ros.actions import Node


def generate_launch_description():
    test_time_synchronizer_node = Node(
        package="cpp_time_synchronizer_experiment",
        executable="test_time_synchronizer",
        name="test_time_synchronizer",
        namespace="",
        output="screen",
    )

    publish_measurements = ExecuteProcess(
        cmd=[
            "bash",
            "-lc",
            (
                "for i in $(seq 1 10); do "
                "t=$((100 + i)); "
                'a="0.$((RANDOM % 1000))"; '
                'b="0.$((RANDOM % 1000))"; '
                'ros2 topic pub --once /sensor_a_topic cpp_time_synchronizer_experiment/msg/SensorA "{time: $t, value: $a}" >/dev/null; '
                'ros2 topic pub --once /sensor_b_topic cpp_time_synchronizer_experiment/msg/SensorB "{time: $t, value: $b}" >/dev/null; '
                "done"
            ),
        ],
        output="screen",
    )

    delayed_publisher = TimerAction(period=2.0, actions=[publish_measurements])

    return LaunchDescription([test_time_synchronizer_node, delayed_publisher])
