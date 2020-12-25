#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

#include "../Config.h"
#include "../Utils.h"
#include "../ui/Avatar.h"
#include "../ui/DropShadow.h"
#include "ChatPage.h"
#include "PopupItem.h"
#include "SuggestionsPopup.h"

SuggestionsPopup::SuggestionsPopup(QWidget *parent)
  : QWidget(parent)
{
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setWindowFlags(Qt::ToolTip | Qt::NoDropShadowWindowHint);

        layout_ = new QVBoxLayout(this);
        layout_->setMargin(0);
        layout_->setSpacing(0);
}

QString
SuggestionsPopup::displayName(QString room, QString user)
{
        return cache::displayName(room, user);
}

void
SuggestionsPopup::addRooms(const std::vector<RoomSearchResult> &rooms)
{
        if (rooms.empty()) {
                hide();
                return;
        }

        const int layoutCount = (int)layout_->count();
        const int roomCount   = (int)rooms.size();

        // Remove the extra widgets from the layout.
        if (roomCount < layoutCount)
                removeLayoutItemsAfter(roomCount - 1);

        for (int i = 0; i < roomCount; ++i) {
                auto item = layout_->itemAt(i);

                // Create a new widget if there isn't already one in that
                // layout position.
                if (!item) {
                        auto room = new RoomItem(this, rooms.at(i));
                        connect(room, &RoomItem::clicked, this, &SuggestionsPopup::itemSelected);
                        layout_->addWidget(room);
                } else {
                        // Update the current widget with the new data.
                        auto room = qobject_cast<RoomItem *>(item->widget());
                        if (room)
                                room->updateItem(rooms.at(i));
                }
        }

        resetSelection();
        adjustSize();

        resize(geometry().width(), 40 * (int)rooms.size());

        selectNextSuggestion();
}

void
SuggestionsPopup::hoverSelection()
{
        resetHovering();
        setHovering(selectedItem_);
        update();
}

void
SuggestionsPopup::selectHoveredSuggestion()
{
        const auto item = layout_->itemAt(selectedItem_);
        if (!item)
                return;

        const auto &widget = qobject_cast<RoomItem *>(item->widget());
        emit itemSelected(displayName(ChatPage::instance()->currentRoom(), widget->selectedText()));

        resetSelection();
}

void
SuggestionsPopup::selectNextSuggestion()
{
        selectedItem_++;
        if (selectedItem_ >= layout_->count())
                selectFirstItem();

        hoverSelection();
}

void
SuggestionsPopup::selectPreviousSuggestion()
{
        selectedItem_--;
        if (selectedItem_ < 0)
                selectLastItem();

        hoverSelection();
}

void
SuggestionsPopup::resetHovering()
{
        for (int i = 0; i < layout_->count(); ++i) {
                const auto item = qobject_cast<PopupItem *>(layout_->itemAt(i)->widget());

                if (item)
                        item->setHovering(false);
        }
}

void
SuggestionsPopup::setHovering(int pos)
{
        const auto &item   = layout_->itemAt(pos);
        const auto &widget = qobject_cast<PopupItem *>(item->widget());

        if (widget)
                widget->setHovering(true);
}

void
SuggestionsPopup::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
SuggestionsPopup::selectLastItem()
{
        selectedItem_ = layout_->count() - 1;
}

void
SuggestionsPopup::removeLayoutItemsAfter(size_t startingPos)
{
        size_t posToRemove = layout_->count() - 1;

        QLayoutItem *item;
        while (startingPos <= posToRemove && (item = layout_->takeAt((int)posToRemove)) != nullptr) {
                delete item->widget();
                delete item;

                posToRemove = layout_->count() - 1;
        }
}
