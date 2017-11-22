#pragma once

#include <QEvent>
#include <QPainter>
#include <QStyleOption>
#include <QWidget>

class OverlayWidget : public QWidget
{
        Q_OBJECT

public:
        explicit OverlayWidget(QWidget *parent = nullptr);

protected:
        bool event(QEvent *event) override;
        bool eventFilter(QObject *obj, QEvent *event) override;

        QRect overlayGeometry() const;
        void paintEvent(QPaintEvent *event) override;
};
