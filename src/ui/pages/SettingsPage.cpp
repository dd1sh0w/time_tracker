#include "SettingsPage.h"
#include "../../Application.h"
#include "../../logging/logger.h"

#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QLocale>
#include <QApplication>
#include <QCoreApplication>

Application* SettingsPage::getApp() const
{
    return qobject_cast<Application*>(QCoreApplication::instance());
}

SettingsPage::SettingsPage(QWidget *parent)
    : BasePage(parent)
    , m_themeManager(nullptr)
{
    auto app = qobject_cast<Application*>(QCoreApplication::instance());
    if (app) {
        m_themeManager = app->themeManager();
    } else {
        qWarning() << "Failed to get ThemeManager instance";
    }

    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SettingsPage", "SettingsPage created", ctx);
    setupUi();
    loadLanguages();
}

SettingsPage::~SettingsPage()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SettingsPage", "SettingsPage destroyed", ctx);
}

void SettingsPage::loadLanguages()
{
    m_languageComboBox->clear();
    
    Application *app = getApp();
    if (!app) return;
    
    // Add available languages
    QStringList languages = app->availableLanguages();
    
    // Map locale codes to display names
    QMap<QString, QString> languageNames = {
        {"en_US", "English"},
        {"ru_RU", "Русский"}
    };
    
    int currentIndex = -1;
    QString currentLanguage = app->currentLanguage();
    
    for (int i = 0; i < languages.size(); ++i) {
        const QString &locale = languages[i];
        QString displayName = languageNames.value(locale, locale);
        m_languageComboBox->addItem(displayName, locale);
        
        if (locale == currentLanguage) {
            currentIndex = i;
        }
    }
    
    if (currentIndex >= 0) {
        m_languageComboBox->setCurrentIndex(currentIndex);
    }
}

void SettingsPage::retranslateUi()
{
    m_titleLabel->setText(tr("Settings"));
    m_themeGroup->setTitle(tr("Theme Settings"));
    m_themeLabel->setText(tr("Select Theme:"));
    m_themePathLabel->setText(tr("Theme Folder:"));
    m_browseThemeButton->setText(tr("Browse..."));
    m_applyThemeButton->setText(tr("Apply"));
    m_languageGroup->setTitle(tr("Language"));
    m_languageLabel->setText(tr("Select Language:"));
}

void SettingsPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    BasePage::changeEvent(event);
}

void SettingsPage::setupUi()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SettingsPage", "Setting up SettingsPage UI", ctx);
    m_mainLayout = new QGridLayout(this);

    // Header
    m_titleLabel = new QLabel(tr("Settings"), this);
    m_titleLabel->setObjectName("settingsTitleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel, 0, 0, 1, 2, Qt::AlignCenter);

    // Theme settings group
    m_themeGroup = new QGroupBox(tr("Theme Settings"), this);
    m_themeLayout = new QGridLayout(m_themeGroup);
    
    // Theme path selection
    m_themePathLabel = new QLabel(tr("Theme Folder:"), this);
    m_browseThemeButton = new QPushButton(tr("Browse..."), this);
    connect(m_browseThemeButton, &QPushButton::clicked, this, &SettingsPage::browseThemeFolder);
    
    // Theme selection
    m_themeLabel = new QLabel(tr("Select Theme:"), this);
    m_themeComboBox = new QComboBox(this);
    m_applyThemeButton = new QPushButton(tr("Apply Theme"), this);
    
    // Add widgets to layout
    int row = 0;
    m_themeLayout->addWidget(m_themePathLabel, row, 0);
    m_themeLayout->addWidget(m_browseThemeButton, row++, 1);
    
    m_themeLayout->addWidget(m_themeLabel, row, 0);
    m_themeLayout->addWidget(m_themeComboBox, row++, 1);
    
    m_themeLayout->addWidget(m_applyThemeButton, row, 0, 1, 2, Qt::AlignCenter);
    
    m_themeGroup->setLayout(m_themeLayout);
    m_mainLayout->addWidget(m_themeGroup, 1, 0, 1, 2);
    
    // Update theme list initially
    updateThemeList();

    // Language settings group
    m_languageGroup = new QGroupBox(tr("Language"), this);
    m_languageLayout = new QGridLayout(m_languageGroup);
    
    m_languageLabel = new QLabel(tr("Select Language:"), this);
    m_languageComboBox = new QComboBox(this);
    
    m_languageLayout->addWidget(m_languageLabel, 0, 0);
    m_languageLayout->addWidget(m_languageComboBox, 0, 1);
    
    m_languageGroup->setLayout(m_languageLayout);
    m_mainLayout->addWidget(m_languageGroup, 2, 0, 1, 2);

    setLayout(m_mainLayout);

    // Connect signals after UI is set up
    connect(m_applyThemeButton, &QPushButton::clicked, this, &SettingsPage::changeTheme);
    connect(m_languageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsPage::changeLanguage);
    
    // Update theme list
    updateThemeList();

    // Read saved language from QSettings (if exists)
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "voodz_d1sh0w", "time_tracker");
    QString savedLocale = settings.value("language", QLocale::system().name()).toString();
    auto ctxLocale = std::map<std::string, std::string>{{{"savedLocale", savedLocale.toStdString()}}};
    LOG_DEBUG("SettingsPage", "Reading saved language", ctxLocale);
    m_languageComboBox->setCurrentText(savedLocale);
}

void SettingsPage::updateThemeList()
{
    if (!m_themeManager) {
        qWarning() << "Theme manager is not available";
        return;
    }
    
    // Show current theme path
    m_themePathLabel->setText(tr("Theme Folder: %1").arg(m_themeManager->baseThemePath()));
    
    // Update theme list
    m_themeComboBox->clear();
    QStringList themes = m_themeManager->availableThemes();
    m_themeComboBox->addItems(themes);
    
    // Select current theme if any
    QString currentTheme = m_themeManager->currentTheme();
    if (!currentTheme.isEmpty()) {
        int index = m_themeComboBox->findText(currentTheme);
        if (index >= 0) {
            m_themeComboBox->setCurrentIndex(index);
        }
    }
}

void SettingsPage::browseThemeFolder()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SettingsPage", "Browsing for theme folder", ctx);
    
    QString currentPath = m_themeManager->baseThemePath();
    QString dir = QFileDialog::getExistingDirectory(this, 
        tr("Select Theme Folder"), 
        currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        
    if (!dir.isEmpty()) {
        m_themeManager->setBaseThemePath(dir);
        
        // Update the UI to show the new theme list
        updateThemeList();
        
        QMessageBox::information(this, 
            tr("Theme Folder Changed"),
            tr("Theme folder updated. Please select and apply a theme."));
    }
}

void SettingsPage::changeTheme()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SettingsPage", "Changing theme", ctx);

    QString themeName = m_themeComboBox->currentText();
    if (themeName.isEmpty()) {
        LOG_WARNING("SettingsPage", "No theme selected", ctx);
        return;
    }

    if (m_themeManager->applyTheme(themeName)) {
        QMessageBox::information(this, tr("Success"), tr("Theme applied successfully!"));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to apply theme!"));
    }
}

void SettingsPage::changeLanguage(int index)
{
    if (index < 0) return;
    
    QString locale = m_languageComboBox->itemData(index).toString();
    
    Application *app = getApp();
    if (!app) return;
    
    // Don't do anything if the language hasn't changed
    if (app->currentLanguage() == locale) {
        return;
    }
    
    // Set the new language
    if (app->setLanguage(locale)) {
        // Update UI to reflect the new language
        retranslateUi();
        
        // Notify user that the language has been changed
        static bool showMessage = true; // Static flag to ensure the message is shown only once
        if (showMessage) {
            QMessageBox::information(this, 
                                  tr("Language Changed"),
                                  tr("The language has been changed. Some changes may require a restart to take effect."));
            showMessage = false;
        }
    }
}
