#pragma once

#include <format>
#include <source_location>
#include <string_view>

// #define DISABLE_LOGGING

#ifdef DISABLE_LOGGING
#define LOG(tag, message)
#define LOGF(tag, fmt, ...)
#else
#define LOG(tag, message) log(tag, message)
#define LOGF(tag, fmt, ...) log(tag, std::format(fmt, __VA_ARGS__))
#endif

void log(std::string_view tag, std::string_view message,
         const std::source_location& source = std::source_location::current());
