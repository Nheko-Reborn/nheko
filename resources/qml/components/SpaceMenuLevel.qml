// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import Qt.labs.platform 1.1 as Platform
import im.nheko 1.0

Platform.Menu {
    id: spacesMenu

    property string roomid
    property Component childMenu

    property int position: modelData == undefined ? -2 : modelData.treeIndex
    title: modelData != undefined ? modelData.name : qsTr("Add or remove from space")
    property bool loadChildren: false

    onAboutToShow: loadChildren = true
    //onAboutToHide: loadChildren = false

    Platform.MenuItemGroup {
        id: modificationGroup
        visible: position != -1
    }

    Platform.MenuItem {
        text: qsTr("Official community for this room")
        group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && modelData.parentValid && modelData.canonical)
        enabled: spacesMenu.position >= 0 && (modelData.canEditChild && modelData.canEditParent)
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, true, true)
    }
    Platform.MenuItem {
        text: qsTr("Affiliated community for this room")
        group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && modelData.parentValid && !modelData.canonical)
        enabled: spacesMenu.position >= 0 && (modelData.canEditChild && modelData.canEditParent)
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, true, false)
    }
    Platform.MenuItem {
        text: qsTr("Listed only for community members")
        group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && !modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild || modelData.childValid) && (!modelData.parentValid || modelData.canEditParent))
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, false, true, false)
    }
    Platform.MenuItem {
        text: qsTr("Listed only for room members")
        group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (!modelData.childValid && modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild) && (modelData.parentValid || modelData.canEditParent))
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, false, false)
    }
    Platform.MenuItem {
        text: qsTr("Not related")
        group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (!modelData.childValid && !modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild || !modelData.childValid) && (!modelData.parentValid || modelData.canEditParent))
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, false, false, false)
    }

    Platform.MenuSeparator {
        text: qsTr("Subcommunities")
        group: modificationGroup
        visible: modificationGroup.visible && inst.model != undefined
    }

    Instantiator {
        id: inst
        model: spacesMenu.loadChildren ? Communities.spaceChildrenListFromIndex(spacesMenu.roomid, spacesMenu.position) : undefined
        onObjectAdded: (idx, o) => {
            spacesMenu.insertMenu(idx + (spacesMenu.position != -1 ? 6 : 0), o)
        }
        //onObjectRemoved: spacesMenu.removeMenu(object)

        delegate: childMenu
    }
}
