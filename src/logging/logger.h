#pragma once
#include <memory>
#include <string>
#include <map>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

// Уровни логирования
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    // Инициализация логгера
    static void init(const std::string& component_name, const std::string& log_dir = "logs");
    // Логирование сообщения
    static void log(LogLevel level, const std::string& component, const std::string& message, const std::map<std::string,std::string>& context = {});
    // Получить уровень spdlog
    static spdlog::level::level_enum toSpdLevel(LogLevel level);
    // Получить строку уровня
    static std::string levelToString(LogLevel level);
    // Установить уровень логирования
    static void setLevel(LogLevel level);
    // Очистка старых файлов (старше 7 дней)
    static void cleanupOldLogs(const std::string& log_dir = "logs");
    // JSON logging
    static void init_json_logger(const std::string& filename);
    static void log_json(LogLevel level, const std::string& component, const std::string& message, const std::map<std::string, std::string>& context);
    static void close_json_logger();
};

// Макросы для удобного логирования
#define LOG_DEBUG(component, msg, ctx)   Logger::log(LogLevel::DEBUG,   component, msg, ctx)
#define LOG_INFO(component, msg, ctx)    Logger::log(LogLevel::INFO,    component, msg, ctx)
#define LOG_WARNING(component, msg, ctx) Logger::log(LogLevel::WARNING, component, msg, ctx)
#define LOG_ERROR(component, msg, ctx)   Logger::log(LogLevel::ERROR,   component, msg, ctx)
#define LOG_FATAL(component, msg, ctx)   Logger::log(LogLevel::FATAL,   component, msg, ctx)