import time
import unittest

import launch
import launch_ros.actions
import launch_testing.actions
import launch_testing.markers
from launch_testing_ros import WaitForTopics
from launch_testing_ros.actions import EnableRmwIsolation
import pytest
from rclpy import qos
from std_msgs.msg import String


@pytest.mark.launch_test
@launch_testing.markers.keep_alive
def generate_test_description():
    return launch.LaunchDescription([
        EnableRmwIsolation(),
        launch_ros.actions.Node(
            package='python_wait_for_topics_trigger_experiment',
            executable='repeater',
            name='wait_for_topics_repeater',
            output='screen',
        ),
        launch_testing.actions.ReadyToTest(),
    ])


def trigger_publish_input(node, text):
    if not hasattr(node, 'trigger_publisher'):
        node.trigger_publisher = node.create_publisher(
            String,
            'input',
            qos.QoSProfile(depth=10),
        )

    start = time.time()
    while node.trigger_publisher.get_subscription_count() == 0:
        if time.time() - start > 5.0:
            raise RuntimeError('Timed out waiting for input subscriber')
        time.sleep(0.05)

    node.trigger_publisher.publish(String(data=text))


class TestWaitForTopicsTrigger(unittest.TestCase):

    def test_wait_for_topics_trigger_callback(self):
        topic_list = [('output', String)]
        waiter = WaitForTopics(topic_list, timeout=10.0, trigger=trigger_publish_input)
        try:
            assert waiter.wait('triggered-message')
            assert waiter.topics_received() == {'output'}
            assert waiter.topics_not_received() == set()
            received = [m.data for m in waiter.received_messages('output')]
            assert 'triggered-message' in received
        finally:
            waiter.shutdown()