<h1 align=center> 🤖 ROS2 - Wormhole Mapping [Humble]</h1>

![Video of node](Turtlebot3_wormhole.mkv)

# 1. 📚 TOC
- [1. 📚 TOC](#1--toc)
- [2. 📔 Introduction](#2-1-introduction)
  - [2.1. 🧾 Features](#21--features)
- [3. 📦 Installation](#3--installation)
  - [3.1. 🛠️ Prerequisites](#31-️-prerequisites)
  - [3.2. 🛠️ Installation Steps](#32-️-installation-steps)
- [4. ⚙️ Running the Package](#4-️-running-the-package)
- [🔍 Working](#-working)

# 2. 📔 Introduction

This is a ROS2 package for managing multiple maps connected via wormholes / doors.

>[!NOTE] This package has been tested on ROS2 HUMBLE and Turtlebot3 simulation.

## 2.1. 🧾 Features

- **Multi-map management**: Manage multiple maps and their connections, via an SQLite database.
- **Automatic map loading**: Automatically load maps based on the current location of the robot's base-link.

# 3. 📦 Installation

## 3.1. 🛠️ Prerequisites

- This package requires ROS2 Humble and the following dependencies:
  
  - [📎 ROS2 Navigation2 stack](https://docs.nav2.org/getting_started/index.html)
  - [📎 SQLite3](https://www.sqlite.org/index.html)
  - libsqlite3-dev
  - [📎 ROS2 Slam-toolbox](https://github.com/SteveMacenski/slam_toolbox)
  - [📎 ROS2 TF2](https://docs.ros.org/en/humble/Tutorials/Intermediate/Tf2/Introduction-To-Tf2.html)
  - [📎 ROS2 Turtlebot3 package](https://emanual.robotis.com/docs/en/platform/turtlebot3/simulation/)

## 3.2. 🛠️ Installation Steps

- Make sure to source ROS2:

    ```bash
    source /opt/ros/humble/setup.bash
    ```

- Clone the repository into your ROS2 workspace and cd into the directory:

- Build the package using colcon

    ```bash
    colcon build --symlink-install
    ```

- Source the workspace:

    ```bash
    source install/setup.bash
    ```

# 4. ⚙️ Running the Package

- Launch the turtlebot3 simulation in Gazebo or use a real Turtlebot3 robot. For example, I have used the following command to launch the simulation:

    ```bash
    ros2 launch turtlebot3_gazebo turtlebot3_house.launch.py
    ```

- Create separate maps for each room using the `slam_toolbox` package. You can use the following command to start the slam toolbox. Follow the [guide](https://emanual.robotis.com/docs/en/platform/turtlebot3/slam/) to learn how to use cartographer slam.

- Save each room map using the following command:

    ```bash
    ros2 run nav2_map_server map_saver_cli -f path/to/save/map
    ```
- After saving the maps, edit the `wormholes.sql` file's `map_a_url` and `map_b_url` fields to point to the saved maps. The `wormholes.sql` file is located in the `wormhole_mapping` package directory. If you have more than two maps, you can add more entries to the `wormholes.sql` file, each entry representing a unique door/wormhole connection between two maps.

- re-create the database using the following command:

    ```bash
    sqlite3 wormholes.db < wormholes.sql
    ```

- Launch the 'nav2_bringup` package to start the navigation stack:

    ```bash
    ros2 launch nav2_bringup bringup_launch.py use_sim_time:=True autostart:=True map:=/path/to/your-map.yaml
    ```

>[!NOTE]
> Change `use_sim_time` and `autostart` to `False` if you are using a real robot.
> Here the `your-map.yaml` will the starting map where the robot is spawned / present, example in this case `room_1.yaml`.

- Launch `rviz2` to visualize the maps and the robot's position:

    ```bash
    ros2 run rviz2 rviz2 -d $(ros2 pkg prefix nav2_bringup)/share/nav2_bringup/rviz/nav2_default_view.rviz
    ```
- Now, run the `map_manager_node` responsible for changing the maps based on travel using the following command:

    ```bash
    ros2 run map_manager map_manager_node --ros-args -p db_path:=/path/to/wormholes.db -p wormhole_id:=1
    ```

>[!NOTE]
> Replace the db_path with the path to your database file and wormhole_id with the id of the wormhole you want to use. The id is the same as the id in the `wormholes.sql` file.

- Now, you can move the robot to the door/wormhole and it will automatically change the map based on the connection defined in the `wormholes.sql` file.

# 🔍 Working

- The `map_manager_node` subscribes to the `/tf` topic and checks if the robot is near a door/wormhole. If it is within 0.5 meters of the wormhole, it will change the map based on the connection defined in the `wormholes.sql` file.
- The map only changes once while within the 0.5 m distance and the robot has to go away from the door/wormhole and come back to change the map again.

>[!IMPORTANT]
> The Initial map is always selected as the first `map_a_url`
