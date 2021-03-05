// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFrame>

class QPushButton;

namespace dialogs {

class Logout : public QFrame
{
        Q_OBJECT
public:
        explicit Logout(QWidget *parent = nullptr);

signals:
        void loggingOut();

private:
        QPushButton *confirmBtn_;
        QPushButton *cancelBtn_;
};
} // dialogs
