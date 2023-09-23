#pragma once

#include <array>
#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "log.hpp"

namespace fs = std::filesystem;

inline constexpr glm::vec2 window_initial_size{1280, 720};
