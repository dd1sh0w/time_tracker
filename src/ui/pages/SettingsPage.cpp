#include "SettingsPage.h"
#include "../../Application.h"
#include "../../core/ThemeManager.h"
#include "../../logging/logger.h"

#include <QSettings>
#include <QDebug>

SettingsPage::SettingsPage(QWidget *parent)
    : BasePage(parent)
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SettingsPage", "SettingsPage created", ctx);
    setupUi();
}

SettingsPage::~SettingsPage()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SettingsPage", "SettingsPage destroyed", ctx);
}

void SettingsPage::setupUi()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SettingsPage", "Setting up SettingsPage UI", ctx);
    m_mainLayout = new QGridLayout(this);

    // Header
    m_titleLabel = new QLabel(tr("Settings Page"), this);
    m_titleLabel->setStyleSheet("font-size: 22px; font-weight: bold;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel, 0, 0, 1, 2, Qt::AlignCenter);

    // Theme settings group
    m_themeGroup = new QGroupBox(tr("Theme Settings"), this);
    m_themeLayout = new QGridLayout(m_themeGroup);
    m_themeLabel = new QLabel(tr("Select Theme:"), this);
    m_themeComboBox = new QComboBox(this);
    m_applyThemeButton = new QPushButton(tr("Apply Theme"), this);
    m_themeLayout->addWidget(m_themeLabel, 0, 0);
    m_themeLayout->addWidget(m_themeComboBox, 0, 1);
    m_themeLayout->addWidget(m_applyThemeButton, 1, 0, 1, 2, Qt::AlignCenter);
    m_themeGroup->setLayout(m_themeLayout);
    m_mainLayout->addWidget(m_themeGroup, 1, 0, 1, 2);

    // Star emoji settings group
    m_emojiGroup = new QGroupBox(tr("Star Emoji Settings"), this);
    m_emojiLayout = new QGridLayout(m_emojiGroup);
    m_emojiLabel = new QLabel(tr("Select Star Emoji:"), this);
    m_emojiComboBox = new QComboBox(this);
    // Fill the list with example emojis (can be expanded)
    m_emojiComboBox->addItem("★");  // classic star
    m_emojiComboBox->addItem("⭐");  // white star
    m_emojiComboBox->addItem("✶");  // alternative variant
    m_applyEmojiButton = new QPushButton(tr("Apply Star Emoji"), this);
    m_emojiLayout->addWidget(m_emojiLabel, 0, 0);
    m_emojiLayout->addWidget(m_emojiComboBox, 0, 1);
    m_emojiLayout->addWidget(m_applyEmojiButton, 1, 0, 1, 2, Qt::AlignCenter);
    m_emojiGroup->setLayout(m_emojiLayout);
    m_mainLayout->addWidget(m_emojiGroup, 2, 0, 1, 2);

    setLayout(m_mainLayout);

    // Fill theme data
    Application *app = qobject_cast<Application *>(qApp);
    if (app && app->themeManager()) {
        auto ctx = std::map<std::string, std::string>{};
        LOG_DEBUG("SettingsPage", "Filling theme data", ctx);
        m_themeComboBox->addItems(app->themeManager()->availableThemes());
        m_themeComboBox->setCurrentText(app->themeManager()->currentTheme());
    }

    // Read saved emoji from QSettings (if exists)
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "voodz_d1sh0w", "time_tracker");
    QString savedEmoji = settings.value("starEmoji", "★").toString();
    auto ctxEmoji = std::map<std::string, std::string>{{{"savedEmoji", savedEmoji.toStdString()}}};
    LOG_DEBUG("SettingsPage", "Reading saved emoji", ctxEmoji);
    m_emojiComboBox->setCurrentText(savedEmoji);

    connect(m_applyThemeButton, &QPushButton::clicked, this, &SettingsPage::changeTheme);
    connect(m_applyEmojiButton, &QPushButton::clicked, this, &SettingsPage::changeStarEmoji);
}

void SettingsPage::changeTheme()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SettingsPage", "Changing theme", ctx);
    Application *app = qobject_cast<Application *>(qApp);
    if (app && app->themeManager()) {
        QString selectedTheme = m_themeComboBox->currentText();
        if (!selectedTheme.isEmpty()) {
            auto ctxTheme = std::map<std::string, std::string>{{{"selectedTheme", selectedTheme.toStdString()}}};
            LOG_DEBUG("SettingsPage", "Applying theme", ctxTheme);
            bool result = app->themeManager()->applyTheme(selectedTheme);
            if (!result) {
                auto ctxErr = std::map<std::string, std::string>{{{"selectedTheme", selectedTheme.toStdString()}}};
                LOG_ERROR("SettingsPage", "Failed to apply theme", ctxErr);
            }
        }
    }
}

void SettingsPage::changeStarEmoji()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SettingsPage", "Changing star emoji", ctx);
    QString selectedEmoji = m_emojiComboBox->currentText();
    if (!selectedEmoji.isEmpty()) {
        auto ctxEmoji = std::map<std::string, std::string>{{{"selectedEmoji", selectedEmoji.toStdString()}}};
        LOG_DEBUG("SettingsPage", "Saving star emoji", ctxEmoji);
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "voodz_d1sh0w", "time_tracker");
        settings.setValue("starEmoji", selectedEmoji);
        // You can emit a signal to update star display in all tasks, for example:
        //emit starEmojiChanged(selectedEmoji);
        auto ctxInfo = std::map<std::string, std::string>{{{"selectedEmoji", selectedEmoji.toStdString()}}};
        LOG_INFO("SettingsPage", "Star emoji changed", ctxInfo);
    }
}
