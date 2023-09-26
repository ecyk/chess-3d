#pragma once

#include <glad/gl.h>

#include <deque>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"

struct GLFWwindow;

struct Transform {
  glm::vec3 position{};
  float rotation{0.0F};
  float scale{1.0F};
};

struct Vertex {
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 tex_coord{};
};

struct Shader {
  fs::path vert_path;
  fs::path frag_path;
  GLuint id{};
};

struct Texture {
  fs::path path;
  GLuint id{};
};

struct Material {
  std::string name;
  Texture* base_color{};
  Texture* rough{};
  Texture* normal{};
};

struct Mesh {
  GLuint vao{};
  GLuint vbo{};
  GLuint ebo{};
  GLsizei index_count{};
  Material* default_;
  Material* white;
  Material* black;
};

struct Model {
  fs::path path;
  Mesh mesh;
};

class Renderer {
 public:
  explicit Renderer(GLFWwindow* window);

  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  Shader* create_shader(const fs::path& vert_path, const fs::path& frag_path);
  static void destroy_shader(Shader* shader);
  void bind_shader(Shader* shader);

#define SET_SHADER_IMPL(type, uniform, ...)                               \
  static void set_shader_uniform(Shader* shader, std::string_view name,   \
                                 type value) {                            \
    const GLint location = glGetUniformLocation(shader->id, name.data()); \
    uniform(location, __VA_ARGS__);                                       \
  }

  SET_SHADER_IMPL(int, glUniform1i, value);
  SET_SHADER_IMPL(float, glUniform1f, value);
  SET_SHADER_IMPL(const glm::vec3&, glUniform3fv, 1, glm::value_ptr(value));
  SET_SHADER_IMPL(const glm::mat4&, glUniformMatrix4fv, 1, GL_FALSE,
                  glm::value_ptr(value));
#undef SET_SHADER_IMPL

  Texture* create_texture(const fs::path& path);
  static void destroy_texture(Texture* texture);

  Model* create_model(const fs::path& path);
  static void destroy_model(Model* model);
  void draw_model(const Transform& transform, Model* model, Material* material);

  static void begin_drawing();
  void end_drawing();

  [[nodiscard]] GLFWwindow* get_window() const { return window_; }

 private:
  GLFWwindow* window_{};

  Shader* bound_shader_{};
  Material* bound_material_{};

  std::deque<Shader> shaders_;
  std::deque<Texture> textures_;
  std::deque<Material> materials_;
  std::deque<Model> models_;
};
