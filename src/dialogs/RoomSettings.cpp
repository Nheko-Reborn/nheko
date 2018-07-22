#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QSharedPointer>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/RoomSettings.h"

#include "ChatPage.h"
#include "Config.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"
#include "ui/Avatar.h"
#include "ui/FlatButton.h"
#include "ui/Painter.h"
#include "ui/TextField.h"
#include "ui/Theme.h"
#include "ui/ToggleButton.h"

using namespace dialogs;
using namespace mtx::events;

constexpr int BUTTON_SIZE       = 36;
constexpr int BUTTON_RADIUS     = BUTTON_SIZE / 2;
constexpr int WIDGET_MARGIN     = 20;
constexpr int TOP_WIDGET_MARGIN = 2 * WIDGET_MARGIN;
constexpr int WIDGET_SPACING    = 15;
constexpr int TEXT_SPACING      = 4;
constexpr int BUTTON_SPACING    = 2 * TEXT_SPACING;

EditModal::EditModal(const QString &roomId, QWidget *parent)
  : QWidget(parent)
  , roomId_{roomId}
{
        setAutoFillBackground(true);
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);

        QFont doubleFont;
        doubleFont.setPointSizeF(doubleFont.pointSizeF() * 2);
        setMinimumWidth(QFontMetrics(doubleFont).averageCharWidth() * 30 - 2 * WIDGET_MARGIN);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto layout = new QVBoxLayout(this);

        QFont buttonFont;
        buttonFont.setPointSizeF(buttonFont.pointSizeF() * 1.3);

        applyBtn_ = new FlatButton(tr("APPLY"), this);
        applyBtn_->setFont(buttonFont);
        applyBtn_->setRippleStyle(ui::RippleStyle::NoRipple);
        applyBtn_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFont(buttonFont);
        cancelBtn_->setRippleStyle(ui::RippleStyle::NoRipple);
        cancelBtn_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto btnLayout = new QHBoxLayout;
        btnLayout->setMargin(5);
        btnLayout->addStretch(1);
        btnLayout->addWidget(applyBtn_);
        btnLayout->addWidget(cancelBtn_);

        nameInput_ = new TextField(this);
        nameInput_->setLabel(tr("Name").toUpper());
        topicInput_ = new TextField(this);
        topicInput_->setLabel(tr("Topic").toUpper());

        errorField_ = new QLabel(this);
        errorField_->setWordWrap(true);
        errorField_->hide();

        layout->addWidget(nameInput_);
        layout->addWidget(topicInput_);
        layout->addLayout(btnLayout, 1);

        auto labelLayout = new QHBoxLayout;
        labelLayout->setAlignment(Qt::AlignHCenter);
        labelLayout->addWidget(errorField_);
        layout->addLayout(labelLayout);

        connect(this, &EditModal::stateEventErrorCb, this, [this](const QString &msg) {
                errorField_->setText(msg);
                errorField_->show();
        });
        connect(this, &EditModal::nameEventSentCb, this, [this](const QString &newName) {
                errorField_->hide();
                emit nameChanged(newName);
                close();
        });
        connect(this, &EditModal::topicEventSentCb, this, [this]() {
                errorField_->hide();
                close();
        });

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

                        http::client()->send_state_event<state::Name, EventType::RoomName>(
                          roomId_.toStdString(),
                          body,
                          [this, newName](const mtx::responses::EventId &,
                                          mtx::http::RequestErr err) {
                                  if (err) {
                                          emit stateEventErrorCb(
                                            QString::fromStdString(err->matrix_error.error));
                                          return;
                                  }

                                  emit nameEventSentCb(newName);
                          });
                }

                if (newTopic != initialTopic_ && !newTopic.isEmpty()) {
                        state::Topic body;
                        body.topic = newTopic.toStdString();

                        http::client()->send_state_event<state::Topic, EventType::RoomTopic>(
                          roomId_.toStdString(),
                          body,
                          [this](const mtx::responses::EventId &, mtx::http::RequestErr err) {
                                  if (err) {
                                          emit stateEventErrorCb(
                                            QString::fromStdString(err->matrix_error.error));
                                          return;
                                  }

                                  emit topicEventSentCb();
                          });
                }
        });
        connect(cancelBtn_, &QPushButton::clicked, this, &EditModal::close);

        auto window = QApplication::activeWindow();
        auto center = window->frameGeometry().center();
        move(center.x() - (width() * 0.5), center.y() - (height() * 0.5));
}

void
EditModal::setFields(const QString &roomName, const QString &roomTopic)
{
        initialName_  = roomName;
        initialTopic_ = roomTopic;

        nameInput_->setText(roomName);
        topicInput_->setText(roomTopic);
}

RoomSettings::RoomSettings(const QString &room_id, QWidget *parent)
  : QFrame(parent)
  , room_id_{std::move(room_id)}
{
        retrieveRoomInfo();

        QFont doubleFont;
        doubleFont.setPointSizeF(doubleFont.pointSizeF() * 2);

        setMinimumWidth(QFontMetrics(doubleFont).averageCharWidth() * 30 - 2 * WIDGET_MARGIN);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(WIDGET_SPACING);
        layout->setContentsMargins(WIDGET_MARGIN, TOP_WIDGET_MARGIN, WIDGET_MARGIN, WIDGET_MARGIN);

        QFont font;
        font.setWeight(65);
        font.setPointSizeF(font.pointSizeF() * 1.2);
        auto settingsLabel = new QLabel(tr("Settings").toUpper(), this);
        settingsLabel->setFont(font);

        auto notifLabel = new QLabel(tr("Notifications"), this);
        auto notifCombo = new QComboBox(this);
        notifCombo->setDisabled(true);
        notifCombo->addItem(tr("Muted"));
        notifCombo->addItem(tr("Mentions only"));
        notifCombo->addItem(tr("All messages"));

        auto notifOptionLayout_ = new QHBoxLayout;
        notifOptionLayout_->setMargin(0);
        notifOptionLayout_->addWidget(notifLabel, Qt::AlignBottom | Qt::AlignLeft);
        notifOptionLayout_->addWidget(notifCombo, 0, Qt::AlignBottom | Qt::AlignRight);

        auto accessLabel = new QLabel(tr("Room access"), this);
        accessCombo      = new QComboBox(this);
        accessCombo->addItem(tr("Anyone and guests"));
        accessCombo->addItem(tr("Anyone"));
        accessCombo->addItem(tr("Invited users"));
        accessCombo->setDisabled(true);

        if (info_.join_rule == JoinRule::Public) {
                if (info_.guest_access) {
                        accessCombo->setCurrentIndex(0);
                } else {
                        accessCombo->setCurrentIndex(1);
                }
        } else {
                accessCombo->setCurrentIndex(2);
        }

        auto accessOptionLayout = new QHBoxLayout();
        accessOptionLayout->setMargin(0);
        accessOptionLayout->addWidget(accessLabel, Qt::AlignBottom | Qt::AlignLeft);
        accessOptionLayout->addWidget(accessCombo, 0, Qt::AlignBottom | Qt::AlignRight);

        auto encryptionLabel = new QLabel(tr("Encryption"), this);
        encryptionToggle_    = new Toggle(this);
        connect(encryptionToggle_, &Toggle::toggled, this, [this](bool isOn) {
                if (isOn)
                        return;

                QMessageBox msgBox;
                msgBox.setIcon(QMessageBox::Question);
                msgBox.setWindowTitle(tr("End-to-End Encryption"));
                msgBox.setText(tr(
                  "Encryption is currently experimental and things might break unexpectedly. <br>"
                  "Please take note that it can't be disabled afterwards."));
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                msgBox.setDefaultButton(QMessageBox::Save);
                int ret = msgBox.exec();

                switch (ret) {
                case QMessageBox::Ok: {
                        encryptionToggle_->setState(false);
                        encryptionToggle_->setEnabled(false);
                        enableEncryption();
                        break;
                }
                default: {
                        encryptionToggle_->setState(true);
                        encryptionToggle_->setEnabled(true);
                        break;
                }
                }
        });

        auto encryptionOptionLayout = new QHBoxLayout;
        encryptionOptionLayout->setMargin(0);
        encryptionOptionLayout->addWidget(encryptionLabel, Qt::AlignBottom | Qt::AlignLeft);
        encryptionOptionLayout->addWidget(encryptionToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto keyRequestsLabel = new QLabel(tr("Respond to key requests"), this);
        keyRequestsLabel->setToolTipDuration(6000);
        keyRequestsLabel->setToolTip(
          tr("Whether or not the client should respond automatically with the session keys\n"
             " upon request. Use with caution, this is a temporary measure to test the\n"
             " E2E implementation until device verification is completed."));
        keyRequestsToggle_ = new Toggle(this);
        connect(keyRequestsToggle_, &Toggle::toggled, this, [this](bool isOn) {
                utils::setKeyRequestsPreference(room_id_, !isOn);
        });

        auto keyRequestsLayout = new QHBoxLayout;
        keyRequestsLayout->setMargin(0);
        keyRequestsLayout->setSpacing(0);
        keyRequestsLayout->addWidget(keyRequestsLabel, Qt::AlignBottom | Qt::AlignLeft);
        keyRequestsLayout->addWidget(keyRequestsToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        // Disable encryption button.
        if (usesEncryption_) {
                encryptionToggle_->setState(false);
                encryptionToggle_->setEnabled(false);

                keyRequestsToggle_->setState(!utils::respondsToKeyRequests(room_id_));
        } else {
                encryptionToggle_->setState(true);

                keyRequestsLabel->hide();
                keyRequestsToggle_->hide();
        }

        // Hide encryption option for public rooms.
        if (!usesEncryption_ && (info_.join_rule == JoinRule::Public)) {
                encryptionToggle_->hide();
                encryptionLabel->hide();

                keyRequestsLabel->hide();
                keyRequestsToggle_->hide();
        }

        avatar_ = new Avatar(this);
        avatar_->setSize(128);
        if (avatarImg_.isNull())
                avatar_->setLetter(utils::firstChar(QString::fromStdString(info_.name)));
        else
                avatar_->setImage(avatarImg_);

        auto roomNameLabel = new QLabel(QString::fromStdString(info_.name), this);
        roomNameLabel->setFont(doubleFont);

        auto membersLabel = new QLabel(tr("%n member(s)", "", info_.member_count), this);

        auto textLayout = new QVBoxLayout;
        textLayout->addWidget(roomNameLabel);
        textLayout->addWidget(membersLabel);
        textLayout->setAlignment(roomNameLabel, Qt::AlignCenter | Qt::AlignTop);
        textLayout->setAlignment(membersLabel, Qt::AlignCenter | Qt::AlignTop);
        textLayout->setSpacing(TEXT_SPACING);
        textLayout->setMargin(0);

        setupEditButton();

        layout->addWidget(avatar_, Qt::AlignCenter | Qt::AlignTop);
        layout->addLayout(textLayout);
        layout->addLayout(btnLayout_);
        layout->addWidget(settingsLabel, Qt::AlignLeft);
        layout->addLayout(notifOptionLayout_);
        layout->addLayout(accessOptionLayout);
        layout->addLayout(encryptionOptionLayout);
        layout->addLayout(keyRequestsLayout);
        layout->addStretch(1);

        connect(this, &RoomSettings::enableEncryptionError, this, [this](const QString &msg) {
                encryptionToggle_->setState(true);
                encryptionToggle_->setEnabled(true);

                emit ChatPage::instance()->showNotification(msg);
        });
}

void
RoomSettings::setupEditButton()
{
        btnLayout_ = new QHBoxLayout;
        btnLayout_->setSpacing(BUTTON_SPACING);
        btnLayout_->setMargin(0);

        try {
                auto userId = utils::localUser().toStdString();

                hasEditRights_ = cache::client()->hasEnoughPowerLevel(
                  {EventType::RoomName, EventType::RoomTopic}, room_id_.toStdString(), userId);
        } catch (const lmdb::error &e) {
                nhlog::db()->warn("lmdb error: {}", e.what());
        }

        if (!hasEditRights_)
                return;

        QIcon editIcon;
        editIcon.addFile(":/icons/icons/ui/edit.png");
        editFieldsBtn_ = new FlatButton(this);
        editFieldsBtn_->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
        editFieldsBtn_->setCornerRadius(BUTTON_RADIUS);
        editFieldsBtn_->setIcon(editIcon);
        editFieldsBtn_->setIcon(editIcon);
        editFieldsBtn_->setIconSize(QSize(BUTTON_RADIUS, BUTTON_RADIUS));

        connect(editFieldsBtn_, &QPushButton::clicked, this, [this]() {
                retrieveRoomInfo();

                auto modal = new EditModal(room_id_, this->parentWidget());
                modal->setFields(QString::fromStdString(info_.name),
                                 QString::fromStdString(info_.topic));
                modal->show();
                connect(modal, &EditModal::nameChanged, this, [](const QString &newName) {
                        Q_UNUSED(newName);
                });
        });

        btnLayout_->addStretch(1);
        btnLayout_->addWidget(editFieldsBtn_);
        btnLayout_->addStretch(1);
}

void
RoomSettings::retrieveRoomInfo()
{
        try {
                usesEncryption_ = cache::client()->isRoomEncrypted(room_id_.toStdString());
                info_           = cache::client()->singleRoomInfo(room_id_.toStdString());
                setAvatar(QImage::fromData(cache::client()->image(info_.avatar_url)));
        } catch (const lmdb::error &e) {
                nhlog::db()->warn("failed to retrieve room info from cache: {}",
                                  room_id_.toStdString());
        }
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
RoomSettings::enableEncryption()
{
        const auto room_id = room_id_.toStdString();
        http::client()->enable_encryption(
          room_id, [room_id, this](const mtx::responses::EventId &, mtx::http::RequestErr err) {
                  if (err) {
                          int status_code = static_cast<int>(err->status_code);
                          nhlog::net()->warn("failed to enable encryption in room ({}): {} {}",
                                             room_id,
                                             err->matrix_error.error,
                                             status_code);
                          emit enableEncryptionError(
                            tr("Failed to enable encryption: %1")
                              .arg(QString::fromStdString(err->matrix_error.error)));
                          return;
                  }

                  nhlog::net()->info("enabled encryption on room ({})", room_id);
          });
}

void
RoomSettings::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
