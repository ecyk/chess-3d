#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <cgltf.h>
#include <stb_image.h>

#include <fstream>

Renderer::Renderer(GLFWwindow* window) : window_{window} {}

Renderer::~Renderer() {
  for (size_t i = framebuffers_.size(); i-- > 0;) {
    destroy_framebuffer(&framebuffers_[i]);
  }
  framebuffers_.clear();

  for (size_t i = models_.size(); i-- > 0;) {
    destroy_model(&models_[i]);
  }
  models_.clear();

  materials_.clear();

  for (size_t i = textures_.size(); i-- > 0;) {
    destroy_texture(&textures_[i]);
  }
  textures_.clear();

  for (size_t i = shaders_.size(); i-- > 0;) {
    destroy_shader(&shaders_[i]);
  }
  shaders_.clear();
}

Shader* Renderer::create_shader(const fs::path& vert_path,
                                const fs::path& frag_path) {
  if (auto it = std::find_if(shaders_.begin(), shaders_.end(),
                             [&](const Shader& shader) {
                               return shader.vert_path == vert_path &&
                                      shader.frag_path == frag_path;
                             });
      it != shaders_.end()) {
    return &(*it);
  }

  auto read_file = [](const fs::path& path) -> std::string {
    std::ifstream file{path};
    if (!file) {
      LOGF("GL", "Failed to open \"{}\"", path.string());
      return {};
    }

    std::string text{std::istreambuf_iterator<char>{file},
                     std::istreambuf_iterator<char>{}};

    if (file) {
      return text;
    }

    LOGF("GL", "Failed to read \"{}\"", path.string());
    return {};
  };

  const std::string vert_code{read_file(vert_path)};
  if (vert_code.empty()) {
    return nullptr;
  }

  const std::string frag_code{read_file(frag_path)};
  if (frag_code.empty()) {
    return nullptr;
  }

  auto gl_create_shader = [](const char* shader_code, GLenum type,
                             std::string& buffer) -> GLuint {
    const GLuint shader{glCreateShader(type)};
    glShaderSource(shader, 1, &shader_code, nullptr);
    glCompileShader(shader);

    GLint success{};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == 0) {
      glGetShaderInfoLog(shader, static_cast<GLsizei>(buffer.size()), nullptr,
                         buffer.data());
      LOG("GL", buffer);
    }

    return shader;
  };

  std::string buffer;
  buffer.resize(1024);

  const GLuint vert_shader{
      gl_create_shader(vert_code.c_str(), GL_VERTEX_SHADER, buffer)};
  const GLuint frag_shader{
      gl_create_shader(frag_code.c_str(), GL_FRAGMENT_SHADER, buffer)};

  const GLuint program{glCreateProgram()};

  glAttachShader(program, vert_shader);
  glAttachShader(program, frag_shader);
  glLinkProgram(program);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  GLint success{};
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (success == 0) {
    glGetProgramInfoLog(program, static_cast<GLsizei>(buffer.size()), nullptr,
                        buffer.data());
    LOG("GL", buffer);
    return nullptr;
  }

  Shader* shader = &shaders_.emplace_back(vert_path, frag_path, program);

  LOGF("GL", "Shader created (vertex: \"{}\") (fragment: \"{}\") (id: {})",
       vert_path.string(), frag_path.string(), program);

  return shader;
}

void Renderer::destroy_shader(Shader* shader) {
  if (shader->id != 0) {
    glDeleteProgram(shader->id);
  }
  LOGF("GL", "Shader destroyed (vertex: \"{}\") (fragment: \"{}\") (id: {})",
       shader->vert_path.string(), shader->frag_path.string(), shader->id);
  *shader = {};
}

void Renderer::bind_shader(Shader* shader) {
  if (bound_shader_ != shader) {
    glUseProgram(shader->id);
    bound_shader_ = shader;
    bound_material_ = nullptr;
  }
}

Texture* Renderer::create_texture(const fs::path& path) {
  if (auto it = std::find_if(
          textures_.begin(), textures_.end(),
          [&](const Texture& texture) { return texture.path == path; });
      it != textures_.end()) {
    return &(*it);
  }

  int width{};
  int height{};
  int components{};

  unsigned char* data =
      stbi_load(path.string().c_str(), &width, &height, &components, 0);
  if (data == nullptr) {
    LOGF("GL", "Failed to load \"{}\"", path.string());
    return nullptr;
  }

  GLuint id{};
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  GLint format{};
  switch (components) {
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
      assert(false && "Unknown format");
      break;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(data);

  Texture* texture = &textures_.emplace_back(path, id);

  LOGF("GL", "Texture created (file: \"{}\") (id: {})", path.string(), id);

  return texture;
}

void Renderer::destroy_texture(Texture* texture) {
  if (texture->id != 0) {
    glDeleteTextures(1, &texture->id);
  }
  LOGF("GL", "Texture destroyed (file: \"{}\") (id: {})",
       texture->path.string(), texture->id);
  *texture = {};
}

Model* Renderer::create_model(const fs::path& path) {
  if (auto it =
          std::find_if(models_.begin(), models_.end(),
                       [&](const Model& model) { return model.path == path; });
      it != models_.end()) {
    return &(*it);
  }

  const cgltf_options options{};
  cgltf_data* data{};
  cgltf_result result{cgltf_parse_file(&options, path.string().c_str(), &data)};
  if (result != cgltf_result_success) {
    LOGF("GL", "Failed to load \"{}\"", path.string());
    return nullptr;
  }

  result = cgltf_load_buffers(&options, data, path.string().c_str());
  if (result != cgltf_result_success) {
    LOGF("GL", "Failed to load buffers for \"{}\"", path.string());
    cgltf_free(data);
    return nullptr;
  }

  result = cgltf_validate(data);
  if (result != cgltf_result_success) {
    LOGF("GL", "Invalid gltf file \"{}\"", path.string());
    cgltf_free(data);
    return nullptr;
  }

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  assert(data->scene->nodes_count == 1);
  const cgltf_node* node = data->scene->nodes[0];

  assert(node->children_count == 0);
  assert(node->mesh->primitives_count == 1);
  const cgltf_primitive* primitive = &node->mesh->primitives[0];

  auto create_material = [this](const fs::path& parent,
                                cgltf_material* cgltf_material) {
    if (auto it = std::find_if(materials_.begin(), materials_.end(),
                               [&](const Material& material) {
                                 return material.name == cgltf_material->name;
                               });
        it != materials_.end()) {
      return &(*it);
    }

    fs::path path{parent};
    path += '/';
    path += cgltf_material->pbr_metallic_roughness.base_color_texture.texture
                ->image->uri;

    Material* material =
        &materials_.emplace_back(cgltf_material->name, create_texture(path));

    return material;
  };

  Material* material{};

  if (primitive->material != nullptr) {
    material = create_material(path.parent_path(), primitive->material);
  }

  assert(primitive->mappings_count == 0 || primitive->mappings_count == 2);

  Material* white{};
  Material* black{};
  if (primitive->mappings_count == 2) {
    white =
        create_material(path.parent_path(), primitive->mappings[0].material);
    black =
        create_material(path.parent_path(), primitive->mappings[1].material);
  }

  const cgltf_accessor* position{};
  const cgltf_accessor* normal{};
  const cgltf_accessor* tex_coord{};
  for (size_t i = 0; i < primitive->attributes_count; ++i) {
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
        assert(false && "Unknown attribute type");
        break;
    }
  }

  size_t vertex_count{vertices.size()};
  vertices.resize(vertex_count + position->count);
  for (size_t i = 0; i < position->count; ++i, ++vertex_count) {
    cgltf_accessor_read_float(
        position, i, glm::value_ptr(vertices[vertex_count].position), 3);
    cgltf_accessor_read_float(normal, i,
                              glm::value_ptr(vertices[vertex_count].normal), 3);
    cgltf_accessor_read_float(
        tex_coord, i, glm::value_ptr(vertices[vertex_count].tex_coord), 2);
  }

  indices.resize(primitive->indices->count);
  for (size_t i = 0; i < primitive->indices->count; ++i) {
    indices[i] = cgltf_accessor_read_index(primitive->indices, i);
  }

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

  Model* model = &models_.emplace_back(
      path, Mesh{vao, vbo, ebo, static_cast<GLsizei>(indices.size()), material,
                 white, black});

  LOGF("GL", "Model created (file: \"{}\")", path.string());

  return model;
}

void Renderer::destroy_model(Model* model) {
  if (model->mesh.vao != 0) {
    glDeleteBuffers(1, &model->mesh.vao);
  }
  if (model->mesh.vbo != 0) {
    glDeleteBuffers(1, &model->mesh.vbo);
  }
  if (model->mesh.ebo != 0) {
    glDeleteBuffers(1, &model->mesh.ebo);
  }
  LOGF("GL", "Model destroyed (file: \"{}\") (vao: {})", model->path.string(),
       model->mesh.vao);
  *model = {};
}

void Renderer::draw_model(const Transform& transform, Model* model,
                          Material* material) {
  if (material != nullptr && bound_material_ != material) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material->base_color->id);
  }

  bound_material_ = material;

  int width{};
  int height{};
  glfwGetWindowSize(window_, &width, &height);

  const float aspect_ratio{static_cast<float>(width) /
                           static_cast<float>(height)};
  set_shader_uniform(
      bound_shader_, "projection",
      glm::perspective(glm::radians(60.0F), aspect_ratio, 0.1F, 125.0F));
  set_shader_uniform(bound_shader_, "view", camera_->calculate_view_matrix());
  set_shader_uniform(
      bound_shader_, "model",
      glm::scale(
          glm::rotate(
              glm::translate(glm::identity<glm::mat4>(), transform.position),
              glm::radians(transform.rotation), {0.0F, 1.0F, 0.0F}),
          glm::vec3{transform.scale}));

  if (bound_material_ != nullptr) {
    set_shader_uniform(bound_shader_, "base_tex", 0);
  }

  glBindVertexArray(model->mesh.vao);
  glDrawElements(GL_TRIANGLES, model->mesh.index_count, GL_UNSIGNED_INT,
                 nullptr);
  glBindVertexArray(0);
}

void Renderer::draw_model_outline(const Transform& transform, Model* model,
                                  float thickness, const glm::vec4& color) {
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00);
  // glDisable(GL_DEPTH_TEST);
  set_shader_uniform(bound_shader_, "outline_thickness", thickness);
  set_shader_uniform(bound_shader_, "color", color);

  draw_model(transform, model, nullptr);

  glStencilMask(0xFF);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  // glEnable(GL_DEPTH_TEST);
}

Framebuffer* Renderer::create_framebuffer(const glm::ivec2& size) {
  GLuint id{};
  GLuint color{};
  GLuint depth{};

  glGenFramebuffers(1, &id);
  glBindFramebuffer(GL_FRAMEBUFFER, id);

  glGenTextures(1, &color);
  glBindTexture(GL_TEXTURE_2D, color);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, size.x, size.y, 0, GL_RED_INTEGER,
               GL_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         color, 0);

  glGenTextures(1, &depth);
  glBindTexture(GL_TEXTURE_2D, depth);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depth, 0);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    return nullptr;
  }

  auto* framebuffer = &framebuffers_.emplace_back(static_cast<Framebuffer>(id));

  LOGF("GL", "Framebuffer created (id: {}) (color: {}) (depth: {})", id, color,
       depth);

  return framebuffer;
}

void Renderer::destroy_framebuffer(Framebuffer* framebuffer) {
  GLuint id{static_cast<GLuint>(*framebuffer)};
  if (id != 0) {
    GLint color{};
    GLint depth{};
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                          &color);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                          &depth);

    auto u_color{static_cast<GLuint>(color)};
    auto u_depth{static_cast<GLuint>(depth)};
    if (color != 0) {
      glDeleteTextures(1, &u_color);
    }
    if (depth != 0) {
      glDeleteTextures(1, &u_depth);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &id);

    LOGF("GL", "Framebuffer destroyed (id: {}) (color: {}) (depth: {})", id,
         u_color, u_depth);
    *framebuffer = {};
  }
}

void Renderer::bind_framebuffer(Framebuffer* framebuffer, GLenum target) {
  if (bound_framebuffer_ != framebuffer) {
    glBindFramebuffer(target, static_cast<GLuint>(*framebuffer));
    bound_framebuffer_ = framebuffer;
  }
}

void Renderer::unbind_framebuffer(GLenum target) {
  glBindFramebuffer(target, 0);
  bound_framebuffer_ = nullptr;
}

void Renderer::clear_framebuffer() {
  std::array<GLint, 4> clear_color{-1, -1, -1, -1};
  glClearBufferiv(GL_COLOR, 0, clear_color.data());

  const GLfloat clear_depth{1.0F};
  glClearBufferfv(GL_DEPTH, 0, &clear_depth);
}

int Renderer::read_pixel(const glm::ivec2& coord) {
  int data{};
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(coord.x, coord.y, 1, 1, GL_RED_INTEGER, GL_INT, &data);
  glReadBuffer(GL_NONE);
  return data;
}

void Renderer::begin_drawing(Camera& camera) {
  glClearColor(0.25F, 0.25F, 0.25F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  camera_ = &camera;
}

void Renderer::end_drawing() {
  camera_ = nullptr;
  glfwSwapBuffers(window_);
}

void Renderer::begin_stencil_writing() {
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0xFF);
}

void Renderer::end_stencil_writing() {
  glStencilMask(0xFF);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
}

void Renderer::clear_stencil() { glClear(GL_STENCIL_BUFFER_BIT); }

void Renderer::begin_wire_mode() { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }

void Renderer::end_wire_mode() { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
