#include "TaskPage.h"
#include "../widgets/PomodoroTimer.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include "../components/DayCard.h"
#include "../../logging/logger.h"

TaskPage::TaskPage(int userId, QWidget *parent, TaskManager* taskManager)
    : BasePage(parent), m_userId(userId), m_taskManager(taskManager), m_pomodoroTimer(nullptr)
{
    setupUi();
    setupConnections();
}

void TaskPage::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_pomodoroTimer = new PomodoroTimer(this, m_taskManager);
    layout->addWidget(m_pomodoroTimer);
    //layout->addStretch();
    setLayout(layout);
}

void TaskPage::setupConnections()
{
    // Timer connections
    connect(m_pomodoroTimer, &PomodoroTimer::phaseCompleted, this, &TaskPage::onPomodoroPhaseCompleted);
    
    // TaskManager connections
    if (m_taskManager) {
        connect(m_taskManager, &TaskManager::taskStarted, this, &TaskPage::onTaskStarted);
        connect(m_taskManager, &TaskManager::activeTaskChanged, this, &TaskPage::onActiveTaskChanged);
    }
}

void TaskPage::startTask(const TaskInfo &task)
{
    qDebug() << "TaskPage: exec startTask";
    if (!m_taskManager || !m_pomodoroTimer) {
        qDebug() << "TaskPage: task manager or pomodoro timer is null";
        return;
    }
    
    qDebug() << "TaskPage: exec startTask - task id:" << task.taskId;
    bool setActiveSuccess = m_taskManager->setActiveTask(task.taskId);
    if (setActiveSuccess) {
        // Обновляем UI сразу, так как setActiveTask уже эмиттирует taskStarted
        m_pomodoroTimer->setActiveTaskName(task.taskName);
    } else {
        showError(tr("Failed to start task"));
    }
}

void TaskPage::onActiveTaskChanged(int taskId)
{
    qDebug() << "TaskPage: Active task changed to task ID:" << taskId;
    if (m_taskManager && m_pomodoroTimer) {
        QVariantMap taskData = m_taskManager->getTaskData(taskId);
        if (!taskData.isEmpty()) {
            m_pomodoroTimer->setActiveTaskName(taskData.value("name").toString());
            // Не запускаем таймер здесь, так как он уже должен быть запущен в onTaskStarted
        }
    }
}

void TaskPage::onTaskStarted(int taskId, const QVariantMap &taskData)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}}};
    LOG_INFO("TaskPage", "Task started", ctx);
    
    if (!m_pomodoroTimer) {
        auto ctxWarn = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}}};
        LOG_WARNING("TaskPage", "PomodoroTimer is null in onTaskStarted", ctxWarn);
        return;
    }
    
    m_pomodoroTimer->setActiveTaskName(taskData.value("name").toString());
    m_pomodoroTimer->startTimer();
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
