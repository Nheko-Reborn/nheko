// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoDropArea.h"

#include <QMimeData>

#include "ChatPage.h"
#include "timeline/InputBar.h"
#include "timeline/TimelineModel.h"
#include "timeline/TimelineViewManager.h"

NhekoDropArea::NhekoDropArea(QQuickItem *parent)
  : QQuickItem(parent)
{
    setFlags(ItemAcceptsDrops);
}

void
NhekoDropArea::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void
NhekoDropArea::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void
NhekoDropArea::dropEvent(QDropEvent *event)
{
    if (event) {
        auto model = ChatPage::instance()->timelineManager()->rooms()->getRoomById(roomid_);
        if (model) {
            model->input()->insertMimeData(event->mimeData());
            ChatPage::instance()->timelineManager()->focusMessageInput();
        }
    }
}
