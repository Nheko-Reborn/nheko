// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.6
import im.nheko 1.0

Item {
    id: d

    required property bool isReply
    property alias child: chooser.child
    property real implicitWidth: (chooser.child && chooser.child.implicitWidth) ? chooser.child.implicitWidth : width
    required property double proportionalHeight
    required property int type
    required property string typeString
    required property int originalWidth
    required property string blurhash
    required property string body
    required property string formattedBody
    required property string eventId
    required property string filename
    required property string filesize
    required property string url
    required property string thumbnailUrl
    required property bool isOnlyEmoji
    required property string userId
    required property string userName
    required property string roomTopic
    required property string roomName
    required property string callType
    required property int encryptionError
    required property int relatedEventCacheBuster

    height: chooser.childrenRect.height

    DelegateChooser {
        id: chooser

        //role: "type" //< not supported in our custom implementation, have to use roleValue
        roleValue: type
        anchors.fill: parent

        DelegateChoice {
            roleValue: MtxEvent.UnknownMessage

            Placeholder {
                typeString: d.typeString
                text: "Unretrieved event"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.TextMessage

            TextMessage {
                formatted: d.formattedBody
                body: d.body
                isOnlyEmoji: d.isOnlyEmoji
                isReply: d.isReply
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.NoticeMessage

            NoticeMessage {
                formatted: d.formattedBody
                body: d.body
                isOnlyEmoji: d.isOnlyEmoji
                isReply: d.isReply
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.EmoteMessage

            NoticeMessage {
                formatted: TimelineManager.escapeEmoji(d.userName) + " " + d.formattedBody
                color: TimelineManager.userColor(d.userId, Nheko.colors.window)
                body: d.body
                isOnlyEmoji: d.isOnlyEmoji
                isReply: d.isReply
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.ImageMessage

            ImageMessage {
                type: d.type
                originalWidth: d.originalWidth
                proportionalHeight: d.proportionalHeight
                url: d.url
                blurhash: d.blurhash
                body: d.body
                filename: d.filename
                isReply: d.isReply
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Sticker

            ImageMessage {
                type: d.type
                originalWidth: d.originalWidth
                proportionalHeight: d.proportionalHeight
                url: d.url
                blurhash: d.blurhash
                body: d.body
                filename: d.filename
                isReply: d.isReply
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.FileMessage

            FileMessage {
                eventId: d.eventId
                filename: d.filename
                filesize: d.filesize
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.VideoMessage

            PlayableMediaMessage {
                proportionalHeight: d.proportionalHeight
                type: d.type
                originalWidth: d.originalWidth
                thumbnailUrl: d.thumbnailUrl
                eventId: d.eventId
                url: d.url
                body: d.body
                filesize: d.filesize
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.AudioMessage

            PlayableMediaMessage {
                proportionalHeight: d.proportionalHeight
                type: d.type
                originalWidth: d.originalWidth
                thumbnailUrl: d.thumbnailUrl
                eventId: d.eventId
                url: d.url
                body: d.body
                filesize: d.filesize
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Redacted

            Pill {
                text: qsTr("removed")
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Redaction

            Pill {
                text: qsTr("removed")
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Encryption

            Pill {
                text: qsTr("Encryption enabled")
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Encrypted

            Encrypted {
                encryptionError: d.encryptionError
                eventId: d.eventId
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Name

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: d.roomName ? qsTr("room name changed to: %1").arg(d.roomName) : qsTr("removed room name")
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Topic

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: d.roomTopic ? qsTr("topic changed to: %1").arg(d.roomTopic) : qsTr("removed topic")
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Avatar

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: qsTr("%1 changed the room avatar").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.RoomCreate

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: qsTr("%1 created and configured room: %2").arg(d.userName).arg(room.roomId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.CallInvite

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: {
                    switch (d.callType) {
                    case "voice":
                        return qsTr("%1 placed a voice call.").arg(d.userName);
                    case "video":
                        return qsTr("%1 placed a video call.").arg(d.userName);
                    default:
                        return qsTr("%1 placed a call.").arg(d.userName);
                    }
                }
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.CallAnswer

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: qsTr("%1 answered the call.").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.CallHangUp

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: qsTr("%1 ended the call.").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.CallCandidates

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: qsTr("Negotiating call...")
            }

        }

        DelegateChoice {
            // TODO: make a more complex formatter for the power levels.
            roleValue: MtxEvent.PowerLevels

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: d.relatedEventCacheBuster, room.formatPowerLevelEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.RoomJoinRules

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: d.relatedEventCacheBuster, room.formatJoinRuleEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.RoomHistoryVisibility

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: d.relatedEventCacheBuster, room.formatHistoryVisibilityEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.RoomGuestAccess

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: d.relatedEventCacheBuster, room.formatGuestAccessEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Member

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: d.relatedEventCacheBuster, room.formatMemberEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationRequest

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationRequest"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationStart

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationStart"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationReady

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationReady"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationCancel

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationCancel"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationKey

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationKey"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationMac

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationMac"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationDone

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationDone"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationDone

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationDone"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationAccept

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                formatted: "KeyVerificationAccept"
            }

        }

        DelegateChoice {
            Placeholder {
                typeString: d.typeString
            }

        }

    }

}
