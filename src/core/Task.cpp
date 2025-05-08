#include "Task.h"
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include "TaskSettingsDialog.h"
#include <QDebug>
#include "../logging/logger.h"

// Constructor initializes default values and sets up the UI
Task::Task(int id, QWidget *parent)
    : QWidget(parent),
      m_id(id),
      m_taskName(""),
      m_description(""),
      m_deadline(QDate::currentDate()),
      m_plannedCycles(0),
      m_remainingCycles(0),
      m_status(TaskStatus::Active)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(id)} }};
    LOG_INFO("Task", "Task widget created", ctx);
    setupUi();
    updateDisplay();
}

Task::~Task() {
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_INFO("Task", "Task widget destroyed", ctx);
}

// Set up task widget UI elements
void Task::setupUi()
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_DEBUG("Task", "Setting up task UI", ctx);
    m_layout = new QHBoxLayout(this);
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(5, 5, 5, 5);

    // Timer start button with a play symbol
    tt_startButton = new QPushButton(this);
    tt_startButton->setObjectName("tt_startButton");
    tt_startButton->setText("▶");

    // Task name button (opens the settings dialog)
    tt_nameButton = new QPushButton(this);
    tt_nameButton->setObjectName("tt_nameButton");
    connect(tt_nameButton, &QPushButton::clicked, this, &Task::openTaskSettings);

    // Delete button
    QPushButton *deleteButton = new QPushButton(this);
    deleteButton->setObjectName("deleteButton");
    deleteButton->setText("×");
    connect(deleteButton, &QPushButton::clicked, this, [this]() {
        emit taskDeleted(m_id);
    });

    tt_deadlineLabel = new QLabel(this);
    tt_deadlineLabel->setObjectName("tt_deadlineLabel");

    tt_cyclesLabel = new QLabel(this);
    tt_cyclesLabel->setObjectName("tt_cyclesLabel");

    tt_descriptionLabel = new QLabel(this);
    tt_descriptionLabel->setObjectName("tt_descriptionLabel");

    m_layout->addWidget(tt_startButton);
    m_layout->addWidget(tt_nameButton);
    m_layout->addWidget(deleteButton);
    m_layout->addWidget(tt_deadlineLabel);
    m_layout->addWidget(tt_cyclesLabel);
    m_layout->addWidget(tt_descriptionLabel);
    m_layout->addStretch();

    // Emit signal to start timer when the button is clicked
    connect(tt_startButton, &QPushButton::clicked, this, &Task::startTimer);

    setLayout(m_layout);
}

// Update the displayed information for the task widget
void Task::updateDisplay()
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_DEBUG("Task", "Updating task display", ctx);
    tt_nameButton->setText(m_taskName);
    tt_deadlineLabel->setText(m_deadline.toString("dd.MM.yyyy"));

    // Retrieve star emoji from QSettings (default is "★")
    QSettings settings;
    QString starEmoji = settings.value("starEmoji", "★").toString();
    QString stars;
    QString completedStars;
    
    // Show total planned cycles with filled/empty stars
    int completedCycles = m_plannedCycles - m_remainingCycles;
    for (int i = 0; i < m_plannedCycles; i++) {
        if (i < completedCycles) {
            stars.append("★"); // Filled star for completed cycles
        } else {
            stars.append("☆"); // Empty star for remaining cycles
        }
    }
    tt_cyclesLabel->setText(stars);
    tt_descriptionLabel->setText(m_description);

    // Update button states based on task status
    tt_startButton->setEnabled(m_status == TaskStatus::Active);
    tt_nameButton->setEnabled(m_status == TaskStatus::Active);
}

// Open task settings dialog; instead of updating immediately, emit updateRequested signal
void Task::openTaskSettings()
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_INFO("Task", "Opening task settings dialog", ctx);
    TaskSettingsDialog dialog(m_taskName, m_description, m_deadline, m_plannedCycles, this);
    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap data;
        data["id"] = m_id;
        data["name"] = dialog.taskName();
        data["description"] = dialog.taskDescription();
        data["deadline"] = dialog.taskDeadline();
        data["planned_cycles"] = dialog.taskCycles();
        data["remaining_cycles"] = dialog.taskCycles();
        data["status"] = toString(m_status); // Используем toString для конвертации статуса в строку
        auto ctx2 = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
        LOG_INFO("Task", "Task settings accepted", ctx2);
        emit taskUpdated(m_id, data);
    } else {
        auto ctx2 = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
        LOG_INFO("Task", "Task settings canceled", ctx2);
    }
}

// Update the task's internal state and refresh the UI
void Task::updateTask(const QString &name, const QString &description, const QDate &deadline, int plannedCycles)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
    LOG_INFO("Task", "Updating task state", ctx);
    setTaskName(name);
    setDescription(description);
    setDeadline(deadline);
    setPlannedCycles(plannedCycles);
    setRemainingCycles(plannedCycles);

    // Emit taskUpdated signal with the updated data
    QVariantMap data;
    data["id"] = m_id;
    data["name"] = name;
    data["description"] = description;
    data["deadline"] = deadline;
    data["planned_cycles"] = plannedCycles;
    data["remaining_cycles"] = plannedCycles;
    data["status"] = toString(m_status);
    emit taskUpdated(m_id, data);
}

// Update remaining cycles and status accordingly, then refresh display
void Task::updateCycles(int cycles)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)}, {"cycles", std::to_string(cycles)} }};
    LOG_INFO("Task", "Updating cycles", ctx);
    setRemainingCycles(m_remainingCycles - cycles);
    if (m_remainingCycles <= 0) {
        setStatus(TaskStatus::Completed);
        QVariantMap data;
        data["name"] = m_taskName;
        data["description"] = m_description;
        data["deadline"] = m_deadline;
        data["planned_cycles"] = m_plannedCycles;
        data["remaining_cycles"] = m_remainingCycles;
        data["status"] = static_cast<int>(m_status);
        auto ctx2 = std::map<std::string, std::string>{{{"taskId", std::to_string(m_id)} }};
        LOG_INFO("Task", "Task completed", ctx2);
        emit taskCompleted(m_id, data);
    }
}
