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

template <typename E>
constexpr auto to_underlying(E e) -> typename std::underlying_type<E>::type {
  return static_cast<typename std::underlying_type<E>::type>(e);
}

inline constexpr glm::vec2 window_initial_size{1280, 720};
