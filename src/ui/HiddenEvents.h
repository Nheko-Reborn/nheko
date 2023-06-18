// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>

#include <mtx/events/event_type.hpp>

class HiddenEvents : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString roomid READ roomid WRITE setRoomid NOTIFY roomidChanged REQUIRED)
    Q_PROPERTY(QVariantList hiddenEvents READ hiddenEvents NOTIFY hiddenEventsChanged)
public:
    explicit HiddenEvents(QObject *p = nullptr)
      : QObject(p)
    {
    }

    Q_INVOKABLE void toggle(int type);
    Q_INVOKABLE void save();

    [[nodiscard]] QString roomid() const { return roomid_; }
    void setRoomid(const QString &r)
    {
        roomid_ = r;
        emit roomidChanged();

        load();
    }

    [[nodiscard]] QVariantList hiddenEvents() const;

signals:
    void roomidChanged();
    void hiddenEventsChanged();

private:
    QString roomid_;
    std::vector<mtx::events::EventType> hiddenEvents_;

    void load();
};
