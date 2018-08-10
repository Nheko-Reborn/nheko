#pragma once

#include "ui/OverlayWidget.h"

class QPaintEvent;

class TypingDisplay : public OverlayWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)
        Q_PROPERTY(QColor backgroundColor WRITE setBackgroundColor READ backgroundColor)

public:
        TypingDisplay(QWidget *parent = nullptr);

        void setUsers(const QStringList &user_ids);

        void setTextColor(const QColor &color) { textColor_ = color; };
        QColor textColor() const { return textColor_; };

        void setBackgroundColor(const QColor &color) { bgColor_ = color; };
        QColor backgroundColor() const { return bgColor_; };

public slots:
        void setOffset(int margin);

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        int offset_;
        QColor textColor_;
        QColor bgColor_;
        QString text_;
};
