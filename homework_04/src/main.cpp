#include <cstdint>
#include <iostream>
#include <fstream>
#include <cmath>
#include <numeric>

struct RobotPosition {
  float x_;
  float y_;
  float angle_;
};

struct RobotParams {
  int ticks_per_revolution_;
  float wheel_radius_m_;
  float wheelbase_m_;
};

struct Sample {
  int64_t timestamp_ms_;
  uint32_t fl_, fr_, bl_, br_;
};

auto update_position(RobotPosition pos, const RobotParams &params, const Sample &prev, const Sample &curr) -> RobotPosition
{
  int32_t d_fl = static_cast<int32_t>(curr.fl_) - static_cast<int32_t>(prev.fl_);
  int32_t d_fr = static_cast<int32_t>(curr.fr_) - static_cast<int32_t>(prev.fr_);
  int32_t d_bl = static_cast<int32_t>(curr.bl_) - static_cast<int32_t>(prev.bl_);
  int32_t d_br = static_cast<int32_t>(curr.br_) - static_cast<int32_t>(prev.br_);

  float d_left = std::midpoint(static_cast<float>(d_fl), static_cast<float>(d_bl));
  float d_right = std::midpoint(static_cast<float>(d_fr), static_cast<float>(d_br));

  float distance_per_tick = 2.0F * static_cast<float>(M_PI) * params.wheel_radius_m_ / static_cast<float>(params.ticks_per_revolution_);

  float dL = d_left * distance_per_tick;
  float dR = d_right * distance_per_tick;

  float d = std::midpoint(dL, dR);
  float dtheta = (dR - dL) / params.wheelbase_m_;

  pos.x_ += d * std::cos(pos.angle_ + dtheta / 2.0F);
  pos.y_ += d * std::sin(pos.angle_ + dtheta / 2.0F);
  pos.angle_ += dtheta;

  return pos;
}
auto main(int argc, char **argv) -> int
{
  // The program expects exactly one argument: a path to telemetry samples.
  if (argc != 2) {
    std::cerr << "usage: ugv_odometry <input_path>\n";
    return 1;
  }

  RobotPosition robot_position{0.0F, 0.0F, 0.0F};
  const RobotParams kRobotParams{1024, 0.3F, 1.0F};

  std::ifstream file(argv[1]);
  if (!file) {
    std::cerr << "error: cannot open file: " << argv[1] << "\n";
    return 1;
  }

  Sample prev{};
  Sample curr{};
  file >> prev.timestamp_ms_ >> prev.fl_ >> prev.fr_ >> prev.bl_ >> prev.br_;

  while (file >> curr.timestamp_ms_ >> curr.fl_ >> curr.fr_ >> curr.bl_ >> curr.br_) {
    robot_position = update_position(robot_position, kRobotParams, prev, curr);

    std::cout << curr.timestamp_ms_ << " " << robot_position.x_ << " " << robot_position.y_ << " " << robot_position.angle_ << "\n";

    prev = curr;
  }

  return 0;
}
