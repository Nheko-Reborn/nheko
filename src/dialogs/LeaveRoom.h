// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFrame>

class QPushButton;

namespace dialogs {

class LeaveRoom : public QFrame
{
    Q_OBJECT
public:
    explicit LeaveRoom(QWidget *parent = nullptr);

signals:
    void leaving();

private:
    QPushButton *confirmBtn_;
    QPushButton *cancelBtn_;
};
} // dialogs
