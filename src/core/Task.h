#ifndef TASK_H
#define TASK_H

#include <QWidget>
#include <QDate>
#include <QVariantMap>

enum class TaskStatus {
    Active,       // Ready to be worked on
    InProgress,   // Currently being worked on
    OnHold,       // Temporarily paused
    Completed,    // Successfully finished
    Review,       // Needs review before finalization
    Overdue,      // Deadline passed but not completed
    Upcoming,     // Scheduled for future
    Cancelled     // Cancelled and won't be completed
};

class QPushButton;
class QLabel;
class QHBoxLayout;

// Task status conversion utility class
class TaskUtil {
public:
    static QString toString(TaskStatus status);
    static TaskStatus fromString(const QString &statusStr);
};

class Task : public QWidget
{
    Q_OBJECT

public:
    explicit Task(int id = 0, QWidget *parent = nullptr);
    ~Task() override;

    int id() const { return m_id; }
    QString taskName() const { return m_taskName; }
    QString description() const { return m_description; }
    QDate deadline() const { return m_deadline; }
    int plannedCycles() const { return m_plannedCycles; }
    int remainingCycles() const { return m_remainingCycles; }
    TaskStatus status() const { return m_status; }

    void setTaskName(const QString &name);
    void setDescription(const QString &description);
    void setDeadline(const QDate &deadline);
    void setPlannedCycles(int cycles);
    void setRemainingCycles(int cycles);
    void setStatus(TaskStatus status);
    void setId(int id);
    QString statusText() const;

    void updateDisplay();
    void updateTask(const QString &name, const QString &description, const QDate &deadline, int plannedCycles);
    void updateCycles(int cycles);

signals:
    void taskDeleted(int taskId);
    void taskUpdated(int taskId, const QVariantMap &data);
    void taskCompleted(int taskId, const QVariantMap &taskData);
    void startTimer();
    void remainingCyclesChanged(int cycles);
    void taskStarted(int taskId, const QVariantMap &taskData);

private:
    int m_id;
    QString m_taskName;
    QString m_description;
    QDate m_deadline;
    int m_plannedCycles;
    int m_remainingCycles;
    TaskStatus m_status;
    QHBoxLayout *m_layout;
    QPushButton *tt_startButton;
    QPushButton *tt_nameButton;
    QLabel *tt_statusLabel;
    QLabel *tt_deadlineLabel;
    QLabel *tt_cyclesLabel;
    QLabel *tt_descriptionLabel;

    void setupUi();
    void openTaskSettings();
};

#endif // TASK_H
