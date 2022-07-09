// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.2
import im.nheko 1.0

Item {
    id: d

    required property bool isReply
    property bool keepFullText: !isReply
    property alias child: chooser.child
    implicitWidth: (chooser.child && chooser.child.implicitWidth) ? chooser.child.implicitWidth : 0
    required property double proportionalHeight
    required property int type
    required property string typeString
    required property int originalWidth
    required property int duration
    required property string blurhash
    required property string body
    required property string formattedBody
    required property string eventId
    required property string filename
    required property string filesize
    required property string url
    required property string thumbnailUrl
    required property bool isOnlyEmoji
    required property bool isStateEvent
    required property string userId
    required property string userName
    required property string roomTopic
    required property string roomName
    required property string callType
    required property int encryptionError
    required property int relatedEventCacheBuster
    property bool fitsMetadata: (chooser.child && chooser.child.fitsMetadata) ? chooser.child.fitsMetadata : false
    property int metadataWidth

    height: chooser.child ? chooser.child.height : Nheko.paddingLarge

    DelegateChooser {
        id: chooser

        //role: "type" //< not supported in our custom implementation, have to use roleValue
        roleValue: type
        //anchors.fill: parent

        width: parent.width? parent.width: 0 // this should get rid of "cannot read property 'width' of null"

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
                keepFullText: d.keepFullText
                metadataWidth: d.metadataWidth
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.NoticeMessage

            NoticeMessage {
                formatted: d.formattedBody
                body: d.body
                isOnlyEmoji: d.isOnlyEmoji
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                metadataWidth: d.metadataWidth
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.EmoteMessage

            NoticeMessage {
                formatted: TimelineManager.escapeEmoji(d.userName) + " " + d.formattedBody
                color: TimelineManager.userColor(d.userId, Nheko.colors.base)
                body: d.body
                isOnlyEmoji: d.isOnlyEmoji
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                metadataWidth: d.metadataWidth
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
                eventId: d.eventId
                metadataWidth: d.metadataWidth
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
                eventId: d.eventId
                metadataWidth: d.metadataWidth
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.FileMessage

            FileMessage {
                eventId: d.eventId
                filename: d.filename
                filesize: d.filesize
                metadataWidth: d.metadataWidth
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
                duration: d.duration
                metadataWidth: d.metadataWidth
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
                duration: d.duration
                metadataWidth: d.metadataWidth
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Redacted

            Redacted {
                metadataWidth: d.metadataWidth
            }
        }

        DelegateChoice {
            roleValue: MtxEvent.Redaction

            Pill {
                text: qsTr("%1 removed a message").arg(d.userName)
                isStateEvent: d.isStateEvent
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Encryption

            Pill {
                text: qsTr("%1 enabled encryption").arg(d.userName)
                isStateEvent: d.isStateEvent
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
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.roomName ? qsTr("%2 changed the room name to: %1").arg(d.roomName).arg(d.userName) : qsTr("%1 removed the room name").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Topic

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.roomTopic ? qsTr("%2 changed the topic to: %1").arg(d.roomTopic).arg(d.userName): qsTr("%1 removed the topic").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Avatar

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: qsTr("%1 changed the room avatar").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.PinnedEvents

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: qsTr("%1 changed the pinned messages.").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.ImagePackInRoom

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.relatedEventCacheBuster, room.formatImagePackEvent(d.eventId)
            }

        }


        DelegateChoice {
            roleValue: MtxEvent.CanonicalAlias

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: qsTr("%1 changed the addresses for this room.").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.SpaceParent

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: qsTr("%1 changed the parent spaces for this room.").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.RoomCreate

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: qsTr("%1 created and configured room: %2").arg(d.userName).arg(room.roomId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.CallInvite

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
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
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: qsTr("%1 answered the call.").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.CallHangUp

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: qsTr("%1 ended the call.").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.CallCandidates

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: qsTr("%1 is negotiating the call...").arg(d.userName)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.PowerLevels

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.relatedEventCacheBuster, room.formatPowerLevelEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.PolicyRuleUser

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.relatedEventCacheBuster, room.formatPolicyRule(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.PolicyRuleRoom

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.relatedEventCacheBuster, room.formatPolicyRule(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.PolicyRuleServer

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.relatedEventCacheBuster, room.formatPolicyRule(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.RoomJoinRules

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.relatedEventCacheBuster, room.formatJoinRuleEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.RoomHistoryVisibility

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.relatedEventCacheBuster, room.formatHistoryVisibilityEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.RoomGuestAccess

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: d.relatedEventCacheBuster, room.formatGuestAccessEvent(d.eventId)
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.Member

            ColumnLayout {
                width: parent.width

                NoticeMessage {
                    body: formatted
                    isOnlyEmoji: false
                    isReply: d.isReply
                    keepFullText: d.keepFullText
                    isStateEvent: d.isStateEvent
                    Layout.fillWidth: true
                    formatted: d.relatedEventCacheBuster, room.formatMemberEvent(d.eventId)
                }

                Button {
                    visible: d.relatedEventCacheBuster, room.showAcceptKnockButton(d.eventId)
                    palette: Nheko.colors
                    text: qsTr("Allow them in")
                    onClicked: room.acceptKnock(eventId)
                }

            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationRequest

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: "KeyVerificationRequest"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationStart

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: "KeyVerificationStart"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationReady

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: "KeyVerificationReady"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationCancel

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: "KeyVerificationCancel"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationKey

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: "KeyVerificationKey"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationMac

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: "KeyVerificationMac"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationDone

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: "KeyVerificationDone"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationDone

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
                formatted: "KeyVerificationDone"
            }

        }

        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationAccept

            NoticeMessage {
                body: formatted
                isOnlyEmoji: false
                isReply: d.isReply
                keepFullText: d.keepFullText
                isStateEvent: d.isStateEvent
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
