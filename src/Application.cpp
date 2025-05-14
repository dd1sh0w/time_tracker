#include "Application.h"
#include "ui/MainWindow.h"
#include "core/DatabaseManager.h"
#include "logging/logger.h"
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QTranslator>
#include <QDebug>

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_themeManager(new ThemeManager(this))
    , m_translator(new QTranslator(this))
    , m_currentLanguage("en_US") // Default to English
{
    // Set application information
    setApplicationName("Time Tracker");
    setApplicationVersion("1.0.0");
    setOrganizationName("voodz_d1sh0w");
    setOrganizationDomain("voodz-d1sh0w.com");

    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("Application", "Application starting", ctx);

    // Setup translations
    setupTranslations();

    // Initialize database
    DatabaseManager::instance();

    // Check if config file exists
    if (!QFile::exists("../config.ini")) {
        auto ctxWarn = std::map<std::string, std::string>{};
        LOG_WARNING("Application", "Configuration file not found!", ctxWarn);
        // in message box also need to add directory where are searching
        QMessageBox::critical(nullptr, "Error", 
            "Configuration file not found!\n"
            "Please copy config.ini.example to config.ini and update the settings.\n"
            "Searching in directory: " + QDir::currentPath());
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

QStringList Application::availableLanguages() const
{
    return {"en_US", "ru_RU"};
}

QString Application::currentLanguage() const
{
    return m_currentLanguage;
}

bool Application::setLanguage(const QString &language)
{
    if (m_currentLanguage == language)
        return true;

    if (!availableLanguages().contains(language))
        return false;

    if (loadLanguage(language)) {
        m_currentLanguage = language;
        emit languageChanged();
        return true;
    }
    return false;
}

bool Application::setupTranslations()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("Application", "Setting up translations", ctx);
    
    // Load saved language or use system default
    QSettings settings;
    QString savedLanguage = settings.value("language", QLocale::system().name()).toString();
    
    // If saved language is not in available languages, use English as fallback
    if (!availableLanguages().contains(savedLanguage)) {
        savedLanguage = "en_US";
    }
    
    return loadLanguage(savedLanguage);
}

bool Application::loadLanguage(const QString &language)
{
    // Remove old translator
    QApplication::removeTranslator(m_translator);
    
    QString baseName = "time_tracker_" + language;
    bool loaded = false;
    
    // Try to load from resources first
    loaded = m_translator->load(baseName, ":/translations");
    
    if (!loaded) {
        // Try to load from filesystem
        loaded = m_translator->load(baseName, 
                                  QApplication::applicationDirPath() + "/translations");
    }
    
    if (loaded) {
        // Install the translator
        QApplication::installTranslator(m_translator);
        m_currentLanguage = language;
        
        // Save the language preference
        QSettings settings;
        settings.setValue("language", language);
        
        auto ctx = std::map<std::string, std::string>{{{"language", language.toStdString()}}};
        LOG_DEBUG("Application", "Loaded translation", ctx);
        return true;
    }
    
    auto ctx = std::map<std::string, std::string>{{{"language", language.toStdString()}}};
    LOG_WARNING("Application", "Failed to load translation", ctx);
    return false;
}