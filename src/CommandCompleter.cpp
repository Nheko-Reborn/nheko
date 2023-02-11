// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
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
                return QStringLiteral("/me ");
            case React:
                return QStringLiteral("/react ");
            case Join:
                return QStringLiteral("/join ");
            case Knock:
                return QStringLiteral("/knock ");
            case Part:
                return QStringLiteral("/part ");
            case Leave:
                return QStringLiteral("/leave ");
            case Invite:
                return QStringLiteral("/invite @");
            case Kick:
                return QStringLiteral("/kick @");
            case Ban:
                return QStringLiteral("/ban @");
            case Unban:
                return QStringLiteral("/unban @");
            case Redact:
                return QStringLiteral("/redact ");
            case Roomnick:
                return QStringLiteral("/roomnick ");
            case Shrug:
                return QStringLiteral("/shrug");
            case Fliptable:
                return QStringLiteral("/fliptable");
            case Unfliptable:
                return QStringLiteral("/unfliptable");
            case Sovietflip:
                return QStringLiteral("/sovietflip");
            case ClearTimeline:
                return QStringLiteral("/clear-timeline");
            case ResetState:
                return QStringLiteral("/reset-state");
            case RotateMegolmSession:
                return QStringLiteral("/rotate-megolm-session");
            case Md:
                return QStringLiteral("/md ");
            case Cmark:
                return QStringLiteral("/cmark ");
            case Plain:
                return QStringLiteral("/plain ");
            case Rainbow:
                return QStringLiteral("/rainbow ");
            case RainbowMe:
                return QStringLiteral("/rainbowme ");
            case Notice:
                return QStringLiteral("/notice ");
            case RainbowNotice:
                return QStringLiteral("/rainbownotice ");
            case Confetti:
                return QStringLiteral("/confetti ");
            case RainbowConfetti:
                return QStringLiteral("/rainbowconfetti ");
            case Goto:
                return QStringLiteral("/goto ");
            case ConvertToDm:
                return QStringLiteral("/converttodm");
            case ConvertToRoom:
                return QStringLiteral("/converttoroom");
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
                return tr("/join <!roomid|#alias> [reason]");
            case Knock:
                return tr("/knock <!roomid|#alias> [reason]");
            case Part:
                return tr("/part [reason]");
            case Leave:
                return tr("/leave [reason]");
            case Invite:
                return tr("/invite <@userid> [reason]");
            case Kick:
                return tr("/kick <@userid> [reason]");
            case Ban:
                return tr("/ban <@userid> [reason]");
            case Unban:
                return tr("/unban <@userid> [reason]");
            case Redact:
                return tr("/redact <$eventid|@userid>");
            case Roomnick:
                return tr("/roomnick <displayname>");
            case Shrug:
                return tr("/shrug [message]");
            case Fliptable:
                return QStringLiteral("/fliptable");
            case Unfliptable:
                return QStringLiteral("/unfliptable");
            case Sovietflip:
                return QStringLiteral("/sovietflip");
            case ClearTimeline:
                return QStringLiteral("/clear-timeline");
            case ResetState:
                return QStringLiteral("/reset-state");
            case RotateMegolmSession:
                return QStringLiteral("/rotate-megolm-session");
            case Md:
                return tr("/md <message>");
            case Cmark:
                return tr("/cmark <message>");
            case Plain:
                return tr("/plain <message>");
            case Rainbow:
                return tr("/rainbow <message>");
            case RainbowMe:
                return tr("/rainbowme <message>");
            case Notice:
                return tr("/notice <message>");
            case RainbowNotice:
                return tr("/rainbownotice <message>");
            case Confetti:
                return tr("/confetti [message]");
            case RainbowConfetti:
                return tr("/rainbowconfetti [message]");
            case Goto:
                return tr("/goto <message reference>");
            case ConvertToDm:
                return QStringLiteral("/converttodm");
            case ConvertToRoom:
                return QStringLiteral("/converttoroom");
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
            case Cmark:
                return tr(
                  "Send a commonmark formatted message disabling most extensions compared to /md.");
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
            case Confetti:
                return tr("Send a message with confetti.");
            case RainbowConfetti:
                return tr("Send a message in rainbow colors with confetti.");
            case Goto:
                return tr("Go to a specific message using an event id, index or matrix: link");
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
