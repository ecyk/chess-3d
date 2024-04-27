#include "log.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>

void log(std::string_view tag, std::string_view message,
         const std::source_location& source) {
  std::cerr
      << std::format(
             "[{:%F %T}] - [{}] {} ({}:{}:{})",
             std::chrono::zoned_time{std::chrono::current_zone(),
                                     std::chrono::system_clock::now()},
             tag, message,
             std::filesystem::path{source.file_name()}.filename().string(),
             source.function_name(), source.line())
      << '\n';
}
