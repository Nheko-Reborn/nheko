#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QWidget>

#include "Avatar.h"
#include "AvatarProvider.h"
#include "Cache.h"
#include "ChatPage.h"

class Avatar;
struct SearchResult;

class PopupItem : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor)
        Q_PROPERTY(bool hovering READ hovering WRITE setHovering)

public:
        PopupItem(QWidget *parent);

        QString selectedText() const { return QString(); }
        QColor hoverColor() const { return hoverColor_; }
        void setHoverColor(QColor &color) { hoverColor_ = color; }

        bool hovering() const { return hovering_; }
        void setHovering(const bool hover) { hovering_ = hover; };

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void clicked(const QString &text);

protected:
        QHBoxLayout *topLayout_;
        Avatar *avatar_;
        QColor hoverColor_;

        //! Set if the item is currently being
        //! hovered during tab completion (cycling).
        bool hovering_;
};

class UserItem : public PopupItem
{
        Q_OBJECT

public:
        UserItem(QWidget *parent, const QString &user_id);
        QString selectedText() const { return userId_; }

protected:
        void mousePressEvent(QMouseEvent *event) override;

private:
        QLabel *userName_;
        QString userId_;
};

class RoomItem : public PopupItem
{
        Q_OBJECT

public:
        RoomItem(QWidget *parent, const RoomSearchResult &res);
        QString selectedText() const { return roomId_; }

protected:
        void mousePressEvent(QMouseEvent *event) override;

private:
        QLabel *roomName_;
        QString roomId_;
        RoomSearchResult info_;
};

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
                  Cache::displayName(ChatPage::instance()->currentRoom(), widget->selectedText()));

                resetSelection();
        }

public slots:
        void addUsers(const QVector<SearchResult> &users);
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
        void removeItems()
        {
                QLayoutItem *item;
                while ((item = layout_->takeAt(0)) != 0) {
                        delete item->widget();
                        delete item;
                }
        }

        QVBoxLayout *layout_;

        //! Counter for tab completion (cycling).
        int selectedItem_ = -1;
};
