// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>

#include <vector>

#include <mtx/events.hpp>
#include <mtx/events/presence.hpp>

class PresenceEmitter final : public QObject
{
    Q_OBJECT

    QML_NAMED_ELEMENT(Presence)
    QML_SINGLETON

public:
    PresenceEmitter(QObject *p)
      : QObject(p)
    {
        instance_ = this;
    }

    static PresenceEmitter *get() { return instance_; }

    static PresenceEmitter *create(QQmlEngine *qmlEngine, QJSEngine *)
    {
        // The instance has to exist before it is used. We cannot replace it.
        Q_ASSERT(instance_);

        // The engine has to have the same thread affinity as the singleton.
        Q_ASSERT(qmlEngine->thread() == instance_->thread());

        // There can only be one engine accessing the singleton.
        static QJSEngine *s_engine = nullptr;
        if (s_engine)
            Q_ASSERT(qmlEngine == s_engine);
        else
            s_engine = qmlEngine;

        QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
        return instance_;
    }

    void sync(const std::vector<mtx::events::Event<mtx::events::presence::Presence>> &presences);

    Q_INVOKABLE QString userPresence(QString id) const;
    Q_INVOKABLE QString userStatus(QString id) const;
    Q_INVOKABLE QString lastActive(QString id) const;

signals:
    void presenceChanged(QString userid);

private:
    inline static PresenceEmitter *instance_ = nullptr;
};
