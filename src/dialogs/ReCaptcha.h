// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class QPushButton;

namespace dialogs {

class ReCaptcha : public QWidget
{
    Q_OBJECT

public:
    ReCaptcha(const QString &session, QWidget *parent = nullptr);

signals:
    void confirmation();
    void cancel();

private:
    QPushButton *openCaptchaBtn_;
    QPushButton *confirmBtn_;
    QPushButton *cancelBtn_;
};
} // dialogs
