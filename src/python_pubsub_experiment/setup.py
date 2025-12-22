from setuptools import find_packages, setup

package_name = 'python_pubsub_experiment'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='pintaudi',
    maintainer_email='pintaudi@axelspace.com',
    description='Experimenting with Python Pub/Sub in ROS2',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            f'test_publisher = {package_name}.test_publisher:main',
            f'test_subscriber = {package_name}.test_subscriber:main'
        ],
    },
)
