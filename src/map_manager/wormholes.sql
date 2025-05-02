CREATE TABLE wormholes(
    wormhole_id INTEGER PRIMARY KEY,
    map_a_url TEXT NOT NULL,
    map_b_url TEXT NOT NULL,
    wormhole_position TEXT NOT NULL
);

INSERT INTO wormholes (wormhole_id, map_a_url, map_b_url, wormhole_position)
VALUES (
    1,
    "/home/ros2_ws/anscer_robotics/wormhole_manager/src/maps/room1.yaml",
    "/home/ros2_ws/anscer_robotics/wormhole_manager/src/maps/room2.yaml",
    "4.4,0.7,0"
);