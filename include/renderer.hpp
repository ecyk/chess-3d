#pragma once

#include <glad/gl.h>

#include <deque>
#include <glm/gtc/type_ptr.hpp>
#include <variant>

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

struct Material {
  GLuint albedo{};
  GLuint roughness{};
  GLuint normal{};
};

struct Model {
  GLuint vao{};
  GLuint vbo{};
  GLuint ebo{};
  GLsizei index_count{};
  const Material* material0{};
  const Material* material1{};
};

struct cgltf_material;

class Renderer {
 public:
  explicit Renderer(GLFWwindow* window, const Camera& camera);

  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  struct ShaderPath {
    std::filesystem::path vert;
    std::filesystem::path frag;
  };

  bool load_shader(std::string name, const ShaderPath& path);
  void install_shader(std::string_view name);
  // clang-format off
  using UniformValue = std::variant<float, glm::vec2, glm::vec3, glm::vec4, int, glm::mat3, glm::mat4>;
  void set_shader_uniform(std::string_view name, UniformValue value) const;
  // clang-format on

  bool load_model(std::string name, const std::filesystem::path& path);
  void draw_model(std::string_view name, const Transform& transform,
                  bool use_alternative_material = false);

  enum class PickingMode : uint8_t { Read, Write };

  void begin_picking(PickingMode picking_mode);
  static void end_picking(PickingMode picking_mode);
  static int read_pixel(const glm::ivec2& coord);

  static void begin_outlining();
  static void end_outlining();
  void draw_model_outline(std::string_view name, const Transform& transform,
                          float thickness, const glm::vec4& color);

  void begin_drawing(const glm::vec3& light_pos);
  void end_drawing() const;

  [[nodiscard]] GLFWwindow* get_window() const { return window_; }

 private:
  void unload_shader(std::string_view name);

  GLuint load_texture(const std::filesystem::path& path);
  void unload_texture(const std::filesystem::path& path);

  const Material* load_material(const std::filesystem::path& parent,
                                const cgltf_material* cgltf_material);

  void unload_model(std::string_view name);

  GLFWwindow* window_{};

  GLuint current_shader_{};
  StringMap<GLuint> shader_map_;

  GLuint current_texture0_{};
  GLuint current_texture1_{};
  GLuint current_texture2_{};
  std::unordered_map<std::filesystem::path, GLuint> texture_map_;

  StringMap<Material> material_map_;
  StringMap<Model> model_map_;

  GLuint picking_fbo_{};

  const Camera* camera_;
};
