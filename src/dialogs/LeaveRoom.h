#pragma once

#include <QFrame>

class FlatButton;

namespace dialogs {

class LeaveRoom : public QFrame
{
        Q_OBJECT
public:
        explicit LeaveRoom(QWidget *parent = nullptr);

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void closing(bool isLeaving);

private:
        FlatButton *confirmBtn_;
        FlatButton *cancelBtn_;
};
} // dialogs
