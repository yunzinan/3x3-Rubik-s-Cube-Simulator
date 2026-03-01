#pragma once

#include <spdlog/spdlog.h>

namespace rubik {

// Thin convenience wrapper – call Logger::init() once at startup.
class Logger {
public:
    static void init();
    static std::shared_ptr<spdlog::logger>& get();

private:
    static std::shared_ptr<spdlog::logger> logger_;
};

} // namespace rubik

// Short-hand macros
#define LOG_TRACE(...)  ::rubik::Logger::get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)  ::rubik::Logger::get()->debug(__VA_ARGS__)
#define LOG_INFO(...)   ::rubik::Logger::get()->info(__VA_ARGS__)
#define LOG_WARN(...)   ::rubik::Logger::get()->warn(__VA_ARGS__)
#define LOG_ERROR(...)  ::rubik::Logger::get()->error(__VA_ARGS__)
