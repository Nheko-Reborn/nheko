#include "Avatar.h"
#include "Config.h"
#include "FlatButton.h"
#include "MatrixClient.h"
#include "Painter.h"
#include "TextField.h"
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

EditModal::EditModal(const QString &roomId, QWidget *parent)
  : QWidget(parent)
  , roomId_{roomId}
{
        setMinimumWidth(360);
        setAutoFillBackground(true);
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);

        auto layout = new QVBoxLayout(this);

        applyBtn_ = new FlatButton(tr("APPLY"), this);
        applyBtn_->setFontSize(conf::btn::fontSize);
        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFontSize(conf::btn::fontSize);

        auto btnLayout = new QHBoxLayout;
        btnLayout->setContentsMargins(5, 20, 5, 5);
        btnLayout->addWidget(applyBtn_);
        btnLayout->addWidget(cancelBtn_);

        nameInput_ = new TextField(this);
        nameInput_->setLabel(tr("Name"));
        topicInput_ = new TextField(this);
        topicInput_->setLabel(tr("Topic"));

        QFont font;
        font.setPixelSize(conf::modals::errorFont);

        errorField_ = new QLabel(this);
        errorField_->setFont(font);
        errorField_->setWordWrap(true);
        errorField_->hide();

        layout->addWidget(nameInput_);
        layout->addWidget(topicInput_);
        layout->addLayout(btnLayout, 1);

        auto labelLayout = new QHBoxLayout;
        labelLayout->setAlignment(Qt::AlignHCenter);
        labelLayout->addWidget(errorField_);
        layout->addLayout(labelLayout);

        connect(applyBtn_, &QPushButton::clicked, [this]() {
                // Check if the values are changed from the originals.
                auto newName  = nameInput_->text().trimmed();
                auto newTopic = topicInput_->text().trimmed();

                errorField_->hide();

                if (newName == initialName_ && newTopic == initialTopic_) {
                        close();
                        return;
                }

                using namespace mtx::events;

                if (newName != initialName_ && !newName.isEmpty()) {
                        state::Name body;
                        body.name = newName.toStdString();

                        auto proxy =
                          http::client()->sendStateEvent<state::Name, EventType::RoomName>(body,
                                                                                           roomId_);
                        connect(proxy.get(),
                                &StateEventProxy::stateEventSent,
                                this,
                                [this, proxy, newName]() {
                                        proxy->deleteLater();
                                        errorField_->hide();
                                        emit nameChanged(newName);
                                        close();
                                });

                        connect(proxy.get(),
                                &StateEventProxy::stateEventError,
                                this,
                                [this, proxy, newName](const QString &msg) {
                                        proxy->deleteLater();
                                        errorField_->setText(msg);
                                        errorField_->show();
                                });
                }

                if (newTopic != initialTopic_ && !newTopic.isEmpty()) {
                        state::Topic body;
                        body.topic = newTopic.toStdString();

                        auto proxy =
                          http::client()->sendStateEvent<state::Topic, EventType::RoomTopic>(
                            body, roomId_);
                        connect(proxy.get(),
                                &StateEventProxy::stateEventSent,
                                this,
                                [this, proxy, newTopic]() {
                                        proxy->deleteLater();
                                        errorField_->hide();
                                        close();
                                });

                        connect(proxy.get(),
                                &StateEventProxy::stateEventError,
                                this,
                                [this, proxy, newTopic](const QString &msg) {
                                        proxy->deleteLater();
                                        errorField_->setText(msg);
                                        errorField_->show();
                                });
                }
        });
        connect(cancelBtn_, &QPushButton::clicked, this, &EditModal::close);
}

void
EditModal::setFields(const QString &roomName, const QString &roomTopic)
{
        initialName_  = roomName;
        initialTopic_ = roomTopic;

        nameInput_->setText(roomName);
        topicInput_->setText(roomTopic);
}

TopSection::TopSection(const RoomInfo &info, const QImage &img, QWidget *parent)
  : QWidget{parent}
  , info_{std::move(info)}
{
        textColor_ = palette().color(QPalette::Text);
        avatar_    = utils::scaleImageToPixmap(img, AvatarSize);

        QSizePolicy policy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        setSizePolicy(policy);
}

void
TopSection::setRoomName(const QString &name)
{
        info_.name = name.toStdString();
        update();
}

QSize
TopSection::sizeHint() const
{
        QFont font;
        font.setPixelSize(18);
        return QSize(340, AvatarSize + QFontMetrics(font).ascent());
}

RoomSettings::RoomSettings(const QString &room_id, QWidget *parent)
  : QFrame(parent)
  , room_id_{std::move(room_id)}
{
        setMaximumWidth(420);

        try {
                info_ = cache::client()->singleRoomInfo(room_id_.toStdString());

                setAvatar(QImage::fromData(cache::client()->image(info_.avatar_url)));
        } catch (const lmdb::error &e) {
                qWarning() << "failed to retrieve room info from cache" << room_id;
        }

        constexpr int SettingsMargin = 2;

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(15);
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
        notifOptionLayout_->setMargin(SettingsMargin);
        auto notifLabel = new QLabel(tr("Notifications"), this);
        auto notifCombo = new QComboBox(this);
        notifCombo->setDisabled(true);
        notifCombo->addItem(tr("Muted"));
        notifCombo->addItem(tr("Mentions only"));
        notifCombo->addItem(tr("All messages"));
        notifLabel->setStyleSheet("font-size: 15px;");

        notifOptionLayout_->addWidget(notifLabel);
        notifOptionLayout_->addWidget(notifCombo, 0, Qt::AlignBottom | Qt::AlignRight);

        auto accessOptionLayout = new QHBoxLayout();
        accessOptionLayout->setMargin(SettingsMargin);
        auto accessLabel = new QLabel(tr("Room access"), this);
        accessCombo      = new QComboBox(this);
        accessCombo->addItem(tr("Anyone and guests"));
        accessCombo->addItem(tr("Anyone"));
        accessCombo->addItem(tr("Invited users"));
        accessCombo->setDisabled(true);
        accessLabel->setStyleSheet("font-size: 15px;");

        if (info_.join_rule == JoinRule::Public) {
                if (info_.guest_access) {
                        accessCombo->setCurrentIndex(0);
                } else {
                        accessCombo->setCurrentIndex(1);
                }
        } else {
                accessCombo->setCurrentIndex(2);
        }

        accessOptionLayout->addWidget(accessLabel);
        accessOptionLayout->addWidget(accessCombo);

        QFont font;
        font.setPixelSize(18);
        font.setWeight(70);

        auto menuLabel = new QLabel("Room Settings", this);
        menuLabel->setFont(font);

        constexpr int buttonSize = 36;
        constexpr int iconSize   = buttonSize / 2;

        QIcon editIcon;
        editIcon.addFile(":/icons/icons/ui/edit.svg");
        editFieldsBtn_ = new FlatButton(this);
        editFieldsBtn_->setFixedSize(buttonSize, buttonSize);
        editFieldsBtn_->setCornerRadius(iconSize);
        editFieldsBtn_->setIcon(editIcon);
        editFieldsBtn_->setIcon(editIcon);
        editFieldsBtn_->setIconSize(QSize(iconSize, iconSize));

        connect(editFieldsBtn_, &QPushButton::clicked, this, [this]() {
                auto modal = new EditModal(room_id_, this->parentWidget());
                modal->setFields(QString::fromStdString(info_.name),
                                 QString::fromStdString(info_.topic));
                modal->show();
                connect(modal, &EditModal::nameChanged, this, [this](const QString &newName) {
                        topSection_->setRoomName(newName);
                });
        });

        topSection_ = new TopSection(info_, avatarImg_, this);

        auto editLayout = new QHBoxLayout;
        editLayout->setMargin(0);
        editLayout->addWidget(topSection_);
        editLayout->addWidget(editFieldsBtn_, 0, Qt::AlignRight | Qt::AlignTop);

        layout->addWidget(menuLabel);
        layout->addLayout(editLayout);
        layout->addLayout(notifOptionLayout_);
        layout->addLayout(accessOptionLayout);
        layout->addLayout(btnLayout);

        connect(cancelBtn_, &QPushButton::clicked, this, &RoomSettings::closing);
        connect(saveBtn_, &QPushButton::clicked, this, &RoomSettings::saveSettings);
}

void
RoomSettings::saveSettings()
{
        // TODO: Save access changes to the room
        if (accessCombo->currentIndex() < 2) {
                if (info_.join_rule != JoinRule::Public) {
                        // Make join_rule Public
                }
                if (accessCombo->currentIndex() == 0) {
                        if (!info_.guest_access) {
                                // Make guest_access CanJoin
                        }
                }
        } else {
                if (info_.join_rule != JoinRule::Invite) {
                        // Make join_rule invite
                }
                if (info_.guest_access) {
                        // Make guest_access forbidden
                }
        }
        closing();
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

        constexpr int textStartX     = AvatarSize + 5 * Padding;
        const int availableTextWidth = width() - textStartX;

        constexpr int nameFont    = 15;
        constexpr int membersFont = 14;

        p.save();
        p.setPen(textColor());
        p.translate(textStartX, 2 * Padding);

        // Draw the name.
        QFont font;
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
