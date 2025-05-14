#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QTranslator>
#include "core/ThemeManager.h"

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application();

    ThemeManager *themeManager() const { return m_themeManager; }
    
    // Language management
    QStringList availableLanguages() const;
    QString currentLanguage() const;
    bool setLanguage(const QString &language);

signals:
    void languageChanged();

private:
    bool setupTranslations();
    bool loadLanguage(const QString &language);

    ThemeManager *m_themeManager;
    QTranslator *m_translator;
    QString m_currentLanguage;
};

#endif // APPLICATION_H
