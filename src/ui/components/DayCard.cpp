#include "DayCard.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QDebug>
#include <QApplication>
#include <QPushButton>
#include <QStyle>
#include <QSettings>
#include "../../logging/logger.h"

DayCard::DayCard(const QDate &date, QWidget *parent)
    : QWidget(parent), m_date(date)
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("DayCard", "DayCard created", ctx);
    setupUi();
    updateDisplay();
}

DayCard::~DayCard() {
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("DayCard", "DayCard destroyed", ctx);
}

void DayCard::setupUi()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("DayCard", "Setting up DayCard UI", ctx);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(5);
    layout->setContentsMargins(10, 10, 10, 10);

    // Set fixed width and expanding height
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setMinimumWidth(50);  // Minimum width for the card
    setMaximumWidth(300);  // Maximum width to maintain readability

    // Card styling
    setObjectName("dayCard");
    setProperty("class", "dayCard");

    // Day label setup
    m_dayLabel = new QLabel(this);
    m_dayLabel->setAlignment(Qt::AlignCenter);
    m_dayLabel->setObjectName("dayLabel");
    m_dayLabel->setProperty("class", "dayLabel");
    m_dayLabel->setWordWrap(true);

    // Separator setup
    m_separator = new QFrame(this);
    m_separator->setFrameShape(QFrame::HLine);
    m_separator->setObjectName("daySeparator");

    // Tasks container setup
    m_tasksContainer = new QScrollArea(this);
    m_tasksContainer->setObjectName("tasksContainer");
    m_tasksContainer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tasksContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tasksContainer->setFrameShape(QFrame::NoFrame);

    QWidget* tasksWidget = new QWidget(m_tasksContainer);
    tasksWidget->setObjectName("tasksWidget");
    
    m_tasksLayout = new QVBoxLayout(tasksWidget);
    m_tasksLayout->setSpacing(8);
    m_tasksLayout->setContentsMargins(5, 5, 5, 5);
    m_tasksLayout->setAlignment(Qt::AlignTop);
    
    tasksWidget->setLayout(m_tasksLayout);
    m_tasksContainer->setWidget(tasksWidget);
    m_tasksContainer->setWidgetResizable(true);

    layout->addWidget(m_dayLabel);
    layout->addWidget(m_separator);
    layout->addWidget(m_tasksContainer, 1);
    setLayout(layout);
}

void DayCard::updateDisplay()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("DayCard", "Updating DayCard display", ctx);
    if (!m_date.isValid()) {
        auto ctxWarn = std::map<std::string, std::string>{};
        LOG_WARNING("DayCard", "Invalid date in DayCard::updateDisplay", ctxWarn);
        m_dayLabel->setText("Invalid Date");
        return;
    }

    m_dayLabel->setText(m_date.toString("dddd\nMMM d, yyyy"));

    // Clear existing tasks
    QLayoutItem *child;
    while ((child = m_tasksLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    // Add new tasks
    for (int i = 0; i < m_tasks.size(); i++) {
        const TaskInfo &task = m_tasks[i];
        QFrame* taskFrame = new QFrame(this);
        taskFrame->setObjectName("taskFrame");
        taskFrame->setProperty("class", "taskFrame");
        taskFrame->setFrameShape(QFrame::StyledPanel);
        taskFrame->setCursor(Qt::PointingHandCursor);

        QVBoxLayout* taskLayout = new QVBoxLayout(taskFrame);
        taskLayout->setSpacing(5);
        taskLayout->setContentsMargins(8, 8, 8, 8);

        QLabel* nameLabel = new QLabel(task.taskName, taskFrame);
        nameLabel->setObjectName("taskNameLabel");
        nameLabel->setProperty("class", "taskNameLabel");
        nameLabel->setWordWrap(true);

        QLabel* statusLabel = new QLabel(task.status, taskFrame);
        statusLabel->setObjectName("taskStatusLabel");
        statusLabel->setProperty("class", "taskStatusLabel");

        // Cycles display with star emojis
        QHBoxLayout* cyclesLayout = new QHBoxLayout();
        QLabel* cyclesLabel = new QLabel(taskFrame);
        cyclesLabel->setObjectName("cyclesLabel");
        cyclesLabel->setProperty("class", "cyclesLabel");
        
        // Create star emojis based on cycles
        QSettings settings;
        QString starEmoji = settings.value("starEmoji", "★").toString();
        QString stars;
        for (int i = 0; i < task.cycles; i++) {
            stars.append(starEmoji);
        }
        cyclesLabel->setText(stars);
        
        cyclesLayout->addWidget(cyclesLabel);
        cyclesLayout->addStretch();

        // Add start button
        QPushButton* startButton = new QPushButton("Start", taskFrame);
        startButton->setObjectName("startButton");
        startButton->setProperty("class", "startButton");
        startButton->setCursor(Qt::PointingHandCursor);
        startButton->setToolTip("Start task timer");
        startButton->setCursor(Qt::PointingHandCursor);
        connect(startButton, &QPushButton::clicked, [this, task]() {
            auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(task.taskId)}}};
            LOG_INFO("DayCard", "Start button clicked", ctx);
            emit startTaskClicked(task);
        });

        // Add edit button
        QPushButton* editButton = new QPushButton("Edit", taskFrame);
        editButton->setObjectName("editButton");
        editButton->setProperty("class", "editButton");
        editButton->setCursor(Qt::PointingHandCursor);
        editButton->setToolTip("Edit task");
        editButton->setCursor(Qt::PointingHandCursor);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        buttonLayout->addWidget(startButton);
        buttonLayout->addWidget(editButton);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(8);

        taskLayout->addWidget(nameLabel);
        taskLayout->addWidget(statusLabel);
        taskLayout->addLayout(cyclesLayout);
        taskLayout->addLayout(buttonLayout);

        m_tasksLayout->addWidget(taskFrame);
        taskFrame->installEventFilter(this);

        // Connect edit button
        connect(editButton, &QPushButton::clicked, [this, task]() {
            auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(task.taskId)}}};
            LOG_INFO("DayCard", "Edit button clicked", ctx);
            emit editTaskClicked(task);
        });
    }

    // Add stretch at the end
    m_tasksLayout->addStretch();
}

void DayCard::setTasks(const QList<TaskInfo> &tasks)
{
    auto ctx = std::map<std::string, std::string>{{{"count", std::to_string(tasks.size())}}};
    LOG_INFO("DayCard", "Tasks set", ctx);
    m_tasks = tasks;
    updateDisplay();
}

void DayCard::setDate(const QDate &date)
{
    auto ctx = std::map<std::string, std::string>{{{"date", date.toString("dd.MM.yyyy").toStdString()}}};
    LOG_INFO("DayCard", "Date set", ctx);
    if (date.isValid()) {
        m_date = date;
        updateDisplay();
    } else {
        auto ctxWarn = std::map<std::string, std::string>{};
        LOG_WARNING("DayCard", "Invalid date in DayCard::setDate", ctxWarn);
    }
}

bool DayCard::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *frame = qobject_cast<QFrame*>(obj);
        if (frame && frame->objectName() == "taskFrame") {
            int index = m_tasksLayout->indexOf(frame);
            if (index >= 0 && index < m_tasks.size()) {
                auto ctx = std::map<std::string, std::string>{{{"taskId", std::to_string(m_tasks[index].taskId)}}};
                LOG_INFO("DayCard", "Task clicked", ctx);
                emit taskClicked(m_tasks[index]);
                return true;
            }
        }
    }
    return QObject::eventFilter(obj, event);
}
