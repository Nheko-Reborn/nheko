// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

Menu {
    id: spacesMenu

    property string roomid
    property Component childMenu

    property int position: modelData == undefined ? -2 : modelData.treeIndex
    title: modelData != undefined ? modelData.name : qsTr("Add or remove from community")
    property bool loadChildren: false

    onAboutToShow: loadChildren = true
    //onAboutToHide: loadChildren = false

    ButtonGroup {
        id: modificationGroup
        //visible: position != -1
    }

    MenuItem {
        text: qsTr("Official community for this room")
        ButtonGroup.group: modificationGroup
        visible: position != -1
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && modelData.parentValid && modelData.canonical)
        enabled: spacesMenu.position >= 0 && (modelData.canEditChild && modelData.canEditParent)
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, true, true)
    }
    MenuItem {
        text: qsTr("Affiliated community for this room")
        ButtonGroup.group: modificationGroup
        visible: position != -1
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && modelData.parentValid && !modelData.canonical)
        enabled: spacesMenu.position >= 0 && (modelData.canEditChild && modelData.canEditParent)
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, true, false)
    }
    MenuItem {
        text: qsTr("Listed only for community members")
        ButtonGroup.group: modificationGroup
        visible: position != -1
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && !modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild || modelData.childValid) && (!modelData.parentValid || modelData.canEditParent))
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, false, true, false)
    }
    MenuItem {
        text: qsTr("Listed only for room members")
        ButtonGroup.group: modificationGroup
        visible: position != -1
        checkable: true
        checked: spacesMenu.position >= 0 && (!modelData.childValid && modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild) && (modelData.parentValid || modelData.canEditParent))
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, false, false)
    }
    MenuItem {
        text: qsTr("Not related")
        ButtonGroup.group: modificationGroup
        visible: position != -1
        checkable: true
        checked: spacesMenu.position >= 0 && (!modelData.childValid && !modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild || !modelData.childValid) && (!modelData.parentValid || modelData.canEditParent))
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, false, false, false)
    }

    MenuSeparator {
        //text: qsTr("Subcommunities")
        ButtonGroup.group: modificationGroup
        visible: position != -1 && inst.model != undefined
    }

    Instantiator {
        id: inst
        model: spacesMenu.loadChildren ? Communities.spaceChildrenListFromIndex(spacesMenu.roomid, spacesMenu.position) : undefined
        onObjectAdded: (idx, o) => {
            spacesMenu.insertMenu(idx + (spacesMenu.position != -1 ? 6 : 0), o)
        }
        onObjectRemoved: (index, object) => spacesMenu.removeMenu(object)

        delegate: childMenu
    }
}
