import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko 1.0

ApplicationWindow {
    id: inviteDialogRoot

    property string roomId
    property string roomName
    property InviteesModel invitees

    function addInvite() {
        if (inviteeEntry.text.match("@.+?:.{3,}"))
        {
            invitees.addUser(inviteeEntry.text);
            inviteeEntry.clear();
        }
    }

    title: qsTr("Invite users to ") + roomName
    x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    height: 380
    width: 340

    // TODO: make this work in the TextField
    Shortcut {
        sequence: "Ctrl+Enter"
        onActivated: inviteDialogRoot.accept()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: qsTr("User ID to invite")
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 10

            TextField {
                id: inviteeEntry

                placeholderText: qsTr("@joe:matrix.org", "Example user id. The name 'joe' can be localized however you want.")
                Layout.fillWidth: true
                onAccepted: if (text !== "") addInvite()
            }

            Button {
                text: qsTr("Add")
                onClicked: if (inviteeEntry.text !== "") addInvite()
            }
        }

        ListView {
            id: inviteesList

            Layout.fillWidth: true
            Layout.fillHeight: true
            model: invitees

            delegate: RowLayout {
                spacing: 10

                Avatar {
                    width: avatarSize
                    height: avatarSize
                    userid: model.mxid
                    url: model.avatarUrl.replace("mxc://", "image://MxcImage/")
                    displayName: model.displayName
                    onClicked: TimelineManager.timeline.openUserProfile(model.mxid)
                }

                ColumnLayout {
                    spacing: 5

                    Label {
                        text: model.displayName
                        color: TimelineManager.userColor(model ? model.mxid : "", colors.window)
                        font.pointSize: 12
                    }

                    Label {
                        text: model.mxid
                        color: colors.buttonText
                        font.pointSize: 10
                    }

                    Item {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                    }
                }
            }
//            delegate: RowLayout {
//                spacing: 10

//                Avatar {
//                    url: model.avatarUrl
//                    width: 20
//                    height: width
//                }

//                Label {
//                    text: model.displayName + " (" + model.mxid + ")"
//                }
//            }
        }
    }

    footer: DialogButtonBox {
        id: buttons

        Button {
            text: qsTr("Invite")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                invitees.accept();
                inviteDialogRoot.close();
            }
        }

        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
            onClicked: inviteDialogRoot.close();
        }
    }
}
