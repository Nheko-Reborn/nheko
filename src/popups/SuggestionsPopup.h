#pragma once

#include <QWidget>

#include "CacheStructs.h"
#include "ChatPage.h"
#include "PopupItem.h"

class SuggestionsPopup : public QWidget
{
        Q_OBJECT

public:
        explicit SuggestionsPopup(QWidget *parent = nullptr);

        template<class Item>
        void selectHoveredSuggestion()
        {
                const auto item = layout_->itemAt(selectedItem_);
                if (!item)
                        return;

                const auto &widget = qobject_cast<Item *>(item->widget());
                emit itemSelected(
                  cache::displayName(ChatPage::instance()->currentRoom(), widget->selectedText()));

                resetSelection();
        }

public slots:
        void addUsers(const std::vector<SearchResult> &users);
        void addRooms(const std::vector<RoomSearchResult> &rooms);

        //! Move to the next available suggestion item.
        void selectNextSuggestion();
        //! Move to the previous available suggestion item.
        void selectPreviousSuggestion();
        //! Remove hovering from all items.
        void resetHovering();
        //! Set hovering to the item in the given layout position.
        void setHovering(int pos);

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void itemSelected(const QString &user);

private:
        void hoverSelection();
        void resetSelection() { selectedItem_ = -1; }
        void selectFirstItem() { selectedItem_ = 0; }
        void selectLastItem() { selectedItem_ = layout_->count() - 1; }
        void removeLayoutItemsAfter(size_t startingPos)
        {
                size_t posToRemove = layout_->count() - 1;

                QLayoutItem *item;
                while (startingPos <= posToRemove &&
                       (item = layout_->takeAt(posToRemove)) != nullptr) {
                        delete item->widget();
                        delete item;

                        posToRemove = layout_->count() - 1;
                }
        }

        QVBoxLayout *layout_;

        //! Counter for tab completion (cycling).
        int selectedItem_ = -1;
};
