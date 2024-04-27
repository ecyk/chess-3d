#pragma once

#include <array>
#include <chrono>
#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "log.hpp"

using namespace std::chrono_literals;

struct StringHash {
  using is_transparent = void;

  [[nodiscard]] size_t operator()(const char* str) const {
    return std::hash<std::string_view>{}(str);
  }
  [[nodiscard]] size_t operator()(std::string_view str) const {
    return std::hash<std::string_view>{}(str);
  }
  [[nodiscard]] size_t operator()(const std::string& str) const {
    return std::hash<std::string>{}(str);
  }
};

// clang-format off
template <typename T>
using StringMap = std::unordered_map<std::string, T, StringHash, std::equal_to<>>;

template <class... Ts>
struct overload : Ts... { using Ts::operator()...; };
template <class... Ts>
overload(Ts...) -> overload<Ts...>;
// clang-format on

template <typename E>
constexpr auto to_underlying(E e) -> std::underlying_type_t<E> {
  return static_cast<std::underlying_type_t<E>>(e);
}
