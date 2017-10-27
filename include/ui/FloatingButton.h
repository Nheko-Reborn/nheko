#pragma once

#include "RaisedButton.h"

constexpr int DIAMETER  = 40;
constexpr int ICON_SIZE = 18;

constexpr int OFFSET_X = 30;
constexpr int OFFSET_Y = 20;

class FloatingButton : public RaisedButton
{
        Q_OBJECT

public:
        FloatingButton(const QIcon &icon, QWidget *parent = nullptr);

        QSize sizeHint() const override { return QSize(DIAMETER, DIAMETER); };
        QRect buttonGeometry() const;

protected:
        bool event(QEvent *event) override;
        bool eventFilter(QObject *obj, QEvent *event) override;

        void paintEvent(QPaintEvent *event) override;
};
