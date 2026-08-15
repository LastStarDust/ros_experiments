#include <rclcpp/rclcpp.hpp>

#include "cpp_time_synchronizer_experiment/msg/sensor_a.hpp"
#include "cpp_time_synchronizer_experiment/msg/sensor_b.hpp"

#include "message_filters/message_traits.hpp"
#include "message_filters/subscriber.hpp"
#include "message_filters/time_synchronizer.hpp"

// Node purpose:
// - Demonstrate exact-time synchronization of two ROS 2 topics using
//   message_filters::TimeSynchronizer.
// - Subscribe to "sensor_a_topic" and "sensor_b_topic", then process pairs
//   that share the same timestamp.
//
// Logic summary:
// 1) Constructor creates subscribers and connects them to the synchronizer.
// 2) callback() runs only when both inputs match exactly in time and logs both
//    messages.
// 3) main() runs this plain ROS 2 node in a MultiThreadedExecutor.
//
// PR #319 demonstration:
// - The synchronizer below uses TimeSynchronizerBase<CustomTimeGetter, ...>.
// - This custom getter extracts timestamps from headerless message content.

using SensorA = cpp_time_synchronizer_experiment::msg::SensorA;
using SensorB = cpp_time_synchronizer_experiment::msg::SensorB;

template <typename M>
struct CustomTimeGetter
{
  static rclcpp::Time getTime(const M& msg)
  {
    return rclcpp::Time(msg.time, 0, RCL_ROS_TIME);
  }
};

class TestTimeSynchronizer : public rclcpp::Node
{
private:
  // Synchronizes two input streams by exact timestamp equality.
  // Queue size (10) controls how many unmatched samples are buffered.
  // Uses a custom getter to read timestamps from headerless message payloads.
  std::unique_ptr<message_filters::TimeSynchronizerBase<CustomTimeGetter, SensorA, SensorB>> time_synchronizer_;

  // First input stream: messages from "sensor_a_topic".
  message_filters::Subscriber<SensorA> sensor_a_subscriber_;

  // Second input stream: messages from "sensor_b_topic".
  message_filters::Subscriber<SensorB> sensor_b_subscriber_;

  // Callback group for subscription callbacks, separated so executor scheduling
  // can be controlled independently from other potential node callbacks.
  rclcpp::CallbackGroup::SharedPtr sub_callback_group_;

  // Invoked only when both topics provide messages with matching timestamps.
  // If timestamps do not line up exactly, this callback will not run.
  void callback(const SensorA::ConstSharedPtr& sensor_a, const SensorB::ConstSharedPtr& sensor_b)
  {
    RCLCPP_INFO(this->get_logger(), "SensorA: time=%d value=%f", sensor_a->time, sensor_a->value);
    RCLCPP_INFO(this->get_logger(), "SensorB: time=%d value=%f", sensor_b->time, sensor_b->value);
  }

public:
  TestTimeSynchronizer()
    : Node("test_time_synchronizer")
    ,
    // MutuallyExclusive guarantees one callback from this group at a time.
    sub_callback_group_(this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive))
  {
    RCLCPP_INFO(get_logger(), "Creating TestTimeSynchronizer Node");

    // Pre-create and wire the synchronizer during node construction.
    time_synchronizer_ =
        std::make_unique<message_filters::TimeSynchronizerBase<CustomTimeGetter, SensorA, SensorB>>(10);

    RCLCPP_INFO(get_logger(), "Subscribing to topic sensor_a_topic");
    {
      rclcpp::SubscriptionOptions options;
      options.callback_group = sub_callback_group_;
      sensor_a_subscriber_.subscribe(this, "sensor_a_topic", rclcpp::QoS(10), options);
    }
    RCLCPP_INFO(get_logger(), "Subscribed to topic sensor_a_topic");

    RCLCPP_INFO(get_logger(), "Subscribing to topic sensor_b_topic");
    {
      rclcpp::SubscriptionOptions options;
      options.callback_group = sub_callback_group_;
      sensor_b_subscriber_.subscribe(this, "sensor_b_topic", rclcpp::QoS(10), options);
    }
    RCLCPP_INFO(get_logger(), "Subscribed to topic sensor_b_topic");

    RCLCPP_INFO(get_logger(), "Connecting TimeSynchronizer");
    time_synchronizer_->connectInput(sensor_a_subscriber_, sensor_b_subscriber_);
    RCLCPP_INFO(get_logger(), "Connected TimeSynchronizer");

    RCLCPP_INFO(get_logger(), "Registering callback");
    time_synchronizer_->registerCallback(
      std::bind(&TestTimeSynchronizer::callback, this, std::placeholders::_1, std::placeholders::_2));
    RCLCPP_INFO(get_logger(), "Registered callback");

    RCLCPP_INFO(get_logger(), "Created TestTimeSynchronizer Node");
  }
};

int main(int argc, char** argv)
{
  // Standard ROS 2 process lifecycle:
  // 1) init client library
  // 2) create executor and node
  // 3) spin to process callbacks until shutdown
  // 4) shutdown and exit
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<TestTimeSynchronizer>();
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}