import math
import threading
import unittest
from typing import Any

import launch
import launch_ros.actions
import launch_testing
import launch_testing.actions
import launch_testing.markers
import pytest
from geometry_msgs.msg import Twist
from launch_testing_ros import WaitForTopics
from launch_testing_ros.actions import EnableRmwIsolation
from rclpy.node import Node
from rclpy.publisher import PublisherEventCallbacks
from turtlesim_msgs.msg import Pose


@pytest.mark.launch_test
@launch_testing.markers.keep_alive
def generate_test_description() -> launch.LaunchDescription:
    """Create the launch description for turtlesim integration tests.

    Returns:
        launch.LaunchDescription: Launch actions that start turtlesim and
            signal when tests can begin.
    """
    return launch.LaunchDescription(
        [
            # Action which enables isolation of ROS communication using rmw_test_fixture
            EnableRmwIsolation(),
            # Node under test: turtlesim_node
            launch_ros.actions.Node(
                package="turtlesim",
                namespace="",
                executable="turtlesim_node",
                name="turtle1",
                output="screen",
            ),
            launch_testing.actions.ReadyToTest(),
        ]
    )


def trigger_publish_twist(node: Node, linear_x: float, angular_z: float) -> None:
    """Publish Twist commands to trigger turtlesim pose updates.

    Args:
        node: rclpy node used to create and reuse the cmd_vel publisher.
        linear_x: Linear x velocity in m/s
        angular_z: Angular z velocity in rad/s

    Returns:
        None: Publishes messages as a side effect.
    """

    matched_event = threading.Event()

    def on_subscriber_matched(info: Any) -> None:
        if info.current_count > 0:
            matched_event.set()

    if hasattr(node, "cmd_vel_publisher"):
        node.destroy_publisher(node.cmd_vel_publisher)

    node.cmd_vel_publisher = node.create_publisher(
        Twist,
        "turtle1/cmd_vel",
        10,
        event_callbacks=PublisherEventCallbacks(matched=on_subscriber_matched),
    )

    if not matched_event.wait(timeout=5.0):
        raise RuntimeError("Timed out waiting for turtlesim cmd_vel subscriber")

    # Publish a Twist message to move the turtle with pi/2 rad/s angular velocity
    msg = Twist()
    msg.linear.x = float(linear_x)
    msg.angular.z = float(angular_z)
    node.cmd_vel_publisher.publish(msg)


class TestTurtleSimWithWaitForTopics(unittest.TestCase):
    def test_publishes_pose_with_wait_for_topics(self) -> None:
        """Verify pose messages are received for turtlesim."""
        with WaitForTopics([("turtle1/pose", Pose)]) as waiter:
            print(waiter.received_messages("turtle1/pose"))
            assert waiter.topics_received() == {"turtle1/pose"}
            assert len(waiter.received_messages("turtle1/pose")) >= 1

    def test_moves_with_triggered_twist(self) -> None:
        """Verify turtle motion after triggering Twist publication."""
        waiter = WaitForTopics([("turtle1/pose", Pose)], trigger=trigger_publish_twist)
        try:
            while True:
                # Wait for the turtle to move by publishing a Twist message with arguments
                # linear_x = 10 m/s and angular_z = 2*pi rad/s
                assert waiter.wait(linear_x=10, angular_z=2 * math.pi)
                assert waiter.topics_received() == {"turtle1/pose"}
                poses = waiter.received_messages("turtle1/pose")
                print(f"Received poses: {poses}")
                assert len(poses) >= 1
                # Check that at least one of the received poses indicates motion compatible with the
                # published Twist command (linear velocity of 10 m/s and angular velocity of 2*pi
                # rad/s)
                if any(
                    math.isclose(p.linear_velocity, 10.0, rel_tol=1e-2)
                    and math.isclose(p.angular_velocity, 2 * math.pi, rel_tol=1e-2)
                    for p in poses
                ):
                    break
        finally:
            waiter.shutdown()

    def test_logs_spawning(self, proc_output: Any) -> None:
        """Verify turtlesim spawn log output is emitted."""
        proc_output.assertWaitFor("Spawning turtle [turtle1] at x=", stream="stderr")


# Post-shutdown tests
@launch_testing.post_shutdown_test()
class TestTurtleSimShutdown(unittest.TestCase):
    def test_exit_codes(self, proc_info: Any) -> None:
        """Verify launched processes exit successfully."""
        launch_testing.asserts.assertExitCodes(proc_info)
