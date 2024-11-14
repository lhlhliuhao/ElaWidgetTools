#include "ElaWidget.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QPainter>
#include <QScreen>
#include <QVBoxLayout>
#include <QtMath>

#include "ElaApplication.h"
#include "ElaTheme.h"
#include "private/ElaWidgetPrivate.h"
Q_TAKEOVER_NATIVEEVENT_CPP(ElaWidget, d_func()->_appBar);
ElaWidget::ElaWidget(QWidget* parent)
    : QWidget{parent}, d_ptr(new ElaWidgetPrivate())
{
    Q_D(ElaWidget);
    d->q_ptr = this;
    resize(500, 500); // 默认宽高
    setWindowTitle("ElaWidget");
    setObjectName("ElaWidget");

    // 自定义AppBar
    d->_appBar = new ElaAppBar(this);
    d->_appBar->setIsStayTop(true);
    d->_appBar->setWindowButtonFlags(ElaAppBarType::StayTopButtonHint | ElaAppBarType::MinimizeButtonHint | ElaAppBarType::MaximizeButtonHint | ElaAppBarType::CloseButtonHint);
    connect(d->_appBar, &ElaAppBar::routeBackButtonClicked, this, &ElaWidget::routeBackButtonClicked);
    connect(d->_appBar, &ElaAppBar::navigationButtonClicked, this, &ElaWidget::navigationButtonClicked);
    connect(d->_appBar, &ElaAppBar::themeChangeButtonClicked, this, &ElaWidget::themeChangeButtonClicked);
    connect(d->_appBar, &ElaAppBar::closeButtonClicked, this, &ElaWidget::closeButtonClicked);

    // 主题
    d->_themeMode = eTheme->getThemeMode();
    connect(eTheme, &ElaTheme::themeModeChanged, this, [=](ElaThemeType::ThemeMode themeMode) {
        d->_themeMode = themeMode;
        update();
    });

    d->_isEnableMica = eApp->getIsEnableMica();
    connect(eApp, &ElaApplication::pIsEnableMicaChanged, this, [=]() {
        d->_isEnableMica = eApp->getIsEnableMica();
        update();
    });
    eApp->syncMica(this);
}

ElaWidget::~ElaWidget()
{
}

void ElaWidget::setIsStayTop(bool isStayTop)
{
    Q_D(ElaWidget);
    d->_appBar->setIsStayTop(isStayTop);
}

bool ElaWidget::getIsStayTop() const
{
    return d_ptr->_appBar->getIsStayTop();
}

void ElaWidget::setIsFixedSize(bool isFixedSize)
{
    Q_D(ElaWidget);
    d->_appBar->setIsFixedSize(isFixedSize);
}

bool ElaWidget::getIsFixedSize() const
{
    return d_ptr->_appBar->getIsFixedSize();
}

void ElaWidget::setIsDefaultClosed(bool isDefaultClosed)
{
    Q_D(ElaWidget);
    d->_appBar->setIsDefaultClosed(isDefaultClosed);
    Q_EMIT pIsDefaultClosedChanged();
}

bool ElaWidget::getIsDefaultClosed() const
{
    Q_D(const ElaWidget);
    return d->_appBar->getIsDefaultClosed();
}

void ElaWidget::moveToCenter()
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

void ElaWidget::moveToMouseScreenCenter()
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

void ElaWidget::adjustWindowSizeWithScreen()
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

void ElaWidget::setWindowButtonFlag(ElaAppBarType::ButtonType buttonFlag, bool isEnable)
{
    Q_D(ElaWidget);
    d->_appBar->setWindowButtonFlag(buttonFlag, isEnable);
}

void ElaWidget::setWindowButtonFlags(ElaAppBarType::ButtonFlags buttonFlags)
{
    Q_D(ElaWidget);
    d->_appBar->setWindowButtonFlags(buttonFlags);
}

ElaAppBarType::ButtonFlags ElaWidget::getWindowButtonFlags() const
{
    return d_ptr->_appBar->getWindowButtonFlags();
}

void ElaWidget::paintEvent(QPaintEvent* event)
{
    Q_D(ElaWidget);
    if (!d->_isEnableMica)
    {
        QPainter painter(this);
        painter.save();
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ElaThemeColor(d->_themeMode, WindowBase));
        painter.drawRect(rect());
        painter.restore();
    }
    QWidget::paintEvent(event);
}
