// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QFrame>

class Avatar;
class QLabel;
class QListWidget;
class QHBoxLayout;
class QVBoxLayout;

namespace dialogs {

class ReceiptItem : public QWidget
{
        Q_OBJECT

public:
        ReceiptItem(QWidget *parent,
                    const QString &user_id,
                    uint64_t timestamp,
                    const QString &room_id);

protected:
        void paintEvent(QPaintEvent *) override;

private:
        QString dateFormat(const QDateTime &then) const;

        QHBoxLayout *topLayout_;
        QVBoxLayout *textLayout_;

        Avatar *avatar_;

        QLabel *userName_;
        QLabel *timestamp_;
};

class ReadReceipts : public QFrame
{
        Q_OBJECT
public:
        explicit ReadReceipts(QWidget *parent = nullptr);

public slots:
        void addUsers(const std::multimap<uint64_t, std::string, std::greater<uint64_t>> &users);

protected:
        void paintEvent(QPaintEvent *event) override;
        void hideEvent(QHideEvent *event) override;

private:
        QLabel *topLabel_;

        QListWidget *userList_;
};
} // dialogs
