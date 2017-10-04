#pragma once

#include <QPaintEvent>
#include <QWidget>

class TypingDisplay : public QWidget
{
        Q_OBJECT

public:
        TypingDisplay(QWidget *parent = nullptr);

        void setUsers(const QStringList &user_ids);

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        QString text_;
        int leftPadding_;
};
