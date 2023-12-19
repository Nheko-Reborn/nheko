// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import Qt.labs.platform 1.1 as P
import QtQuick
import im.nheko

P.MessageDialog {
    id: deleteStickerPackRoot

    property SingleImagePackModel imagePack

    text: qsTr("Are you sure you wish to delete the sticker pack '%1'?").arg(imagePack.packname)
    modality: Qt.ApplicationModal
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    buttons: P.MessageDialog.Yes | P.MessageDialog.No
        
    onAccepted: {
        console.info("deleting image pack " + imagePack.packname);
        imagePack.remove()
    }
}
