#include "Avatar.h"
#include "Config.h"
#include "FlatButton.h"
#include "Painter.h"
#include "Utils.h"
#include "dialogs/RoomSettings.hpp"

#include <QComboBox>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSharedPointer>
#include <QStyleOption>
#include <QVBoxLayout>

using namespace dialogs;

RoomSettings::RoomSettings(const QString &room_id, QWidget *parent)
  : QFrame(parent)
  , room_id_{std::move(room_id)}
{
        setMaximumWidth(385);

        try {
                auto res = cache::client()->getRoomInfo({room_id_.toStdString()});
                info_    = res[room_id_];

                setAvatar(QImage::fromData(cache::client()->image(info_.avatar_url)));
        } catch (const lmdb::error &e) {
                qWarning() << "failed to retrieve room info from cache" << room_id;
        }

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(30);
        layout->setMargin(20);

        saveBtn_ = new FlatButton("SAVE", this);
        saveBtn_->setFontSize(conf::btn::fontSize);
        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFontSize(conf::btn::fontSize);

        auto btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(0);
        btnLayout->setMargin(0);
        btnLayout->addStretch(1);
        btnLayout->addWidget(saveBtn_);
        btnLayout->addWidget(cancelBtn_);

        auto notifOptionLayout_ = new QHBoxLayout;
        notifOptionLayout_->setMargin(5);
        auto notifLabel = new QLabel(tr("Notifications"), this);
        auto notifCombo = new QComboBox(this);
        notifCombo->setDisabled(true);
        notifCombo->addItem(tr("Muted"));
        notifCombo->addItem(tr("Mentions only"));
        notifCombo->addItem(tr("All messages"));
        notifLabel->setStyleSheet("font-size: 15px;");

        notifOptionLayout_->addWidget(notifLabel);
        notifOptionLayout_->addWidget(notifCombo, 0, Qt::AlignBottom | Qt::AlignRight);

        layout->addWidget(new TopSection(info_, avatarImg_, this));
        layout->addLayout(notifOptionLayout_);
        layout->addLayout(notifOptionLayout_);
        layout->addLayout(btnLayout);

        connect(cancelBtn_, &FlatButton::clicked, this, &RoomSettings::closing);
        connect(saveBtn_, &FlatButton::clicked, this, [this]() { emit closing(); });
}

void
RoomSettings::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
TopSection::paintEvent(QPaintEvent *)
{
        Painter p(this);
        PainterHighQualityEnabler hq(p);

        constexpr int textPadding    = 23;
        constexpr int textStartX     = AvatarSize + 5 * Padding;
        const int availableTextWidth = width() - textStartX;

        constexpr int nameFont    = 15;
        constexpr int membersFont = 14;
        constexpr int labelFont   = 18;

        QFont font;
        font.setPixelSize(labelFont);
        font.setWeight(70);

        p.setFont(font);
        p.setPen(textColor());
        p.drawTextLeft(Padding, Padding, "Room settings");
        p.translate(0, textPadding + QFontMetrics(p.font()).ascent());

        p.save();
        p.translate(textStartX, 2 * Padding);

        // Draw the name.
        font.setPixelSize(membersFont);
        const auto members = QString("%1 members").arg(info_.member_count);

        font.setPixelSize(nameFont);
        const auto name = QFontMetrics(font).elidedText(
          QString::fromStdString(info_.name), Qt::ElideRight, availableTextWidth - 4 * Padding);

        font.setWeight(60);
        p.setFont(font);
        p.drawTextLeft(0, 0, name);

        // Draw the number of members
        p.translate(0, QFontMetrics(p.font()).ascent() + 2 * Padding);

        font.setPixelSize(membersFont);
        font.setWeight(50);
        p.setFont(font);
        p.drawTextLeft(0, 0, members);
        p.restore();

        if (avatar_.isNull()) {
                font.setPixelSize(AvatarSize / 2);
                font.setWeight(60);
                p.setFont(font);

                p.translate(Padding, Padding);
                p.drawLetterAvatar(utils::firstChar(name),
                                   QColor("white"),
                                   QColor("black"),
                                   AvatarSize + Padding,
                                   AvatarSize + Padding,
                                   AvatarSize);
        } else {
                QRect avatarRegion(Padding, Padding, AvatarSize, AvatarSize);

                QPainterPath pp;
                pp.addEllipse(avatarRegion.center(), AvatarSize, AvatarSize);

                p.setClipPath(pp);
                p.drawPixmap(avatarRegion, avatar_);
        }
}
