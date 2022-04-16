// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.2
import im.nheko
import im.nheko

Item {
    id: d

    required property string blurhash
    required property string body
    required property string callType
    property alias child: chooser.child
    required property int duration
    required property int encryptionError
    required property string eventId
    required property string filename
    required property string filesize
    property bool fitsMetadata: (chooser.child && chooser.child.fitsMetadata) ? chooser.child.fitsMetadata : false
    required property string formattedBody
    required property bool isOnlyEmoji
    required property bool isReply
    required property bool isStateEvent
    property int metadataWidth
    required property int originalWidth
    required property double proportionalHeight
    required property int relatedEventCacheBuster
    required property string roomName
    required property string roomTopic
    required property string thumbnailUrl
    required property int type
    required property string typeString
    required property string url
    required property string userId
    required property string userName

    height: chooser.child ? chooser.child.height : Nheko.paddingLarge
    implicitWidth: (chooser.child && chooser.child.implicitWidth) ? chooser.child.implicitWidth : 0

    DelegateChooser {
        id: chooser

        //role: "type" //< not supported in our custom implementation, have to use roleValue
        roleValue: type
        //anchors.fill: parent
        width: parent.width ? parent.width : 0 // this should get rid of "cannot read property 'width' of null"

        DelegateChoice {
            roleValue: MtxEvent.UnknownMessage

            Placeholder {
                text: "Unretrieved event"
                typeString: d.typeString
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.TextMessage

            TextMessage {
                body: d.body
                formatted: d.formattedBody
                isOnlyEmoji: d.isOnlyEmoji
                isReply: d.isReply
                metadataWidth: d.metadataWidth
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.NoticeMessage

            NoticeMessage {
                body: d.body
                formatted: d.formattedBody
                isOnlyEmoji: d.isOnlyEmoji
                isReply: d.isReply
                isStateEvent: d.isStateEvent
                metadataWidth: d.metadataWidth
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.EmoteMessage

            NoticeMessage {
                body: d.body
                color: TimelineManager.userColor(d.userId, timelineRoot.palette.base)
                formatted: TimelineManager.escapeEmoji(d.userName) + " " + d.formattedBody
                isOnlyEmoji: d.isOnlyEmoji
                isReply: d.isReply
                isStateEvent: d.isStateEvent
                metadataWidth: d.metadataWidth
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.ImageMessage

            ImageMessage {
                blurhash: d.blurhash
                body: d.body
                eventId: d.eventId
                filename: d.filename
                isReply: d.isReply
                metadataWidth: d.metadataWidth
                originalWidth: d.originalWidth
                proportionalHeight: d.proportionalHeight
                type: d.type
                url: d.url
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.Sticker

            ImageMessage {
                blurhash: d.blurhash
                body: d.body
                eventId: d.eventId
                filename: d.filename
                isReply: d.isReply
                metadataWidth: d.metadataWidth
                originalWidth: d.originalWidth
                proportionalHeight: d.proportionalHeight
                type: d.type
                url: d.url
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
                body: d.body
                duration: d.duration
                eventId: d.eventId
                filesize: d.filesize
                metadataWidth: d.metadataWidth
                originalWidth: d.originalWidth
                proportionalHeight: d.proportionalHeight
                thumbnailUrl: d.thumbnailUrl
                type: d.type
                url: d.url
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.AudioMessage

            PlayableMediaMessage {
                body: d.body
                duration: d.duration
                eventId: d.eventId
                filesize: d.filesize
                metadataWidth: d.metadataWidth
                originalWidth: d.originalWidth
                proportionalHeight: d.proportionalHeight
                thumbnailUrl: d.thumbnailUrl
                type: d.type
                url: d.url
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
                isStateEvent: d.isStateEvent
                text: qsTr("%1 removed a message").arg(d.userName)
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.Encryption

            Pill {
                isStateEvent: d.isStateEvent
                text: qsTr("%1 enabled encryption").arg(d.userName)
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
                formatted: d.roomName ? qsTr("%2 changed the room name to: %1").arg(d.roomName).arg(d.userName) : qsTr("%1 removed the room name").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.Topic

            NoticeMessage {
                body: formatted
                formatted: d.roomTopic ? qsTr("%2 changed the topic to: %1").arg(d.roomTopic).arg(d.userName) : qsTr("%1 removed the topic").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.Avatar

            NoticeMessage {
                body: formatted
                formatted: qsTr("%1 changed the room avatar").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.PinnedEvents

            NoticeMessage {
                body: formatted
                formatted: qsTr("%1 changed the pinned messages.").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.ImagePackInRoom

            NoticeMessage {
                body: formatted
                formatted: d.relatedEventCacheBuster, room.formatImagePackEvent(d.eventId)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.CanonicalAlias

            NoticeMessage {
                body: formatted
                formatted: qsTr("%1 changed the addresses for this room.").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.SpaceParent

            NoticeMessage {
                body: formatted
                formatted: qsTr("%1 changed the parent spaces for this room.").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.RoomCreate

            NoticeMessage {
                body: formatted
                formatted: qsTr("%1 created and configured room: %2").arg(d.userName).arg(room.roomId)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.CallInvite

            NoticeMessage {
                body: formatted
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
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.CallAnswer

            NoticeMessage {
                body: formatted
                formatted: qsTr("%1 answered the call.").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.CallHangUp

            NoticeMessage {
                body: formatted
                formatted: qsTr("%1 ended the call.").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.CallCandidates

            NoticeMessage {
                body: formatted
                formatted: qsTr("%1 is negotiating the call...").arg(d.userName)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.PowerLevels

            NoticeMessage {
                body: formatted
                formatted: d.relatedEventCacheBuster, room.formatPowerLevelEvent(d.eventId)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.RoomJoinRules

            NoticeMessage {
                body: formatted
                formatted: d.relatedEventCacheBuster, room.formatJoinRuleEvent(d.eventId)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.RoomHistoryVisibility

            NoticeMessage {
                body: formatted
                formatted: d.relatedEventCacheBuster, room.formatHistoryVisibilityEvent(d.eventId)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.RoomGuestAccess

            NoticeMessage {
                body: formatted
                formatted: d.relatedEventCacheBuster, room.formatGuestAccessEvent(d.eventId)
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.Member

            ColumnLayout {
                width: parent?.width

                NoticeMessage {
                    Layout.fillWidth: true
                    body: formatted
                    formatted: d.relatedEventCacheBuster, room.formatMemberEvent(d.eventId)
                    isOnlyEmoji: false
                    isReply: d.isReply
                    isStateEvent: d.isStateEvent
                }
                Button {
                    palette: timelineRoot.palette
                    text: qsTr("Allow them in")
                    visible: d.relatedEventCacheBuster, room.showAcceptKnockButton(d.eventId)

                    onClicked: room.acceptKnock(eventId)
                }
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationRequest

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationRequest"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationStart

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationStart"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationReady

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationReady"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationCancel

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationCancel"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationKey

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationKey"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationMac

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationMac"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationDone

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationDone"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationDone

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationDone"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            roleValue: MtxEvent.KeyVerificationAccept

            NoticeMessage {
                body: formatted
                formatted: "KeyVerificationAccept"
                isOnlyEmoji: false
                isReply: d.isReply
                isStateEvent: d.isStateEvent
            }
        }
        DelegateChoice {
            Placeholder {
                typeString: d.typeString
            }
        }
    }
}
