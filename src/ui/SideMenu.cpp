#include "SideMenu.h"
#include <QIcon>
#include <QDebug>
#include <QEvent>
#include <QStyle>
#include "../logging/logger.h"

SideMenu::SideMenu(QWidget *parent)
    : QWidget(parent), m_menuWidth(m_collapsedWidth), m_isOpen(false)
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SideMenu", "SideMenu created", ctx);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    initWidgets();
    initLayout();
    initConnections();
    updateMenuAppearance();

    setFixedWidth(m_menuWidth);

    m_animation = new QPropertyAnimation(this, "menuWidth", this);
    m_animation->setDuration(300);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    

    toggleMenu();
    toggleMenu();
}

SideMenu::~SideMenu() {
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SideMenu", "SideMenu destroyed", ctx);
}

void SideMenu::initWidgets()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SideMenu", "Initializing SideMenu widgets", ctx);
    menuButton = new QPushButton(this);
    menuButton->setToolTip(tr("Menu"));
    menuButton->setObjectName("menuButton");

    timerButton = new QPushButton(this);
    timerButton->setToolTip(tr("Timer"));
    timerButton->setObjectName("timerButton");

    histButton = new QPushButton(this);
    histButton->setToolTip(tr("History"));
    histButton->setObjectName("histButton");

    settButton = new QPushButton(this);
    settButton->setToolTip(tr("Settings"));
    settButton->setObjectName("settButton");

    profButton = new QPushButton(this);
    profButton->setToolTip(tr("Profile"));
    profButton->setObjectName("profButton");
}

void SideMenu::initLayout()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SideMenu", "Initializing SideMenu layout", ctx);
    menuLayout = new QVBoxLayout(this);
    menuLayout->setContentsMargins(0, 0, 0, 0);
    menuLayout->setSpacing(0);
    menuLayout->addWidget(menuButton);
    menuLayout->addWidget(timerButton);
    menuLayout->addWidget(histButton);
    menuLayout->addWidget(settButton);
    menuLayout->addWidget(profButton);
    menuLayout->addStretch();
    setLayout(menuLayout);
}

void SideMenu::initConnections()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SideMenu", "Initializing SideMenu connections", ctx);
    connect(menuButton, &QPushButton::clicked, this, &SideMenu::toggleMenu);
    connect(timerButton, &QPushButton::clicked, [this]() { 
        auto ctx = std::map<std::string, std::string>{{{"index", "0"}}};
        LOG_INFO("SideMenu", "Menu item clicked", ctx);
        emit menuItemClicked(0);
        emit pageShown(0);
    });
    connect(histButton, &QPushButton::clicked, [this]() { 
        auto ctx = std::map<std::string, std::string>{{{"index", "1"}}};
        LOG_INFO("SideMenu", "Menu item clicked", ctx);
        emit menuItemClicked(1);
        emit pageShown(1);
    });
    connect(settButton, &QPushButton::clicked, [this]() { 
        auto ctx = std::map<std::string, std::string>{{{"index", "2"}}};
        LOG_INFO("SideMenu", "Menu item clicked", ctx);
        emit menuItemClicked(2);
        emit pageShown(2);
    });
    connect(profButton, &QPushButton::clicked, [this]() { 
        auto ctx = std::map<std::string, std::string>{{{"index", "3"}}};
        LOG_INFO("SideMenu", "Menu item clicked", ctx);
        emit menuItemClicked(3);
        emit pageShown(3);
    });
}

void SideMenu::setMenuWidth(int width)
{
    auto ctx = std::map<std::string, std::string>{{{"width", std::to_string(width)}}};
    LOG_DEBUG("SideMenu", "Setting SideMenu width", ctx);
    m_menuWidth = width;
    setFixedWidth(m_menuWidth);
    //updateMenuAppearance();
}

void SideMenu::toggleMenu()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_INFO("SideMenu", "Toggling SideMenu", ctx);
    int startWidth = m_isOpen ? m_expandedWidth : m_collapsedWidth;
    int endWidth = m_isOpen ? m_collapsedWidth : m_expandedWidth;

    m_animation->stop();
    m_animation->setStartValue(startWidth);
    m_animation->setEndValue(endWidth);
    m_animation->start();

    m_isOpen = !m_isOpen;
    emit menuToggled(m_isOpen);
}

void SideMenu::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void SideMenu::retranslateUi()
{
    // Update tooltips
    menuButton->setToolTip(tr("Menu"));
    timerButton->setToolTip(tr("Timer"));
    histButton->setToolTip(tr("History"));
    settButton->setToolTip(tr("Settings"));
    profButton->setToolTip(tr("Profile"));
    
    // Update button texts if menu is open
    if (m_isOpen) {
        updateMenuAppearance();
    }
}

void SideMenu::updateMenuAppearance()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SideMenu", "Updating SideMenu appearance", ctx);
    
    // Set icons
    menuButton->setIcon(QIcon(":/icons/menu.png"));
    timerButton->setIcon(QIcon(":/icons/timer.png"));
    histButton->setIcon(QIcon(":/icons/history.png"));
    settButton->setIcon(QIcon(":/icons/settings.png"));
    profButton->setIcon(QIcon(":/icons/profile.png"));
    
    timerButton->setText(tr("Timer"));
    histButton->setText(tr("History"));
    settButton->setText(tr("Settings"));
    profButton->setText(tr("Profile"));
    
    // Update styles
    setStyleSheet("QPushButton { text-align: left; padding: 10px; border: none; }");
    
    // Update layout
    if (menuLayout) {
        menuLayout->update();
    }
    
    // Force update the widget
    updateGeometry();
    update();
}
