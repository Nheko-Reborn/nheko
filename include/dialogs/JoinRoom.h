#pragma once

#include <QFrame>

class FlatButton;
class TextField;

namespace dialogs {

class JoinRoom : public QFrame
{
        Q_OBJECT
public:
        JoinRoom(QWidget *parent = nullptr);

signals:
        void closing(bool isJoining, const QString &room);

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        FlatButton *confirmBtn_;
        FlatButton *cancelBtn_;

        TextField *roomInput_;
};

} // dialogs
