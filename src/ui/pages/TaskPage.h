#ifndef TASKPAGE_H
#define TASKPAGE_H

#include "BasePage.h"
#include "../components/TaskInfo.h"
#include "../../core/TaskManager.h"
#include "../widgets/PomodoroTimer.h"

class QVBoxLayout;
class QPushButton;
class PomodoroTimer;

class TaskPage : public BasePage
{
    Q_OBJECT
public:
    explicit TaskPage(int userId, QWidget *parent = nullptr, TaskManager* taskManager = nullptr);

public slots:
    void startTask(const TaskInfo &task);
    void handleStartTaskClicked(const TaskInfo &task);
    void onTaskStarted(int taskId, const QVariantMap &taskData);
    void onActiveTaskChanged(int taskId);
    
private slots:
    void onPomodoroPhaseCompleted(PomodoroPhase phase);
    void showError(const QString &message);

private:
    void setupUi();
    void setupConnections();

    int m_userId;
    TaskManager *m_taskManager;
    PomodoroTimer *m_pomodoroTimer;
};

#endif // TASKPAGE_H
