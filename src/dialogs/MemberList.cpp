#include <QListWidgetItem>
#include <QPainter>
#include <QPushButton>
#include <QShortcut>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/MemberList.h"

#include "AvatarProvider.h"
#include "Cache.h"
#include "ChatPage.h"
#include "Config.h"
#include "Utils.h"
#include "ui/Avatar.h"

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

        QFont nameFont;
        nameFont.setPointSizeF(nameFont.pointSizeF() * 1.1);

        userId_   = new QLabel(member.user_id, this);
        userName_ = new QLabel(member.display_name, this);
        userName_->setFont(nameFont);

        textLayout_->addWidget(userName_);
        textLayout_->addWidget(userId_);

        topLayout_->addWidget(avatar_);
        topLayout_->addLayout(textLayout_, 1);
}

void
MemberItem::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

MemberList::MemberList(const QString &room_id, QWidget *parent)
  : QFrame(parent)
  , room_id_{room_id}
{
        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose, true);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        list_ = new QListWidget;
        list_->setFrameStyle(QFrame::NoFrame);
        list_->setSelectionMode(QAbstractItemView::NoSelection);
        list_->setSpacing(5);

        QFont doubleFont;
        doubleFont.setPointSizeF(doubleFont.pointSizeF() * 2);

        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        setMinimumHeight(list_->sizeHint().height() * 2);
        setMinimumWidth(std::max(list_->sizeHint().width() + 4 * conf::modals::WIDGET_MARGIN,
                                 QFontMetrics(doubleFont).averageCharWidth() * 30 -
                                   2 * conf::modals::WIDGET_MARGIN));

        QFont font;
        font.setPointSizeF(font.pointSizeF() * conf::modals::LABEL_MEDIUM_SIZE_RATIO);

        topLabel_ = new QLabel(tr("Room members"), this);
        topLabel_->setAlignment(Qt::AlignCenter);
        topLabel_->setFont(font);

        layout->addWidget(topLabel_);
        layout->addWidget(list_);

        list_->clear();

        // Add button at the bottom.
        moreBtn_ = new QPushButton(tr("Show more"), this);
        moreBtn_->setFlat(true);
        auto item = new QListWidgetItem;
        item->setSizeHint(moreBtn_->minimumSizeHint());
        item->setFlags(Qt::NoItemFlags);
        item->setTextAlignment(Qt::AlignCenter);
        list_->insertItem(0, item);
        list_->setItemWidget(item, moreBtn_);

        connect(moreBtn_, &QPushButton::clicked, this, [this]() {
                const size_t numMembers = list_->count() - 1;

                if (numMembers > 0)
                        addUsers(cache::client()->getMembers(room_id_.toStdString(), numMembers));
        });

        try {
                addUsers(cache::client()->getMembers(room_id_.toStdString()));
        } catch (const lmdb::error &e) {
                qCritical() << e.what();
        }

        auto closeShortcut = new QShortcut(QKeySequence(tr("ESC")), this);
        connect(closeShortcut, &QShortcut::activated, this, &MemberList::close);
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
