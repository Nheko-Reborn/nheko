#include <QListWidgetItem>
#include <QPainter>
#include <QStyleOption>
#include <QVBoxLayout>

#include "AvatarProvider.h"
#include "ChatPage.h"
#include "Config.h"
#include "FlatButton.h"
#include "Utils.h"

#include "Avatar.h"
#include "Cache.h"
#include "dialogs/MemberList.hpp"

using namespace dialogs;

MemberItem::MemberItem(const RoomMember &member, QWidget *parent)
  : QWidget(parent)
{
        topLayout_ = new QHBoxLayout(this);
        topLayout_->setMargin(0);

        textLayout_ = new QVBoxLayout;
        textLayout_->setMargin(0);
        textLayout_->setSpacing(0);

        avatar_ = new Avatar(this);
        avatar_->setSize(44);
        avatar_->setLetter(utils::firstChar(member.display_name));

        if (!member.avatar.isNull())
                avatar_->setImage(member.avatar);
        else
                AvatarProvider::resolve(ChatPage::instance()->currentRoom(),
                                        member.user_id,
                                        this,
                                        [this](const QImage &img) { avatar_->setImage(img); });

        QFont nameFont, idFont;
        nameFont.setWeight(65);
        nameFont.setPixelSize(conf::receipts::font + 1);
        idFont.setWeight(50);
        idFont.setPixelSize(conf::receipts::font);

        userName_ = new QLabel(member.display_name, this);
        userName_->setFont(nameFont);

        userId_ = new QLabel(member.user_id, this);
        userId_->setFont(idFont);

        textLayout_->addWidget(userName_);
        textLayout_->addWidget(userId_);

        topLayout_->addWidget(avatar_);
        topLayout_->addLayout(textLayout_, 1);
}

MemberList::MemberList(const QString &room_id, QWidget *parent)
  : QFrame(parent)
  , room_id_{room_id}
{
        setMaximumSize(420, 380);
        setAttribute(Qt::WA_DeleteOnClose, true);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(30);
        layout->setMargin(20);

        list_ = new QListWidget;
        list_->setFrameStyle(QFrame::NoFrame);
        list_->setSelectionMode(QAbstractItemView::NoSelection);
        list_->setAttribute(Qt::WA_MacShowFocusRect, 0);
        list_->setSpacing(5);

        QFont font;
        font.setPixelSize(conf::headerFontSize);

        topLabel_ = new QLabel(tr("Room members"), this);
        topLabel_->setAlignment(Qt::AlignCenter);
        topLabel_->setFont(font);

        layout->addWidget(topLabel_);
        layout->addWidget(list_);

        list_->clear();

        // Add button at the bottom.
        moreBtn_  = new FlatButton(tr("SHOW MORE"), this);
        auto item = new QListWidgetItem;
        item->setSizeHint(moreBtn_->minimumSizeHint());
        item->setFlags(Qt::NoItemFlags);
        item->setTextAlignment(Qt::AlignCenter);
        list_->insertItem(0, item);
        list_->setItemWidget(item, moreBtn_);

        connect(moreBtn_, &FlatButton::clicked, this, [this]() {
                const size_t numMembers = list_->count() - 1;

                if (numMembers > 0)
                        addUsers(cache::client()->getMembers(room_id_.toStdString(), numMembers));
        });

        try {
                addUsers(cache::client()->getMembers(room_id_.toStdString()));
        } catch (const lmdb::error &e) {
                qCritical() << e.what();
        }
}

void
MemberList::moveButtonToBottom()
{
        auto item = new QListWidgetItem(list_);
        item->setSizeHint(moreBtn_->minimumSizeHint());
        item->setFlags(Qt::NoItemFlags);
        item->setTextAlignment(Qt::AlignCenter);
        list_->setItemWidget(item, moreBtn_);
        list_->addItem(item);
}

void
MemberList::addUsers(const std::vector<RoomMember> &members)
{
        if (members.size() == 0) {
                moreBtn_->hide();
        } else {
                moreBtn_->show();
        }

        for (const auto &member : members) {
                auto user = new MemberItem(member, this);
                auto item = new QListWidgetItem;

                item->setSizeHint(user->minimumSizeHint());
                item->setFlags(Qt::NoItemFlags);
                item->setTextAlignment(Qt::AlignCenter);

                list_->insertItem(list_->count() - 1, item);
                list_->setItemWidget(item, user);
        }
}

void
MemberList::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
