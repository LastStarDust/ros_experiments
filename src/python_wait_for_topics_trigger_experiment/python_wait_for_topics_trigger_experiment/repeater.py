from rclpy.executors import ExternalShutdownException
import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class Repeater(Node):
    """Republish incoming strings from input to output."""

    def __init__(self):
        super().__init__('wait_for_topics_repeater')
        self.subscription = self.create_subscription(
            String,
            'input',
            self._on_input,
            10,
        )
        self.publisher = self.create_publisher(String, 'output', 10)

    def _on_input(self, input_msg: String):
        self.publisher.publish(String(data=input_msg.data))


def main(args=None):
    try:
        with rclpy.init(args=args):
            node = Repeater()
            rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass


if __name__ == '__main__':
    main()
