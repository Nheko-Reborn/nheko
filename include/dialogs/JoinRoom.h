#pragma once

#include <QFrame>
#include <QLineEdit>

class FlatButton;

namespace dialogs {

class JoinRoom : public QFrame
{
        Q_OBJECT
public:
        JoinRoom(QWidget *parent = nullptr);

signals:
        void closing(bool isJoining, QString roomAlias);

private:
        FlatButton *confirmBtn_;
        FlatButton *cancelBtn_;

        QLineEdit *roomAliasEdit_;
};

} // dialogs
