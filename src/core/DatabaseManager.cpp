#include "DatabaseManager.h"
#include <QDebug>
#include "../logging/logger.h"

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if(m_db.isOpen()){
        m_db.close();
    }
}

bool DatabaseManager::openDB(const QString &host, int port, const QString &dbName,
                       const QString &user, const QString &password)
{
    auto ctx = std::map<std::string, std::string>{{{"host", host.toStdString()}, {"dbName", dbName.toStdString()}, {"user", user.toStdString()}}};
    LOG_INFO("DatabaseManager", "Opening database connection", ctx);
    m_db = QSqlDatabase::addDatabase("QPSQL");
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);
    if(!m_db.open()){
        auto ctxErr = std::map<std::string, std::string>{{{"error", m_db.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Database connection error", ctxErr);
        return false;
    }
    auto ctxOk = std::map<std::string, std::string>{};
    LOG_INFO("DatabaseManager", "Database opened successfully", ctxOk);
    return true;
}

int DatabaseManager::registerUser(const QString &username, const QString &passwordHash, const QString &email)
{
    auto ctx = std::map<std::string, std::string>{{{"username", username.toStdString()}, {"email", email.toStdString()}}};
    LOG_INFO("DatabaseManager", "Registering user", ctx);
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password_hash, email) VALUES (:username, :password_hash, :email) RETURNING id");
    query.bindValue(":username", username);
    query.bindValue(":password_hash", passwordHash);
    query.bindValue(":email", email);
    if(!query.exec()){
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Registration error", ctxErr);
        return -1;
    }
    if(query.next()){
        auto ctxOk = std::map<std::string, std::string>{{{"userId", std::to_string(query.value(0).toInt())}}};
        LOG_INFO("DatabaseManager", "User registered", ctxOk);
        return query.value(0).toInt();
    }
    auto ctxWarn = std::map<std::string, std::string>{};
    LOG_WARNING("DatabaseManager", "Registration query did not return userId", ctxWarn);
    return -1;
}

int DatabaseManager::loginUser(const QString &username, const QString &passwordHash)
{
    auto ctx = std::map<std::string, std::string>{{{"username", username.toStdString()}}};
    LOG_INFO("DatabaseManager", "User login attempt", ctx);
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM users WHERE username = :username AND password_hash = :password_hash");
    query.bindValue(":username", username);
    query.bindValue(":password_hash", passwordHash);
    if(!query.exec()){
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Login error", ctxErr);
        return -1;
    }
    if(query.next()){
        auto ctxOk = std::map<std::string, std::string>{{{"userId", std::to_string(query.value(0).toInt())}}};
        LOG_INFO("DatabaseManager", "User login successful", ctxOk);
        return query.value(0).toInt();
    }
    auto ctxWarn = std::map<std::string, std::string>{};
    LOG_WARNING("DatabaseManager", "Login failed: user not found or password incorrect", ctxWarn);
    return -1;
}

QVariantMap DatabaseManager::getUserProfile(int userId)
{
    auto ctx = std::map<std::string, std::string>{{{"userId", std::to_string(userId)} }};
    LOG_DEBUG("DatabaseManager", "Fetching user profile", ctx);
    QVariantMap profile;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, username, email, created_at FROM users WHERE id = :id");
    query.bindValue(":id", userId);
    if(query.exec() && query.next()){
        profile["id"] = query.value("id");
        profile["username"] = query.value("username");
        profile["email"] = query.value("email");
        profile["created_at"] = query.value("created_at");
        auto ctxOk = std::map<std::string, std::string>{{{"userId", std::to_string(userId)} }};
        LOG_INFO("DatabaseManager", "User profile fetched", ctxOk);
    } else {
        auto ctxWarn = std::map<std::string, std::string>{{{"userId", std::to_string(userId)} }};
        LOG_WARNING("DatabaseManager", "User profile not found", ctxWarn);
    }
    return profile;
}

bool DatabaseManager::updateUserProfile(int userId, const QVariantMap &profile)
{
    auto ctx = std::map<std::string, std::string>{{{"userId", std::to_string(userId)} }};
    LOG_INFO("DatabaseManager", "Updating user profile", ctx);
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET email = :email, updated_at = NOW() WHERE id = :id");
    query.bindValue(":email", profile.value("email"));
    query.bindValue(":id", userId);
    if(!query.exec()){
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Profile update error", ctxErr);
        return false;
    }
    auto ctxOk = std::map<std::string, std::string>{{{"userId", std::to_string(userId)} }};
    LOG_INFO("DatabaseManager", "User profile updated", ctxOk);
    return true;
}

int DatabaseManager::addTask(int userId,
                             const QString &name,
                             const QString &description,
                             const QDateTime &deadline,
                             int plannedCycles,
                             int remainingCycles,
                             const QString &status)
{
    auto ctx = std::map<std::string, std::string>{{{"userId", std::to_string(userId)}, {"name", name.toStdString()} }};
    LOG_INFO("DatabaseManager", "Adding task to DB", ctx);
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO tasks (user_id, name, description, deadline, planned_cycles, remaining_cycles, status) "
                  "VALUES (:user_id, :name, :description, :deadline, :planned_cycles, :remaining_cycles, :status) RETURNING id");
    query.bindValue(":user_id", userId);
    query.bindValue(":name", name);
    query.bindValue(":description", description);
    query.bindValue(":deadline", deadline);
    query.bindValue(":planned_cycles", plannedCycles);
    query.bindValue(":remaining_cycles", remainingCycles);
    query.bindValue(":status", status);
    if(!query.exec()){
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Failed to add task", ctxErr);
        return -1;
    }
    if(query.next()){
        auto ctxOk = std::map<std::string, std::string>{{{"taskId", std::to_string(query.value(0).toInt())} }};
        LOG_INFO("DatabaseManager", "Task added", ctxOk);
        return query.value(0).toInt();
    }
    auto ctxWarn = std::map<std::string, std::string>{};
    LOG_WARNING("DatabaseManager", "Add task query did not return taskId", ctxWarn);
    return -1;
}

QList<QVariantMap> DatabaseManager::getTasks(int userId)
{
    auto ctx = std::map<std::string, std::string>{{{"userId", std::to_string(userId)} }};
    LOG_DEBUG("DatabaseManager", "Fetching tasks for user", ctx);
    QList<QVariantMap> tasks;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM tasks WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    if(query.exec()){
        while(query.next()){
            QVariantMap task;
            for(int i=0; i < query.record().count(); ++i)
                task[query.record().fieldName(i)] = query.value(i);
            tasks.append(task);
        }
        auto ctxOk = std::map<std::string, std::string>{{{"count", std::to_string(tasks.size())} }};
        LOG_INFO("DatabaseManager", "Tasks fetched", ctxOk);
    } else {
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Error fetching tasks", ctxErr);
    }
    return tasks;
}

bool DatabaseManager::updateTask(const QVariantMap &taskData)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", taskData["id"].toString().toStdString()} }};
    LOG_INFO("DatabaseManager", "Updating task in DB", ctx);
    QSqlQuery query(m_db);
    query.prepare("UPDATE tasks SET name = :name, description = :description, deadline = :deadline, "
                  "planned_cycles = :planned_cycles, "
                  "remaining_cycles = :remaining_cycles, "
                  "status = :status "
                  "WHERE id = :id");
    query.bindValue(":id", taskData["id"]);
    query.bindValue(":name", taskData["name"]);
    query.bindValue(":description", taskData["description"]);
    query.bindValue(":deadline", taskData["deadline"]);
    query.bindValue(":planned_cycles", taskData["planned_cycles"]);
    query.bindValue(":remaining_cycles", taskData["remaining_cycles"]);
    query.bindValue(":status", taskData["status"]);
    if (!query.exec()) {
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "ERROR: update task is failed", ctxErr);
        return false;
    }
    auto ctxOk = std::map<std::string, std::string>{{{"taskId", taskData["id"].toString().toStdString()} }};
    LOG_INFO("DatabaseManager", "Task updated", ctxOk);
    emit taskUpdated(taskData);
    return true;
}

bool DatabaseManager::recordPomodoro(const QVariantMap &pomodoroData)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", pomodoroData["task_id"].toString().toStdString()}, {"userId", pomodoroData["user_id"].toString().toStdString()}}};
    LOG_INFO("DatabaseManager", "Recording pomodoro in DB", ctx);
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO task_history (user_id, task_id, completed_cycles, interruptions, executed_at) "
                 "VALUES (:user_id, :task_id, 1, 0, :timestamp) RETURNING user_id");
    query.bindValue(":user_id", pomodoroData["user_id"]);
    query.bindValue(":task_id", pomodoroData["task_id"]);
    query.bindValue(":timestamp", pomodoroData["timestamp"]);
    if (!query.exec()) {
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Failed to record pomodoro", ctxErr);
        return false;
    }
    // Update statistics for the user
    QSqlQuery statsQuery(m_db);
    statsQuery.prepare(
        "UPDATE statistics "
        "SET total_tasks = total_tasks + 1, "
        "    last_updated = NOW() "
        "WHERE user_id = :user_id"
    );
    statsQuery.bindValue(":user_id", pomodoroData["user_id"]);
    if (!statsQuery.exec()) {
        auto ctxErr = std::map<std::string, std::string>{{{"error", statsQuery.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Failed to update statistics", ctxErr);
        return false;
    }
    return true;
}

int DatabaseManager::getCompletedPomodoros(int taskId, int userId) {
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}, {"userId", std::to_string(userId)}}};
    LOG_DEBUG("DatabaseManager", "Getting completed pomodoros from DB", ctx);
    QSqlQuery query(m_db);
    query.prepare("SELECT SUM(completed_cycles) FROM task_history WHERE user_id = :user_id AND task_id = :taskId");
    query.bindValue(":user_id", userId);
    query.bindValue(":taskId", taskId);
    if (!query.exec()) {
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Failed to get completed pomodoros", ctxErr);
        return -1;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QList<QVariantMap> DatabaseManager::getPomodoroStats(int taskId, int userId)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}, {"userId", std::to_string(userId)}}};
    LOG_DEBUG("DatabaseManager", "Fetching pomodoro stats", ctx);
    QList<QVariantMap> stats;
    QSqlQuery query(m_db);
    query.prepare("SELECT executed_at, completed_cycles, interruptions "
                 "FROM task_history "
                 "WHERE user_id = :user_id AND task_id = :taskId "
                 "ORDER BY executed_at DESC");
    query.bindValue(":user_id", userId);
    query.bindValue(":taskId", taskId);
    if (query.exec()) {
        while (query.next()) {
            QVariantMap record;
            record["timestamp"] = query.value("executed_at").toDateTime();
            record["completed_cycles"] = query.value("completed_cycles").toInt();
            record["interruptions"] = query.value("interruptions").toInt();
            stats.append(record);
        }
        auto ctxOk = std::map<std::string, std::string>{{{"count", std::to_string(stats.size())} }};
        LOG_INFO("DatabaseManager", "Pomodoro stats fetched", ctxOk);
    } else {
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Error fetching pomodoro stats", ctxErr);
    }
    return stats;
}

bool DatabaseManager::deleteTask(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)} }};
    LOG_INFO("DatabaseManager", "Deleting task from DB", ctx);
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);
    if (!query.exec()) {
        auto ctxErr = std::map<std::string, std::string>{{{"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Error deleting task", ctxErr);
        return false;
    }
    // Update statistics for the user
    QSqlQuery statsQuery(m_db);
    statsQuery.prepare(
        "UPDATE statistics "
        "SET total_tasks = total_tasks - 1, "
        "pending_tasks = pending_tasks - 1 "
        "WHERE user_id = (SELECT user_id FROM tasks WHERE id = :id)"
    );
    statsQuery.bindValue(":id", taskId);
    if (!statsQuery.exec()) {
        auto ctxErr = std::map<std::string, std::string>{{{"error", statsQuery.lastError().text().toStdString()}}};
        LOG_ERROR("DatabaseManager", "Error updating statistics after delete", ctxErr);
    }
    auto ctxOk = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)} }};
    LOG_INFO("DatabaseManager", "Task deleted from DB", ctxOk);
    return true;
}
