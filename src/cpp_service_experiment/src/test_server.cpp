#include <chrono>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <std_srvs/srv/empty.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <stdexcept>
#include <thread>

class TestServer : public rclcpp_lifecycle::LifecycleNode {
private:
  // Service that triggers an exception after a delay of 1 second
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr trigger_exception_service_;

public:
  // Constructor
  TestServer() : LifecycleNode("test_server") {
    // Create a service that triggers an exception after a delay of 1 second
    trigger_exception_service_ = create_service<std_srvs::srv::Trigger>(
        "trigger_exception",
        [this](const std::shared_ptr<std_srvs::srv::Trigger::Request>,
               std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
          response->success = true;
          response->message = "Exception will be triggered in 1 second.";
          RCLCPP_INFO(get_logger(), "Service 'trigger_exception' called, "
                                    "throwing exception after 1 second.");
          std::thread([response] {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
            throw std::runtime_error("Intentional exception triggered by "
                                     "'trigger_exception' service.");
          }).detach();
        });
    RCLCPP_INFO(get_logger(), "Service 'trigger_exception' is ready.");
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<TestServer>();
  executor.add_node(node->get_node_base_interface());
  executor.spin();
  rclcpp::shutdown();
  return 0;
}