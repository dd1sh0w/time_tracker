#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QtSql>
#include <QVariantMap>


class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance() {
        static DatabaseManager _instance;
        return _instance;
    }

    bool openDB(const QString &host, int port, const QString &dbName,
                const QString &user, const QString &password);

    // Register a new user
    // Returns the new user's ID (>= 1) or -1 if there's an error
    int registerUser(const QString &username, const QString &passwordHash, const QString &email);

    // Login function. Returns user ID if successful, -1 if failed.
    int loginUser(const QString &username, const QString &passwordHash);

    // Get user profile
    QVariantMap getUserProfile(int userId);

    // Update user profile (e.g., email)
    bool updateUserProfile(int userId, const QVariantMap &profile);

    int addTask(const QVariantMap &taskData);
    QList<QVariantMap> getTasks(int userId);
    bool updateTask(const QVariantMap &taskData);
    bool deleteTask(int taskId);
    bool recordPomodoro(const QVariantMap &pomodoroData);
    int getCompletedPomodoros(int taskId, int userId); 
    QList<QVariantMap> getPomodoroStats(int taskId, int userId); 

signals:
    void taskUpdated(const QVariantMap &taskData);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    QSqlDatabase m_db;
};

#endif // DatabaseManager_H
