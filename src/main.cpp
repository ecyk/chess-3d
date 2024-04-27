#include "game.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

GLFWwindow* glfw_init();
void glfw_destroy();

int main() {
  GLFWwindow* window{glfw_init()};
  if (window == nullptr) {
    return 1;
  }

  {
    Game game{window};
    game.run();
  }

  glfw_destroy();
  return 0;
}

void gl_callback_pre(const char* name, GLADapiproc apiproc, int /*len_args*/,
                     ...) {
  if (apiproc == nullptr) {
    LOGF("GL", "{} is NULL", name);
    return;
  }
  if (glad_glGetError == nullptr) {
    LOG("GL", "glGetError is NULL");
    return;
  }

  (void)glad_glGetError();
}

void gl_callback_post(void* /*ret*/, const char* name, GLADapiproc /*apiproc*/,
                      int /*len_args*/, ...) {
  if (GLenum error_code = glad_glGetError(); error_code != GL_NO_ERROR) {
    LOGF("GL", "Error in {} ({})", name, error_code);
  }
}

void error_callback(int /*error_code*/, const char* description) {
  LOG("GLFW", description);
}

void framebuffer_resize_callback(GLFWwindow* /*window*/, int width,
                                 int height) {
  glViewport(0, 0, width, height);
}

GLFWwindow* glfw_init() {
  glfwSetErrorCallback(error_callback);

  if (glfwInit() != 1) {
    LOG("GLFW", "Failed to initialize");
    return nullptr;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_SAMPLES, 8);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow* window{glfwCreateWindow(static_cast<int>(k_window_size.x),
                                      static_cast<int>(k_window_size.y),
                                      "chess-3d", nullptr, nullptr)};
  if (window == nullptr) {
    LOG("GLFW", "Failed to initialize window");
    glfw_destroy();
    return nullptr;
  }

  glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  gladSetGLPreCallback(gl_callback_pre);
  gladSetGLPostCallback(gl_callback_post);

  if (const int version = gladLoadGL(glfwGetProcAddress); version == 0) {
    LOG("GLFW", "Failed to load GL");
    glfw_destroy();
    return nullptr;
  }

  return window;
}

void glfw_destroy() { glfwTerminate(); }
