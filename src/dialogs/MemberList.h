// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFrame>
#include <QListWidget>

class Avatar;
class QPushButton;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;

struct RoomMember;

template<class T>
class QSharedPointer;

namespace dialogs {

class MemberItem : public QWidget
{
        Q_OBJECT

public:
        MemberItem(const RoomMember &member, QWidget *parent);

protected:
        void paintEvent(QPaintEvent *) override;

private:
        QHBoxLayout *topLayout_;
        QVBoxLayout *textLayout_;

        Avatar *avatar_;

        QLabel *userName_;
        QLabel *userId_;
};

class MemberList : public QFrame
{
        Q_OBJECT
public:
        MemberList(const QString &room_id, QWidget *parent = nullptr);

public slots:
        void addUsers(const std::vector<RoomMember> &users);

private:
        QString room_id_;
        QLabel *topLabel_;
        QListWidget *list_;
};
} // dialogs
