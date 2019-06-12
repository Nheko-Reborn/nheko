#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

#include "../Utils.h"
#include "../ui/Avatar.h"
#include "PopupItem.h"

constexpr int PopupHMargin    = 4;
constexpr int PopupItemMargin = 3;

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

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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
        auto displayName = Cache::displayName(ChatPage::instance()->currentRoom(), userId_);

        avatar_->setSize(conf::popup::avatar);
        avatar_->setLetter(utils::firstChar(displayName));

        // If it's a matrix id we use the second letter.
        if (displayName.size() > 1 && displayName.at(0) == '@')
                avatar_->setLetter(QChar(displayName.at(1)));

        userName_ = new QLabel(displayName, this);

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