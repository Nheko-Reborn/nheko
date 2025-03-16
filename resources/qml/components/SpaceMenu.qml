// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import im.nheko

Menu {
    id: spacesMenu

    property string roomid

    property bool loadChildren: false

    title: qsTr("Add or remove from community...")

    onAboutToShow: loadChildren = true
    //onAboutToHide: loadChildren = false
    
    Component {
        id: level

        SpaceMenuLevel {
            childMenu: level
            roomid: spacesMenu.roomid
        }
    }
    
    Instantiator {
        id: inst
        model: spacesMenu.loadChildren ? Communities.spaceChildrenListFromIndex(spacesMenu.roomid, -1) : undefined
        onObjectAdded: (idx, o) => {
            spacesMenu.insertMenu(idx, o)
        }
        onObjectRemoved: (index, object) => spacesMenu.removeMenu(object)

        delegate: level
    }
}
