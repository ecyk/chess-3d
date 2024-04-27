#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <cgltf.h>
#include <stb_image.h>

#include <fstream>
#include <ranges>

Renderer::Renderer(GLFWwindow* window, const Camera& camera)
    : window_{window}, camera_{&camera} {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glEnable(GL_MULTISAMPLE);

  glGenFramebuffers(1, &picking_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fbo_);

  const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  const int width{mode->width};
  const int height{mode->height};

  GLuint color{};
  glGenTextures(1, &color);
  glBindTexture(GL_TEXTURE_2D, color);
  // clang-format off
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

  GLuint depth{};
  glGenTextures(1, &depth);
  glBindTexture(GL_TEXTURE_2D, depth);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
  // clang-format on

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

Renderer::~Renderer() {
  GLint color{};
  GLint depth{};
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fbo_);
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                        &color);
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                        &depth);

  const auto u_color{static_cast<GLuint>(color)};
  const auto u_depth{static_cast<GLuint>(depth)};
  glDeleteTextures(1, &u_color);
  glDeleteTextures(1, &u_depth);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &picking_fbo_);

  for (const auto& name : model_map_ | std::views::keys) {
    unload_model(name);
  }
  for (const auto& path : texture_map_ | std::views::keys) {
    unload_texture(path);
  }
  for (const auto& name : shader_map_ | std::views::keys) {
    unload_shader(name);
  }
}

namespace {
std::string read_file(const std::filesystem::path& path) {
  std::ifstream file{path};
  using istreambuf_iter = std::istreambuf_iterator<char>;
  std::string text{istreambuf_iter{file}, istreambuf_iter{}};
  if (!file) {
    LOGF("GL", "Failed to read \"{}\"", path.string());
    return {};
  }
  return text;
}

GLuint gl_create_shader(const char* code, GLenum type, std::string& buffer) {
  const GLuint shader{glCreateShader(type)};
  glShaderSource(shader, 1, &code, nullptr);
  glCompileShader(shader);

  GLint success{};
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == 0) {
    const auto size{static_cast<GLsizei>(buffer.size())};
    glGetShaderInfoLog(shader, size, nullptr, buffer.data());
    LOG("GL", buffer);
  }

  return shader;
}

struct Shader {
  GLuint vert;
  GLuint frag;
};

GLuint gl_create_program(Shader shader, std::string& buffer) {
  const GLuint program{glCreateProgram()};

  glAttachShader(program, shader.vert);
  glAttachShader(program, shader.frag);
  glLinkProgram(program);

  glDeleteShader(shader.vert);
  glDeleteShader(shader.frag);

  GLint success{};
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (success == 0) {
    const auto size{static_cast<GLsizei>(buffer.size())};
    glGetProgramInfoLog(program, size, nullptr, buffer.data());
    glDeleteProgram(program);
    LOG("GL", buffer);
    return 0;
  }

  return program;
}

struct Texture {
  unsigned char* data{};
  int width{};
  int height{};
  int components{};
};

Texture load_texture(const std::filesystem::path& path) {
  int width{};
  int height{};
  int components{};
  unsigned char* data{
      stbi_load(path.string().c_str(), &width, &height, &components, 0)};
  if (data == nullptr) {
    LOGF("GL", "Failed to load \"{}\"", path.string());
    return {};
  }
  return {data, width, height, components};
}

GLuint gl_create_texture(const Texture& texture) {
  GLuint id{};
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  GLint format{};
  switch (texture.components) {
    case 1:
      format = GL_RED;
      break;
    case 2:
      format = GL_RG;
      break;
    case 3:
      format = GL_RGB;
      break;
    case 4:
      format = GL_RGBA;
      break;
    default:
      assert(false);
      break;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0,
               format, GL_UNSIGNED_BYTE, texture.data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(texture.data);
  return id;
}
}  // namespace

bool Renderer::load_shader(std::string name, const ShaderPath& path) {
  assert(!shader_map_.contains(name));
  std::string buffer;
  buffer.resize(1024);

  const std::string vert_code{read_file(path.vert)};
  const std::string frag_code{read_file(path.frag)};
  // clang-format off
  const GLuint vert_shader{gl_create_shader(vert_code.c_str(), GL_VERTEX_SHADER, buffer)};
  const GLuint frag_shader{gl_create_shader(frag_code.c_str(), GL_FRAGMENT_SHADER, buffer)};
  const GLuint program{gl_create_program({vert_shader, frag_shader}, buffer)};
  if (program == 0) {
    return false;
  }
  shader_map_.insert_or_assign(std::move(name), program);
  LOGF("GL", "Shader loaded (vertex: \"{}\") (fragment: \"{}\") (id: {})", path.vert.string(), path.frag.string(), program);
  // clang-format on

  return true;
}

void Renderer::unload_shader(std::string_view name) {
  const auto it{shader_map_.find(name)};
  assert(it != shader_map_.end());
  const GLuint id{it->second};
  glDeleteProgram(id);
  LOGF("GL", "Shader deleted (id: {})", id);
}

void Renderer::install_shader(std::string_view name) {
  const auto it{shader_map_.find(name)};
  assert(it != shader_map_.end());
  if (const GLuint id = it->second; current_shader_ != id) {
    glUseProgram(id);
    current_shader_ = id;
  }
}

void Renderer::set_shader_uniform(std::string_view name,
                                  UniformValue value) const {
  const GLint location{glGetUniformLocation(current_shader_, name.data())};
  // clang-format off
  std::visit(
      overload{
          [location](float value) { glUniform1fv(location, 1, &value); },
          [location](glm::vec2 value) { glUniform2fv(location, 1, value_ptr(value)); },
          [location](const glm::vec3& value) { glUniform3fv(location, 1, value_ptr(value)); },
          [location](const glm::vec4& value) { glUniform4fv(location, 1, value_ptr(value)); },
          [location](int value) { glUniform1iv(location, 1, &value); },
          [location](const glm::mat3& value) { glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(value)); },
          [location](const glm::mat4& value) { glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value)); }},
      value);
  // clang-format on
}

GLuint Renderer::load_texture(const std::filesystem::path& path) {
  if (const auto it = texture_map_.find(path); it != texture_map_.end()) {
    return it->second;
  }
  const Texture texture{::load_texture(path)};
  const GLuint id{gl_create_texture(texture)};
  texture_map_.insert_or_assign(path, id);
  LOGF("GL", "Texture loaded (file: \"{}\") (id: {})", path.string(), id);
  return id;
}

void Renderer::unload_texture(const std::filesystem::path& path) {
  const auto it{texture_map_.find(path)};
  assert(it != texture_map_.end());
  const GLuint id{it->second};
  glDeleteTextures(1, &id);
  LOGF("GL", "Texture deleted (file: \"{}\") (id: {})", path.string(), id);
}

const Material* Renderer::load_material(const std::filesystem::path& parent,
                                        const cgltf_material* cgltf_material) {
  if (const auto it = material_map_.find(cgltf_material->name);
      it != material_map_.end()) {
    return &it->second;
  }

  auto get_texture = [this, &parent](const char* uri) {
    std::filesystem::path path{parent};
    path += '/';
    path += uri;
    return load_texture(path);
  };

  GLuint albedo{};
  GLuint roughness{};
  GLuint normal{};
  const auto& pbr = cgltf_material->pbr_metallic_roughness;
  if (pbr.base_color_texture.texture != nullptr) {
    albedo = get_texture(pbr.base_color_texture.texture->image->uri);
  }
  if (pbr.metallic_roughness_texture.texture != nullptr) {
    roughness = get_texture(pbr.metallic_roughness_texture.texture->image->uri);
  }
  if (cgltf_material->normal_texture.texture != nullptr) {
    normal = get_texture(cgltf_material->normal_texture.texture->image->uri);
  }
  auto it = material_map_.insert_or_assign(cgltf_material->name,
                                           Material{albedo, roughness, normal});
  return &it.first->second;
}

bool Renderer::load_model(std::string name, const std::filesystem::path& path) {
  const auto it{model_map_.find(name)};
  assert(it == model_map_.end());

  constexpr cgltf_options options{};
  cgltf_data* data{};
  cgltf_result result{cgltf_parse_file(&options, path.string().c_str(), &data)};
  if (result != cgltf_result_success) {
    LOGF("GL", "Failed to load \"{}\"", path.string());
    return false;
  }

  result = cgltf_load_buffers(&options, data, path.string().c_str());
  if (result != cgltf_result_success) {
    LOGF("GL", "Failed to load buffers for \"{}\"", path.string());
    cgltf_free(data);
    return false;
  }

  result = cgltf_validate(data);
  if (result != cgltf_result_success) {
    LOGF("GL", "Invalid gltf file \"{}\"", path.string());
    cgltf_free(data);
    return false;
  }

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  assert(data->scene->nodes_count == 1);
  const cgltf_node* node = data->scene->nodes[0];

  assert(node->children_count == 0);
  assert(node->mesh->primitives_count == 1);
  const cgltf_primitive* primitive = &node->mesh->primitives[0];

  const Material* material0{};
  if (primitive->material != nullptr) {
    material0 = load_material(path.parent_path(), primitive->material);
  }

  assert(primitive->mappings_count == 0 || primitive->mappings_count == 2);

  const Material* material1{};
  if (primitive->mappings_count == 2) {
    // clang-format off
    material0 = load_material(path.parent_path(), primitive->mappings[0].material);
    material1 = load_material(path.parent_path(), primitive->mappings[1].material);
    // clang-format on
  }

  const cgltf_accessor* position{};
  const cgltf_accessor* normal{};
  const cgltf_accessor* tex_coord{};
  for (size_t i = 0; i < primitive->attributes_count; i++) {
    const cgltf_attribute* attribute = &primitive->attributes[i];
    const cgltf_accessor* accessor = attribute->data;

    switch (attribute->type) {
      case cgltf_attribute_type_position:
        position = accessor;
        break;
      case cgltf_attribute_type_normal:
        normal = accessor;
        break;
      case cgltf_attribute_type_texcoord:
        tex_coord = accessor;
        break;
      default:
        assert(false);
        break;
    }
  }

  size_t vertex_count{vertices.size()};
  vertices.resize(vertex_count + position->count);
  for (size_t i = 0; i < position->count; i++, vertex_count++) {
    // clang-format off
    cgltf_accessor_read_float(position, i, value_ptr(vertices[vertex_count].position), 3);
    cgltf_accessor_read_float(normal, i, value_ptr(vertices[vertex_count].normal), 3);
    cgltf_accessor_read_float(tex_coord, i, value_ptr(vertices[vertex_count].tex_coord), 2);
    // clang-format on
  }

  indices.resize(primitive->indices->count);
  for (cgltf_size i = 0; i < primitive->indices->count; i++) {
    indices[i] = static_cast<unsigned int>(
        cgltf_accessor_read_index(primitive->indices, i));
  }

  cgltf_free(data);

  GLuint vao{};
  GLuint vbo{};
  GLuint ebo{};

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
               indices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(offsetof(Vertex, position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(offsetof(Vertex, normal)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(offsetof(Vertex, tex_coord)));

  glBindVertexArray(0);

  model_map_.insert_or_assign(
      std::move(name),
      Model{vao, vbo, ebo, static_cast<GLsizei>(indices.size()), material0,
            material1});
  LOGF("GL", "Model loaded (file: \"{}\")", path.string());
  return true;
}

void Renderer::unload_model(std::string_view name) {
  const auto it{model_map_.find(name)};
  assert(it != model_map_.end());
  const Model& model{it->second};
  glDeleteBuffers(1, &model.vao);
  glDeleteBuffers(1, &model.vbo);
  glDeleteBuffers(1, &model.ebo);
  LOGF("GL", "Model deleted (name: \"{}\") (vao: {})", name, model.vao);
}

void Renderer::draw_model(std::string_view name, const Transform& transform,
                          bool use_alternative_material) {
  const auto it{model_map_.find(name)};
  assert(it != model_map_.end());

  const Model& model{it->second};
  const Material* material = model.material0;
  if (use_alternative_material) {
    material = model.material1;
  }
  if (material->albedo != 0 && current_texture0_ != material->albedo) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material->albedo);
    set_shader_uniform("albedo_tex", 0);
  }
  if (material->roughness != 0 && current_texture1_ != material->roughness) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, material->roughness);
    set_shader_uniform("roughness_tex", 1);
  }
  if (material->normal != 0 && current_texture2_ != material->normal) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, material->normal);
    set_shader_uniform("normal_tex", 2);
  }

  int width{};
  int height{};
  glfwGetWindowSize(window_, &width, &height);

  const auto fwidth{static_cast<float>(width)};
  const auto fheight{static_cast<float>(height)};
  // clang-format off
  set_shader_uniform("projection", glm::perspective(glm::radians(60.0F), fwidth / fheight, 0.1F, 125.0F));
  set_shader_uniform("view", camera_->calculate_view_matrix());
  auto model_mat{scale(
      rotate(
          translate(glm::identity<glm::mat4>(), transform.position),
          glm::radians(transform.rotation), {0.0F, 1.0F, 0.0F}), glm::vec3{transform.scale})};
  set_shader_uniform("model", model_mat);
  set_shader_uniform("normal_mat", glm::mat3{transpose(inverse(glm::mat3(model_mat)))});
  // clang-format on

  glBindVertexArray(model.vao);
  glDrawElements(GL_TRIANGLES, model.index_count, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

void Renderer::draw_model_outline(std::string_view name,
                                  const Transform& transform, float thickness,
                                  const glm::vec4& color) {
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00);
  // glDisable(GL_DEPTH_TEST);
  set_shader_uniform("outline_thickness", thickness);
  set_shader_uniform("color", color);

  draw_model(name, transform);
  // glEnable(GL_DEPTH_TEST);
  glStencilMask(0xFF);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
}

void Renderer::begin_picking(PickingMode picking_mode) {
  switch (picking_mode) {
    case PickingMode::Read:
      glBindFramebuffer(GL_READ_FRAMEBUFFER, picking_fbo_);
      break;
    case PickingMode::Write:
      install_shader("picking");
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, picking_fbo_);

      constexpr std::array<GLint, 4> clear_color{-1, -1, -1, -1};
      glClearBufferiv(GL_COLOR, 0, clear_color.data());
      constexpr GLfloat clear_depth{1.0F};
      glClearBufferfv(GL_DEPTH, 0, &clear_depth);
      break;
  }
}

void Renderer::end_picking(PickingMode picking_mode) {
  switch (picking_mode) {
    case PickingMode::Read:
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      break;
    case PickingMode::Write:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glFlush();
      glFinish();
      break;
  }
}

int Renderer::read_pixel(const glm::ivec2& coord) {
  int data{};
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(coord.x, coord.y, 1, 1, GL_RED_INTEGER, GL_INT, &data);
  glReadBuffer(GL_NONE);
  return data;
}

void Renderer::begin_outlining() {
  glClear(GL_STENCIL_BUFFER_BIT);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0xFF);
}

void Renderer::end_outlining() {
  glStencilMask(0xFF);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
}

void Renderer::begin_drawing(const glm::vec3& light_pos) {
  glClearColor(0.25F, 0.25F, 0.25F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  install_shader("lighting");
  set_shader_uniform("light_pos", light_pos);
  set_shader_uniform("view_pos", camera_->get_position());
}

void Renderer::end_drawing() const { glfwSwapBuffers(window_); }
