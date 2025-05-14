#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);

    QStringList availableThemes() const;

    bool applyTheme(const QString &themeName);

    QString currentTheme() const { return m_currentTheme; }

    QString baseThemePath() const { return m_baseThemePath; }
    
    // Set the base path where themes are located
    void setBaseThemePath(const QString &path) {
        if (m_baseThemePath != path) {
            m_baseThemePath = path;
            // Ensure the path ends with a separator
            if (!m_baseThemePath.endsWith('/') && !m_baseThemePath.endsWith('\\')) {
                m_baseThemePath += '/';
            }
            
            // Save to settings
            QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "voodz_d1sh0w", "time_tracker");
            settings.setValue("themePath", m_baseThemePath);
            
            emit themePathChanged();
        }
    }

signals:
    void themePathChanged();

private:
    QString m_currentTheme;
    QString m_baseThemePath;
};

#endif // THEMEMANAGER_H
