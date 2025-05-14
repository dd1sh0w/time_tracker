#include "Task.h"
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMessageBox>
#include "../logging/logger.h"
#include <QSettings>
#include "TaskSettingsDialog.h"
#include "../logging/logger.h"

// ----------------------------------------------------------------
// TaskUtil implementation (status ↔ QString)
// ----------------------------------------------------------------
QString TaskUtil::toString(TaskStatus status) {
    switch (status) {
    case TaskStatus::Active:     return "Active";
    case TaskStatus::InProgress: return "InProgress";
    case TaskStatus::OnHold:     return "OnHold";
    case TaskStatus::Completed:  return "Completed";
    case TaskStatus::Review:     return "Review";
    case TaskStatus::Overdue:    return "Overdue";
    case TaskStatus::Upcoming:   return "Upcoming";
    case TaskStatus::Cancelled:  return "Cancelled";
    default:                     return "Unknown";
    }
}

TaskStatus TaskUtil::fromString(const QString &statusStr) {
    if (statusStr == "Active")       return TaskStatus::Active;
    if (statusStr == "InProgress")   return TaskStatus::InProgress;
    if (statusStr == "OnHold")       return TaskStatus::OnHold;
    if (statusStr == "Completed")    return TaskStatus::Completed;
    if (statusStr == "Review")       return TaskStatus::Review;
    if (statusStr == "Overdue")      return TaskStatus::Overdue;
    if (statusStr == "Upcoming")     return TaskStatus::Upcoming;
    if (statusStr == "Cancelled")    return TaskStatus::Cancelled;
    return TaskStatus::Active; // fallback
}

// ----------------------------------------------------------------
// Task widget
// ----------------------------------------------------------------
Task::Task(int id, QWidget *parent)
    : QWidget(parent)
    , m_id(id)
    , m_taskName("")
    , m_description("")
    , m_deadline(QDate::currentDate())
    , m_plannedCycles(0)
    , m_remainingCycles(0)
    , m_status(TaskStatus::Active)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(id)} }};
    LOG_INFO("Task", "Task widget created", ctx);
    setupUi();
    updateDisplay();
}

Task::~Task() {
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_INFO("Task", "Destroyed", ctx);
}

void Task::setupUi()
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_DEBUG("Task", "Setup UI", ctx);
    m_layout = new QHBoxLayout(this);
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(5,5,5,5);

    // ▶ start button
    tt_startButton = new QPushButton("▶", this);
    tt_startButton->setObjectName("tt_startButton");
    connect(tt_startButton, &QPushButton::clicked, this, &Task::startTimer);

    // Task name opens settings
    tt_nameButton = new QPushButton(this);
    tt_nameButton->setObjectName("tt_nameButton");
    connect(tt_nameButton, &QPushButton::clicked, this, &Task::openTaskSettings);

    // Status label
    tt_statusLabel = new QLabel(this);
    tt_statusLabel->setObjectName("tt_statusLabel");

    // Delete button
    QPushButton *deleteButton = new QPushButton("×", this);
    deleteButton->setObjectName("deleteButton");
    connect(deleteButton, &QPushButton::clicked,
            this, [this](){ emit taskDeleted(m_id); });

    tt_deadlineLabel    = new QLabel(this);
    tt_deadlineLabel->setObjectName("tt_deadlineLabel");
    tt_cyclesLabel      = new QLabel(this);
    tt_cyclesLabel->setObjectName("tt_cyclesLabel");
    tt_descriptionLabel = new QLabel(this);
    tt_descriptionLabel->setObjectName("tt_descriptionLabel");

    // Assemble
    m_layout->addWidget(tt_startButton);
    m_layout->addWidget(tt_nameButton);
    m_layout->addWidget(tt_statusLabel);
    m_layout->addWidget(deleteButton);
    m_layout->addWidget(tt_deadlineLabel);
    m_layout->addWidget(tt_cyclesLabel);
    m_layout->addWidget(tt_descriptionLabel);
    m_layout->addStretch();

    setLayout(m_layout);
}

void Task::updateDisplay()
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_DEBUG("Task", "Update UI", ctx);
    // Name & description
    tt_nameButton->setText(m_taskName);
    tt_descriptionLabel->setText(m_description);

    // Deadline
    tt_deadlineLabel->setText(m_deadline.toString("dd.MM.yyyy"));

    // Cycles: filled ★ vs empty ☆
    QSettings settings;
    QString filled = settings.value("starEmoji", "★").toString();
    QString empty  = settings.value("emptyStarEmoji", "☆").toString();
    int done = m_plannedCycles - m_remainingCycles;
    QString stars;
    for(int i=0; i<m_plannedCycles; ++i)
        stars += (i<done ? filled : empty);
    tt_cyclesLabel->setText(stars);

    // Status
    QString statusText = TaskUtil::toString(m_status);
    tt_statusLabel->setText(statusText);
    tt_statusLabel->setToolTip(statusText);

    // Set object name based on status for styling
    QString statusName = TaskUtil::toString(m_status).toLower();
    tt_statusLabel->setProperty("status", statusName);
    tt_statusLabel->style()->unpolish(tt_statusLabel);
    tt_statusLabel->style()->polish(tt_statusLabel);

    // Enable start/name only when Active
    bool active = (m_status == TaskStatus::Active);
    tt_startButton->setEnabled(active);
    tt_nameButton ->setEnabled(active);
}

// ----------------------------------------------------------------
// Setters (emit display update on status change)
// ----------------------------------------------------------------
void Task::setTaskName(const QString &name) {
    m_taskName = name;
    updateDisplay();
}

void Task::setDescription(const QString &description) {
    m_description = description;
    updateDisplay();
}

void Task::setDeadline(const QDate &deadline) {
    m_deadline = deadline;
    updateDisplay();
}

void Task::setPlannedCycles(int cycles) {
    m_plannedCycles = cycles;
    updateDisplay();
}

void Task::setRemainingCycles(int cycles) {
    m_remainingCycles = cycles;
    updateDisplay();
}

void Task::setStatus(TaskStatus status) {
    if (m_status == status) return;
    m_status = status;
    auto ctx = std::map<std::string, std::string>{
        {"taskId", std::to_string(m_id)},
        {"newStatus", TaskUtil::toString(m_status).toStdString()}
    };
    LOG_INFO("Task", "Status changed", ctx);
    emit remainingCyclesChanged(m_remainingCycles);
    updateDisplay();
}

void Task::setId(int id) {
    m_id = id;
}

// ----------------------------------------------------------------
// Open the settings dialog & emit update
// ----------------------------------------------------------------
void Task::openTaskSettings()
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_INFO("Task", "Open settings", ctx);
    TaskSettingsDialog dlg(m_taskName, m_description, m_deadline, m_plannedCycles, TaskUtil::toString(m_status), this);
    if (dlg.exec() == QDialog::Accepted) {
        QVariantMap data;
        data["id"]              = m_id;
        data["name"]            = dlg.taskName();
        data["description"]     = dlg.taskDescription();
        data["deadline"]        = dlg.taskDeadline();
        data["planned_cycles"]  = dlg.taskCycles();
        data["remaining_cycles"] = dlg.taskCycles();
        data["status"]           = dlg.taskStatus();
        
        // Update the local status
        setStatus(TaskUtil::fromString(dlg.taskStatus()));

        auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
        LOG_INFO("Task", "Settings accepted", ctx);
        emit taskUpdated(m_id, data);
    } else {
        auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
        LOG_INFO("Task", "Settings canceled", ctx);
    }
}

// ----------------------------------------------------------------
// Update via external call & emit
// ----------------------------------------------------------------
void Task::updateTask(const QString &name, const QString &description,
                      const QDate &deadline, int plannedCycles)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_INFO("Task", "External update", ctx);
    setTaskName(name);
    setDescription(description);
    setDeadline(deadline);
    setPlannedCycles(plannedCycles);
    setRemainingCycles(plannedCycles);
    
    // Set default status to Active for new or updated tasks if not set
    if (m_status == TaskStatus::Cancelled) {
        setStatus(TaskStatus::Active);
    }

    QVariantMap data;
    data["id"]               = m_id;
    data["name"]             = name;
    data["description"]      = description;
    data["deadline"]         = deadline;
    data["planned_cycles"]   = plannedCycles;
    data["remaining_cycles"] = plannedCycles;
    data["status"] = TaskUtil::toString(m_status);
    data["status"]           = TaskUtil::toString(m_status);

    emit taskUpdated(m_id, data);
}

// ----------------------------------------------------------------
// When a pomodoro finishes, decrement & maybe complete
// ----------------------------------------------------------------
void Task::updateCycles(int cycles)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)},
                                          {"cycles", std::to_string(cycles)}}};
    LOG_INFO("Task", "Recording cycles", ctx);
    setRemainingCycles(m_remainingCycles - cycles);
    if (m_remainingCycles <= 0) {
        setStatus(TaskStatus::Completed);
        QVariantMap data;
        data["id"]               = m_id;
        data["name"]             = m_taskName;
        data["description"]      = m_description;
        data["deadline"]         = m_deadline;
        data["planned_cycles"]   = m_plannedCycles;
        data["remaining_cycles"] = 0;
        data["status"]           = TaskUtil::toString(m_status);
        emit taskCompleted(m_id, data);
    }
}

// ----------------------------------------------------------------
// Helper: get the status string for logging
// ----------------------------------------------------------------
QString Task::statusText() const {
    return TaskUtil::toString(m_status);
}
