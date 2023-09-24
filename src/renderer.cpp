#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <fstream>

Renderer::~Renderer() {
  for (size_t i = textures_.size(); i-- > 0;) {
    if (textures_[i].id != 0) {
      destroy_texture(&textures_[i]);
    }
  }
  textures_.clear();

  for (size_t i = shaders_.size(); i-- > 0;) {
    if (shaders_[i].id != 0) {
      destroy_shader(&shaders_[i]);
    }
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
  glDeleteProgram(shader->id);
  LOGF("GL", "Shader destroyed (vertex: \"{}\") (fragment: \"{}\") (id: {})",
       shader->vert_path.string(), shader->frag_path.string(), shader->id);
  *shader = {};
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
  glDeleteTextures(1, &texture->id);
  LOGF("GL", "Texture destroyed (file: \"{}\") (id: {})",
       texture->path.string(), texture->id);
  *texture = {};
}

void Renderer::begin_drawing() {
  glClearColor(0.25F, 0.25F, 0.25F, 1.0F);
  glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::end_drawing() { glfwSwapBuffers(window_); }
