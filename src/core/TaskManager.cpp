#include "TaskManager.h"
#include "DatabaseManager.h"
#include <QDateTime>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include "../logging/logger.h"

TaskManager::TaskManager(int userId, QObject *parent)
    : QObject(parent), m_userId(userId)
{
    auto ctx = std::map<std::string, std::string>{{}};
    LOG_INFO("TaskManager", "Initializing TaskManager", ctx);
    // Initialize active task to -1 (no active task)
    QSqlQuery query;
    query.prepare("INSERT INTO active_task (user_id, task_id) VALUES (?, ?) ON CONFLICT (user_id) DO UPDATE SET task_id = EXCLUDED.task_id");
    //query.prepare("INSERT OR REPLACE INTO active_task (user_id, task_id) VALUES (?, ?)");
    query.addBindValue(m_userId);
    query.addBindValue(-1);
    if (!query.exec()) {
        auto ctx = std::map<std::string, std::string>{{ {"error", query.lastError().text().toStdString() } }};
        LOG_ERROR("TaskManager", "Error initializing active task", ctx);
    }
}

QList<QVariantMap> TaskManager::getAllTasks() const
{
    auto ctx = std::map<std::string, std::string>{{}};
    LOG_DEBUG("TaskManager", "Fetching all tasks", ctx);
    return DatabaseManager::instance().getTasks(m_userId);
}

int TaskManager::createTask(const QString &name, const QDate &deadline)
{
    auto ctx = std::map<std::string, std::string>{{ {"name", name.toStdString()} }};
    LOG_INFO("TaskManager", "Creating task", ctx);
    QVariantMap taskData;
    taskData["name"] = name;
    taskData["description"] = tr("New task description...");
    taskData["deadline"] = QDateTime(deadline, QTime(23, 59, 59));
    taskData["planned_cycles"] = 1;
    taskData["remaining_cycles"] = 1;
    taskData["status"] = "Active"; // Using Active status to match database constraint
    taskData["user_id"] = m_userId;
    int taskId = DatabaseManager::instance().addTask(
        taskData
    );

    if (taskId != -1) {
        emit taskCreated(taskId, taskData);
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
        LOG_INFO("TaskManager", "Task created successfully", ctx);
    } else {
        auto ctx = std::map<std::string, std::string>{{ {"name", name.toStdString()} }};
        LOG_ERROR("TaskManager", "Failed to create task", ctx);
        emit error(tr("Failed to create task"), taskId);
        return -1;
    }
    return taskId;
}

bool TaskManager::startTask(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_INFO("TaskManager", "Starting task", ctx);
    
    QVariantMap taskData;
    taskData["id"] = taskId;
    taskData["status"] = TaskUtil::toString(TaskStatus::InProgress);
    taskData["user_id"] = m_userId;
    
    try {
        if (DatabaseManager::instance().updateTask(taskData)) {
            emit taskUpdated(taskId, taskData);
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("TaskManager", "Exception starting task", ctx);
        emit error(tr("Failed to start task: ") + QString::fromStdString(e.what()), taskId);
        return false;
    }
}

bool TaskManager::updateTask(int taskId, const QVariantMap &data)
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_INFO("TaskManager", "Updating task", ctx);
    // Проверяем наличие обязательных полей
    if (!data.contains("name") || !data.contains("status")) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
        LOG_WARNING("TaskManager", "Missing required fields for task update", ctx);
        emit error(tr("Missing required fields for task update"), taskId);
        return false;
    }

    // Получаем текущие данные задачи
    QVariantMap currentTask = getTaskData(taskId);
    if (currentTask.isEmpty()) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
        LOG_WARNING("TaskManager", "Task not found for update", ctx);
        emit error(tr("Task not found"), taskId);
        return false;
    }

    // Подготавливаем данные для обновления
    QVariantMap updateData;
    updateData["id"] = taskId;
    updateData["name"] = data.contains("name") ? data["name"] : currentTask["name"];
    updateData["description"] = data.contains("description") ? data["description"] : currentTask["description"];
    updateData["deadline"] = data.contains("deadline") ? data["deadline"] : currentTask["deadline"];
    updateData["planned_cycles"] = data.contains("planned_cycles") ? data["planned_cycles"] : currentTask["planned_cycles"];
    updateData["remaining_cycles"] = data.contains("remaining_cycles") ? data["remaining_cycles"] : currentTask["remaining_cycles"];
    updateData["status"] = TaskUtil::toString(statusFromString(data["status"].toString())); // Convert TaskStatus to string before storing in QVariantMap

    try {
        if (DatabaseManager::instance().updateTask(updateData)) {
            auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
            LOG_INFO("TaskManager", "Task updated successfully", ctx);
            emit taskUpdated(taskId, updateData);
            return true;
        }
    } catch (const std::exception& e) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)}, {"error", e.what()} }};
        LOG_ERROR("TaskManager", "Exception updating task", ctx);
        emit error(tr("Failed to update task: ") + QString::fromStdString(e.what()), taskId);
        return false;
    }
    return false;
}

int TaskManager::getTotalCycles(int taskId) const
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_DEBUG("TaskManager", "Getting total cycles", ctx);
    
    QVariantMap taskData = getTaskData(taskId);
    if (taskData.isEmpty()) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
        LOG_WARNING("TaskManager", "Task not found", ctx);
        return 0;
    }
    
    return taskData.value("planned_cycles", 0).toInt();
}

bool TaskManager::deleteTask(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_INFO("TaskManager", "Deleting task", ctx);
    QVariantMap currentTask = getTaskData(taskId);
    if (currentTask.isEmpty()) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
        LOG_WARNING("TaskManager", "Task not found for deletion", ctx);
        emit error(tr("Task not found"), taskId);
        return false;
    }

    try {
        // Check if the task being deleted is the active task
        int activeTaskId = getActiveTask();
        if (activeTaskId == taskId) {
            LOG_INFO("TaskManager", "Clearing active task before deletion", ctx);
            // Set active task to -1 (no active task)
            QSqlQuery query;
            query.prepare("UPDATE active_task SET task_id = -1 WHERE user_id = ? AND task_id = ?");
            query.addBindValue(m_userId);
            query.addBindValue(taskId);
            if (!query.exec()) {
                auto ctxErr = std::map<std::string, std::string>{{ {"error", query.lastError().text().toStdString()} }};
                LOG_ERROR("TaskManager", "Failed to clear active task before deletion", ctxErr);
                // Continue with deletion even if clearing active task fails
            }
        }

        if (DatabaseManager::instance().deleteTask(taskId)) {
            auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
            LOG_INFO("TaskManager", "Task deleted successfully", ctx);
            emit taskDeleted(taskId, currentTask);
            return true;
        } else {
            auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
            LOG_ERROR("TaskManager", "Failed to delete task from database", ctx);
            emit error(tr("Failed to delete task from database"), taskId);
            return false;
        }
    } catch (const std::exception& e) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)}, {"exception", e.what()} }};
        LOG_ERROR("TaskManager", "Exception during task deletion", ctx);
        emit error(tr("Failed to delete task: ") + QString::fromStdString(e.what()), taskId);
        return false;
    }
}

bool TaskManager::completeTask(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_INFO("TaskManager", "Completing task", ctx);
    QVariantMap taskData;
    taskData["id"] = taskId;
    taskData["user_id"] = m_userId;
    taskData["status"] = TaskUtil::toString(TaskStatus::Completed);
    taskData["completed_at"] = QDateTime::currentDateTime();

    try {
        if (DatabaseManager::instance().updateTask(taskData)) {
            auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
            LOG_INFO("TaskManager", "Task completed successfully", ctx);
            emit taskCompleted(taskId, taskData);
            return true;
        } else {
            auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
            LOG_ERROR("TaskManager", "Failed to complete task", ctx);
            emit error(tr("Failed to complete task"), taskId);
            return false;
        }
    } catch (const std::exception& e) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)}, {"exception", e.what()} }};
        LOG_ERROR("TaskManager", "Exception during task completion", ctx);
        emit error(tr("Failed to complete task"), taskId);
        return false;
    }
}

bool TaskManager::recordPomodoro(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_INFO("TaskManager", "Recording pomodoro", ctx);
    QVariantMap pomodoroData;
    pomodoroData["task_id"] = taskId;
    pomodoroData["user_id"] = m_userId;
    pomodoroData["timestamp"] = QDateTime::currentDateTime();
    
    try {
        if (DatabaseManager::instance().recordPomodoro(pomodoroData)) {
            // Update task's remaining cycles
            QVariantMap taskData;
            taskData["id"] = taskId;
            taskData["user_id"] = m_userId;
            
            auto statsList = DatabaseManager::instance().getPomodoroStats(taskId, m_userId);
            QVariantMap stats = statsList.isEmpty() ? QVariantMap() : statsList.first();
            emit pomodoroRecorded(taskId, stats);
            return true;
        } else {
            LOG_ERROR("TaskManager", "Failed to record pomodoro: DB error", ctx);
            emit error(tr("Failed to record pomodoro: DB error"), taskId);
            return false;
        }
    } catch (const std::exception& e) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)}, {"exception", e.what()} }};
        LOG_ERROR("TaskManager", "Exception during pomodoro recording", ctx);
        emit error(tr("Failed to record pomodoro: TM error"), taskId);
        return false;
    }
}

int TaskManager::getCompletedPomodoros(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_DEBUG("TaskManager", "Getting completed pomodoros", ctx);
    try {
        return DatabaseManager::instance().getCompletedPomodoros(taskId, m_userId);
    } catch (const std::exception& e) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)}, {"exception", e.what()} }};
        LOG_ERROR("TaskManager", "Exception getting completed pomodoros", ctx);
        emit error(tr("Failed to get completed pomodoros"), taskId);
        return -1;
    }
}

QList<QVariantMap> TaskManager::getPomodoroStats(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_DEBUG("TaskManager", "Getting pomodoro stats", ctx);
    try {
        return DatabaseManager::instance().getPomodoroStats(taskId, m_userId);
    } catch (const std::exception& e) {
        auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)}, {"exception", e.what()} }};
        LOG_ERROR("TaskManager", "Exception getting pomodoro stats", ctx);
        emit error(tr("Failed to get pomodoro stats"), taskId);
        return {};
    }
}

QVariantMap TaskManager::getTaskData(int taskId) const
{
    auto ctx = std::map<std::string, std::string>{{ {"taskId", std::to_string(taskId)} }};
    LOG_DEBUG("TaskManager", "Fetching task data", ctx);
    QList<QVariantMap> tasks = DatabaseManager::instance().getTasks(m_userId);
    for (const QVariantMap &task : tasks) {
        if (task["id"].toInt() == taskId) {
            return task;
        }
    }
    return QVariantMap();
}

// Using TaskUtil for status conversion
TaskStatus TaskManager::statusFromString(const QString &statusStr)
{
    return TaskUtil::fromString(statusStr);
}

int TaskManager::getActiveTask() const
{
    auto ctx = std::map<std::string, std::string>{{}};
    LOG_DEBUG("TaskManager", "Getting active task", ctx);
    QSqlQuery query;
    query.prepare("SELECT task_id FROM active_task WHERE user_id = ?");
    query.addBindValue(m_userId);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1; // No active task
}

bool TaskManager::setActiveTask(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}}};
    LOG_INFO("TaskManager", "Setting active task", ctx);
    
    try {
        QSqlQuery query;
        query.prepare("INSERT INTO active_task (user_id, task_id) VALUES (?, ?) ON CONFLICT (user_id) DO UPDATE SET task_id = EXCLUDED.task_id");
        query.addBindValue(m_userId);
        query.addBindValue(taskId);
        
        if (query.exec()) {
            auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}}};
            LOG_INFO("TaskManager", "Active task set", ctx);
            
            // Получаем данные задачи для эмиссии сигнала
            QVariantMap taskData = getTaskData(taskId);
            if (taskData.isEmpty()) {
                auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}}};
                LOG_ERROR("TaskManager", "Failed to get task data", ctx);
                emit error(tr("Failed to get task data"), taskId);
                return false;
            }
            emit taskStarted(taskId, taskData);
            emit activeTaskChanged(taskId);
            return true;
        }
        
        auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}, {"error", query.lastError().text().toStdString()}}};
        LOG_ERROR("TaskManager", "Error setting active task", ctx);
        emit error(tr("Failed to set active task: DB error"), taskId);
        return false;
    } catch (const std::exception& e) {
        auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}, {"exception", e.what()}}};
        LOG_ERROR("TaskManager", "Exception setting active task", ctx);
        emit error(tr("Failed to set active task: TM error"), taskId);
        return false;
    }
}
