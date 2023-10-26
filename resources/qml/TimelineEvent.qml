// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.2
import QtQuick.Window 2.13
import im.nheko 1.0

EventDelegateChooser {
    id: wrapper

    required property bool isStateEvent

    // qmllint disable required
    EventDelegateChoice {
        roleValues: [MtxEvent.TextMessage, MtxEvent.NoticeMessage, MtxEvent.ElementEffectMessage, MtxEvent.UnknownMessage,]

        TextMessage {
            required property string formattedBody
            required property int type
            required property string userId
            required property string userName

            Layout.fillWidth: true
            //Layout.maximumWidth: implicitWidth

            color: type == MtxEvent.NoticeMessage ? palette.active.buttonText : palette.active.text
            font.italic: type == MtxEvent.NoticeMessage
            formatted: formattedBody
            keepFullText: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.EmoteMessage,]

        TextMessage {
            required property string formattedBody
            required property string userId
            required property string userName

            Layout.fillWidth: true
            //Layout.maximumWidth: implicitWidth

            color: TimelineManager.userColor(userId, palette.active.base)
            font.italic: true
            formatted: TimelineManager.escapeEmoji(userName) + " " + formattedBody
            keepFullText: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.CanonicalAlias, MtxEvent.ServerAcl, MtxEvent.Name, MtxEvent.Topic, MtxEvent.Avatar, MtxEvent.PinnedEvents, MtxEvent.ImagePackInRoom, MtxEvent.SpaceParent, MtxEvent.RoomCreate, MtxEvent.PowerLevels, MtxEvent.PolicyRuleUser, MtxEvent.PolicyRuleRoom, MtxEvent.PolicyRuleServer, MtxEvent.RoomJoinRules, MtxEvent.RoomHistoryVisibility, MtxEvent.RoomGuestAccess,]

        TextMessage {
            required property string formattedStateEvent
            required property string userId
            required property string userName

            body: ''
            color: palette.active.buttonText
            font.italic: true
            font.pointSize: Settings.fontSize * 0.8
            formatted: ''
            horizontalAlignment: Text.AlignHCenter
            isOnlyEmoji: false
            keepFullText: true
            text: formattedStateEvent
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.CallInvite,]

        TextMessage {
            required property string callType
            required property string userId
            required property string userName

            Layout.fillWidth: true
            body: formatted
            color: palette.active.buttonText
            font.italic: true
            formatted: {
                switch (callType) {
                case "voice":
                    return qsTr("%1 placed a voice call.").arg(TimelineManager.escapeEmoji(userName));
                case "video":
                    return qsTr("%1 placed a video call.").arg(TimelineManager.escapeEmoji(userName));
                default:
                    return qsTr("%1 placed a call.").arg(TimelineManager.escapeEmoji(userName));
                }
            }
            isOnlyEmoji: false
            keepFullText: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.CallAnswer, MtxEvent.CallReject, MtxEvent.CallSelectAnswer, MtxEvent.CallHangUp, MtxEvent.CallCandidates, MtxEvent.CallNegotiate,]

        TextMessage {
            required property int type
            required property string userId
            required property string userName

            Layout.fillWidth: true
            body: formatted
            color: palette.active.buttonText
            font.italic: true
            formatted: {
                switch (type) {
                case MtxEvent.CallAnswer:
                    return qsTr("%1 answered the call.").arg(TimelineManager.escapeEmoji(userName));
                case MtxEvent.CallReject:
                    return qsTr("%1 rejected the call.").arg(TimelineManager.escapeEmoji(userName));
                case MtxEvent.CallSelectAnswer:
                    return qsTr("%1 selected answer.").arg(TimelineManager.escapeEmoji(userName));
                case MtxEvent.CallHangUp:
                    return qsTr("%1 ended the call.").arg(TimelineManager.escapeEmoji(userName));
                case MtxEvent.CallCandidates:
                    return qsTr("%1 is negotiating the call...").arg(TimelineManager.escapeEmoji(userName));
                case MtxEvent.CallNegotiate:
                    return qsTr("%1 is negotiating the call...").arg(TimelineManager.escapeEmoji(userName));
                }
            }
            isOnlyEmoji: false
            keepFullText: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.ImageMessage, MtxEvent.Sticker,]

        ImageMessage {
            required property string userId
            required property string userName

            Layout.fillWidth: true
            //Layout.maximumWidth: tempWidth
            //Layout.maximumHeight: timelineView.height / 8
            containerHeight: timelineView.height
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.FileMessage,]

        FileMessage {
            required property string userId
            required property string userName

            Layout.fillWidth: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.VideoMessage, MtxEvent.AudioMessage,]

        PlayableMediaMessage {
            required property string userId
            required property string userName

            Layout.fillWidth: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.Encrypted,]

        Encrypted {
            required property string userId
            required property string userName

            Layout.fillWidth: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.Encryption,]

        EncryptionEnabled {
            required property string userId

            Layout.fillWidth: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.Redacted]

        Redacted {
            required property string userId
            required property string userName

            Layout.fillWidth: true
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.Member]

        ColumnLayout {
            id: member

            required property string formattedStateEvent
            required property Room room
            required property string userId
            required property string userName

            NoticeMessage {
                Layout.fillWidth: true
                body: formatted
                formatted: member.formattedStateEvent
                isOnlyEmoji: false
                isReply: EventDelegateChooser.isReply
                isStateEvent: true
                keepFullText: true
            }
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Allow them in")
                visible: member.room.showAcceptKnockButton(member.eventId)

                onClicked: member.room.acceptKnock(member.eventId)
            }
        }
    }
    EventDelegateChoice {
        roleValues: [MtxEvent.Tombstone]

        ColumnLayout {
            id: tombstone

            required property string body
            required property string eventId
            required property Room room
            required property string userId
            required property string userName

            NoticeMessage {
                Layout.fillWidth: true
                body: formatted
                formatted: qsTr("This room was replaced for the following reason: %1").arg(tombstone.body)
                isOnlyEmoji: false
                isReply: EventDelegateChooser.isReply
                isStateEvent: true
                keepFullText: true
            }
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Go to replacement room")

                onClicked: tombstone.room.joinReplacementRoom(tombstone.eventId)
            }
        }
    }
    EventDelegateChoice {
        roleValues: []

        MatrixText {
            required property string typeString
            required property string userId
            required property string userName

            Layout.fillWidth: true
            text: "Unsupported: " + typeString
        }
    }
    // qmllint enable required
}
