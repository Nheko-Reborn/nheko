// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PresenceEmitter.h"

#include <QCache>
#include <Utils.h>

#include "Cache.h"

namespace {
struct CacheEntry
{
    QString status;
    mtx::presence::PresenceState state;
    QDateTime lastActiveTime;
};
}

static QCache<QString, CacheEntry> presences;

static QString
presenceToStr(mtx::presence::PresenceState state)
{
    switch (state) {
    case mtx::presence::PresenceState::offline:
        return QStringLiteral("offline");
    case mtx::presence::PresenceState::unavailable:
        return QStringLiteral("unavailable");
    case mtx::presence::PresenceState::online:
    default:
        return QStringLiteral("online");
    }
}

static CacheEntry *
pullPresence(const QString &id)
{
    auto p = cache::presence(id.toStdString());

    auto statusMsg = QString::fromStdString(p.status_msg);
    if (statusMsg.size() > 255) {
        statusMsg.truncate(255);
        statusMsg.append(u'…');
    }

    QDateTime lastActiveTime;
    if (p.last_active_ago > 0)
        lastActiveTime = QDateTime::currentDateTime().addMSecs(-(qint64)p.last_active_ago);

    auto c = new CacheEntry{
      utils::replaceEmoji(std::move(statusMsg).toHtmlEscaped()), p.presence, lastActiveTime};
    presences.insert(id, c);
    return c;
}

void
PresenceEmitter::sync(
  const std::vector<mtx::events::Event<mtx::events::presence::Presence>> &presences_)
{
    for (const auto &p : presences_) {
        auto id = QString::fromStdString(p.sender);
        presences.remove(std::move(id));
        emit presenceChanged(id);
    }
}

QString
PresenceEmitter::userPresence(QString id) const
{
    if (id.isEmpty())
        return {};
    else if (auto p = presences[id])
        return presenceToStr(p->state);
    else
        return presenceToStr(pullPresence(id)->state);
}

QString
PresenceEmitter::userStatus(QString id) const
{
    if (id.isEmpty())
        return {};
    else if (auto p = presences[id])
        return p->status;
    else
        return pullPresence(id)->status;
}

QString
PresenceEmitter::lastActive(QString id) const
{
    if (id.isEmpty())
        return {};

    QString status;
    mtx::presence::PresenceState state;
    QDateTime lastActiveTime;

    if (auto p = presences[id]) {
        status         = p->status;
        state          = p->state;
        lastActiveTime = p->lastActiveTime;
    } else {
        auto fetched   = pullPresence(id);
        status         = fetched->status;
        state          = fetched->state;
        lastActiveTime = fetched->lastActiveTime;
    }

    if (state == mtx::presence::PresenceState::online)
        return QStringLiteral("Online") + (status.isEmpty() ? "" : " - " + status);

    if (state == mtx::presence::PresenceState::unavailable)
        return QStringLiteral("Idle") + (status.isEmpty() ? "" : " - " + status);

    if (lastActiveTime.isValid())
        return QStringLiteral("Last active: ") + utils::descriptiveTime(lastActiveTime) +
               (status.isEmpty() ? "" : " - " + status);

    return status.isEmpty() ? QStringLiteral("Offline") : status;
}

#include "moc_PresenceEmitter.cpp"
