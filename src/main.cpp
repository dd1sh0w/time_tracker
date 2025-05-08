#include "Application.h"
#include "logging/logger.h"

int main(int argc, char *argv[])
{
    system("chcp 65001");
    Logger::init("Main", "logs");
    // Logger::init_json_logger("logs/log.json"); // Удалено, чтобы не было двойной инициализации
    LOG_INFO("Main", "Application starting", {});
    Application app(argc, argv);
    int result = app.exec();
    auto ctx = std::map<std::string, std::string>{{"exit_code", std::to_string(result)}};
    LOG_INFO("Main", "Application exiting", ctx);
    Logger::close_json_logger();
    return result;
}
