#include "SideMenu.h"
#include <QIcon>
#include <QDebug>
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
    menuButton->setToolTip("Menu");
    menuButton->setObjectName("menuButton");

    listButton = new QPushButton(this);
    listButton->setToolTip("Tasks");
    listButton->setObjectName("listButton");

    histButton = new QPushButton(this);
    histButton->setToolTip("History");
    histButton->setObjectName("histButton");

    settButton = new QPushButton(this);
    settButton->setToolTip("Settings");
    settButton->setObjectName("settButton");

    profButton = new QPushButton(this);
    profButton->setToolTip("Profile");
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
    menuLayout->addWidget(listButton);
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
    connect(listButton, &QPushButton::clicked, [this]() { 
        auto ctx = std::map<std::string, std::string>{{{"index", "0"}}};
        LOG_INFO("SideMenu", "Menu item clicked", ctx);
        emit menuItemClicked(0); 
    });
    connect(histButton, &QPushButton::clicked, [this]() { 
        auto ctx = std::map<std::string, std::string>{{{"index", "1"}}};
        LOG_INFO("SideMenu", "Menu item clicked", ctx);
        emit menuItemClicked(1); 
    });
    connect(settButton, &QPushButton::clicked, [this]() { 
        auto ctx = std::map<std::string, std::string>{{{"index", "2"}}};
        LOG_INFO("SideMenu", "Menu item clicked", ctx);
        emit menuItemClicked(2); 
    });
    connect(profButton, &QPushButton::clicked, [this]() { 
        auto ctx = std::map<std::string, std::string>{{{"index", "3"}}};
        LOG_INFO("SideMenu", "Menu item clicked", ctx);
        emit menuItemClicked(3); 
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

void SideMenu::updateMenuAppearance()
{
    auto ctx = std::map<std::string, std::string>{};
    LOG_DEBUG("SideMenu", "Updating SideMenu appearance", ctx);
    listButton->setText("List");
    histButton->setText("History");
    settButton->setText("Settings");
    profButton->setText("Profile");
}
