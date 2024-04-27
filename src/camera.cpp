#include "camera.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

Camera::Camera(const glm::vec3& position, const glm::vec3& target)
    : position_{position}, target_{target} {}

glm::mat4 Camera::calculate_view_matrix() const {
  return lookAt(position_, target_, calculate_up());
}

void Camera::process_mouse_movement(float offset_x, float offset_y) {
  offset_x *= k_sensitivity;
  offset_y *= k_sensitivity;

  constexpr glm::mat4 identity{1.0F};
  glm::vec3 target_position{target_ - position_};
  glm::vec3 up{calculate_up()};
  glm::mat4 rotation{rotate(identity, glm::radians(-offset_x), up)};
  target_position = rotation * glm::vec4{target_position, 1.0F};
  position_ = target_ - target_position;

  target_position = target_ - position_;
  up = calculate_up();

  float max_angle_up{angle(up, normalize(target_position))};
  max_angle_up -= 0.001F;

  float max_angle_down{-angle(-up, normalize(target_position))};
  max_angle_down += 0.001F;

  offset_y = glm::clamp(offset_y, glm::degrees(max_angle_down),
                        glm::degrees(max_angle_up));

  const glm::vec3 right{calculate_right()};
  rotation = rotate(identity, glm::radians(offset_y), right);
  target_position = rotation * glm::vec4{target_position, 1.0F};
  position_ = target_ - target_position;
}

void Camera::process_mouse_scroll(float offset_y) {
  float distance{length(position_ - target_)};
  distance -= offset_y;
  distance = glm::clamp(distance, k_min_distance, k_max_distance);
  const glm::vec3 forward{calculate_forward()};
  position_ = target_ + forward * -distance;
}
