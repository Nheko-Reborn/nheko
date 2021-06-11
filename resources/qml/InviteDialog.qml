import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import im.nheko 1.0
import "./types"

ApplicationWindow {
    id: inviteDialogRoot

    property string roomId
    property string roomName
    property list<Invitee> invitees

    function addInvite() {
        if (inviteeEntry.text.match("@.+?:.{3,}"))
        {
            invitees.push(inviteeComponent.createObject(
                              inviteDialogRoot, {
                                  "invitee": inviteeEntry.text
                              }));
            inviteeEntry.clear();
        }
    }

    function accept() {
        if (inviteeEntry.text !== "")
            addInvite();

        var inviteeStringList = ["temp"]; // the "temp" element exists to declare this as a string array
        for (var i = 0; i < invitees.length; ++i)
            inviteeStringList.push(invitees[i].invitee);
        inviteeStringList.shift(); // remove the first item

        TimelineManager.inviteUsers(inviteDialogRoot.roomId, inviteeStringList);
    }

    title: qsTr("Invite users to ") + roomName
    x: MainWindow.x + (MainWindow.width / 2) - (width / 2)
    y: MainWindow.y + (MainWindow.height / 2) - (height / 2)
    height: 380
    width: 340

    Component {
        id: inviteeComponent

        Invitee {}
    }

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
                text: qsTr("Invite")
                onClicked: if (inviteeEntry.text !== "") addInvite()
            }
        }

        ListView {
            id: inviteesList

            Layout.fillWidth: true
            Layout.fillHeight: true
            model: invitees
            delegate: Label {
                text: model.invitee
            }
        }
    }

    footer: DialogButtonBox {
        id: buttons

        Button {
            text: qsTr("Invite")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                inviteDialogRoot.accept();
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
