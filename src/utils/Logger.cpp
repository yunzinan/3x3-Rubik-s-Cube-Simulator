#include "utils/Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace rubik {

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::init() {
    logger_ = spdlog::stdout_color_mt("RubiksCube");
    logger_->set_level(spdlog::level::trace);
    logger_->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
}

std::shared_ptr<spdlog::logger>& Logger::get() {
    return logger_;
}

} // namespace rubik
