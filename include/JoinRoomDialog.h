#pragma once

#include <QFrame>
#include <QLineEdit>

#include "FlatButton.h"

class JoinRoomDialog : public QFrame
{
        Q_OBJECT
public:
        JoinRoomDialog(QWidget *parent = nullptr);

signals:
        void closing(bool isJoining, QString roomAlias);

private:
        FlatButton *confirmBtn_;
        FlatButton *cancelBtn_;

        QLineEdit *roomAliasEdit_;
};
