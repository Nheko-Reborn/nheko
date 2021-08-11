// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class QPushButton;
class QLabel;
class TextField;

namespace dialogs {

class TokenRegistration : public QWidget
{
        Q_OBJECT

public:
        TokenRegistration(QWidget *parent = nullptr);

signals:
        void confirmation(std::string token);
        void cancel();

private:
        QPushButton *confirmBtn_;
        QPushButton *cancelBtn_;
        TextField *tokenInput_;
};
} // dialogs
