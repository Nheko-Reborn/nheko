// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Window 2.13
import im.nheko 1.0

ApplicationWindow {
    id: dialog

    property var flow

    color: palette.window
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    height: stack.currentItem.implicitHeight + 2 * Nheko.paddingMedium
    //height: stack.currentItem.implicitHeight
    minimumHeight: stack.currentItem.implicitHeight + 2 * Nheko.paddingLarge
    minimumWidth: 400
    modality: Qt.NonModal
    title: stack.currentItem ? (stack.currentItem.title_ || "") : ""
    width: 400

    background: Rectangle {
        color: palette.window
    }

    onClosing: VerificationManager.removeVerificationFlow(flow)

    StackView {
        id: stack

        anchors.centerIn: parent
        implicitHeight: dialog.height - 2 * Nheko.paddingMedium
        implicitWidth: dialog.width - 2 * Nheko.paddingMedium
        initialItem: newVerificationRequest
    }
    Component {
        id: newVerificationRequest

        NewVerificationRequest {
        }
    }
    Component {
        id: waiting

        Waiting {
        }
    }
    Component {
        id: success

        Success {
        }
    }
    Component {
        id: failed

        Failed {
        }
    }
    Component {
        id: digitVerification

        DigitVerification {
        }
    }
    Component {
        id: emojiVerification

        EmojiVerification {
        }
    }
    Item {
        state: flow.state

        states: [
            State {
                name: "PromptStartVerification"

                StateChangeScript {
                    script: stack.replace(null, newVerificationRequest)
                }
            },
            State {
                name: "CompareEmoji"

                StateChangeScript {
                    script: stack.replace(null, emojiVerification)
                }
            },
            State {
                name: "CompareNumber"

                StateChangeScript {
                    script: stack.replace(null, digitVerification)
                }
            },
            State {
                name: "WaitingForKeys"

                StateChangeScript {
                    script: stack.replace(null, waiting)
                }
            },
            State {
                name: "WaitingForOtherToAccept"

                StateChangeScript {
                    script: stack.replace(null, waiting)
                }
            },
            State {
                name: "WaitingForMac"

                StateChangeScript {
                    script: stack.replace(null, waiting)
                }
            },
            State {
                name: "Success"

                StateChangeScript {
                    script: stack.replace(null, success)
                }
            },
            State {
                name: "Failed"

                StateChangeScript {
                    script: stack.replace(null, failed)
                }
            }
        ]
    }
}
