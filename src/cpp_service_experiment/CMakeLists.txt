# ##################################################################################################
# CMake configuration for cpp_service_experiment
# ##################################################################################################

cmake_minimum_required(VERSION 3.24)
project(cpp_service_experiment VERSION 0.0.0 LANGUAGES C CXX)

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
find_package(lifecycle_msgs REQUIRED)
find_package(std_srvs REQUIRED)

set(AMENT_DEPENDENCIES
    "rclcpp"
    "rclcpp_lifecycle"
    "lifecycle_msgs"
    "std_srvs")

# ##################################################################################################
# Add compiler options
# ##################################################################################################

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options("$<$<CONFIG:Debug>:-g;-Wall;-Wextra;-Wpedantic;-Wshadow;-Werror;-pedantic>")
endif()

# ##################################################################################################
# Build targets
# ##################################################################################################

add_executable(test_server "src/test_server.cpp")
ament_target_dependencies(test_server ${AMENT_DEPENDENCIES})

add_executable(test_client "src/test_client.cpp")
ament_target_dependencies(test_client ${AMENT_DEPENDENCIES})

# ##################################################################################################
# Install targets
# ##################################################################################################

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

install(TARGETS
  test_server
  test_client
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
