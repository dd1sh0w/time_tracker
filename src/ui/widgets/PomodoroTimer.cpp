#include "PomodoroTimer.h"
#include <QPainter>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QPaintEvent>
#include <QDebug>
#include "../../logging/logger.h"
#include "../MainWindow.h"
#include "../../core/TaskManager.h"
#include <QPixmap>
#include <QFileInfo>

PomodoroTimer::PomodoroTimer(QWidget *parent, TaskManager *taskManager)
    : QWidget(parent)
    , m_currentPhase(PomodoroPhase::Work)
    , m_isRunning(false)
    , m_completedPomodoros(0)
    , m_activeTaskName("")
    , m_currentPhaseDuration(WORK_DURATION)
    , m_taskRemainingCycles(0)
    , m_taskManager(taskManager)
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("PomodoroTimer", "Initializing PomodoroTimer", ctx);
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("pomodoroTimer");
    setProperty("phase", "Work");
    style()->unpolish(this);
    style()->polish(this);
    
    // Initialize UI components
    m_timeLabel = new QLabel(this);
    m_phaseLabel = new QLabel(this);
    m_cycleLabel = new QLabel(this);
    m_activeTaskNameLabel = new QLabel(this);
    m_startPauseButton = new QPushButton(this);
    m_resetButton = new QPushButton(this);
    m_skipButton = new QPushButton(this);
    
    // Initialize timer
    m_timer.setInterval(1000); // 1 second
    
    setupUi();
    resetTimer();
    updateButtonStates();
    connect(&m_timer, &QTimer::timeout, this, &PomodoroTimer::onTimerTick);
    
    // Initialize labels 
    updateTimerDisplay();
    m_phaseLabel->setText(tr("Work"));
    m_cycleLabel->setText(tr("Cycles: 0/0"));
    m_activeTaskNameLabel->setText(tr("No task selected"));
}

void PomodoroTimer::startTimer()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("PomodoroTimer", "Starting timer", ctx);
    
    if (m_isRunning) {
        auto ctxWarn = std::map<std::string, std::string>{};
        LOG_WARNING("PomodoroTimer", "Attempted to start timer but already running", ctxWarn);
        return;
    }
    
    // Only start if we have an active task
    if (m_activeTaskName.isEmpty()) {
        auto ctxWarn = std::map<std::string, std::string>{};
        LOG_WARNING("PomodoroTimer", "Cannot start timer without active task", ctxWarn);
        return;
    }
    
    m_timer.stop();
    m_isRunning = true;
    m_remainingSeconds = m_currentPhaseDuration;
    updateTimerDisplay();
    updateCyclesDisplay();  // Update cycles display when starting
    m_startPauseButton->setText(tr("Pause"));
    m_timer.start(1000);
}

void PomodoroTimer::pauseTimer()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("PomodoroTimer", "Pausing timer", ctx);
    if (!m_isRunning) {
        auto ctxWarn = std::map<std::string, std::string>{};
        LOG_WARNING("PomodoroTimer", "Attempted to pause timer but not running", ctxWarn);
        return;
    }
    // Stop the timer first
    m_timer.stop();
    // Process any pending timer events to ensure no more timeouts are processed
    QCoreApplication::processEvents();
    // Then update the running state
    m_isRunning = false;
    m_startPauseButton->setText(tr("Resume"));
    // Force update the display to show the correct time
    updateTimerDisplay();
    update();
}

void PomodoroTimer::resetTimer()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("PomodoroTimer", "Resetting timer", ctx);
    m_timer.stop();
    m_isRunning = false;
    m_currentPhase = PomodoroPhase::Work;
    setProperty("phase", "Work");
    m_currentPhaseDuration = WORK_DURATION;
    m_remainingSeconds = m_currentPhaseDuration;
    updateTimerDisplay();
    updateButtonStates();
    update();
}

void PomodoroTimer::skipPhase()
{
    auto ctx = std::map<std::string, std::string>{{{"phase", std::to_string(static_cast<int>(m_currentPhase))}}};
    LOG_INFO("PomodoroTimer", "Skipping phase", ctx);
    m_timer.stop();
    m_isRunning = false;
    
    // Show notification for the ending phase
    switch (m_currentPhase) {
        case PomodoroPhase::Work:
            showNotification(tr("Work Phase Complete"), 
                          tr("Great job! Time for a break."));
            break;
        case PomodoroPhase::ShortBreak:
            showNotification(tr("Short Break Over"), 
                          tr("Break time is over. Let's get back to work!"));
            break;
        case PomodoroPhase::LongBreak:
            showNotification(tr("Long Break Over"), 
                          tr("Break time is over. Ready for another work session?"));
            break;
    }
    
    if (m_currentPhase == PomodoroPhase::Work && !m_activeTaskName.isEmpty()) {
        m_completedPomodoros++;
        auto ctx2 = std::map<std::string, std::string>{{{"completedPomodoros", std::to_string(m_completedPomodoros)}}};
        LOG_INFO("PomodoroTimer", "Work phase skipped, incrementing pomodoros", ctx2);
        
        if (!m_taskManager) {
            auto ctxWarn = std::map<std::string, std::string>{{{"phase", std::to_string(static_cast<int>(m_currentPhase))}}};
            LOG_ERROR("PomodoroTimer", "TaskManager is not available", ctxWarn);
            return;
        }
        
        int activeId = m_taskManager->getActiveTask();
        auto ctxActiveId = std::map<std::string, std::string>{{"activeId", std::to_string(activeId)}};
        LOG_DEBUG("PomodoroTimer", "Active task ID", ctxActiveId);
        
        if (activeId > 0) {
            auto ctxTaskData = std::map<std::string, std::string>{{"taskId", std::to_string(activeId)}};
            LOG_DEBUG("PomodoroTimer", "Getting task data", ctxTaskData);
            QVariantMap taskData = m_taskManager->getTaskData(activeId);
            if (taskData.isEmpty()) {
                LOG_ERROR("PomodoroTimer", "Failed to get task data", ctxTaskData);
                return;
            }
            
            int remainingCycles = taskData.value("remaining_cycles", 0).toInt();
            if (remainingCycles > 0) {
                remainingCycles--;
                taskData["remaining_cycles"] = remainingCycles;
                
                if (!m_taskManager->updateTask(activeId, taskData)) {
                    LOG_ERROR("PomodoroTimer", "Failed to update task cycles", ctxTaskData);
                    return;
                }
                
                // Refresh task data to ensure we have the latest values
                taskData = m_taskManager->getTaskData(activeId);
                m_taskRemainingCycles = taskData.value("remaining_cycles", 0).toInt();
                updateCyclesDisplay();
                
                // If all cycles are completed, reset active task
                if (remainingCycles == 0) {
                    auto ctx4 = std::map<std::string, std::string>{{{"taskId", std::to_string(activeId)}}};
                    LOG_INFO("PomodoroTimer", "All cycles completed, resetting active task", ctx4);
                    m_taskManager->setActiveTask(-1); // Reset active task
                    m_activeTaskName.clear();
                    m_taskRemainingCycles = 0;
                    updateCyclesDisplay();
                    emit activeTaskChanged(QString());
                }
                
                auto ctxUpdate = std::map<std::string, std::string>{
                    {"taskId", std::to_string(activeId)}, 
                    {"remainingCycles", std::to_string(remainingCycles)}
                };
                LOG_DEBUG("PomodoroTimer", "Updated task cycles", ctxUpdate);
            } else {
                auto ctxNoCycles = std::map<std::string, std::string>{{{"taskId", std::to_string(activeId)}}};
                LOG_WARNING("PomodoroTimer", "No remaining cycles to decrement", ctxNoCycles);
            }
        } else {
            auto ctxNoActiveTask = std::map<std::string, std::string>{};
            LOG_WARNING("PomodoroTimer", "No active task found", ctxNoActiveTask);
        }
        
        emit phaseCompleted(m_currentPhase);
        if (m_completedPomodoros % POMODOROS_BEFORE_LONG_BREAK == 0) {
            auto ctx3 = std::map<std::string, std::string>{};
            LOG_INFO("PomodoroTimer", "Cycle completed (skip)", ctx3);
            emit cycleCompleted();
        }
    }
    startNextPhase();
    updateCyclesDisplay();  // Update cycles display after skipping
}

void PomodoroTimer::showNotification(const QString &title, const QString &message) {
    // Check if system tray is available
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QSystemTrayIcon trayIcon;
        
        // Set the application icon as the tray icon
        QString iconPath = QCoreApplication::applicationDirPath() + "/../../src/themes/program_icon.ico";
        QFileInfo iconFile(iconPath);
        
        if (iconFile.exists()) {
            trayIcon.setIcon(QIcon(iconPath));
        } else {
            // Fallback to system icon if our icon is not found
            trayIcon.setIcon(QIcon::fromTheme("dialog-information"));
        }
        
        trayIcon.show();
        trayIcon.showMessage(title, message, QSystemTrayIcon::Information, 5000);
    }
}

void PomodoroTimer::startNextPhase()
{
    // Show notification for the ending phase
    switch (m_currentPhase) {
        case PomodoroPhase::Work:
            showNotification(tr("Work Phase Complete"), 
                          tr("Great job! Time for a break."));
            break;
        case PomodoroPhase::ShortBreak:
            showNotification(tr("Short Break Over"), 
                          tr("Break time is over. Let's get back to work!"));
            break;
        case PomodoroPhase::LongBreak:
            showNotification(tr("Long Break Over"), 
                          tr("Break time is over. Ready for more focused work?"));
            break;
    }

    auto ctx = std::map<std::string, std::string>{{{"prevPhase", std::to_string(static_cast<int>(m_currentPhase))}, {"completedPomodoros", std::to_string(m_completedPomodoros)}}};
    LOG_INFO("PomodoroTimer", "Starting next phase", ctx);
    
    PomodoroPhase oldPhase = m_currentPhase;
    
    switch (m_currentPhase) {
        case PomodoroPhase::Work:
            m_currentPhase = (m_completedPomodoros % POMODOROS_BEFORE_LONG_BREAK == 0) 
                ? PomodoroPhase::LongBreak : PomodoroPhase::ShortBreak;
            setProperty("phase", toString(m_currentPhase));
            m_currentPhaseDuration = (m_currentPhase == PomodoroPhase::LongBreak) ? LONG_BREAK_DURATION : SHORT_BREAK_DURATION;
            break;
        case PomodoroPhase::ShortBreak:
        case PomodoroPhase::LongBreak:
            m_currentPhase = PomodoroPhase::Work;
            m_currentPhaseDuration = WORK_DURATION;
            break;
    }
    m_remainingSeconds = m_currentPhaseDuration;
    setProperty("phase", toString(m_currentPhase));
    style()->unpolish(this);
    style()->polish(this);
    updateTimerDisplay();
    updateButtonStates();
    update();
    // Автоматически запускаем таймер для следующей фазы
    if (m_taskRemainingCycles > 0 || m_currentPhase != PomodoroPhase::Work) {
        // Если это рабочая фаза и циклы есть, либо это перерыв
        m_isRunning = false; // Сбросим, чтобы startTimer сработал
        startTimer();
    } else {
        // Если циклы закончились, не запускаем таймер
        m_isRunning = false;
        m_startPauseButton->setText(tr("Start"));
    }
}

void PomodoroTimer::onTimerTick()
{
    // Double check if we should be running
    if (!m_isRunning) {
        m_timer.stop(); // Ensure timer is stopped if we somehow got here while not running
        return;
    }
    
    if (m_remainingSeconds > 0) {
        if (m_isRunning) {
            m_remainingSeconds--;
            updateTimerDisplay();
            update();
        }
        
        // If somehow the timer is still running but we're not supposed to be, stop it
        if (!m_isRunning) {
            m_timer.stop();
            return;
        }
    } else {
        // Stop the timer first to prevent reentrancy
        m_timer.stop();
        
        // Only proceed if we're still in running state
        if (m_isRunning) {
            m_isRunning = false;
            
            // Update task cycles in TaskManager
            if (m_currentPhase == PomodoroPhase::Work) {
                QWidget* widget = this;
                while (widget) {
                    MainWindow* mainWin = qobject_cast<MainWindow*>(widget);
                    if (mainWin) {
                        TaskManager* manager = mainWin->findChild<TaskManager*>();
                        if (manager) {
                            int activeId = manager->getActiveTask();
                            if (activeId > 0) {
                                QVariantMap taskData;
                                taskData["id"] = activeId;
                                taskData["remaining_cycles"] = m_taskRemainingCycles;
                                manager->updateTask(activeId, taskData);
                            }
                        }
                        break;
                    }
                    widget = widget->parentWidget();
                }
            }
            
            emit phaseCompleted(m_currentPhase);
            if (m_completedPomodoros % POMODOROS_BEFORE_LONG_BREAK == 0) {
                emit cycleCompleted();
            }
            startNextPhase();
        }
    }
}

void PomodoroTimer::updateTimerDisplay()
{
    auto ctx = std::map<std::string, std::string>{{{"remainingSeconds", std::to_string(m_remainingSeconds)}, {"phase", std::to_string(static_cast<int>(m_currentPhase))}}};
    LOG_DEBUG("PomodoroTimer", "Updating timer display", ctx);
    m_timeLabel->setText(formatTime(m_remainingSeconds));
    
    QString phaseText;
    switch (m_currentPhase) {
        case PomodoroPhase::Work:
            phaseText = tr("Work");
            break;
        case PomodoroPhase::ShortBreak:
            phaseText = tr("Short Break");
            break;
        case PomodoroPhase::LongBreak:
            phaseText = tr("Long Break");
            break;
    }
    m_phaseLabel->setText(phaseText);
}

void PomodoroTimer::updateButtonStates()
{
    auto ctx = std::map<std::string, std::string>{{{"isRunning", m_isRunning ? "true" : "false"}}};
    LOG_DEBUG("PomodoroTimer", "Updating button states", ctx);
    m_resetButton->setEnabled(!m_isRunning || m_remainingSeconds < m_currentPhaseDuration);
    m_skipButton->setEnabled(!m_isRunning);
}

void PomodoroTimer::setActiveTaskName(const QString &name)
{
    auto ctx = std::map<std::string, std::string>{{{"name", name.toStdString()}}};
    LOG_INFO("PomodoroTimer", "Setting active task name", ctx);
    
    m_activeTaskName = name;
    m_activeTaskNameLabel->setText(name.isEmpty() ? tr("No task selected") : name);
    
    // Reset timer when task changes
    resetTimer();
    
    // Update cycles display when task changes
    if (name.isEmpty()) {
        m_taskRemainingCycles = 0;
    } else if (m_taskManager) {
        int activeId = m_taskManager->getActiveTask();
        if (activeId > 0) {
            QVariantMap taskData = m_taskManager->getTaskData(activeId);
            if (!taskData.isEmpty()) {
                m_taskRemainingCycles = taskData.value("remaining_cycles", 0).toInt();
            } else {
                auto ctxWarn = std::map<std::string, std::string>{{{"activeId", std::to_string(activeId)}}};
                LOG_ERROR("PomodoroTimer", "Failed to get task data for active task", ctxWarn);
            }
        }
    }
    
    updateCyclesDisplay();
    
    // If task is selected, start timer
    if (!name.isEmpty()) {
        m_isRunning = false; // Ensure timer is ready to start
        startTimer();
    } else {
        m_startPauseButton->setText(tr("Start"));
    }
    
    emit activeTaskChanged(name);
}

void PomodoroTimer::updateCyclesDisplay()
{
    // Get total cycles from TaskManager
    int totalCycles = 0;
    if (m_taskManager && !m_activeTaskName.isEmpty()) {
        int activeId = m_taskManager->getActiveTask();
        if (activeId > 0) {
            totalCycles = m_taskManager->getTotalCycles(activeId);
            QVariantMap taskData = m_taskManager->getTaskData(activeId);
            if (!taskData.isEmpty()) {
                m_taskRemainingCycles = taskData.value("remaining_cycles", 0).toInt();
            } else {
                auto ctxWarn = std::map<std::string, std::string>{{{"activeId", std::to_string(activeId)}}};
                LOG_ERROR("PomodoroTimer", "Failed to get task data for active task", ctxWarn);
            }
        }
    }

    // Update the cycle label
    m_cycleLabel->setText(tr("Cycles: ") + QString::number(m_taskRemainingCycles)
                              + "/" + QString::number(totalCycles));
    m_cycleLabel->setAlignment(Qt::AlignCenter);
    m_cycleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_cycleLabel->setMinimumHeight(30);
    m_cycleLabel->setStyleSheet("QLabel { padding: 5px; }");
}
PomodoroTimer::~PomodoroTimer() {
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("PomodoroTimer", "PomodoroTimer destroyed", ctx);
}

void PomodoroTimer::setupUi()
{
    // Set minimum size for the timer
    setMinimumSize(300, 300);
    
    // Create a container widget for labels
    QWidget *labelContainer = new QWidget(this);
    labelContainer->setObjectName("labelContainer");
    labelContainer->setMinimumSize(200, 200);
    
    QVBoxLayout *labelLayout = new QVBoxLayout(labelContainer);
    labelLayout->setAlignment(Qt::AlignCenter);
    labelLayout->setSpacing(10);
    labelLayout->setContentsMargins(20, 20, 20, 20);
    
    // Create a main layout for the widget
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(labelContainer, 0, Qt::AlignCenter);

    // Active task label
    QLabel *activeTaskLabel = new QLabel(tr("Current Task:"), this);
    activeTaskLabel->setObjectName("activeTaskLabel");
    activeTaskLabel->setAlignment(Qt::AlignCenter);
    labelLayout->addWidget(activeTaskLabel);
    
    m_activeTaskNameLabel = new QLabel(this);
    m_activeTaskNameLabel->setObjectName("activeTaskNameLabel");
    m_activeTaskNameLabel->setAlignment(Qt::AlignCenter);
    m_activeTaskNameLabel->setWordWrap(true);
    labelLayout->addWidget(m_activeTaskNameLabel);
    
    // Phase label
    m_phaseLabel = new QLabel(this);
    m_phaseLabel->setObjectName("phaseLabel");
    m_phaseLabel->setAlignment(Qt::AlignCenter);
    labelLayout->addWidget(m_phaseLabel);
    
    // Time label
    m_timeLabel = new QLabel(this);
    m_timeLabel->setObjectName("timeLabel");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    labelLayout->addWidget(m_timeLabel);
    
    // Cycle label
    m_cycleLabel = new QLabel(this);
    m_cycleLabel->setObjectName("cycleLabel");
    m_cycleLabel->setAlignment(Qt::AlignCenter);
    labelLayout->addWidget(m_cycleLabel);

    // Create main layou

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(10);
    
    m_startPauseButton = new QPushButton(tr("Start"), this);
    m_startPauseButton->setObjectName("startPauseButton");
    m_startPauseButton->setFixedWidth(80);
    
    m_resetButton = new QPushButton(tr("Reset"), this);
    m_resetButton->setObjectName("resetButton");
    m_resetButton->setFixedWidth(80);
    
    m_skipButton = new QPushButton(tr("Skip"), this);
    m_skipButton->setObjectName("skipButton");
    m_skipButton->setFixedWidth(80);
    
    buttonLayout->addWidget(m_startPauseButton);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addWidget(m_skipButton);
    
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(20);  // Add space below buttons
    
    // Connect buttons
    connect(m_startPauseButton, &QPushButton::clicked, this, [this]() {
        if (m_isRunning) {
            pauseTimer();
        } else {
            startTimer();
        }
    });
    
    connect(m_resetButton, &QPushButton::clicked, this, &PomodoroTimer::resetTimer);
    connect(m_skipButton, &QPushButton::clicked, this, &PomodoroTimer::skipPhase);
    
    // Initial update
    updateTimerDisplay();
    updateButtonStates();
}

void PomodoroTimer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Calculate circle parameters
    int size = qMin(width(), height()) - 40; // Leave some margin
    m_radius = size / 2;
    m_center = QPoint(width() / 2, height() / 2);
    
    // Draw background circle
    QPen pen(palette().color(QPalette::WindowText));
    pen.setWidth(m_timerCircleWidth); // Use the QSS property for width
    painter.setPen(pen);
    
    // Create a rectangle for the circle
    QRect circleRect(m_center.x() - m_radius, m_center.y() - m_radius, size, size);
    painter.drawEllipse(circleRect);
    
    // Draw progress arc
    if (m_currentPhaseDuration > 0) {
        QColor progressColor;
        switch (m_currentPhase) {
            case PomodoroPhase::Work:
                progressColor = QColor("#2ecc71"); // Green
                break;
            case PomodoroPhase::ShortBreak:
                progressColor = QColor("#3498db"); // Blue
                break;
            case PomodoroPhase::LongBreak:
                progressColor = QColor("#9b59b6"); // Purple
                break;
        }
        
        pen.setColor(progressColor);
        painter.setPen(pen);
        
        qreal progress = static_cast<qreal>(m_remainingSeconds) / m_currentPhaseDuration;
        int startAngle = 90 * 16; // Start from top (90 degrees * 16 for QPainter angle format)
        int spanAngle = -progress * 360 * 16; // Negative for clockwise, multiply by 16 for QPainter
        
        painter.drawArc(circleRect, startAngle, spanAngle);
    }
    
    // Position the label container in the center of the circle
    QWidget *labelContainer = findChild<QWidget*>("labelContainer");
    if (labelContainer) {
        labelContainer->setGeometry(circleRect);
    }
}

void PomodoroTimer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Ensure the widget has a minimum size
    if (width() < 300 || height() < 300) {
        setMinimumSize(300, 300);
    }
    
    update();
}

QString PomodoroTimer::formatTime(int seconds) const
{
    int minutes = seconds / 60;
    seconds = seconds % 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}
