#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

#include "../Config.h"
#include "../Utils.h"
#include "../ui/Avatar.h"
#include "../ui/DropShadow.h"
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

void
SuggestionsPopup::addRooms(const std::vector<RoomSearchResult> &rooms)
{
        if (rooms.empty()) {
                hide();
                return;
        }

        const size_t layoutCount = layout_->count();
        const size_t roomCount   = rooms.size();

        // Remove the extra widgets from the layout.
        if (roomCount < layoutCount)
                removeLayoutItemsAfter(roomCount - 1);

        for (size_t i = 0; i < roomCount; ++i) {
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

        resize(geometry().width(), 40 * rooms.size());

        selectNextSuggestion();
}

void
SuggestionsPopup::addUsers(const std::vector<SearchResult> &users)
{
        if (users.empty()) {
                hide();
                return;
        }

        const size_t layoutCount = layout_->count();
        const size_t userCount   = users.size();

        // Remove the extra widgets from the layout.
        if (userCount < layoutCount)
                removeLayoutItemsAfter(userCount - 1);

        for (size_t i = 0; i < userCount; ++i) {
                auto item = layout_->itemAt(i);

                // Create a new widget if there isn't already one in that
                // layout position.
                if (!item) {
                        auto user = new UserItem(this, users.at(i).user_id);
                        connect(user, &UserItem::clicked, this, &SuggestionsPopup::itemSelected);
                        layout_->addWidget(user);
                } else {
                        // Update the current widget with the new data.
                        auto userWidget = qobject_cast<UserItem *>(item->widget());
                        if (userWidget)
                                userWidget->updateItem(users.at(i).user_id);
                }
        }

        resetSelection();
        adjustSize();

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
