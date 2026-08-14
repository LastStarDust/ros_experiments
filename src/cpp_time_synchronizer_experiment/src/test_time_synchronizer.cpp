#include <builtin_interfaces/msg/time.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include <cmath>
#include <cstdint>

#include "message_filters/message_traits.hpp"
#include "message_filters/subscriber.hpp"
#include "message_filters/time_synchronizer.hpp"

// Node purpose:
// - Demonstrate exact-time synchronization of two ROS 2 topics using
//   message_filters::TimeSynchronizer.
// - Subscribe to "time_topic" (builtin_interfaces/Time) and "point_topic"
//   (geometry_msgs/Point), then process pairs that share the same timestamp.
//
// Logic summary:
// 1) Constructor creates the synchronizer and registers lifecycle hooks.
// 2) configure() subscribes to both topics, connects them to the synchronizer,
//    and registers the paired-message callback.
// 3) callback() runs only when both inputs match exactly in time and logs both
//    messages.
// 4) cleanup() tears down subscribers and resets synchronizer state so future
//    configure transitions start cleanly.
// 5) main() runs the lifecycle node in a MultiThreadedExecutor.
//
// PR #319 demonstration:
// - The synchronizer below uses TimeSynchronizerBase<CustomTimeGetter, ...>.
// - This custom getter extracts timestamps from headerless message content.
// - For this demo, geometry_msgs/Point encodes timestamp as:
//   point.x -> sec, point.y -> nanosec.

template<typename M>
struct CustomTimeGetter : message_filters::message_traits::DefaultTimeGetter<M>
{};

template<>
struct CustomTimeGetter<builtin_interfaces::msg::Time>
{
  static rclcpp::Time getTime(const builtin_interfaces::msg::Time & msg)
  {
    return rclcpp::Time(msg.sec, msg.nanosec, RCL_ROS_TIME);
  }
};

template<>
struct CustomTimeGetter<geometry_msgs::msg::Point>
{
  static rclcpp::Time getTime(const geometry_msgs::msg::Point & msg)
  {
    // Demo convention: point.x carries seconds, point.y carries nanoseconds.
    // The publisher must provide integer-like values in these fields.
    const auto sec = static_cast<int32_t>(std::llround(msg.x));
    const auto nanosec = static_cast<uint32_t>(std::llround(msg.y));
    return rclcpp::Time(sec, nanosec, RCL_ROS_TIME);
  }
};

class TestTimeSynchronizer : public rclcpp_lifecycle::LifecycleNode
{
private:
  // Synchronizes two input streams by exact timestamp equality.
  // Queue size (10) controls how many unmatched samples are buffered.
  // Uses a custom getter to read timestamps from headerless message payloads.
  std::unique_ptr<
      message_filters::TimeSynchronizerBase<CustomTimeGetter, builtin_interfaces::msg::Time, geometry_msgs::msg::Point>>
      time_synchronizer_;

  // First input stream: messages from "time_topic".
  message_filters::Subscriber<builtin_interfaces::msg::Time> time_subscriber_;

  // Second input stream: messages from "point_topic".
  message_filters::Subscriber<geometry_msgs::msg::Point> point_subscriber_;

  // Callback group for subscription callbacks, separated so executor scheduling
  // can be controlled independently from other potential node callbacks.
  rclcpp::CallbackGroup::SharedPtr sub_callback_group_;

  // Invoked only when both topics provide messages with matching timestamps.
  // If timestamps do not line up exactly, this callback will not run.
  void callback(const builtin_interfaces::msg::Time::ConstSharedPtr &time,
                const geometry_msgs::msg::Point::ConstSharedPtr &point) {
    const auto point_stamp = CustomTimeGetter<geometry_msgs::msg::Point>::getTime(*point);

    RCLCPP_INFO(this->get_logger(), "Time: %d.%d", time->sec, time->nanosec);
    RCLCPP_INFO(this->get_logger(), "Point: x=%f, y=%f, z=%f", point->x,
                point->y, point->z);
    RCLCPP_INFO(this->get_logger(),
          "Custom point stamp interpreted as: %d.%u",
          point_stamp.seconds(),
          point_stamp.nanoseconds() % 1000000000ULL);
  }

  // Lifecycle transition: unconfigured -> inactive.
  // This is where subscriptions and synchronizer wiring are established.
  CallbackReturn configure(const rclcpp_lifecycle::State &) {
    RCLCPP_INFO(get_logger(), "Configuring TestTimeSynchronizer Node");

    RCLCPP_INFO(get_logger(), "Subscribing to topic time_topic");
    {
      // Subscription options let us assign this subscriber to our callback group.
      rclcpp::SubscriptionOptions options;
      options.callback_group = sub_callback_group_;
      time_subscriber_.subscribe(this, "time_topic", rclcpp::QoS(10), options);
    }
    RCLCPP_INFO(get_logger(), "Subscribed to topic time_topic");

    RCLCPP_INFO(get_logger(), "Subscribing to topic point_topic");
    {
      rclcpp::SubscriptionOptions options;
      options.callback_group = sub_callback_group_;
      point_subscriber_.subscribe(this, "point_topic", rclcpp::QoS(10),
                                  options);
    }
    RCLCPP_INFO(get_logger(), "Subscribed to topic point_topic");

    RCLCPP_INFO(get_logger(), "Connecting TimeSynchronizer");
    // Attach subscribers to the synchronizer so it can begin pairing messages.
    time_synchronizer_->connectInput(time_subscriber_, point_subscriber_);
    RCLCPP_INFO(get_logger(), "Connected TimeSynchronizer");

    RCLCPP_INFO(get_logger(), "Registering callback");
    // Register the synchronized callback that receives both matched messages.
    time_synchronizer_->registerCallback(
        std::bind(&TestTimeSynchronizer::callback, this, std::placeholders::_1,
                  std::placeholders::_2));
    RCLCPP_INFO(get_logger(), "Registered callback");

    RCLCPP_INFO(get_logger(), "Configured TestTimeSynchronizer Node");

    return CallbackReturn::SUCCESS;
  }

  // Lifecycle transition: inactive -> unconfigured.
  // Release runtime resources and restore a clean pre-configured state.
  CallbackReturn cleanup(const rclcpp_lifecycle::State &) {
    RCLCPP_INFO(get_logger(), "Cleaning up TestTimeSynchronizer Node");

    RCLCPP_INFO(get_logger(), "Resetting TimeSynchronizer");
    // Drop existing synchronizer state, including internal message queues.
    time_synchronizer_.reset();
    RCLCPP_INFO(get_logger(), "Reset TimeSynchronizer");

    RCLCPP_INFO(get_logger(), "Recreating TimeSynchronizer");
    // Recreate the synchronizer so a later configure starts from a clean object.
    time_synchronizer_ =
      std::make_unique<message_filters::TimeSynchronizerBase<
        CustomTimeGetter,
        builtin_interfaces::msg::Time, geometry_msgs::msg::Point>>(10);
    RCLCPP_INFO(get_logger(), "Recreated TimeSynchronizer");

    RCLCPP_INFO(get_logger(), "Unsubscribing from topics");
    // Explicitly unsubscribe to stop incoming traffic while unconfigured.
    time_subscriber_.unsubscribe();
    point_subscriber_.unsubscribe();
    RCLCPP_INFO(get_logger(), "Unsubscribed from topics");

    return CallbackReturn::SUCCESS;
  }

public:
  TestTimeSynchronizer()
      : LifecycleNode("test_time_synchronizer"),
        // MutuallyExclusive guarantees one callback from this group at a time.
        sub_callback_group_(this->create_callback_group(
            rclcpp::CallbackGroupType::MutuallyExclusive)) {
    RCLCPP_INFO(get_logger(), "Creating TestTimeSynchronizer Node");

    // Pre-create the synchronizer so lifecycle callbacks can reuse it.
    time_synchronizer_ = std::make_unique<message_filters::TimeSynchronizerBase<
      CustomTimeGetter,
        builtin_interfaces::msg::Time, geometry_msgs::msg::Point>>(10);

    // Register lifecycle handlers so transitions call our configure/cleanup code.
    auto configure_cb = [this](const rclcpp_lifecycle::State &state) {
      return this->configure(state);
    };
    auto cleanup_cb = [this](const rclcpp_lifecycle::State &state) {
      return this->cleanup(state);
    };

    this->register_on_configure(configure_cb);
    this->register_on_cleanup(cleanup_cb);

    RCLCPP_INFO(get_logger(), "Created TestTimeSynchronizer Node");
  }
};

int main(int argc, char **argv) {
  // Standard ROS 2 process lifecycle:
  // 1) init client library
  // 2) create executor and node
  // 3) spin to process callbacks until shutdown
  // 4) shutdown and exit
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<TestTimeSynchronizer>();
  executor.add_node(node->get_node_base_interface());
  executor.spin();
  rclcpp::shutdown();
  return 0;
}