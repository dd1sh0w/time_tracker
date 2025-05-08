#include "MainWindow.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QThread>
#include "logging/logger.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_currentUserId(-1), m_startPage(nullptr)
{
    LOG_INFO("MainWindow", "Initializing MainWindow", {});
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("time_tracker");
    setupUi();
    setupConnections();
    showLoginPage();
    LOG_INFO("MainWindow", "MainWindow initialized and login page shown", {});
}

MainWindow::~MainWindow() { 
    LOG_INFO("MainWindow", "MainWindow destroyed", {});
}

void MainWindow::setupUi()
{
    LOG_DEBUG("MainWindow", "Setting up UI", {});
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_sideMenu = new SideMenu(this);
    m_sideMenu->setObjectName("sideMenu");
    m_sideMenu->hide();
    
    m_pages = new QStackedWidget(this);
    m_pages->setObjectName("stackedWidget");
    
    m_startPage = new StartPage(this);
    m_startPage->setObjectName("startPage");

    mainLayout->addWidget(m_sideMenu);
    mainLayout->addWidget(m_pages);

    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    LOG_DEBUG("MainWindow", "UI setup complete", {});
}

void MainWindow::setupConnections()
{
    LOG_DEBUG("MainWindow", "Setting up connections", {});
    connect(m_sideMenu, &SideMenu::menuItemClicked, this, &MainWindow::onMenuItemClicked);
    connect(m_startPage, &StartPage::loginSuccess, this, &MainWindow::onLoginSuccess);
    
    // Connect TaskManager signals to TaskPage slots
    if (m_taskPage) {
        connect(m_taskManager, &TaskManager::taskStarted, m_taskPage, &TaskPage::onTaskStarted);
        connect(m_taskManager, &TaskManager::activeTaskChanged, m_taskPage, &TaskPage::onActiveTaskChanged);
    }
    
    LOG_DEBUG("MainWindow", "Connections setup complete", {});
}

void MainWindow::showLoginPage()
{
    LOG_INFO("MainWindow", "Showing login page", {});
    // Hide side menu first
    m_sideMenu->hide();
    
    // Remove all pages except start page
    while (m_pages->count() > 0) {
        QWidget *widget = m_pages->widget(0);
        if (widget != m_startPage) {
            widget->disconnect();
            m_pages->removeWidget(widget);
            widget->deleteLater();
        } else {
            m_pages->removeWidget(widget);
        }
    }
    
    // Add and show start page
    m_pages->addWidget(m_startPage);
    m_pages->setCurrentWidget(m_startPage);

    // Try auto-login
    if (!m_startPage->tryAutoLogin()) {
        LOG_DEBUG("MainWindow", "Auto-login not possible", {});
        // No auto-login possible, stay on login page
    } else {
        LOG_DEBUG("MainWindow", "Auto-login succeeded", {});
    }
}

void MainWindow::showMainContent(int userId)
{
    auto ctxUserId = std::map<std::string, std::string>{{"userId", std::to_string(userId)}};
    LOG_INFO("MainWindow", "Showing main content", ctxUserId);
    m_currentUserId = userId;
    
    // Remove all pages except start page
    while (m_pages->count() > 0) {
        QWidget *widget = m_pages->widget(0);
        if (widget != m_startPage) {
            widget->disconnect();
            m_pages->removeWidget(widget);
            widget->deleteLater();
        } else {
            m_pages->removeWidget(widget);
        }
    }
    
    m_taskPage = new TaskPage(userId, this, m_taskManager);
    m_taskPage->setObjectName("taskPage");

    // Create and add all main pages
    m_historyPage = new HistoryPage(userId, this, m_taskManager);
    m_historyPage->setObjectName("historyPage");
    connect(m_historyPage, &HistoryPage::startTaskClicked, this, &MainWindow::onStartTaskFromHistory);

    m_settingsPage = new SettingsPage(this);
    m_settingsPage->setObjectName("settingsPage");
    
    m_profilePage = new ProfilePage(userId, this);
    m_profilePage->setObjectName("profilePage");
    
    // Connect logout signal from profile page
    connect(m_profilePage, &ProfilePage::logoutRequested, this, &MainWindow::onLogoutRequested, Qt::QueuedConnection);

    m_pages->addWidget(m_taskPage);
    m_pages->addWidget(m_historyPage);
    m_pages->addWidget(m_settingsPage);
    m_pages->addWidget(m_profilePage);

    // Show side menu and first page
    m_sideMenu->show();
    m_pages->setCurrentIndex(0);
    LOG_INFO("MainWindow", "Main content shown", ctxUserId);
}

void MainWindow::onMenuItemClicked(int index)
{
    auto ctxIndex = std::map<std::string, std::string>{{"index", std::to_string(index)}};
    LOG_INFO("MainWindow", "Menu item clicked", ctxIndex);
    if (m_currentUserId == -1) {
        LOG_WARNING("MainWindow", "Menu item clicked but user not logged in", {});
        return;
    }
    m_pages->setCurrentIndex(index);
}

void MainWindow::onLoginSuccess(int userId)
{
    auto ctxLogin = std::map<std::string, std::string>{{"userId", std::to_string(userId)}};
    LOG_INFO("MainWindow", "Login success", ctxLogin);
    m_taskManager = new TaskManager(userId, this);
    LOG_INFO("MainWindow", "TaskManager created after login", ctxLogin);
    showMainContent(userId);
}

void MainWindow::onLogoutRequested()
{
    auto ctxLogout = std::map<std::string, std::string>{{"userId", std::to_string(m_currentUserId)}};
    LOG_INFO("MainWindow", "Logout requested", ctxLogout);
    // First set current user id to -1
    m_currentUserId = -1;
    m_taskManager = nullptr;
    // Then show login page, which will clean up other pages
    showLoginPage();
    LOG_INFO("MainWindow", "User logged out and login page shown", {});
}

void MainWindow::onTaskStarted(int taskId, const QVariantMap &taskData)
{
    auto ctxTask = std::map<std::string, std::string>{{"taskId", std::to_string(taskId)}};
    LOG_INFO("MainWindow", "Task started", ctxTask);
    
    // Update PomodoroTimer with task information
    if (PomodoroTimer* timer = findChild<PomodoroTimer*>()) {
        timer->setActiveTaskName(taskData["name"].toString());
        timer->startTimer();
    }
}

void MainWindow::onStartTaskFromHistory(const TaskInfo &task) {
    auto ctxTaskId = std::map<std::string, std::string>{{"taskId", std::to_string(task.taskId)}};
    LOG_INFO("MainWindow", "Start task from history", ctxTaskId);
    m_pages->setCurrentIndex(0);
    //add dealy on 10 milisecond
    QThread::msleep(10);
    m_taskManager->setActiveTask(task.taskId);
    m_taskPage->startTask(task);
    LOG_INFO("MainWindow", "Task started from history", ctxTaskId);
}