// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class QPushButton;
class QLabel;

namespace dialogs {

class FallbackAuth : public QWidget
{
    Q_OBJECT

public:
    FallbackAuth(const QString &authType, const QString &session, QWidget *parent = nullptr);

signals:
    void confirmation();
    void cancel();

private:
    QPushButton *openBtn_;
    QPushButton *confirmBtn_;
    QPushButton *cancelBtn_;
};
} // dialogs
