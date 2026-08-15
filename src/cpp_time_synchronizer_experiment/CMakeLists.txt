# ##################################################################################################
# CMake configuration for cpp_time_synchronizer_experiment
# ##################################################################################################

cmake_minimum_required(VERSION 3.24)
project(cpp_time_synchronizer_experiment VERSION 0.0.0 LANGUAGES C CXX)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++23
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()


# ##################################################################################################
# Dependencies
# ##################################################################################################

find_package(rclcpp REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(message_filters REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rosidl_typesupport_cpp REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/SensorA.msg"
  "msg/SensorB.msg"
)

# ##################################################################################################
# Add compiler options
# ##################################################################################################

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options("$<$<CONFIG:Debug>:-g;-Wall;-Wextra;-Wpedantic;-Wshadow;-Werror;-pedantic>")
endif()

# ##################################################################################################
# Build targets
# ##################################################################################################

add_executable(test_time_synchronizer "src/test_time_synchronizer.cpp")
target_link_libraries(test_time_synchronizer
  rclcpp::rclcpp
  rclcpp_lifecycle::rclcpp_lifecycle
  message_filters::message_filters
)

rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(test_time_synchronizer "${cpp_typesupport_target}")

# ##################################################################################################
# Install targets
# ##################################################################################################

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

install(TARGETS
  test_time_synchronizer
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
