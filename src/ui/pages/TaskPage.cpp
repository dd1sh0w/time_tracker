#include "TaskPage.h"
#include "../widgets/PomodoroTimer.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include "../components/DayCard.h"

TaskPage::TaskPage(int userId, QWidget *parent, TaskManager* taskManager)
    : BasePage(parent), m_userId(userId), m_taskManager(taskManager)
{
    setupUi();
    setupConnections();
}

void TaskPage::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_pomodoroTimer = new PomodoroTimer(this);
    layout->addWidget(m_pomodoroTimer);
    //layout->addStretch();
    setLayout(layout);
}

void TaskPage::setupConnections()
{
    // Timer connections
    connect(m_pomodoroTimer, &PomodoroTimer::phaseCompleted, this, &TaskPage::onPomodoroPhaseCompleted);
}

void TaskPage::startTask(const TaskInfo &task)
{
    qDebug() << "TaskPage: exec startTask";
    if (m_taskManager && m_pomodoroTimer) {
        qDebug() << "TaskPage: exec startTask - task id:" << task.taskId;
        bool setActiveSuccess = m_taskManager->setActiveTask(task.taskId);
        if (setActiveSuccess) {
            // Wait for the activeTaskChanged signal to ensure the task is properly set
            connect(m_taskManager, &TaskManager::activeTaskChanged, this, [this, task, setActiveSuccess]() {
                if (setActiveSuccess && m_taskManager->getActiveTask() == task.taskId) {
                    m_pomodoroTimer->setActiveTaskName(task.taskName);
                    m_pomodoroTimer->startTimer();
                }
            }, Qt::QueuedConnection);
        } else {
            showError(tr("Failed to start task"));
        }
    }
}

void TaskPage::onActiveTaskChanged(int taskId)
{
    qDebug() << "TaskPage: Active task changed to task ID:" << taskId;
    if (m_taskManager && m_pomodoroTimer) {
        QVariantMap taskData = m_taskManager->getTaskData(taskId);
        if (!taskData.isEmpty()) {
            m_pomodoroTimer->setActiveTaskName(taskData.value("name").toString());
            m_pomodoroTimer->startTimer();
        }
    }
}

void TaskPage::onTaskStarted(int taskId, const QVariantMap &taskData)
{
    TaskInfo task;
    task.taskId = taskId;
    task.taskName = taskData.value("name").toString();
    task.cycles = taskData.value("planned_cycles").toInt();
    task.status = taskData.value("status").toString();
    task.priority = taskData.value("priority").toString();
    task.description = taskData.value("description").toString();
    task.deadline = taskData.value("deadline").toDate();
    task.completedAt = taskData.value("completed_at").toDateTime();
    task.details = taskData.value("details").toString();
    
    handleStartTaskClicked(task);
}

void TaskPage::onPomodoroPhaseCompleted(PomodoroPhase phase)
{
    qDebug() << "TaskPage: exec onPomodoroPhaseCompleted";
    if (phase == PomodoroPhase::Work) {
        // Record completed pomodoro for the active task
        m_taskManager->recordPomodoro(m_taskManager->getActiveTask());
    }
}

void TaskPage::showError(const QString &message)
{
    qDebug() << "TaskPage: exec showError";
    QMessageBox::critical(this, tr("Error"), message);
}

void TaskPage::handleStartTaskClicked(const TaskInfo &task)
{
    qDebug() << "TaskPage: exec handleStartTaskClicked";
    startTask(task);
}
