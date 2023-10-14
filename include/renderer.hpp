#pragma once

#include <glad/gl.h>

#include <deque>
#include <glm/gtc/type_ptr.hpp>

#include "camera.hpp"

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

#define SET_SHADER_UNIFORM_IMPL(type, uniform, ...)                       \
  static void set_shader_uniform(Shader* shader, std::string_view name,   \
                                 type value) {                            \
    const GLint location = glGetUniformLocation(shader->id, name.data()); \
    uniform(location, __VA_ARGS__);                                       \
  }

  SET_SHADER_UNIFORM_IMPL(int, glUniform1i, value);
  SET_SHADER_UNIFORM_IMPL(float, glUniform1f, value);
  SET_SHADER_UNIFORM_IMPL(const glm::vec3&, glUniform3fv, 1,
                          glm::value_ptr(value));
  SET_SHADER_UNIFORM_IMPL(const glm::vec4&, glUniform4fv, 1,
                          glm::value_ptr(value));
  SET_SHADER_UNIFORM_IMPL(const glm::mat4&, glUniformMatrix4fv, 1, GL_FALSE,
                          glm::value_ptr(value));
#undef SET_SHADER_UNIFORM_IMPL

  Texture* create_texture(const fs::path& path);
  static void destroy_texture(Texture* texture);

  Model* create_model(const fs::path& path);
  static void destroy_model(Model* model);
  void draw_model(const Transform& transform, Model* model,
                  Material* material = nullptr);

  void begin_picking_texture_writing() const;
  void end_picking_texture_writing();
  void bind_picking_texture_id(int id);
  [[nodiscard]] int get_picking_texture_id(const glm::ivec2& position) const;

  void begin_outline_drawing(float thickness, const glm::vec4& color);
  void end_outline_drawing();

  static void begin_stencil_writing();
  static void end_stencil_writing();

  void begin_drawing(Camera& camera);
  void end_drawing();

  [[nodiscard]] GLFWwindow* get_window() const { return window_; }

 private:
  GLFWwindow* window_{};

  Camera* camera_{};

  Shader* bound_shader_{};
  Material* bound_material_{};

  std::deque<Shader> shaders_;
  std::deque<Texture> textures_;
  std::deque<Material> materials_;
  std::deque<Model> models_;

  struct PickingTexture {
    GLuint fbo{};
    GLuint picking{};
    GLuint depth{};
  };

  PickingTexture picking_texture_;

  void init_picking_texture();
  void destroy_picking_texture();
};
