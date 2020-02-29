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
  , avatar_{new Avatar(this, conf::popup::avatar)}
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

UserItem::UserItem(QWidget *parent)
  : PopupItem(parent)
{
        userName_ = new QLabel("Placeholder", this);
        avatar_->setLetter("P");
        topLayout_->addWidget(avatar_);
        topLayout_->addWidget(userName_, 1);
}

UserItem::UserItem(QWidget *parent, const QString &user_id)
  : PopupItem(parent)
  , userId_{user_id}
{
        auto displayName = cache::displayName(ChatPage::instance()->currentRoom(), userId_);

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

        auto displayName = cache::displayName(ChatPage::instance()->currentRoom(), userId_);

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
        avatar_->setImage(ChatPage::instance()->currentRoom(), user_id);
}

void
UserItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() != Qt::RightButton)
                emit clicked(
                  cache::displayName(ChatPage::instance()->currentRoom(), selectedText()));

        QWidget::mousePressEvent(event);
}

RoomItem::RoomItem(QWidget *parent, const RoomSearchResult &res)
  : PopupItem(parent)
  , roomId_{QString::fromStdString(res.room_id)}
{
        auto name = QFontMetrics(QFont()).elidedText(
          QString::fromStdString(res.info.name), Qt::ElideRight, parentWidget()->width() - 10);

        avatar_->setLetter(utils::firstChar(name));

        roomName_ = new QLabel(name, this);
        roomName_->setMargin(0);

        topLayout_->addWidget(avatar_);
        topLayout_->addWidget(roomName_, 1);

        avatar_->setImage(QString::fromStdString(res.info.avatar_url));
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

        avatar_->setImage(QString::fromStdString(result.info.avatar_url));
}

void
RoomItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() != Qt::RightButton)
                emit clicked(selectedText());

        QWidget::mousePressEvent(event);
}
