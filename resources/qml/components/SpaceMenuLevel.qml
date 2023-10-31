// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import Qt.labs.platform 1.1 as Platform
import im.nheko 1.0

Platform.Menu {
    id: spacesMenu

    property Component childMenu
    property bool loadChildren: false
    property int position: modelData == undefined ? -2 : modelData.treeIndex
    property string roomid

    title: modelData != undefined ? modelData.name : qsTr("Add or remove from community")

    onAboutToShow: loadChildren = true

    //onAboutToHide: loadChildren = false

    Platform.MenuItemGroup {
        id: modificationGroup

        visible: position != -1
    }
    Platform.MenuItem {
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && modelData.parentValid && modelData.canonical)
        enabled: spacesMenu.position >= 0 && (modelData.canEditChild && modelData.canEditParent)
        group: modificationGroup
        text: qsTr("Official community for this room")

        onTriggered: if (checked)
            Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, true, true)
    }
    Platform.MenuItem {
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && modelData.parentValid && !modelData.canonical)
        enabled: spacesMenu.position >= 0 && (modelData.canEditChild && modelData.canEditParent)
        group: modificationGroup
        text: qsTr("Affiliated community for this room")

        onTriggered: if (checked)
            Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, true, false)
    }
    Platform.MenuItem {
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && !modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild || modelData.childValid) && (!modelData.parentValid || modelData.canEditParent))
        group: modificationGroup
        text: qsTr("Listed only for community members")

        onTriggered: if (checked)
            Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, false, true, false)
    }
    Platform.MenuItem {
        checkable: true
        checked: spacesMenu.position >= 0 && (!modelData.childValid && modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild) && (modelData.parentValid || modelData.canEditParent))
        group: modificationGroup
        text: qsTr("Listed only for room members")

        onTriggered: if (checked)
            Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, false, false)
    }
    Platform.MenuItem {
        checkable: true
        checked: spacesMenu.position >= 0 && (!modelData.childValid && !modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild || !modelData.childValid) && (!modelData.parentValid || modelData.canEditParent))
        group: modificationGroup
        text: qsTr("Not related")

        onTriggered: if (checked)
            Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, false, false, false)
    }
    Platform.MenuSeparator {
        group: modificationGroup
        text: qsTr("Subcommunities")
        visible: modificationGroup.visible && inst.model != undefined
    }
    Instantiator {
        id: inst

        //onObjectRemoved: spacesMenu.removeMenu(object)

        delegate: childMenu
        model: spacesMenu.loadChildren ? Communities.spaceChildrenListFromIndex(spacesMenu.roomid, spacesMenu.position) : undefined

        onObjectAdded: (idx, o) => {
            spacesMenu.insertMenu(idx + (spacesMenu.position != -1 ? 6 : 0), o);
        }
    }
}
