#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "SideMenu.h"
#include "pages/TaskPage.h"
#include "pages/HistoryPage.h"
#include "pages/SettingsPage.h"
#include "pages/ProfilePage.h"
#include "pages/StartPage.h"
#include "../core/TaskManager.h"
#include "ui/widgets/PomodoroTimer.h"
#include "components/TaskInfo.h"

class QStackedWidget;
class StartPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onMenuItemClicked(int index);
    void onLoginSuccess(int userId);
    void onLogoutRequested();
    void onStartTaskFromHistory(const TaskInfo &task);
    void onTaskStarted(int taskId, const QVariantMap &taskData);

private:
    void setupUi();
    void setupConnections();
    void showLoginPage();
    void showMainContent(int userId);

    SideMenu* m_sideMenu;
    QStackedWidget* m_pages;
    StartPage* m_startPage;
    int m_currentUserId;
    TaskManager* m_taskManager;
    HistoryPage* m_historyPage;
    TaskPage* m_taskPage;
    SettingsPage* m_settingsPage;
    ProfilePage* m_profilePage;
};

#endif // MAINWINDOW_H
