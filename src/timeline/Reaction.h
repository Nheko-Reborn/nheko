// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

struct Reaction
{
    Q_GADGET
    Q_PROPERTY(QString key READ key)
    Q_PROPERTY(QString users READ users)
    Q_PROPERTY(QString selfReactedEvent READ selfReactedEvent)
    Q_PROPERTY(int count READ count)

public:
    QString key() const { return key_.toHtmlEscaped(); }
    QString users() const { return users_.toHtmlEscaped(); }
    QString selfReactedEvent() const { return selfReactedEvent_; }
    int count() const { return count_; }

    QString key_;
    QString users_;
    QString selfReactedEvent_;
    int count_;
};
