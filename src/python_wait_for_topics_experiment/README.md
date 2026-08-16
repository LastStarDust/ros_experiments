# python_wait_for_topics_experiment

This package demonstrates how to use `launch_testing_ros.WaitForTopics` with the
`trigger` callback in a turtlesim integration-test scenario.

## Why this example exists

The ROS 2 integration-testing tutorial uses turtlesim and shows active tests for:
- verifying `turtle1/pose` is published
- checking spawn logs
- optionally publishing twist and asserting movement

This package follows that flow but replaces manual timing/spin loops with
`WaitForTopics` synchronization.

This package is intended as a runnable example for integration-testing
documentation updates.

## Build and test

From the workspace root:

```bash
colcon build --packages-select python_wait_for_topics_experiment
colcon test --packages-select python_wait_for_topics_experiment
colcon test-result --verbose
```

## Key tests

The turtlesim launch test in `test/test_turtlesim_wait_for_topics_launch.py`:
- launches `turtlesim_node` as `turtle1`
- uses `WaitForTopics([('turtle1/pose', Pose)])` for pose publication checks
- uses `WaitForTopics(..., trigger=trigger_publish_twist)` to publish a `Twist`
	only after subscribers/publishers are connected
- verifies that at least one received pose reports non-zero velocity
- checks the spawn log and process exit codes

