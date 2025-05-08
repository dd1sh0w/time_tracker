#ifndef TASKINFO_H
#define TASKINFO_H

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QMap>

// Structure to hold task information
struct TaskInfo {
    int taskId;
    QString taskName;
    int cycles;
    QString status;
    QString priority;
    QString description;
    QDate deadline;
    QDateTime completedAt;
    QString details;
};


#endif // TASKINFO_H