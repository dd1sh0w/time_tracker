#include "HistoryPage.h"
#include "../../core/DatabaseManager.h"
#include "../../core/Task.h"
#include "../components/DayCard.h"
#include <QCalendarWidget>
#include <QMessageBox>
#include <QDebug>
#include <algorithm>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QScrollBar>
#include <QApplication>
#include <QStyle>
#include <QVariantMap>
#include <QLineEdit>
#include <QDateEdit>
#include "../../logging/logger.h"

HistoryPage::HistoryPage(int userId, QWidget *parent, TaskManager* taskManager)
    : QWidget(parent), m_userId(userId), m_firstCardIndex(0), m_taskManager(taskManager)
{
    auto ctx = std::map<std::string, std::string>{{{"userId", std::to_string(userId)}}};
    LOG_INFO("HistoryPage", "HistoryPage created", ctx);
    if (userId <= 0) {
        auto ctxWarn = std::map<std::string, std::string>{{{"userId", std::to_string(userId)}}};
        LOG_WARNING("HistoryPage", "Invalid user ID in HistoryPage constructor", ctxWarn);
    }
    m_baseDate = QDate::currentDate();
    setupUi();
    loadHistory();
}

void HistoryPage::onAddTaskClicked()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("HistoryPage", "Add task from history clicked", ctx);
    qDebug() << "HistoryPage: exec onAddTaskClicked";
    QDialog dialog(this);
    dialog.setWindowTitle("Create new task");
    dialog.setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    QLineEdit* taskNameInput = new QLineEdit(&dialog);
    taskNameInput->setPlaceholderText("Task name");
    layout->addWidget(taskNameInput);

    QDateEdit* dateEdit = new QDateEdit(&dialog);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDate(QDate::currentDate());
    layout->addWidget(dateEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        &dialog
    );
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString taskName = taskNameInput->text();
        if (taskName.isEmpty()) {
            auto ctxErr = std::map<std::string, std::string>{};
            LOG_ERROR("HistoryPage", "Task name cannot be empty", ctxErr);
            QMessageBox::critical(this, "Error", "Task name cannot be empty");
            return;
        }
        QDate date = dateEdit->date();
        int taskId = DatabaseManager::instance().addTask(
            m_userId, 
            taskName,
            "",  // Empty description
            QDateTime(date, QTime(0, 0, 0)),  // Convert QDate to QDateTime at midnight
            1,  // Default planned cycles
            1,  // Default remaining cycles
            "Active"  // Default status
        );
        if (taskId > 0) {
            auto ctx2 = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}}};
            LOG_INFO("HistoryPage", "Task added successfully", ctx2);
            m_historyData[date].append({taskId, taskName});
            m_firstCardIndex = 0;
            updateDayCards();
            loadHistory();
        } else {
            auto ctxErr = std::map<std::string, std::string>{};
            LOG_ERROR("HistoryPage", "Failed to add task", ctxErr);
        }
    }
}

HistoryPage::onStartTaskClicked(const TaskInfo& task)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(task.taskId)}}};
    LOG_INFO("HistoryPage", "Start task from history clicked", ctx);
    emit startTaskClicked(task);
    return 1;
}

HistoryPage::~HistoryPage() {
    auto ctx = std::map<std::string, std::string>{{{"userId", std::to_string(m_userId)}}};
    LOG_INFO("HistoryPage", "HistoryPage destroyed", ctx);
}

void HistoryPage::setupUi()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("HistoryPage", "Setting up HistoryPage UI", ctx);
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);

    // Header
    QLabel* headerLabel = new QLabel("Task History", this);
    headerLabel->setObjectName("historyHeader");
    headerLabel->setProperty("class", "historyHeader");
    headerLabel->setStyleSheet("font-size: 24px; font-weight: bold;");

    // Add Task button
    m_addTaskButton = new QPushButton("Add Task", this);
    m_addTaskButton->setObjectName("addTaskButton");
    m_addTaskButton->setProperty("class", "addTaskButton");
    connect(m_addTaskButton, &QPushButton::clicked, this, &HistoryPage::onAddTaskClicked);

    // Header layout
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_addTaskButton);
    m_mainLayout->addLayout(headerLayout);

    // Main content container
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(10);
    
    // Left navigation button
    m_leftButton = new QPushButton(this);
    m_leftButton->setIcon(QIcon(":/icons/arrow-left.png"));
    m_leftButton->setIconSize(QSize(24, 24));
    m_leftButton->setFixedSize(40, 200);
    m_leftButton->setObjectName("navButton");
    m_leftButton->setProperty("class", "navButton");
    m_leftButton->setCursor(Qt::PointingHandCursor);
    
    // Cards container
    QWidget* cardsWidget = new QWidget(this);
    cardsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_dayCardsLayout = new QHBoxLayout(cardsWidget);
    m_dayCardsLayout->setSpacing(15);  // Increased spacing between cards
    m_dayCardsLayout->setContentsMargins(0, 0, 0, 0);
    m_dayCardsLayout->setAlignment(Qt::AlignCenter);  // Center the cards
    
    // Create 5 day cards
    for (int i = 0; i < 5; ++i) {
        DayCard* card = new DayCard(m_baseDate.addDays(i), this);
        m_dayCards.append(card);
        m_dayCardsLayout->addWidget(card, 1);  // Add stretch factor of 1 to make cards equal width
        connect(card, &DayCard::taskClicked, this, &HistoryPage::onTaskClicked);
        connect(card, &DayCard::editTaskClicked, this, &HistoryPage::onEditTaskClicked);
        connect(card, &DayCard::startTaskClicked, this, &HistoryPage::onStartTaskClicked);
    }
    
    // Right navigation button
    m_rightButton = new QPushButton(this);
    m_rightButton->setIcon(QIcon(":/icons/arrow-right.png"));
    m_rightButton->setIconSize(QSize(24, 24));
    m_rightButton->setFixedSize(40, 200);
    m_rightButton->setObjectName("navButton");
    m_rightButton->setProperty("class", "navButton");
    m_rightButton->setCursor(Qt::PointingHandCursor);

    contentLayout->addWidget(m_leftButton);
    contentLayout->addWidget(cardsWidget, 1);
    contentLayout->addWidget(m_rightButton);
    
    // Date selection
    QHBoxLayout* dateSelectionLayout = new QHBoxLayout();
    m_chooseDayButton = new QPushButton("Select Date", this);
    m_chooseDayButton->setObjectName("chooseDayButton");
    m_chooseDayButton->setProperty("class", "chooseDayButton");
    m_chooseDayButton->setCursor(Qt::PointingHandCursor);
    dateSelectionLayout->addStretch();
    dateSelectionLayout->addWidget(m_chooseDayButton);
    dateSelectionLayout->addStretch();

    // Task details frame
    m_detailsFrame = new QFrame(this);
    m_detailsFrame->setObjectName("detailsFrame");
    m_detailsFrame->setProperty("class", "detailsFrame");
    m_detailsFrame->setFrameShape(QFrame::StyledPanel);
    
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsFrame);
    detailsLayout->setSpacing(10);
    detailsLayout->setContentsMargins(15, 15, 15, 15);

    QLabel* detailsHeaderLabel = new QLabel("Task Description", m_detailsFrame);
    detailsHeaderLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    
    m_detailsLabel = new QLabel(m_detailsFrame);
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->setTextFormat(Qt::RichText);
    m_detailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_detailsLabel->setOpenExternalLinks(true);
    
    detailsLayout->addWidget(detailsHeaderLabel);
    detailsLayout->addWidget(m_detailsLabel);

    m_mainLayout->addLayout(contentLayout);
    m_mainLayout->addLayout(dateSelectionLayout);
    m_mainLayout->addWidget(m_detailsFrame);

    // Connect signals
    connect(m_leftButton, &QPushButton::clicked, this, &HistoryPage::shiftLeft);
    connect(m_rightButton, &QPushButton::clicked, this, &HistoryPage::shiftRight);
    connect(m_chooseDayButton, &QPushButton::clicked, this, &HistoryPage::chooseDay);

    // Subscribe to task updates
    connect(&DatabaseManager::instance(), &DatabaseManager::taskUpdated,
            this, &HistoryPage::onTaskUpdated);
}

void HistoryPage::loadHistory()
{
    auto ctx = std::map<std::string, std::string>{{{"userId", std::to_string(m_userId)}}};
    LOG_INFO("HistoryPage", "Loading history", ctx);
    if (m_userId <= 0) {
        auto ctxErr = std::map<std::string, std::string>{{{"userId", std::to_string(m_userId)}}};
        LOG_ERROR("HistoryPage", "Invalid user ID in loadHistory", ctxErr);
        return;
    }

    // Get active tasks from tasks table
    QList<QVariantMap> tasks = DatabaseManager::instance().getTasks(m_userId);
    QMap<QDate, QList<TaskInfo>> historyMap;
    for (const QVariantMap &record : tasks) {
        TaskInfo info;
        info.taskId = record.value("id").toInt();
        info.taskName = record.value("name").toString();
        info.cycles = record.value("planned_cycles").toInt();
        info.status = record.value("status").toString();
        info.completedAt = record.value("updated_at").toDateTime();
        info.details = formatTaskDetails(info);
        historyMap[info.completedAt.date()].append(info);
    }
    m_historyData = historyMap;
    updateDayCards();
    auto ctx2 = std::map<std::string, std::string>{};
    LOG_INFO("HistoryPage", "History loaded successfully", ctx2);
}

QString HistoryPage::formatTaskDetails(const TaskInfo &task) const
{
    QString details = QString("<b>Name:</b> %1<br>").arg(task.taskName.toHtmlEscaped());
    details += QString("<b>Cycles:</b> %1<br>").arg(task.cycles);
    details += QString("<b>Status:</b> %1<br>").arg(task.status.toHtmlEscaped());
    
    if (!task.priority.isEmpty())
        details += QString("<b>Priority:</b> %1<br>").arg(task.priority.toHtmlEscaped());
    
    if (task.deadline.isValid()) {
        QString deadlineColor = task.deadline < QDate::currentDate() ? "red" : "inherit";
        details += QString("<b>Deadline:</b> <span style='color: %1'>%2</span><br>")
                    .arg(deadlineColor)
                    .arg(task.deadline.toString("MMMM d, yyyy"));
    }
    
    if (!task.description.isEmpty())
        details += QString("<b>Description:</b> %1<br>").arg(task.description.toHtmlEscaped());
    
    if (task.completedAt.isValid())
        details += QString("<b>Completed:</b> %1")
                    .arg(task.completedAt.toString("MMMM d, yyyy hh:mm:ss"));
    
    return details;
}

void HistoryPage::updateDayCards()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("HistoryPage", "Updating day cards", ctx);
    for (int i = 0; i < m_dayCards.size(); ++i) {
        QDate cardDate = m_baseDate.addDays(i + m_firstCardIndex);
        if (!cardDate.isValid()) {
            auto ctxWarn = std::map<std::string, std::string>{};
            LOG_WARNING("HistoryPage", "Invalid date calculated in updateDayCards", ctxWarn);
            continue;
        }
        
        DayCard *card = m_dayCards[i];
        if (!card) {
            auto ctxErr = std::map<std::string, std::string>{{{"index", std::to_string(i)}}};
            LOG_ERROR("HistoryPage", "Null card pointer at index", ctxErr);
            continue;
        }
        
        card->setDate(cardDate);
        card->setTasks(m_historyData.value(cardDate));
    }
}

void HistoryPage::shiftLeft()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("HistoryPage", "Shift left clicked", ctx);
    m_firstCardIndex--;
    updateDayCards();
}

void HistoryPage::shiftRight()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("HistoryPage", "Shift right clicked", ctx);
    m_firstCardIndex++;
    updateDayCards();
}

void HistoryPage::chooseDay()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("HistoryPage", "Choose day clicked", ctx);
    QDialog dialog(this);
    dialog.setWindowTitle("Select Date");
    dialog.setModal(true);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);
    
    QCalendarWidget* calendar = new QCalendarWidget(&dialog);
    calendar->setSelectedDate(m_baseDate);
    calendar->setGridVisible(true);
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    calendar->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        &dialog
    );
    
    layout->addWidget(calendar);
    layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        m_baseDate = calendar->selectedDate();
        m_firstCardIndex = 0;
        updateDayCards();
    }
}

void HistoryPage::onTaskClicked(const TaskInfo &task)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(task.taskId)}}};
    LOG_INFO("HistoryPage", "Task clicked", ctx);
    if (!task.taskName.isEmpty()) {
        m_detailsLabel->setText(task.description);
        m_detailsFrame->setProperty("taskId", task.taskId);
    }
}

void HistoryPage::onEditTaskClicked(const TaskInfo &task)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(task.taskId)}}};
    LOG_INFO("HistoryPage", "Edit task clicked", ctx);
    TaskSettingsDialog dialog(task.taskName,
                             task.description,
                             task.deadline,
                             task.cycles,
                             this);
    
    // Connect the delete signal
    connect(&dialog, &TaskSettingsDialog::taskDeleted, this, [this, task](){
        onTaskDeleted(task.taskId);
    });
    
    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap taskData;
        taskData["id"] = task.taskId;
        taskData["name"] = dialog.taskName();
        taskData["description"] = dialog.taskDescription();
        taskData["deadline"] = dialog.taskDeadline();
        taskData["planned_cycles"] = dialog.taskCycles();
        taskData["remaining_cycles"] = dialog.taskCycles();
        taskData["status"] = Task::toString(TaskStatus::Active); // Add status if needed
        
        DatabaseManager::instance().updateTask(taskData);
        loadHistory();
    }
}

void HistoryPage::onTaskDeleted(int taskId)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskId)}}};
    LOG_INFO("HistoryPage", "Task deleted", ctx);
    // Delete the task from the database
    DatabaseManager::instance().deleteTask(taskId);
    
    // Clear the details
    m_detailsLabel->clear();
    m_detailsFrame->setProperty("taskId", QVariant());
    
    // Reload the history
    loadHistory();  
}

void HistoryPage::onTaskUpdated(const QVariantMap &taskData)
{
    auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(taskData.value("id").toInt())}}};
    LOG_INFO("HistoryPage", "Task updated", ctx);
    if (taskData.contains("id")) {
        int taskId = taskData.value("id").toInt();
        if (taskId > 0) {
            // Update specific task data
            QList<QVariantMap> tasks = DatabaseManager::instance().getTasks(m_userId);
            for (const QVariantMap &task : tasks) {
                if (task.value("id").toInt() == taskId) {
                    // Update the task in history
                    for (auto &dateTasks : m_historyData) {
                        for (auto &histTask : dateTasks) {
                            if (histTask.taskId == taskId) {
                                histTask.cycles = task.value("planned_cycles").toInt() - task.value("remaining_cycles").toInt();
                                histTask.status = task.value("status").toString();
                                histTask.details = formatTaskDetails(histTask);
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            updateDayCards();
        }
    }
}
