from setuptools import find_packages, setup

package_name = 'python_wait_for_topics_trigger_experiment'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        (
            'share/ament_index/resource_index/packages',
            ['resource/' + package_name]
        ),
        ('share/' + package_name, ['package.xml']),
        (
            'share/' + package_name + '/launch',
            [
                'launch/launch_experiment.py',
                'launch/launch_turtlesim_experiment.py',
            ],
        ),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='pintaudi',
    maintainer_email='pintaudi@axelspace.com',
    description=(
        'Experiment package showcasing WaitForTopics trigger callback in '
        'launch_testing_ros integration tests.'
    ),
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            f'repeater = {package_name}.repeater:main',
        ],
    },
)
