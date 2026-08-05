#include <builtin_interfaces/msg/time.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "message_filters/subscriber.h"
#include "message_filters/time_synchronizer.h"

static const rmw_qos_profile_t sub_qos_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_LAST,
    10,
    RMW_QOS_POLICY_RELIABILITY_RELIABLE,
    RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL,
    RMW_QOS_DEADLINE_DEFAULT, // Infinite
    RMW_QOS_LIFESPAN_DEFAULT, // Infinite
    RMW_QOS_POLICY_LIVELINESS_AUTOMATIC,
    RMW_QOS_LIVELINESS_LEASE_DURATION_DEFAULT, // Infinite
    false};

class TestTimeSynchronizer : public rclcpp_lifecycle::LifecycleNode {
private:
  std::unique_ptr<message_filters::TimeSynchronizer<
      builtin_interfaces::msg::Time, geometry_msgs::msg::Point>>
      time_synchronizer_;

  message_filters::Subscriber<builtin_interfaces::msg::Time,
                              rclcpp_lifecycle::LifecycleNode>
      time_subscriber_;

  message_filters::Subscriber<geometry_msgs::msg::Point,
                              rclcpp_lifecycle::LifecycleNode>
      point_subscriber_;

  rclcpp::CallbackGroup::SharedPtr sub_callback_group_;

  void callback(const builtin_interfaces::msg::Time::ConstSharedPtr &time,
                const geometry_msgs::msg::Point::ConstSharedPtr &point) {
    RCLCPP_INFO(this->get_logger(), "Time: %d.%d", time->sec, time->nanosec);
    RCLCPP_INFO(this->get_logger(), "Point: x=%f, y=%f, z=%f", point->x,
                point->y, point->z);
  }

  CallbackReturn configure(const rclcpp_lifecycle::State &) {
    RCLCPP_INFO(get_logger(), "Configuring TestTimeSynchronizer Node");

    RCLCPP_INFO(get_logger(), "Subscribing to topic time_topic");
    {
      rclcpp::SubscriptionOptions options;
      options.callback_group = sub_callback_group_;
      time_subscriber_.subscribe(this, "time_topic", sub_qos_profile, options);
    }
    RCLCPP_INFO(get_logger(), "Subscribed to topic time_topic");

    RCLCPP_INFO(get_logger(), "Subscribing to topic point_topic");
    {
      rclcpp::SubscriptionOptions options;
      options.callback_group = sub_callback_group_;
      point_subscriber_.subscribe(this, "point_topic", sub_qos_profile,
                                  options);
    }
    RCLCPP_INFO(get_logger(), "Subscribed to topic point_topic");

    RCLCPP_INFO(get_logger(), "Connecting TimeSynchronizer");
    time_synchronizer_->connectInput(time_subscriber_, point_subscriber_);
    RCLCPP_INFO(get_logger(), "Connected TimeSynchronizer");

    RCLCPP_INFO(get_logger(), "Registering callback");
    time_synchronizer_->registerCallback(
        std::bind(&TestTimeSynchronizer::callback, this, std::placeholders::_1,
                  std::placeholders::_2));
    RCLCPP_INFO(get_logger(), "Registered callback");

    RCLCPP_INFO(get_logger(), "Configured TestTimeSynchronizer Node");

    return CallbackReturn::SUCCESS;
  }

  CallbackReturn cleanup(const rclcpp_lifecycle::State &) {
    RCLCPP_INFO(get_logger(), "Cleaning up TestTimeSynchronizer Node");

    RCLCPP_INFO(get_logger(), "Resetting TimeSynchronizer");
    time_synchronizer_.reset();
    RCLCPP_INFO(get_logger(), "Reset TimeSynchronizer");

    RCLCPP_INFO(get_logger(), "Recreating TimeSynchronizer");
    time_synchronizer_ = std::make_unique<message_filters::TimeSynchronizer<
        builtin_interfaces::msg::Time, geometry_msgs::msg::Point>>(10);
    RCLCPP_INFO(get_logger(), "Recreated TimeSynchronizer");

    RCLCPP_INFO(get_logger(), "Unsubscribing from topics");
    time_subscriber_.unsubscribe();
    point_subscriber_.unsubscribe();
    RCLCPP_INFO(get_logger(), "Unsubscribed from topics");

    return CallbackReturn::SUCCESS;
  }

public:
  TestTimeSynchronizer()
      : LifecycleNode("test_time_synchronizer"),
        sub_callback_group_(this->create_callback_group(
            rclcpp::CallbackGroupType::MutuallyExclusive)) {
    RCLCPP_INFO(get_logger(), "Creating TestTimeSynchronizer Node");

    time_synchronizer_ = std::make_unique<message_filters::TimeSynchronizer<
        builtin_interfaces::msg::Time, geometry_msgs::msg::Point>>(10);

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
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<TestTimeSynchronizer>();
  executor.add_node(node->get_node_base_interface());
  executor.spin();
  rclcpp::shutdown();
  return 0;
}