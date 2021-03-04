// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFrame>

class QPushButton;
class TextField;

namespace dialogs {

class JoinRoom : public QFrame
{
        Q_OBJECT
public:
        JoinRoom(QWidget *parent = nullptr);

signals:
        void joinRoom(const QString &room);

protected:
        void showEvent(QShowEvent *event) override;

private slots:
        void handleInput();

private:
        QPushButton *confirmBtn_;
        QPushButton *cancelBtn_;

        TextField *roomInput_;
};

} // dialogs
