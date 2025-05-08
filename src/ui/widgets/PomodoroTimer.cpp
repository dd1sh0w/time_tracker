#include "PomodoroTimer.h"
#include <QPainter>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QDebug>
#include "../../logging/logger.h"
#include "../MainWindow.h"
#include "../../core/TaskManager.h"
#include <QPixmap>

PomodoroTimer::PomodoroTimer(QWidget *parent)
    : QWidget(parent)
    , m_currentPhase(PomodoroPhase::Work)
    , m_isRunning(false)
    , m_completedPomodoros(0)
    , m_activeTaskName("")
    , m_currentPhaseDuration(WORK_DURATION)
    , m_taskRemainingCycles(0)
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("PomodoroTimer", "Initializing PomodoroTimer", ctx);
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("pomodoroTimer");
    
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
    m_remainingSeconds = m_currentPhaseDuration;
    updateTimerDisplay();
    m_isRunning = true;
    m_timer.start(1000); // Update every second
    m_startPauseButton->setText(tr("Pause"));
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
    m_isRunning = false;
    m_timer.stop();
    m_startPauseButton->setText(tr("Resume"));
}

void PomodoroTimer::resetTimer()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("PomodoroTimer", "Resetting timer", ctx);
    m_timer.stop();
    m_isRunning = false;
    m_currentPhase = PomodoroPhase::Work;
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
    
    if (m_currentPhase == PomodoroPhase::Work) {
        m_completedPomodoros++;
        auto ctx2 = std::map<std::string, std::string>{{{"completedPomodoros", std::to_string(m_completedPomodoros)}}};
        LOG_INFO("PomodoroTimer", "Work phase skipped, incrementing pomodoros", ctx2);
        emit phaseCompleted(m_currentPhase);
        if (m_completedPomodoros % POMODOROS_BEFORE_LONG_BREAK == 0) {
            auto ctx3 = std::map<std::string, std::string>{};
            LOG_INFO("PomodoroTimer", "Cycle completed (skip)", ctx3);
            emit cycleCompleted();
        }
    }
    startNextPhase();
}

void PomodoroTimer::startNextPhase()
{
    auto ctx = std::map<std::string, std::string>{{{"prevPhase", std::to_string(static_cast<int>(m_currentPhase))}, {"completedPomodoros", std::to_string(m_completedPomodoros)}}};
    LOG_INFO("PomodoroTimer", "Starting next phase", ctx);
    switch (m_currentPhase) {
        case PomodoroPhase::Work:
            m_currentPhase = (m_completedPomodoros % POMODOROS_BEFORE_LONG_BREAK == 0) 
                ? PomodoroPhase::LongBreak : PomodoroPhase::ShortBreak;
            m_currentPhaseDuration = (m_currentPhase == PomodoroPhase::LongBreak) ? LONG_BREAK_DURATION : SHORT_BREAK_DURATION;
            break;
        case PomodoroPhase::ShortBreak:
        case PomodoroPhase::LongBreak:
            m_currentPhase = PomodoroPhase::Work;
            m_currentPhaseDuration = WORK_DURATION;
            break;
    }
    m_remainingSeconds = m_currentPhaseDuration;
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
    if (!m_isRunning) return;
    
    if (m_remainingSeconds > 0) {
        m_remainingSeconds--;
        updateTimerDisplay();
        update();
    } else {
        m_timer.stop();
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
    // Получаем оставшиеся циклы задачи через TaskManager, который находится в MainWindow
    int remainingCycles = 0;
    if (!name.isEmpty()) {
        // Найти MainWindow через цепочку parentWidget()
        QWidget* widget = this;
        while (widget) {
            MainWindow* mainWin = qobject_cast<MainWindow*>(widget);
            if (mainWin) {
                TaskManager* manager = mainWin->findChild<TaskManager*>();
                if (manager) {
                    int activeId = manager->getActiveTask();
                    QVariantMap taskData = manager->getTaskData(activeId);
                    remainingCycles = taskData.value("remaining_cycles", 0).toInt();
                }
                break;
            }
            widget = widget->parentWidget();
        }
    }
    m_taskRemainingCycles = remainingCycles;
    updateCyclesDisplay();
}

void PomodoroTimer::updateCyclesDisplay()
{
    // Get total cycles from TaskManager
    int totalCycles = 0;
    if (!m_activeTaskName.isEmpty()) {
        QWidget* widget = this;
        while (widget) {
            MainWindow* mainWin = qobject_cast<MainWindow*>(widget);
            if (mainWin) {
                TaskManager* manager = mainWin->findChild<TaskManager*>();
                if (manager) {
                    int activeId = manager->getActiveTask();
                    QVariantMap taskData = manager->getTaskData(activeId);
                    totalCycles = taskData.value("total_cycles", 0).toInt();
                }
                break;
            }
            widget = widget->parentWidget();
        }
    }

    // Create a horizontal layout for stars
    QHBoxLayout *starLayout = new QHBoxLayout;
    starLayout->setSpacing(2);
    starLayout->setContentsMargins(0, 0, 0, 0);

    // Load star icons
    QPixmap remainingStarPixmap(":/style/icons/star_remaining.svg");
    QPixmap totalStarPixmap(":/style/icons/star_total.svg");

    // Add remaining cycles stars
    for (int i = 0; i < m_taskRemainingCycles; i++) {
        QLabel *starLabel = new QLabel(this);
        starLabel->setPixmap(remainingStarPixmap);
        starLayout->addWidget(starLabel);
    }

    // Add total cycles stars (gray)
    for (int i = 0; i < totalCycles - m_taskRemainingCycles; i++) {
        QLabel *starLabel = new QLabel(this);
        starLabel->setPixmap(totalStarPixmap);
        starLayout->addWidget(starLabel);
    }

    // Create a widget to hold the stars layout
    QWidget *starsWidget = new QWidget(this);
    starsWidget->setLayout(starLayout);
    starsWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Update the cycle label
    m_cycleLabel->setText(tr("Cycles: ") + QString::number(m_taskRemainingCycles) + "/" + QString::number(totalCycles));
    m_cycleLabel->setAlignment(Qt::AlignCenter);
    m_cycleLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_cycleLabel->setFixedHeight(30);

    // Clear previous stars
    QLayoutItem *child;
    while ((child = m_cycleLabel->layout()->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    // Add the new stars layout
    m_cycleLabel->layout()->addWidget(starsWidget);
}

PomodoroTimer::~PomodoroTimer() {
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("PomodoroTimer", "PomodoroTimer destroyed", ctx);
}

void PomodoroTimer::setupUi()
{
    // Create a container widget for labels
    QWidget *labelContainer = new QWidget(this);
    labelContainer->setObjectName("labelContainer");
    QVBoxLayout *labelLayout = new QVBoxLayout(labelContainer);
    labelLayout->setAlignment(Qt::AlignCenter);
    labelLayout->setSpacing(5);
    labelLayout->setContentsMargins(0, 0, 0, 0);

    // Active task label
    QLabel *activeTaskLabel = new QLabel(tr("Current Task:"), this);
    activeTaskLabel->setObjectName("activeTaskLabel");
    activeTaskLabel->setAlignment(Qt::AlignCenter);
    labelLayout->addWidget(activeTaskLabel);
    
    m_activeTaskNameLabel = new QLabel(this);
    m_activeTaskNameLabel->setObjectName("activeTaskNameLabel");
    m_activeTaskNameLabel->setAlignment(Qt::AlignCenter);
    m_activeTaskNameLabel->setWordWrap(true);
    //m_activeTaskNameLabel->setStyleSheet("QLabel { font-size: 14px; }");
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

    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(labelContainer);

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
    int size = qMin(width(), height()) - 80; // Leave some margin
    m_radius = size / 2;
    m_center = rect().center();
    
    // Draw background circle
    QPen pen(QColor(200, 200, 200));
    pen.setWidth(30);
    painter.setPen(pen);
    
    // Create a rectangle for the circle
    QRect circleRect(m_center.x() - m_radius, m_center.y() - m_radius, size, size);
    painter.drawEllipse(circleRect);
    
    // Draw progress arc
    if (m_currentPhaseDuration > 0) {
        QColor progressColor;
        switch (m_currentPhase) {
            case PomodoroPhase::Work:
                progressColor = QColor(46, 204, 113); // Green
                break;
            case PomodoroPhase::ShortBreak:
                progressColor = QColor(52, 152, 219); // Blue
                break;
            case PomodoroPhase::LongBreak:
                progressColor = QColor(155, 89, 182); // Purple
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
    Q_UNUSED(event);
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
