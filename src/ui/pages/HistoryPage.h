#ifndef HISTORYPAGE_H
#define HISTORYPAGE_H

#include <QWidget>
#include <QDate>
#include <QDateTime>
#include <QMap>
#include "../../core/TaskSettingsDialog.h"
#include "TaskPage.h"
#include "../components/TaskInfo.h"
#include "../../core/TaskManager.h"

class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QLabel;
class QFrame;
class DayCard;


// Main history page widget
class HistoryPage : public QWidget {
    Q_OBJECT
public:
    explicit HistoryPage(int userId, QWidget *parent = nullptr, TaskManager* taskManager = nullptr);
    ~HistoryPage();
    void loadHistory();

signals:
    void taskClicked(const TaskInfo &task);
    void editTaskClicked(const TaskInfo &task);
    void startTaskClicked(const TaskInfo &task);

private slots:
    // Navigation
    void shiftLeft();
    void shiftRight();
    void chooseDay();
    
    // Task interaction
    void onTaskClicked(const TaskInfo &task);
    void onTaskUpdated(const QVariantMap &taskData);
    void onAddTaskClicked();
    void onEditTaskClicked(const TaskInfo &task);
    void onTaskDeleted(int taskId);
    int onStartTaskClicked(const TaskInfo& task);

private:
    void setupUi();
    void updateDayCards();
    QString formatTaskDetails(const TaskInfo &task) const;

    int m_userId;
    QDate m_baseDate;
    int m_firstCardIndex;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout *m_dayCardsLayout;
    TaskManager* m_taskManager;


    // Navigation buttons
    QPushButton *m_leftButton;
    QPushButton *m_rightButton;
    QPushButton *m_chooseDayButton;
    QPushButton *m_addTaskButton;
    QPushButton *m_editTaskButton;
    

    // Day cards
    QList<DayCard*> m_dayCards;

    // Task details display
    QFrame *m_detailsFrame;
    QLabel *m_detailsLabel;

    // History data storage
    QMap<QDate, QList<TaskInfo>> m_historyData;
};

#endif // HISTORYPAGE_H
