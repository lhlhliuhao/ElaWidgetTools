#ifndef ELAWIDGET_H
#define ELAWIDGET_H

#include <QWidget>

#include "Def.h"
#include "ElaAppBar.h"
class ElaWidgetPrivate;
class ELA_EXPORT ElaWidget : public QWidget
{
    Q_OBJECT
    Q_Q_CREATE(ElaWidget)
    Q_PROPERTY_CREATE_Q_H(bool, IsStayTop)
    Q_PROPERTY_CREATE_Q_H(bool, IsFixedSize)
    Q_PROPERTY_CREATE_Q_H(bool, IsDefaultClosed)
    Q_TAKEOVER_NATIVEEVENT_H
public:
    explicit ElaWidget(QWidget* parent = nullptr);
    ~ElaWidget();
    /**
     * @brief 移动到窗口当前所在屏幕的中心，如果窗口不在任意屏幕中则移动到里窗口中心距离最近的屏幕中心
     */
    void moveToCenter();
    /**
     * @brief 移动到鼠标所在屏幕的中心
     */
    void moveToMouseScreenCenter();
    void adjustWindowSizeWithScreen();

    void setWindowButtonFlag(ElaAppBarType::ButtonType buttonFlag, bool isEnable = true);
    void setWindowButtonFlags(ElaAppBarType::ButtonFlags buttonFlags);
    ElaAppBarType::ButtonFlags getWindowButtonFlags() const;
Q_SIGNALS:
    Q_SIGNAL void routeBackButtonClicked();
    Q_SIGNAL void navigationButtonClicked();
    Q_SIGNAL void themeChangeButtonClicked();
    Q_SIGNAL void closeButtonClicked();

protected:
    virtual void paintEvent(QPaintEvent* event) override;
};

#endif // ELAWIDGET_H
