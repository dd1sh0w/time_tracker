#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "BasePage.h"
#include <QLabel>
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include "../../Application.h"
#include "../../core/ThemeManager.h"

class SettingsPage : public BasePage
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);
    ~SettingsPage();

protected:
    void setupUi() override;
    void changeEvent(QEvent *event) override;

private slots:
    void changeTheme();
    void changeLanguage(int index);
    void browseThemeFolder();
    void updateThemeList();
    void retranslateUi();

private:
    void loadLanguages();
    Application* getApp() const;

    QGridLayout *m_mainLayout;
    QLabel *m_titleLabel;
    
    // Theme settings
    QGroupBox *m_themeGroup;
    QGridLayout *m_themeLayout;
    QLabel *m_themeLabel;
    QLabel *m_themePathLabel;
    QPushButton *m_browseThemeButton;
    QComboBox *m_themeComboBox;
    QPushButton *m_applyThemeButton;

    // Language settings
    QGroupBox *m_languageGroup;
    QGridLayout *m_languageLayout;
    QLabel *m_languageLabel;
    QComboBox *m_languageComboBox;

    ThemeManager *m_themeManager;
};

#endif // SETTINGSPAGE_H
