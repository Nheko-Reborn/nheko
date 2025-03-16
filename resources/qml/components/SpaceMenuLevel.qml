// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

Menu {
    id: spacesMenu

    required property string roomid
    required property Component childMenu
    required property var modelData

    property int position: modelData.treeIndex
    title: modelData.name
    property bool loadChildren: false

    onAboutToShow: loadChildren = true
    //onAboutToHide: loadChildren = false

    ButtonGroup {
        id: modificationGroup
    }

    MenuItem {
        text: qsTr("Official community for this room")
        ButtonGroup.group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && modelData.parentValid && modelData.canonical)
        enabled: spacesMenu.position >= 0 && (modelData.canEditChild && modelData.canEditParent)
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, true, true)
    }
    MenuItem {
        text: qsTr("Affiliated community for this room")
        ButtonGroup.group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && modelData.parentValid && !modelData.canonical)
        enabled: spacesMenu.position >= 0 && (modelData.canEditChild && modelData.canEditParent)
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, true, false)
    }
    MenuItem {
        text: qsTr("Listed only for community members")
        ButtonGroup.group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (modelData.childValid && !modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild || modelData.childValid) && (!modelData.parentValid || modelData.canEditParent))
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, false, true, false)
    }
    MenuItem {
        text: qsTr("Listed only for room members")
        ButtonGroup.group: modificationGroup
        checkable: true
        checked: spacesMenu.position >= 0 && (!modelData.childValid && modelData.parentValid)
        enabled: spacesMenu.position >= 0 && ((modelData.canEditChild) && (modelData.parentValid || modelData.canEditParent))
        onTriggered: if (checked) Communities.updateSpaceStatus(modelData.roomid, spacesMenu.roomid, true, false, false)
    }
    MenuItem {
        text: qsTr("Not related")
        ButtonGroup.group: modificationGroup
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
            spacesMenu.insertMenu(idx + 6, o)
        }
        onObjectRemoved: (index, object) => spacesMenu.removeMenu(object)

        delegate: childMenu
    }
}
