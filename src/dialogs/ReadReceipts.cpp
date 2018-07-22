#include <QDebug>
#include <QIcon>
#include <QListWidgetItem>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>

#include "dialogs/ReadReceipts.h"

#include "AvatarProvider.h"
#include "Cache.h"
#include "ChatPage.h"
#include "Config.h"
#include "Utils.h"
#include "ui/Avatar.h"

using namespace dialogs;

ReceiptItem::ReceiptItem(QWidget *parent,
                         const QString &user_id,
                         uint64_t timestamp,
                         const QString &room_id)
  : QWidget(parent)
{
        topLayout_ = new QHBoxLayout(this);
        topLayout_->setMargin(0);

        textLayout_ = new QVBoxLayout;
        textLayout_->setMargin(0);
        textLayout_->setSpacing(conf::modals::TEXT_SPACING);

        QFont nameFont;
        nameFont.setWeight(65);
        nameFont.setPointSizeF(nameFont.pointSizeF() * 1.1);

        auto displayName = Cache::displayName(room_id, user_id);

        avatar_ = new Avatar(this);
        avatar_->setSize(44);
        avatar_->setLetter(utils::firstChar(displayName));

        // If it's a matrix id we use the second letter.
        if (displayName.size() > 1 && displayName.at(0) == '@')
                avatar_->setLetter(QChar(displayName.at(1)));

        userName_ = new QLabel(displayName, this);
        userName_->setFont(nameFont);

        timestamp_ = new QLabel(dateFormat(QDateTime::fromMSecsSinceEpoch(timestamp)), this);

        textLayout_->addWidget(userName_);
        textLayout_->addWidget(timestamp_);

        topLayout_->addWidget(avatar_);
        topLayout_->addLayout(textLayout_, 1);

        AvatarProvider::resolve(ChatPage::instance()->currentRoom(),
                                user_id,
                                this,
                                [this](const QImage &img) { avatar_->setImage(img); });
}

QString
ReceiptItem::dateFormat(const QDateTime &then) const
{
        auto now  = QDateTime::currentDateTime();
        auto days = then.daysTo(now);

        if (days == 0)
                return QString("Today %1").arg(then.toString("HH:mm"));
        else if (days < 2)
                return QString("Yesterday %1").arg(then.toString("HH:mm"));
        else if (days < 365)
                return then.toString("dd/MM HH:mm");

        return then.toString("dd/MM/yy");
}

ReadReceipts::ReadReceipts(QWidget *parent)
  : QFrame(parent)
{
        setMinimumSize(conf::modals::MIN_WIDGET_WIDTH, conf::modals::MIN_WIDGET_HEIGHT);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        userList_ = new QListWidget;
        userList_->setFrameStyle(QFrame::NoFrame);
        userList_->setSelectionMode(QAbstractItemView::NoSelection);
        userList_->setAttribute(Qt::WA_MacShowFocusRect, 0);
        userList_->setSpacing(conf::modals::TEXT_SPACING);

        QFont font;
        font.setPointSizeF(font.pointSizeF() * conf::modals::LABEL_MEDIUM_SIZE_RATIO);

        topLabel_ = new QLabel(tr("Read receipts"), this);
        topLabel_->setAlignment(Qt::AlignCenter);
        topLabel_->setFont(font);

        layout->addWidget(topLabel_);
        layout->addWidget(userList_);
}

void
ReadReceipts::addUsers(const std::multimap<uint64_t, std::string, std::greater<uint64_t>> &receipts)
{
        // We want to remove any previous items that have been set.
        userList_->clear();

        for (const auto &receipt : receipts) {
                auto user = new ReceiptItem(this,
                                            QString::fromStdString(receipt.second),
                                            receipt.first,
                                            ChatPage::instance()->currentRoom());
                auto item = new QListWidgetItem(userList_);

                item->setSizeHint(user->minimumSizeHint());
                item->setFlags(Qt::NoItemFlags);
                item->setTextAlignment(Qt::AlignCenter);

                userList_->setItemWidget(item, user);
        }
}

void
ReadReceipts::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
