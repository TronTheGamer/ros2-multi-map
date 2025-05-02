#include <rclcpp/rclcpp.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <nav2_msgs/srv/load_map.hpp>
#include <sqlite3.h>

class MapLoaderNode : public rclcpp::Node {
public:
  MapLoaderNode()
    : Node("map_loader_node"), tf_buffer_(this->get_clock()), tf_listener_(tf_buffer_) {

    this->declare_parameter<int>("wormhole_id", 1);
    this->declare_parameter<std::string>("db_path", "../wormholes.db");

    this->get_parameter("wormhole_id", wormhole_id_);
    this->get_parameter("db_path", db_path_);

    client_ = this->create_client<nav2_msgs::srv::LoadMap>("/map_server/load_map");
    timer_ = this->create_wall_timer(std::chrono::milliseconds(500),
      std::bind(&MapLoaderNode::check_position_and_trigger, this));

    load_wormhole_data();
  }

private:
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Client<nav2_msgs::srv::LoadMap>::SharedPtr client_;
  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  std::string db_path_;
  int wormhole_id_;
  double x_, y_, z_;
  std::string map_a_, map_b_;
  std::string current_map_;

  void load_wormhole_data() {
    sqlite3 *db;
    sqlite3_open(db_path_.c_str(), &db);

    std::string query = "SELECT map_a_url, map_b_url, wormhole_position FROM wormholes WHERE wormhole_id = " + std::to_string(wormhole_id_) + ";";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      RCLCPP_ERROR(this->get_logger(), "Failed to prepare SQL query");
      return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      map_a_ = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      map_b_ = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      std::string pos = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
      sscanf(pos.c_str(), "%lf,%lf,%lf", &x_, &y_, &z_);
      current_map_ = map_a_;  // start with A by default
      RCLCPP_INFO(this->get_logger(), "Loaded wormhole %d: A=%s, B=%s, Pos=[%.2f, %.2f, %.2f]",
                  wormhole_id_, map_a_.c_str(), map_b_.c_str(), x_, y_, z_);
    } else {
      RCLCPP_ERROR(this->get_logger(), "No wormhole with ID %d found", wormhole_id_);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
  }

  void check_position_and_trigger() {
    geometry_msgs::msg::TransformStamped transform;
    try {
      transform = tf_buffer_.lookupTransform("map", "base_link", tf2::TimePointZero);
    } catch (const tf2::TransformException &ex) {
      RCLCPP_WARN(this->get_logger(), "Could not transform: %s", ex.what());
      return;
    }

    double dx = transform.transform.translation.x - x_;
    double dy = transform.transform.translation.y - y_;
    double dz = transform.transform.translation.z - z_;
    double distance = sqrt(dx*dx + dy*dy + dz*dz);

    bool is_changed = false;

    if (distance < 0.5) {  // trigger range

                // std::string next_map = (current_map_ == map_a_) ? map_b_ : map_a_;
                std::string next_map = (dx>0) ? map_b_ : map_a_; 
                auto request = std::make_shared<nav2_msgs::srv::LoadMap::Request>();
                request->map_url = next_map;

                if (!client_->wait_for_service(std::chrono::seconds(1))) {
                  RCLCPP_ERROR(this->get_logger(), "Service not available");
                  return;
                }

            if( is_changed == false){
                client_->async_send_request(request, [this, next_map](rclcpp::Client<nav2_msgs::srv::LoadMap>::SharedFuture future) {
                    auto response = future.get();
                    if (response->result == nav2_msgs::srv::LoadMap::Response::RESULT_SUCCESS) {
                        RCLCPP_INFO(this->get_logger(), "Map loaded successfully: %s", next_map.c_str());
                        // current_map_ = next_map;
                        
                    } else {
                        RCLCPP_ERROR(this->get_logger(), "Failed to load map: %s", next_map.c_str());
                    }

                    // is_changed = true;
                  });
                    
                    is_changed = true;
                }

                current_map_ = next_map;
        }
        else{
            is_changed = false;
        }
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MapLoaderNode>());
  rclcpp::shutdown();
  return 0;
}