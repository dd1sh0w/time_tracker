#ifndef SIDEMENU_H
#define SIDEMENU_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPropertyAnimation>

class SideMenu : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int menuWidth READ menuWidth WRITE setMenuWidth)

public:
    explicit SideMenu(QWidget *parent = nullptr);
    ~SideMenu();
    
    void toggleMenu();
    bool isMenuOpen() const { return m_isOpen; }

    int menuWidth() const { return m_menuWidth; }
    void setMenuWidth(int width);

signals:
    void menuItemClicked(int index);
    void menuToggled(bool isOpen);
    void pageShown(int index);

private:
    void initWidgets();
    void initLayout();
    void initConnections();
    void updateMenuAppearance();
    
protected:
    void changeEvent(QEvent *event) override;
    
public:
    void retranslateUi();

    QPushButton *menuButton;
    QPushButton *timerButton;
    QPushButton *histButton;
    QPushButton *settButton;
    QPushButton *profButton;

    QVBoxLayout *menuLayout;
    QPropertyAnimation *m_animation;

    int m_menuWidth;
    bool m_isOpen;

    const int m_collapsedWidth = 50;
    const int m_expandedWidth = 200;
};

#endif // SIDEMENU_H
