// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "./delegates"
import "./device-verification"
import "./emoji"
import "./voip"
import Qt.labs.platform 1.1 as Platform
import QtGraphicalEffects 1.0
import QtQuick 2.9
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import im.nheko 1.0
import im.nheko.EmojiModel 1.0

Item {
    id: timelineView

    Label {
        visible: !TimelineManager.timeline && !TimelineManager.isInitialSync
        anchors.centerIn: parent
        text: qsTr("No room open")
        font.pointSize: 24
        color: Nheko.colors.text
    }

    BusyIndicator {
        visible: running
        anchors.centerIn: parent
        running: TimelineManager.isInitialSync
        height: 200
        width: 200
        z: 3
    }

    ColumnLayout {
        id: timelineLayout

        visible: TimelineManager.timeline != null
        anchors.fill: parent
        spacing: 0

        TopBar {
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            z: 3
            color: Nheko.theme.separator
        }

        Rectangle {
            id: msgView

            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Nheko.colors.base

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                StackLayout {
                    id: stackLayout

                    currentIndex: 0

                    Connections {
                        function onActiveTimelineChanged() {
                            stackLayout.currentIndex = 0;
                        }

                        target: TimelineManager
                    }

                    MessageView {
                        Layout.fillWidth: true
                        implicitHeight: msgView.height - typingIndicator.height
                    }

                    Loader {
                        source: CallManager.isOnCall && CallManager.callType != CallType.VOICE ? "voip/VideoCall.qml" : ""
                        onLoaded: TimelineManager.setVideoCallItem()
                    }

                }

                TypingIndicator {
                    id: typingIndicator
                }

            }

        }

        CallInviteBar {
            id: callInviteBar

            Layout.fillWidth: true
            z: 3
        }

        ActiveCallBar {
            Layout.fillWidth: true
            z: 3
        }

        Rectangle {
            Layout.fillWidth: true
            z: 3
            height: 1
            color: Nheko.theme.separator
        }

        ReplyPopup {
        }

        MessageInput {
        }

    }

    NhekoDropArea {
        anchors.fill: parent
        roomid: TimelineManager.timeline ? TimelineManager.timeline.roomId() : ""
    }

}
