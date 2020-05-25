#pragma once

#include <QWidget>

#include <mtx/identifiers.hpp>

class QPushButton;
class QLabel;

class InviteeItem : public QWidget
{
        Q_OBJECT

public:
        InviteeItem(mtx::identifiers::User user, QWidget *parent = nullptr);

        QString userID() { return user_; }

signals:
        void removeItem();

private:
        QString user_;

        QLabel *name_;
        QPushButton *removeUserBtn_;
};
