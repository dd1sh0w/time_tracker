#ifndef POMODORO_TIMER_H
#define POMODORO_TIMER_H

#include <QWidget>
#include <QTimer>
#include <QSystemTrayIcon>

class QPushButton;
class QLabel;
class TaskManager;

enum class PomodoroPhase {
    Work,           // 25 minutes
    ShortBreak,     // 5 minutes
    LongBreak       // 20 minutes
};

inline const char* toString(PomodoroPhase phase) {
    switch (phase) {
        case PomodoroPhase::Work: return "Work";
        case PomodoroPhase::ShortBreak: return "ShortBreak";
        case PomodoroPhase::LongBreak: return "LongBreak";
        default: return "Work";
    }
}

class PomodoroTimer : public QWidget
{
    Q_OBJECT
public:
    explicit PomodoroTimer(QWidget *parent = nullptr, TaskManager *taskManager = nullptr);
    ~PomodoroTimer();

signals:
    void phaseCompleted(PomodoroPhase phase);
    void cycleCompleted(); // Emitted after 4 work phases
    void activeTaskChanged(const QString &taskName); // Emitted when active task changes

public slots:
    void startTimer();
    void pauseTimer();
    void resetTimer();
    void skipPhase();
    void setActiveTaskName(const QString &name);
    void onTimerTick(); // Новый слот для обработки тика таймера

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void showNotification(const QString &title, const QString &message);
    static const int WORK_DURATION = 25 * 60;        // 25 minutes in seconds
    static const int SHORT_BREAK_DURATION = 5 * 60;  // 5 minutes in seconds
    static const int LONG_BREAK_DURATION = 20 * 60;  // 20 minutes in seconds
    static const int POMODOROS_BEFORE_LONG_BREAK = 4;

    void setupUi();
    void updateTimerDisplay();
    void updateProgressCircle();
    void startNextPhase();
    QString formatTime(int seconds) const;
    void updateButtonStates();
    void updateCyclesDisplay(); // Новый метод для отображения циклов

    QTimer m_timer;
    PomodoroPhase m_currentPhase;
    int m_remainingSeconds;
    int m_completedPomodoros;
    bool m_isRunning;
    QString m_activeTaskName;

    // UI elements
    QPushButton *m_startPauseButton;
    QPushButton *m_resetButton;
    QPushButton *m_skipButton;
    QLabel *m_timeLabel;
    QLabel *m_phaseLabel;
    QLabel *m_cycleLabel;
    QLabel *m_activeTaskNameLabel;

    // Circle progress parameters
    int m_radius;
    QPoint m_center;
    int m_currentPhaseDuration;
    int m_taskRemainingCycles = 0; // Храним оставшиеся циклы задачи
    TaskManager *m_taskManager; // Указатель на TaskManager
    
    // Style properties
    Q_PROPERTY(int timerCircleWidth READ timerCircleWidth WRITE setTimerCircleWidth)
    int m_timerCircleWidth = 8; // Default width in pixels
    
    // Property getters/setters
    int timerCircleWidth() const { return m_timerCircleWidth; }
    void setTimerCircleWidth(int width) { m_timerCircleWidth = width; update(); }
};

#endif // POMODORO_TIMER_H
