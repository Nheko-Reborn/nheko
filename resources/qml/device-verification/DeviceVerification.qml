// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Window 2.10
import im.nheko 1.0

ApplicationWindow {
    id: dialog

    property var flow

    onClosing: TimelineManager.removeVerificationFlow(flow)
    title: stack.currentItem.title
    flags: Qt.Dialog
    palette: colors
    height: stack.implicitHeight
    width: stack.implicitWidth

    StackView {
        id: stack

        initialItem: newVerificationRequest
        implicitWidth: currentItem.implicitWidth
        implicitHeight: currentItem.implicitHeight
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
                    script: stack.replace(newVerificationRequest)
                }

            },
            State {
                name: "CompareEmoji"

                StateChangeScript {
                    script: stack.replace(emojiVerification)
                }

            },
            State {
                name: "CompareNumber"

                StateChangeScript {
                    script: stack.replace(digitVerification)
                }

            },
            State {
                name: "WaitingForKeys"

                StateChangeScript {
                    script: stack.replace(waiting)
                }

            },
            State {
                name: "WaitingForOtherToAccept"

                StateChangeScript {
                    script: stack.replace(waiting)
                }

            },
            State {
                name: "WaitingForMac"

                StateChangeScript {
                    script: stack.replace(waiting)
                }

            },
            State {
                name: "Success"

                StateChangeScript {
                    script: stack.replace(success)
                }

            },
            State {
                name: "Failed"

                StateChangeScript {
                    script: stack.replace(failed)
                }

            }
        ]
    }

}
