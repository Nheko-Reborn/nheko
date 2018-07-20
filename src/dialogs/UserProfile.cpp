#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPaintEvent>
#include <QSettings>
#include <QStyleOption>
#include <QVBoxLayout>

#include "AvatarProvider.h"
#include "Cache.h"
#include "Utils.h"
#include "dialogs/UserProfile.h"
#include "ui/Avatar.h"
#include "ui/FlatButton.h"

using namespace dialogs;

constexpr int BUTTON_SIZE = 36;

DeviceItem::DeviceItem(QWidget *parent, QString deviceName)
  : QWidget(parent)
  , name_(deviceName)
{}

UserProfile::UserProfile(QWidget *parent)
  : QWidget(parent)
{
        QIcon banIcon, kickIcon, ignoreIcon, startChatIcon;

        banIcon.addFile(":/icons/icons/ui/do-not-disturb-rounded-sign.png");
        banBtn_ = new FlatButton(this);
        banBtn_->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
        banBtn_->setCornerRadius(BUTTON_SIZE / 2);
        banBtn_->setIcon(banIcon);
        banBtn_->setIconSize(QSize(BUTTON_SIZE / 2, BUTTON_SIZE / 2));
        banBtn_->setToolTip(tr("Ban the user from the room"));

        ignoreIcon.addFile(":/icons/icons/ui/volume-off-indicator.png");
        ignoreBtn_ = new FlatButton(this);
        ignoreBtn_->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
        ignoreBtn_->setCornerRadius(BUTTON_SIZE / 2);
        ignoreBtn_->setIcon(ignoreIcon);
        ignoreBtn_->setIconSize(QSize(BUTTON_SIZE / 2, BUTTON_SIZE / 2));
        ignoreBtn_->setToolTip(tr("Ignore messages from this user"));

        kickIcon.addFile(":/icons/icons/ui/round-remove-button.png");
        kickBtn_ = new FlatButton(this);
        kickBtn_->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
        kickBtn_->setCornerRadius(BUTTON_SIZE / 2);
        kickBtn_->setIcon(kickIcon);
        kickBtn_->setIconSize(QSize(BUTTON_SIZE / 2, BUTTON_SIZE / 2));
        kickBtn_->setToolTip(tr("Kick the user from the room"));

        startChatIcon.addFile(":/icons/icons/ui/black-bubble-speech.png");
        startChat_ = new FlatButton(this);
        startChat_->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
        startChat_->setCornerRadius(BUTTON_SIZE / 2);
        startChat_->setIcon(startChatIcon);
        startChat_->setIconSize(QSize(BUTTON_SIZE / 2, BUTTON_SIZE / 2));
        startChat_->setToolTip(tr("Start a conversation"));

        // Button line
        auto btnLayout = new QHBoxLayout;
        btnLayout->addWidget(startChat_);
        btnLayout->addWidget(ignoreBtn_);

        // TODO: check if the user has enough power level given the room_id
        // in which the profile was opened.
        btnLayout->addWidget(kickBtn_);
        btnLayout->addWidget(banBtn_);
        btnLayout->setSpacing(8);
        btnLayout->setMargin(0);

        avatar_ = new Avatar(this);
        avatar_->setLetter("X");
        avatar_->setSize(148);

        QFont font;
        font.setPointSizeF(font.pointSizeF() * 2);

        userIdLabel_      = new QLabel(this);
        displayNameLabel_ = new QLabel(this);
        displayNameLabel_->setFont(font);

        auto textLayout = new QVBoxLayout;
        textLayout->addWidget(displayNameLabel_);
        textLayout->addWidget(userIdLabel_);
        textLayout->setAlignment(displayNameLabel_, Qt::AlignCenter | Qt::AlignTop);
        textLayout->setAlignment(userIdLabel_, Qt::AlignCenter | Qt::AlignTop);
        textLayout->setSpacing(4);
        textLayout->setMargin(0);

        auto vlayout = new QVBoxLayout{this};
        vlayout->addWidget(avatar_);
        vlayout->addLayout(textLayout);
        vlayout->addLayout(btnLayout);

        vlayout->setAlignment(avatar_, Qt::AlignCenter | Qt::AlignTop);
        vlayout->setAlignment(userIdLabel_, Qt::AlignCenter | Qt::AlignTop);

        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);

        setMinimumWidth(340);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        vlayout->setSpacing(15);
        vlayout->setContentsMargins(20, 40, 20, 20);
}

void
UserProfile::init(const QString &userId, const QString &roomId)
{
        auto displayName = Cache::displayName(roomId, userId);

        userIdLabel_->setText(userId);
        displayNameLabel_->setText(displayName);
        avatar_->setLetter(utils::firstChar(displayName));

        AvatarProvider::resolve(
          roomId, userId, this, [this](const QImage &img) { avatar_->setImage(img); });

        QSettings settings;
        auto localUser = settings.value("auth/user_id").toString();

        if (localUser == userId) {
                qDebug() << "the local user should have edit rights on avatar & display name";
                // TODO: click on display name & avatar to change.
        }

        try {
                bool hasMemberRights =
                  cache::client()->hasEnoughPowerLevel({mtx::events::EventType::RoomMember},
                                                       roomId.toStdString(),
                                                       localUser.toStdString());
                if (!hasMemberRights) {
                        kickBtn_->hide();
                        banBtn_->hide();
                }
        } catch (const lmdb::error &e) {
                nhlog::db()->warn("lmdb error: {}", e.what());
        }
}

void
UserProfile::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
