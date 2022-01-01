// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

signals:
    // Notify that the user wants to login in.
    void userLogin();

    // Notify that the user wants to register.
    void userRegister();
};
