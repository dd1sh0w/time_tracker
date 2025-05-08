// logger.cpp
#include "logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <spdlog/details/os.h>
#include <QDir>
#include <QDateTime>
#include <QFileInfoList>
#include <QThread>
#include <csignal>
#include <exception>
#include <cstdlib>
#include <QtGlobal>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    #ifdef ERROR
        #undef ERROR
    #endif
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
#else
    #include <execinfo.h>
#endif

namespace fs = std::filesystem;

namespace {
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<spdlog::logger> json_logger;
    std::string g_component_name;
    std::string g_log_dir;
    constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
    constexpr size_t MAX_FILES = 10; // Количество файлов ротации
    constexpr int MAX_DAYS = 7;

    std::string iso8601_now() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        #ifdef _WIN32
            localtime_s(&tm, &t);
        #else
            localtime_r(&t, &tm);
        #endif
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
        int offset = static_cast<int>(spdlog::details::os::utc_minutes_offset());
        char offset_buf[8];
        snprintf(offset_buf, sizeof(offset_buf), "%+03d:%02d", offset/60, abs(offset%60));
        char result[80];
        snprintf(result, sizeof(result), "%s.%03lld%s", buf, static_cast<long long>(ms.count()), offset_buf);
        return result;
    }

    std::string contextToString(const std::map<std::string, std::string>& context) {
        if (context.empty()) return "";
        std::ostringstream oss;
        for (const auto& kv : context) {
            if (oss.tellp() > 0) oss << ", ";
            oss << kv.first << "=" << kv.second;
        }
        return oss.str();
    }

    void logBacktrace() {
        #ifdef _WIN32
        const int max_frames = 62;
        void* stack[max_frames];
        USHORT frames = CaptureStackBackTrace(0, max_frames, stack, NULL);
        HANDLE process = GetCurrentProcess();
        SymInitialize(process, NULL, TRUE);
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        LOG_FATAL(g_component_name, "Stack trace:", {});
        for (USHORT i = 0; i < frames; i++) {
            SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
            std::ostringstream oss;
            oss << "[" << i << "] " << symbol->Name << " - 0x" << std::hex << symbol->Address;
            LOG_FATAL(g_component_name, oss.str(), {});
        }
        free(symbol);
        #else
        const int max_frames = 50;
        void* frames[max_frames];
        int frame_count = backtrace(frames, max_frames);
        char** symbols = backtrace_symbols(frames, frame_count);
        LOG_FATAL(g_component_name, "Stack trace:", {});
        for (int i = 0; i < frame_count; ++i) {
            LOG_FATAL(g_component_name, symbols[i], {});
        }
        free(symbols);
        #endif
    }

    static void terminateHandler() {
        if (auto eptr = std::current_exception()) {
            try { std::rethrow_exception(eptr); }
            catch (const std::exception& e) {
                LOG_FATAL(g_component_name, std::string("Unhandled exception: ") + e.what(), {});
            } catch (...) {
                LOG_FATAL(g_component_name, "Unhandled non-std::exception", {});
            }
        } else {
            LOG_FATAL(g_component_name, "Terminate called without active exception", {});
        }
        logBacktrace();
        spdlog::shutdown();
        std::abort();
    }

    static void signalHandler(int signum) {
        LOG_FATAL(g_component_name, std::string("Received signal ") + std::to_string(signum), {});
        logBacktrace();
        spdlog::shutdown();
        std::signal(signum, SIG_DFL);
        std::raise(signum);
    }

    static void qtMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
        if (type == QtFatalMsg || type == QtCriticalMsg) {
            QString details = QString("%1 (%2:%3, %4)")
                .arg(msg)
                .arg(ctx.file ? ctx.file : "")
                .arg(ctx.line)
                .arg(ctx.function ? ctx.function : "");
            LOG_FATAL(g_component_name, details.toStdString(), {});
            logBacktrace();
            spdlog::shutdown();
        } else {
            QByteArray localMsg = msg.toLocal8Bit();
            fprintf(stderr, "QtLog: %s\n", localMsg.constData());
        }
    }
}

void Logger::init(const std::string& component_name, const std::string& log_dir) {
    g_component_name = component_name;
    g_log_dir = log_dir;
    QDir().mkpath(QString::fromStdString(log_dir));

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_dir + "/log.txt", MAX_FILE_SIZE, MAX_FILES);
    file_sink->set_pattern("%v");

    init_json_logger(log_dir + "/log.json");

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("%v");

    logger = std::make_shared<spdlog::logger>("main",
        spdlog::sinks_init_list{file_sink, console_sink});
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::info);
    spdlog::register_logger(logger);

    cleanupOldLogs(log_dir);

    std::set_terminate(terminateHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGFPE, signalHandler);
    std::signal(SIGILL, signalHandler);
    std::signal(SIGTERM, signalHandler);
    qInstallMessageHandler(qtMessageHandler);
}

void Logger::init_json_logger(const std::string& filename) {
    json_logger = spdlog::rotating_logger_mt("json_logger", filename, 1048576 * 5, 3);
}

void Logger::log(LogLevel level, const std::string& component,
                 const std::string& message,
                 const std::map<std::string, std::string>& context) {
    if (!logger) return;
    std::string ts = iso8601_now();
    std::string lvl = levelToString(level);
    std::string thread_id = std::to_string((quint64)QThread::currentThreadId());
    std::string ctx = contextToString(context);
    char buf[2048];
    snprintf(buf, sizeof(buf), "%s %5s %-16s [Thread:%s] %s %s",
        ts.c_str(), lvl.c_str(), component.c_str(), thread_id.c_str(),
        ctx.empty()?"":"|", ctx.c_str());
    std::string fullmsg = std::string(buf) + message;
    logger->log(toSpdLevel(level), fullmsg);

    // Записываем только строку JSON вручную, без внешней библиотеки
    if (json_logger) {
        int pid =
        #ifdef _WIN32
            _getpid();
        #else
            getpid();
        #endif
        std::ostringstream oss;
        oss << "{"
            << "\"time\": \"" << ts << "\", "
            << "\"name\": \"" << component << "\", "
            << "\"level\": \"" << lvl << "\", "
            << "\"process\": " << pid << ", "
            << "\"thread\": " << thread_id << ", "
            << "\"message\": \"" << message << "\"";
        if (!ctx.empty()) oss << ", \"context\": \"" << ctx << "\"";
        oss << "}";
        json_logger->log(toSpdLevel(level), oss.str());
    }
}

void Logger::close_json_logger() {
    spdlog::drop("json_logger");
}

spdlog::level::level_enum Logger::toSpdLevel(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return spdlog::level::debug;
        case LogLevel::INFO:    return spdlog::level::info;
        case LogLevel::WARNING: return spdlog::level::warn;
        case LogLevel::ERROR:   return spdlog::level::err;
        case LogLevel::FATAL:   return spdlog::level::critical;
    }
    return spdlog::level::info;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return " INFO";
        case LogLevel::WARNING: return " WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
    }
    return " INFO";
}

void Logger::setLevel(LogLevel level) {
    if (logger) logger->set_level(toSpdLevel(level));
}

void Logger::cleanupOldLogs(const std::string& log_dir) {
    QDir dir(QString::fromStdString(log_dir));
    QFileInfoList files = dir.entryInfoList(QStringList() << "*.txt" << "*.json", QDir::Files);
    QDateTime now = QDateTime::currentDateTime();
    for (const QFileInfo& fi : files) {
        if (fi.lastModified().daysTo(now) > MAX_DAYS) {
            QFile::remove(fi.absoluteFilePath());
        }
    }
}
