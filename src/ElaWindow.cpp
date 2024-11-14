#include "ElaWindow.h"

#include <QApplication>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QScreen>
#include <QStackedWidget>
#include <QStyleOption>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QtMath>

#include "ElaApplication.h"
#include "ElaCentralStackedWidget.h"
#include "ElaEventBus.h"
#include "ElaMenu.h"
#include "ElaNavigationBar.h"
#include "ElaNavigationRouter.h"
#include "ElaTheme.h"
#include "ElaWindowStyle.h"
#include "private/ElaAppBarPrivate.h"
#include "private/ElaNavigationBarPrivate.h"
#include "private/ElaWindowPrivate.h"
Q_PROPERTY_CREATE_Q_CPP(ElaWindow, int, ThemeChangeTime)
Q_PROPERTY_CREATE_Q_CPP(ElaWindow, ElaNavigationType::NavigationDisplayMode, NavigationBarDisplayMode)
Q_TAKEOVER_NATIVEEVENT_CPP(ElaWindow, d_func()->_appBar);
ElaWindow::ElaWindow(QWidget* parent)
    : QMainWindow{parent}, d_ptr(new ElaWindowPrivate())
{
    Q_D(ElaWindow);
    d->q_ptr = this;

    setProperty("ElaBaseClassName", "ElaWindow");
    resize(1020, 680); // 默认宽高

    d->_pThemeChangeTime = 700;
    d->_pNavigationBarDisplayMode = ElaNavigationType::NavigationDisplayMode::Auto;
    connect(this, &ElaWindow::pNavigationBarDisplayModeChanged, d, &ElaWindowPrivate::onDisplayModeChanged);

    // 自定义AppBar
    d->_appBar = new ElaAppBar(this);
    connect(d->_appBar, &ElaAppBar::routeBackButtonClicked, this, []() {
        ElaNavigationRouter::getInstance()->navigationRouteBack();
    });
    connect(d->_appBar, &ElaAppBar::closeButtonClicked, this, &ElaWindow::closeButtonClicked);
    // 导航栏
    d->_navigationBar = new ElaNavigationBar(this);
    // 返回按钮状态变更
    connect(ElaNavigationRouter::getInstance(), &ElaNavigationRouter::navigationRouterStateChanged, this, [d](bool isEnable) {
        d->_appBar->setRouteBackButtonEnable(isEnable);
    });

    // 转发用户卡片点击信号
    connect(d->_navigationBar, &ElaNavigationBar::userInfoCardClicked, this, &ElaWindow::userInfoCardClicked);
    // 转发点击信号
    connect(d->_navigationBar, &ElaNavigationBar::navigationNodeClicked, this, &ElaWindow::navigationNodeClicked);
    //跳转处理
    connect(d->_navigationBar, &ElaNavigationBar::navigationNodeClicked, d, &ElaWindowPrivate::onNavigationNodeClicked);
    //新增窗口
    connect(d->_navigationBar, &ElaNavigationBar::navigationNodeAdded, d, &ElaWindowPrivate::onNavigationNodeAdded);

    // 中心堆栈窗口
    d->_centerStackedWidget = new ElaCentralStackedWidget(this);
    d->_centerStackedWidget->setContentsMargins(0, 0, 0, 0);
    QWidget* centralWidget = new QWidget(this);
    d->_centerLayout = new QHBoxLayout(centralWidget);
    d->_centerLayout->setSpacing(0);
    d->_centerLayout->addWidget(d->_navigationBar);
    d->_centerLayout->addWidget(d->_centerStackedWidget);
    d->_centerLayout->setContentsMargins(d->_contentsMargins, 0, 0, 0);

    // 事件总线
    d->_focusEvent = new ElaEvent("WMWindowClicked", "onWMWindowClickedEvent", d);
    d->_focusEvent->registerAndInit();

    // 展开导航栏
    connect(d->_appBar, &ElaAppBar::navigationButtonClicked, d, &ElaWindowPrivate::onNavigationButtonClicked);

    // 主题变更动画
    d->_themeMode = eTheme->getThemeMode();
    connect(eTheme, &ElaTheme::themeModeChanged, d, &ElaWindowPrivate::onThemeModeChanged);
    connect(d->_appBar, &ElaAppBar::themeChangeButtonClicked, d, &ElaWindowPrivate::onThemeReadyChange);
    d->_isInitFinished = true;
    setCentralWidget(centralWidget);
    centralWidget->installEventFilter(this);

    setObjectName("ElaWindow");
    setStyleSheet("#ElaWindow{background-color:transparent;}");
    setStyle(new ElaWindowStyle(style()));

    //延时渲染
    QTimer::singleShot(1, this, [=] {
        QPalette palette = this->palette();
        palette.setBrush(QPalette::Window, ElaThemeColor(d->_themeMode, WindowBase));
        this->setPalette(palette);
    });
    eApp->syncMica(this);
    connect(eApp, &ElaApplication::pIsEnableMicaChanged, this, [=]() {
        d->onThemeModeChanged(d->_themeMode);
    });
}

ElaWindow::~ElaWindow()
{
}

void ElaWindow::setIsStayTop(bool isStayTop)
{
    Q_D(ElaWindow);
    d->_appBar->setIsStayTop(isStayTop);
    Q_EMIT pIsStayTopChanged();
}

bool ElaWindow::getIsStayTop() const
{
    return d_ptr->_appBar->getIsStayTop();
}

void ElaWindow::setIsFixedSize(bool isFixedSize)
{
    Q_D(ElaWindow);
    d->_appBar->setIsFixedSize(isFixedSize);
    Q_EMIT pIsFixedSizeChanged();
}

bool ElaWindow::getIsFixedSize() const
{
    return d_ptr->_appBar->getIsFixedSize();
}

void ElaWindow::setIsDefaultClosed(bool isDefaultClosed)
{
    Q_D(ElaWindow);
    d->_appBar->setIsDefaultClosed(isDefaultClosed);
    Q_EMIT pIsDefaultClosedChanged();
}

bool ElaWindow::getIsDefaultClosed() const
{
    return d_ptr->_appBar->getIsDefaultClosed();
}

void ElaWindow::setAppBarHeight(int appBarHeight)
{
    Q_D(ElaWindow);
    d->_appBar->setAppBarHeight(appBarHeight);
    Q_EMIT pAppBarHeightChanged();
}

QWidget* ElaWindow::getCustomWidget() const
{
    Q_D(const ElaWindow);
    return d->_appBar->getCustomWidget();
}

void ElaWindow::setCustomWidgetMaximumWidth(int width)
{
    Q_D(ElaWindow);
    d->_appBar->setCustomWidgetMaximumWidth(width);
    Q_EMIT pCustomWidgetMaximumWidthChanged();
}

int ElaWindow::getCustomWidgetMaximumWidth() const
{
    Q_D(const ElaWindow);
    return d->_appBar->getCustomWidgetMaximumWidth();
}

void ElaWindow::setIsCentralStackedWidgetTransparent(bool isTransparent)
{
    Q_D(ElaWindow);
    d->_centerStackedWidget->setIsTransparent(isTransparent);
}

bool ElaWindow::getIsCentralStackedWidgetTransparent() const
{
    Q_D(const ElaWindow);
    return d->_centerStackedWidget->getIsTransparent();
}

void ElaWindow::moveToCenter()
{
    if (isMaximized() || isFullScreen())
    {
        return;
    }

    QPoint widgetCenter = this->geometry().center();
    QScreen *targetScreen = nullptr;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    targetScreen = screen();//QGuiApplication::screenAt(widgetCenter);
#else
    targetScreen = qApp->screenAt(widgetCenter);
#endif

    // 如果 widgetCenter 不在任何屏幕内，则找距离最近的屏幕
    if (!targetScreen)
    {
        int minDistance = INT_MAX;
        foreach (QScreen *screen, QGuiApplication::screens()) {
            QPoint screenCenter = screen->geometry().center();
            int distance = qSqrt(qPow(screenCenter.x() - widgetCenter.x(), 2) +
                                 qPow(screenCenter.y() - widgetCenter.y(), 2));

            if (distance < minDistance)
            {
                minDistance = distance;
                targetScreen = screen;
            }
        }
    }

    // 将窗口移动到目标屏幕的中心
    if (targetScreen)
    {
        QRect screenGeometry = targetScreen->availableGeometry();
        int newX = (screenGeometry.left() + screenGeometry.right() - width()) / 2;
        int newY = (screenGeometry.top() + screenGeometry.bottom() - height()) / 2;
        setGeometry(newX, newY, width(), height());
    }
}

void ElaWindow::moveToMouseScreenCenter()
{
    if (isMaximized() || isFullScreen())
    {
        return;
    }

    // 获取鼠标当前的位置
    QPoint mousePos = QCursor::pos();
    QScreen *targetScreen = nullptr;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    targetScreen = QGuiApplication::screenAt(mousePos);
#else
    targetScreen = qApp->screenAt(mousePos);
#endif

    // 如果 widgetCenter 不在任何屏幕内，则找距离最近的屏幕
    if (!targetScreen)
    {
        int minDistance = INT_MAX;
        foreach (QScreen *screen, QGuiApplication::screens()) {
            QPoint screenCenter = screen->geometry().center();
            int distance = qSqrt(qPow(screenCenter.x() - mousePos.x(), 2) +
                                 qPow(screenCenter.y() - mousePos.y(), 2));

            if (distance < minDistance)
            {
                minDistance = distance;
                targetScreen = screen;
            }
        }
    }

    // 将窗口移动到目标屏幕的中心
    if (targetScreen)
    {
        QRect screenGeometry = targetScreen->availableGeometry();
        int newX = (screenGeometry.left() + screenGeometry.right() - width()) / 2;
        int newY = (screenGeometry.top() + screenGeometry.bottom() - height()) / 2;
        setGeometry(newX, newY, width(), height());
    }
}

void ElaWindow::adjustWindowSizeWithScreen()
{
    // 获取当前窗口的几何尺寸
    QRect windowGeometry = this->geometry();
    int currentWidth = windowGeometry.width();
    int currentHeight = windowGeometry.height();
    QPoint windowCenter = windowGeometry.center();

    // 获取当前屏幕的可用几何区域
    QScreen *screen = QGuiApplication::screenAt(windowCenter);
    if (!screen)
    {
        // 如果没有找到屏幕，直接返回
        return;
    }

    QRect screenGeometry = screen->availableGeometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // 调整窗口宽度，如果当前宽度大于屏幕宽度，则设置为屏幕宽度
    int newWidth = currentWidth > screenWidth ? screenWidth : currentWidth;

    // 调整窗口高度，如果当前高度大于屏幕高度，则设置为屏幕高度
    int newHeight = currentHeight > screenHeight ? screenHeight : currentHeight;

    // 计算调整后的窗口左上角位置，确保窗口中心不变
    int newX = windowCenter.x() - newWidth / 2;
    int newY = windowCenter.y() - newHeight / 2;

    // 如果计算后的左上角位置超出了屏幕范围，进行修正
    newX = std::max(newX, screenGeometry.left());
    newY = std::max(newY, screenGeometry.top());

    // 设置新的窗口大小和位置
    this->setGeometry(newX, newY, newWidth, newHeight);
}

void ElaWindow::setCustomWidget(ElaAppBarType::CustomArea customArea, QWidget* widget)
{
    Q_D(ElaWindow);
    d->_appBar->setCustomWidget(customArea, widget);
    Q_EMIT customWidgetChanged();
}

int ElaWindow::getAppBarHeight() const
{
    Q_D(const ElaWindow);
    return d->_appBar->getAppBarHeight();
}

void ElaWindow::setIsNavigationBarEnable(bool isVisible)
{
    Q_D(ElaWindow);
    d->_isNavigationEnable = isVisible;
    d->_navigationBar->setVisible(isVisible);
    d->_centerLayout->setContentsMargins(isVisible ? d->_contentsMargins : 0, 0, 0, 0);
    d->_centerStackedWidget->setIsHasRadius(isVisible);
}

bool ElaWindow::getIsNavigationBarEnable() const
{
    return d_ptr->_isNavigationEnable;
}

void ElaWindow::setUserInfoCardVisible(bool isVisible)
{
    Q_D(ElaWindow);
    d->_navigationBar->setUserInfoCardVisible(isVisible);
}

void ElaWindow::setUserInfoCardPixmap(QPixmap pix)
{
    Q_D(ElaWindow);
    d->_navigationBar->setUserInfoCardPixmap(pix);
}

void ElaWindow::setUserInfoCardTitle(QString title)
{
    Q_D(ElaWindow);
    d->_navigationBar->setUserInfoCardTitle(title);
}

void ElaWindow::setUserInfoCardSubTitle(QString subTitle)
{
    Q_D(ElaWindow);
    d->_navigationBar->setUserInfoCardSubTitle(subTitle);
}

ElaNavigationType::NodeOperateReturnType ElaWindow::addExpanderNode(QString expanderTitle, QString& expanderKey, ElaIconType::IconName awesome) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->addExpanderNode(expanderTitle, expanderKey, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaWindow::addExpanderNode(QString expanderTitle, QString& expanderKey, QString targetExpanderKey, ElaIconType::IconName awesome) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->addExpanderNode(expanderTitle, expanderKey, targetExpanderKey, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaWindow::addPageNode(QString pageTitle, QWidget* page, ElaIconType::IconName awesome) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->addPageNode(pageTitle, page, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaWindow::addPageNode(QString pageTitle, QWidget* page, QString targetExpanderKey, ElaIconType::IconName awesome) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->addPageNode(pageTitle, page, targetExpanderKey, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaWindow::addPageNode(QString pageTitle, QWidget* page, int keyPoints, ElaIconType::IconName awesome) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->addPageNode(pageTitle, page, keyPoints, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaWindow::addPageNode(QString pageTitle, QWidget* page, QString targetExpanderKey, int keyPoints, ElaIconType::IconName awesome) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->addPageNode(pageTitle, page, targetExpanderKey, keyPoints, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaWindow::addFooterNode(QString footerTitle, QString& footerKey, int keyPoints, ElaIconType::IconName awesome) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->addFooterNode(footerTitle, nullptr, footerKey, keyPoints, awesome);
}

ElaNavigationType::NodeOperateReturnType ElaWindow::addFooterNode(QString footerTitle, QWidget* page, QString& footerKey, int keyPoints, ElaIconType::IconName awesome) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->addFooterNode(footerTitle, page, footerKey, keyPoints, awesome);
}

void ElaWindow::setNodeKeyPoints(QString nodeKey, int keyPoints)
{
    Q_D(ElaWindow);
    d->_navigationBar->setNodeKeyPoints(nodeKey, keyPoints);
}

int ElaWindow::getNodeKeyPoints(QString nodeKey) const
{
    Q_D(const ElaWindow);
    return d->_navigationBar->getNodeKeyPoints(nodeKey);
}

void ElaWindow::navigation(QString pageKey)
{
    Q_D(ElaWindow);
    d->_navigationBar->navigation(pageKey);
}

void ElaWindow::setWindowButtonFlag(ElaAppBarType::ButtonType buttonFlag, bool isEnable)
{
    Q_D(ElaWindow);
    d->_appBar->setWindowButtonFlag(buttonFlag, isEnable);
}

void ElaWindow::setWindowButtonFlags(ElaAppBarType::ButtonFlags buttonFlags)
{
    Q_D(ElaWindow);
    d->_appBar->setWindowButtonFlags(buttonFlags);
}

ElaAppBarType::ButtonFlags ElaWindow::getWindowButtonFlags() const
{
    return d_ptr->_appBar->getWindowButtonFlags();
}

void ElaWindow::closeWindow()
{
    Q_D(ElaWindow);
    d->_isWindowClosing = true;
    d->_appBar->closeWindow();
}

bool ElaWindow::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ElaWindow);
    switch (event->type())
    {
    case QEvent::Resize:
    {
        d->_doNavigationDisplayModeChange();
        break;
    }
    default:
    {
        break;
    }
    }
    return QMainWindow::eventFilter(watched, event);
}

QMenu* ElaWindow::createPopupMenu()
{
    ElaMenu* menu = nullptr;
    QList<QDockWidget*> dockwidgets = findChildren<QDockWidget*>();
    if (dockwidgets.size())
    {
        menu = new ElaMenu(this);
        for (int i = 0; i < dockwidgets.size(); ++i)
        {
            QDockWidget* dockWidget = dockwidgets.at(i);
            if (dockWidget->parentWidget() == this)
            {
                menu->addAction(dockwidgets.at(i)->toggleViewAction());
            }
        }
        menu->addSeparator();
    }

    QList<QToolBar*> toolbars = findChildren<QToolBar*>();
    if (toolbars.size())
    {
        if (!menu)
        {
            menu = new ElaMenu(this);
        }
        for (int i = 0; i < toolbars.size(); ++i)
        {
            QToolBar* toolBar = toolbars.at(i);
            if (toolBar->parentWidget() == this)
            {
                menu->addAction(toolbars.at(i)->toggleViewAction());
            }
        }
    }
    if (menu)
    {
        menu->setMenuItemHeight(28);
    }
    return menu;
}
