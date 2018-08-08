#pragma once

#include <QPaintEvent>
#include <QWidget>

class TypingDisplay : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)

public:
        TypingDisplay(QWidget *parent = nullptr);

        void setUsers(const QStringList &user_ids);

        void setTextColor(const QColor &color) { textColor_ = color; };
        QColor textColor() const { return textColor_; };

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        QColor textColor_;
        QString text_;
};
