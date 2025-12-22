#include <chrono>
#include <lifecycle_msgs/srv/get_state.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/empty.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <stdexcept>
#include <thread>

class TestClient : public rclcpp::Node {
private:
  // Callback group for the clients
  rclcpp::CallbackGroup::SharedPtr client_callback_group_;

  // Client for the trigger_exception service (that triggers an exception in the
  // test_server node after a delay of 1 second)
  rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr trigger_exception_client_;

  // CLient for the GetState service of lifecycle nodes
  rclcpp::Client<lifecycle_msgs::srv::GetState>::SharedPtr get_state_client_;

  // ---------------------------

  // Callback group for the timers
  rclcpp::CallbackGroup::SharedPtr timer_callback_group_;

  // Timer to call the trigger_exception service after a 10-second delay
  rclcpp::TimerBase::SharedPtr exception_timer_;

  // Timer to call the GetState service periodically with a 3-second interval
  rclcpp::TimerBase::SharedPtr get_state_timer_;

public:
  TestClient() : Node("test_client") {
    using namespace std::chrono_literals;

    // Create callback groups
    client_callback_group_ =
        create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    timer_callback_group_ =
        create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

    // Create a client for the GetState service of lifecycle nodes
    get_state_client_ = create_client<lifecycle_msgs::srv::GetState>(
        "test_server/get_state",
        rclcpp::QoS({RMW_QOS_POLICY_HISTORY_KEEP_LAST, 10},
                    rmw_qos_profile_services_default),
        client_callback_group_);
    // Wait for the GetState service to be available
    while (!get_state_client_->wait_for_service(1s)) {
      if (!rclcpp::ok()) {
        throw std::runtime_error(
            "Interrupted while waiting for the service. Exiting.");
      } else {
        RCLCPP_INFO(get_logger(),
                    "Waiting for 'test_server/get_state' to be available...");
      }
    }
    RCLCPP_INFO(get_logger(), "'test_server/get_state' is now available.");

    // Create a timer to call the GetState service every 3 seconds
    auto get_state_timer_callback = [this]() {
      auto get_state_request =
          std::make_shared<lifecycle_msgs::srv::GetState::Request>();
      auto future = get_state_client_->async_send_request(get_state_request);
      if (not future.valid()) {
        RCLCPP_ERROR(get_logger(),
                     "Failed to send request to 'test_server/get_state'.");
      } else {
        auto future_status = future.wait_for(1s);
        if (future_status == std::future_status::ready) {
          auto response = future.get();
          RCLCPP_INFO(get_logger(), "'test_server' current state: %u (%s)",
                      response->current_state.id,
                      response->current_state.label.c_str());
        } else {
          RCLCPP_ERROR(get_logger(),
                       "'test_server/get_state' request timed out.");
        }
      }
    };
    get_state_timer_ =
        create_wall_timer(3s, get_state_timer_callback, timer_callback_group_);

    // Create a client for the trigger_exception service
    trigger_exception_client_ = create_client<std_srvs::srv::Trigger>(
        "trigger_exception",
        rclcpp::QoS({RMW_QOS_POLICY_HISTORY_KEEP_LAST, 10},
                    rmw_qos_profile_services_default),
        client_callback_group_);

    // Wait for the trigger_exception service to be available
    while (!trigger_exception_client_->wait_for_service(1s)) {
      if (!rclcpp::ok()) {
        throw std::runtime_error(
            "Interrupted while waiting for the service. Exiting.");
      } else {
        RCLCPP_INFO(get_logger(),
                    "Waiting for 'trigger_exception' to be available...");
      }
    }
    RCLCPP_INFO(get_logger(), "'trigger_exception' is now available.");

    // Create a timer to call the trigger_exception service after 10 seconds
    auto exception_timer_callback = [this]() {
      auto trigger_request =
          std::make_shared<std_srvs::srv::Trigger::Request>();
      auto future =
          trigger_exception_client_->async_send_request(trigger_request);
      if (not future.valid()) {
        RCLCPP_ERROR(get_logger(),
                     "Failed to send request to 'trigger_exception'.");
      } else {
        auto future_status = future.wait_for(1s);
        if (future_status == std::future_status::ready) {
          auto response = future.get();
          RCLCPP_INFO(get_logger(),
                      "'trigger_exception' response: success=%d, message=%s",
                      response->success, response->message.c_str());
        } else {
          RCLCPP_ERROR(get_logger(), "'trigger_exception' request timed out.");
        }
      }
      if (exception_timer_)
        exception_timer_->cancel();
    };
    exception_timer_ =
        create_wall_timer(10s, exception_timer_callback, timer_callback_group_);
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<TestClient>();
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}