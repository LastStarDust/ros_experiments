import math
import time
import unittest

import launch
import launch_ros.actions
import launch_testing
import launch_testing.actions
import launch_testing.markers
from launch_testing_ros import WaitForTopics
from launch_testing_ros.actions import EnableRmwIsolation
import pytest
from geometry_msgs.msg import Twist
from rclpy import qos
from turtlesim_msgs.msg import Pose


@pytest.mark.launch_test
@launch_testing.markers.keep_alive
def generate_test_description():
    return launch.LaunchDescription([
        EnableRmwIsolation(),
        launch_ros.actions.Node(
            package='turtlesim',
            namespace='',
            executable='turtlesim_node',
            name='turtle1',
            output='screen',
        ),
        launch_testing.actions.ReadyToTest(),
    ])


def trigger_publish_twist(node, linear_x, angular_z):
    if not hasattr(node, 'cmd_vel_publisher'):
        node.cmd_vel_publisher = node.create_publisher(
            Twist,
            'turtle1/cmd_vel',
            qos.QoSProfile(depth=10),
        )

    start = time.time()
    while node.cmd_vel_publisher.get_subscription_count() == 0:
        if time.time() - start > 5.0:
            raise RuntimeError('Timed out waiting for turtlesim cmd_vel subscriber')
        time.sleep(0.05)

    # Force wait() to observe at least one post-trigger pose sample.
    node.msg_event_object.clear()

    msg = Twist()
    msg.linear.x = linear_x
    msg.angular.z = angular_z
    for _ in range(5):
        node.cmd_vel_publisher.publish(msg)
        time.sleep(0.1)


class TestTurtleSimWithWaitForTopics(unittest.TestCase):

    def test_publishes_pose_with_wait_for_topics(self):
        with WaitForTopics(
            [('turtle1/pose', Pose)],
            timeout=10.0,
            messages_received_buffer_length=50,
        ) as waiter:
            assert waiter.topics_received() == {'turtle1/pose'}
            assert waiter.topics_not_received() == set()
            assert len(waiter.received_messages('turtle1/pose')) >= 1

    def test_moves_with_triggered_twist(self):
        waiter = WaitForTopics(
            [('turtle1/pose', Pose)],
            timeout=10.0,
            messages_received_buffer_length=200,
            trigger=trigger_publish_twist,
        )
        try:
            assert waiter.wait(2.0, math.pi / 6.0)
            poses = waiter.received_messages('turtle1/pose')
            assert len(poses) >= 1
            assert any(
                abs(p.linear_velocity) > 0.0 or abs(p.angular_velocity) > 0.0
                for p in poses
            )
        finally:
            waiter.shutdown()

    def test_logs_spawning(self, proc_output):
        proc_output.assertWaitFor(
            'Spawning turtle [turtle1] at x=',
            timeout=5,
            stream='stderr',
        )


@launch_testing.post_shutdown_test()
class TestTurtleSimShutdown(unittest.TestCase):

    def test_exit_codes(self, proc_info):
        launch_testing.asserts.assertExitCodes(proc_info)
