// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

#include <mtx/identifiers.hpp>

class QPushButton;
class QLabel;

class InviteeItem : public QWidget
{
        Q_OBJECT

public:
        InviteeItem(mtx::identifiers::User user, QWidget *parent = nullptr);

        QString userID() { return user_; }

signals:
        void removeItem();

private:
        QString user_;

        QLabel *name_;
        QPushButton *removeUserBtn_;
};
