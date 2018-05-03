#include "Avatar.h"
#include "AvatarProvider.h"
#include "Config.h"
#include "DropShadow.h"
#include "SuggestionsPopup.hpp"
#include "Utils.h"

#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

constexpr int PopupHMargin    = 5;
constexpr int PopupItemMargin = 4;

PopupItem::PopupItem(QWidget *parent)
  : QWidget(parent)
  , avatar_{new Avatar(this)}
  , hovering_{false}
{
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setContentsMargins(
          PopupHMargin, PopupItemMargin, PopupHMargin, PopupItemMargin);
}

void
PopupItem::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        if (underMouse() || hovering_)
                p.fillRect(rect(), hoverColor_);
}

UserItem::UserItem(QWidget *parent, const QString &user_id)
  : PopupItem(parent)
  , userId_{user_id}
{
        QFont font;
        font.setPixelSize(conf::popup::font);

        auto displayName = Cache::displayName(ChatPage::instance()->currentRoom(), userId_);

        avatar_->setSize(conf::popup::avatar);
        avatar_->setLetter(utils::firstChar(displayName));

        // If it's a matrix id we use the second letter.
        if (displayName.size() > 1 && displayName.at(0) == '@')
                avatar_->setLetter(QChar(displayName.at(1)));

        userName_ = new QLabel(displayName, this);
        userName_->setFont(font);

        topLayout_->addWidget(avatar_);
        topLayout_->addWidget(userName_, 1);

        resolveAvatar(user_id);
}

void
UserItem::updateItem(const QString &user_id)
{
        userId_ = user_id;

        auto displayName = Cache::displayName(ChatPage::instance()->currentRoom(), userId_);

        // If it's a matrix id we use the second letter.
        if (displayName.size() > 1 && displayName.at(0) == '@')
                avatar_->setLetter(QChar(displayName.at(1)));
        else
                avatar_->setLetter(utils::firstChar(displayName));

        userName_->setText(displayName);
        resolveAvatar(user_id);
}

void
UserItem::resolveAvatar(const QString &user_id)
{
        AvatarProvider::resolve(
          ChatPage::instance()->currentRoom(), userId_, this, [this, user_id](const QImage &img) {
                  // The user on the widget when the avatar is resolved,
                  // might be different from the user that made the call.
                  if (user_id == userId_)
                          avatar_->setImage(img);
                  else
                          // We try to resolve the avatar again.
                          resolveAvatar(userId_);
          });
}

void
UserItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() != Qt::RightButton)
                emit clicked(
                  Cache::displayName(ChatPage::instance()->currentRoom(), selectedText()));

        QWidget::mousePressEvent(event);
}

RoomItem::RoomItem(QWidget *parent, const RoomSearchResult &res)
  : PopupItem(parent)
  , roomId_{QString::fromStdString(res.room_id)}
{
        auto name = QFontMetrics(QFont()).elidedText(
          QString::fromStdString(res.info.name), Qt::ElideRight, parentWidget()->width() - 10);

        avatar_->setSize(conf::popup::avatar + 6);
        avatar_->setLetter(utils::firstChar(name));

        roomName_ = new QLabel(name, this);
        roomName_->setMargin(0);

        topLayout_->addWidget(avatar_);
        topLayout_->addWidget(roomName_, 1);

        if (!res.img.isNull())
                avatar_->setImage(res.img);
}

void
RoomItem::updateItem(const RoomSearchResult &result)
{
        roomId_ = QString::fromStdString(std::move(result.room_id));

        auto name =
          QFontMetrics(QFont()).elidedText(QString::fromStdString(std::move(result.info.name)),
                                           Qt::ElideRight,
                                           parentWidget()->width() - 10);

        roomName_->setText(name);

        if (!result.img.isNull())
                avatar_->setImage(result.img);
        else
                avatar_->setLetter(utils::firstChar(name));
}

void
RoomItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() != Qt::RightButton)
                emit clicked(selectedText());

        QWidget::mousePressEvent(event);
}

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
SuggestionsPopup::addUsers(const QVector<SearchResult> &users)
{
        if (users.isEmpty()) {
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
