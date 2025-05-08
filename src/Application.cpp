#include "Application.h"
#include "ui/MainWindow.h"
#include "core/DatabaseManager.h"
#include "logging/logger.h"
#include <QMessageBox>
#include <QSettings>
#include <QFile>

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    auto ctxStart = std::map<std::string, std::string>{};
    LOG_INFO("Application", "Starting application", ctxStart);
    setupTranslations();
    m_themeManager = new ThemeManager(this);

    QCoreApplication::setOrganizationName("voodz_d1sh0w");
    QCoreApplication::setApplicationName("time_tracker");

    // Check if config file exists
    if (!QFile::exists("../config.ini")) {
        auto ctxWarn = std::map<std::string, std::string>{};
        LOG_WARNING("Application", "Configuration file not found!", ctxWarn);
        QMessageBox::critical(nullptr, "Error", 
            "Configuration file not found!\n"
            "Please copy config.ini.example to config.ini and update the settings.");
        return;
    }

    // Load database configuration
    QSettings settings("../config.ini", QSettings::IniFormat);
    settings.beginGroup("Database");
    QString host = settings.value("host", "localhost").toString();
    int port = settings.value("port", 5432).toInt();
    QString dbName = settings.value("name", "").toString();
    QString user = settings.value("user", "").toString();
    QString password = settings.value("password", "").toString();
    settings.endGroup();

    auto ctxDb = std::map<std::string, std::string>{
        {"host", host.toStdString()},
        {"port", std::to_string(port)},
        {"dbName", dbName.toStdString()},
        {"user", user.toStdString()},
        {"password_length", std::to_string(password.length())}
    };
    LOG_INFO("Application", "Loaded database settings", ctxDb);

    // Try to connect to database
    auto ctxConn = std::map<std::string, std::string>{
        {"host", host.toStdString()},
        {"port", std::to_string(port)},
        {"dbName", dbName.toStdString()},
        {"user", user.toStdString()}
    };
    LOG_INFO("Application", "Attempting database connection", ctxConn);
    bool ok = DatabaseManager::instance().openDB(host, port, dbName, user, password);
    if (!ok) {
        LOG_WARNING("Application", "Failed to connect to database", ctxConn);
        QMessageBox::critical(nullptr, "Error", "Failed to connect to database. Check your configuration.");
        return;
    }

    auto ctxOk = std::map<std::string, std::string>{};
    LOG_INFO("Application", "Database connection successful", ctxOk);
    // Create and show main window
    auto ctxMainWindow = std::map<std::string, std::string>{};
    LOG_INFO("Application", "Creating main window", ctxMainWindow);
    MainWindow *w = new MainWindow;
    w->show();
    auto ctxMainWindowShown = std::map<std::string, std::string>{};
    LOG_INFO("Application", "Main window shown", ctxMainWindowShown);
}

Application::~Application()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("Application", "Application shutting down", ctx);
}

void Application::setupTranslations()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("Application", "Setting up translations", ctx);
}
