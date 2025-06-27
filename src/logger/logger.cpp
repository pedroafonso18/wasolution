#include "logger.h"
#include "spdlog/spdlog.h"

Logger::Logger(const std::string& file) : filepath_(file) {
    setup_logger();
}

Logger::~Logger() {
    spdlog::drop(filepath_);
}

void Logger::setup_logger() {
    logger_ = spdlog::basic_logger_mt(filepath_, filepath_);
    logger_->set_level(spdlog::level::debug);
    logger_->flush_on(spdlog::level::err);
}

void Logger::info(const std::string& message) {
    logger_->info(message);
}

void Logger::error(const std::string& message) {
    logger_->error(message);
}

void Logger::debug(const std::string& message) {
    logger_->debug(message);
}

void Logger::warn(const std::string& message) {
    logger_->warn(message);
}