#pragma once

#include "ui/OverlayWidget.h"

class QPaintEvent;

class TypingDisplay : public OverlayWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)

public:
        TypingDisplay(QWidget *parent = nullptr);

        void setUsers(const QStringList &user_ids);

        void setTextColor(const QColor &color) { textColor_ = color; };
        QColor textColor() const { return textColor_; };

public slots:
        void setOffset(int margin);

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        int offset_;
        QColor textColor_;
        QString text_;
};
