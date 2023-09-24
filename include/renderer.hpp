#pragma once

#include <glad/gl.h>

#include <deque>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"

struct GLFWwindow;

struct Shader {
  fs::path vert_path;
  fs::path frag_path;
  GLuint id{};
};

struct Texture {
  fs::path path;
  GLuint id{};
};

class Renderer {
 public:
  explicit Renderer(GLFWwindow* window) : window_{window} {}

  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  Shader* create_shader(const fs::path& vert_path, const fs::path& frag_path);
  static void destroy_shader(Shader* shader);

#define SHADER_SET_IMPL(type, uniform, ...)                               \
  static void set_shader_uniform(Shader* shader, std::string_view name,   \
                                 type value) {                            \
    const GLint location = glGetUniformLocation(shader->id, name.data()); \
    uniform(location, __VA_ARGS__);                                       \
  }

  SHADER_SET_IMPL(int, glUniform1i, value);
  SHADER_SET_IMPL(float, glUniform1f, value);
  SHADER_SET_IMPL(const glm::vec3&, glUniform3fv, 1, glm::value_ptr(value));
  SHADER_SET_IMPL(const glm::mat4&, glUniformMatrix4fv, 1, GL_FALSE,
                  glm::value_ptr(value));

#undef SHADER_SET_IMPL

  Texture* create_texture(const fs::path& path);
  static void destroy_texture(Texture* texture);

  static void begin_drawing();
  void end_drawing();

  [[nodiscard]] GLFWwindow* get_window() const { return window_; }

 private:
  GLFWwindow* window_{};

  std::deque<Shader> shaders_;
  std::deque<Texture> textures_;
};
