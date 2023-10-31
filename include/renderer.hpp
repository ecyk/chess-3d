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

enum class Framebuffer : GLuint;

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
  void draw_model(const Transform& transform, Model* model, Material* material);
  void draw_model_outline(const Transform& transform, Model* model,
                          float thickness, const glm::vec4& color);

  Framebuffer* create_framebuffer(const glm::ivec2& size);
  static void destroy_framebuffer(Framebuffer* framebuffer);
  void bind_framebuffer(Framebuffer* framebuffer, GLenum target);
  void unbind_framebuffer(GLenum target);
  static void clear_framebuffer();

  static int read_pixel(const glm::ivec2& coord);

  void begin_drawing(Camera& camera);
  void end_drawing();

  static void begin_stencil_writing();
  static void end_stencil_writing();
  static void clear_stencil();

  static void begin_wire_mode();
  static void end_wire_mode();

  [[nodiscard]] GLFWwindow* get_window() const { return window_; }

 private:
  GLFWwindow* window_{};

  Camera* camera_{};

  Shader* bound_shader_{};
  Material* bound_material_{};
  Framebuffer* bound_framebuffer_{};

  std::deque<Shader> shaders_;
  std::deque<Texture> textures_;
  std::deque<Material> materials_;
  std::deque<Model> models_;
  std::deque<Framebuffer> framebuffers_;
};
