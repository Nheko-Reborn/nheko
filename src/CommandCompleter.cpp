// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CommandCompleter.h"

#include "CompletionModelRoles.h"

CommandCompleter::CommandCompleter(QObject *parent)
  : QAbstractListModel(parent)
{
}

QHash<int, QByteArray>
CommandCompleter::roleNames() const
{
    return {
      {CompletionModel::CompletionRole, "completionRole"},
      {CompletionModel::SearchRole, "searchRole"},
      {CompletionModel::SearchRole2, "searchRole2"},
      {Roles::Name, "name"},
      {Roles::Description, "description"},
    };
}

QVariant
CommandCompleter::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        switch (role) {
        case CompletionModel::CompletionRole:
            // append space where applicable in a completion
            switch (index.row()) {
            case Me:
                return QString("/me ");
            case React:
                return QString("/react ");
            case Join:
                return QString("/join ");
            case Knock:
                return QString("/knock ");
            case Part:
                return QString("/part ");
            case Leave:
                return QString("/leave ");
            case Invite:
                return QString("/invite @");
            case Kick:
                return QString("/kick @");
            case Ban:
                return QString("/ban @");
            case Unban:
                return QString("/unban @");
            case Redact:
                return QString("/redact ");
            case Roomnick:
                return QString("/roomnick ");
            case Shrug:
                return QString("/shrug");
            case Fliptable:
                return QString("/fliptable");
            case Unfliptable:
                return QString("/unfliptable");
            case Sovietflip:
                return QString("/sovietflip");
            case ClearTimeline:
                return QString("/clear-timeline");
            case ResetState:
                return QString("/reset-state");
            case RotateMegolmSession:
                return QString("/rotate-megolm-session");
            case Md:
                return QString("/md ");
            case Plain:
                return QString("/plain ");
            case Rainbow:
                return QString("/rainbow ");
            case RainbowMe:
                return QString("/rainbowme ");
            case Notice:
                return QString("/notice ");
            case RainbowNotice:
                return QString("/rainbownotice ");
            case Goto:
                return QString("/goto ");
            case ConvertToDm:
                return QString("/converttodm");
            case ConvertToRoom:
                return QString("/converttoroom");
            default:
                return {};
            }
        case CompletionModel::SearchRole:
        case Qt::DisplayRole:
        case Roles::Name:
            switch (index.row()) {
            case Me:
                return tr("/me <message>");
            case React:
                return tr("/react <text>");
            case Join:
                return tr("/join (!roomid|#alias) [reason]");
            case Knock:
                return tr("/knock (!roomid|#alias) [reason]");
            case Part:
                return tr("/part [reason]");
            case Leave:
                return tr("/leave [reason]");
            case Invite:
                return tr("/invite @userid [reason]");
            case Kick:
                return tr("/kick @userid [reason]");
            case Ban:
                return tr("/ban @userid [reason]");
            case Unban:
                return tr("/unban @userid [reason]");
            case Redact:
                return tr("/redact ($eventid|@userid)");
            case Roomnick:
                return tr("/roomnick <displayname>");
            case Shrug:
                return tr("/shrug [message]");
            case Fliptable:
                return tr("/fliptable");
            case Unfliptable:
                return tr("/unfliptable");
            case Sovietflip:
                return tr("/sovietflip");
            case ClearTimeline:
                return tr("/clear-timeline");
            case ResetState:
                return tr("/reset-state");
            case RotateMegolmSession:
                return tr("/rotate-megolm-session");
            case Md:
                return tr("/md [message]");
            case Plain:
                return tr("/plain [message]");
            case Rainbow:
                return tr("/rainbow [message]");
            case RainbowMe:
                return tr("/rainbowme [message]");
            case Notice:
                return tr("/notice [message]");
            case RainbowNotice:
                return tr("/rainbownotice [message]");
            case Goto:
                return tr("/goto ($eventid|message index|matrix:r/room/e/event)");
            case ConvertToDm:
                return tr("/converttodm");
            case ConvertToRoom:
                return tr("/converttoroom");
            default:
                return {};
            }
        case CompletionModel::SearchRole2:
        case Roles::Description:
            switch (index.row()) {
            case Me:
                return tr("Send a message expressing an action.");
            case React:
                return tr("Send <text> as a reaction when you’re replying to a message.");
            case Join:
                return tr("Join a room. Reason is optional.");
            case Knock:
                return tr("Ask to join a room. Reason is optional.");
            case Part:
                return tr("Leave a room. Reason is optional.");
            case Leave:
                return tr("Leave a room. Reason is optional.");
            case Invite:
                return tr("Invite a user into the current room. Reason is optional.");
            case Kick:
                return tr("Kick a user from the current room. Reason is optional.");
            case Ban:
                return tr("Ban a user from the current room. Reason is optional.");
            case Unban:
                return tr("Unban a user in the current room. Reason is optional.");
            case Redact:
                return tr("Redact an event or all locally cached messages of a user.");
            case Roomnick:
                return tr("Change your displayname in this room.");
            case Shrug:
                return tr("¯\\_(ツ)_/¯ with an optional message.");
            case Fliptable:
                return tr("(╯°□°)╯︵ ┻━┻");
            case Unfliptable:
                return tr("┯━┯╭( º _ º╭)");
            case Sovietflip:
                return tr("ノ┬─┬ノ ︵ ( \\o°o)\\");
            case ClearTimeline:
                return tr("Clear the currently cached messages in this room.");
            case ResetState:
                return tr("Refetch the state in this room.");
            case RotateMegolmSession:
                return tr("Rotate the current symmetric encryption key.");
            case Md:
                return tr("Send a markdown formatted message (ignoring the global setting).");
            case Plain:
                return tr("Send an unformatted message (ignoring the global setting).");
            case Rainbow:
                return tr("Send a message in rainbow colors.");
            case RainbowMe:
                return tr("Send /me in rainbow colors.");
            case Notice:
                return tr("Send a bot message.");
            case RainbowNotice:
                return tr("Send a bot message in rainbow colors.");
            case Goto:
                return tr("Go to this event or link.");
            case ConvertToDm:
                return tr("Convert this room to a direct chat.");
            case ConvertToRoom:
                return tr("Convert this direct chat into a room.");
            default:
                return {};
            }
        }
    }
    return {};
}
