#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QList>
#include "Task.h"

class TaskManager : public QObject
{
    Q_OBJECT
public:
    explicit TaskManager(int userId, QObject *parent = nullptr);
    QList<QVariantMap> getAllTasks() const;
    int createTask(const QString &name, const QDate &deadline = QDate::currentDate());
    bool startTask(int taskId);
    bool completeTask(int taskId);
    int getTotalCycles(int taskId) const;
    bool deleteTask(int taskId);
    bool recordPomodoro(int taskId);
    int getCompletedPomodoros(int taskId);
    QList<QVariantMap> getPomodoroStats(int taskId);
    QVariantMap getTaskData(int taskId) const;
    int getActiveTask() const;
    bool setActiveTask(int taskId);
    bool pauseTask(int taskId);
    bool resumeTask(int taskId);
    bool updateTask(int taskId, const QVariantMap &data);

signals:
    void taskCreated(int taskId, const QVariantMap &taskData);
    void taskUpdated(int taskId, const QVariantMap &taskData);
    void taskDeleted(int taskId, const QVariantMap &taskData);
    void taskCompleted(int taskId, const QVariantMap &taskData);
    void taskStarted(int taskId, const QVariantMap &taskData);
    void pomodoroRecorded(int taskId, const QVariantMap &stats);
    void error(const QString &message, int taskId);
    void activeTaskChanged(int taskId);
    void totalCyclesChanged(int taskId, int totalCycles);

private:
    int m_userId;
    TaskStatus statusFromString(const QString &statusStr);
};

#endif // TASKMANAGER_H
