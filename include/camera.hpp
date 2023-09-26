#pragma once

#include "common.hpp"

class Camera {
  static constexpr float k_min_distance{4.0F};
  static constexpr float k_max_distance{10.0F};
  static constexpr float k_sensitivity{0.1F};

 public:
  Camera(const glm::vec3& position, const glm::vec3& target);

  [[nodiscard]] glm::mat4 calculate_view_matrix() const;

  void process_mouse_movement(float offset_x, float offset_y);
  void process_mouse_scroll(float offset_y);

 private:
  [[nodiscard]] glm::vec3 calculate_forward() const {
    return glm::normalize(target_ - position_);
  }
  [[nodiscard]] glm::vec3 calculate_up() const { return glm::normalize(up_); }
  [[nodiscard]] glm::vec3 calculate_right() const {
    return glm::cross(calculate_forward(), calculate_up());
  }

  glm::vec3 position_;
  glm::vec3 target_;
  glm::vec3 up_{0.0F, 1.0F, 0.0F};
};
