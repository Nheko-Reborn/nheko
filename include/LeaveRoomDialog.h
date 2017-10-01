#pragma once

#include <QFrame>

#include "FlatButton.h"

class LeaveRoomDialog : public QFrame
{
        Q_OBJECT
public:
        explicit LeaveRoomDialog(QWidget *parent = nullptr);

signals:
        void closing(bool isLeaving);

private:
        FlatButton *confirmBtn_;
        FlatButton *cancelBtn_;
};
