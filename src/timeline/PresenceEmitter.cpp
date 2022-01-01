// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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
    auto c = new CacheEntry{
      utils::replaceEmoji(QString::fromStdString(p.status_msg).toHtmlEscaped()), p.presence};
    presences.insert(id, c);
    return c;
}

void
PresenceEmitter::sync(
  const std::vector<mtx::events::Event<mtx::events::presence::Presence>> &presences_)
{
    for (const auto &p : presences_) {
        auto id = QString::fromStdString(p.sender);
        presences.remove(id);
        emit presenceChanged(std::move(id));
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
