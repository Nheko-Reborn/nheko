#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "ChatPage.h"
#include "Config.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "WebRTCSession.h"
#include "dialogs/PlaceCall.h"
#include "ui/Avatar.h"

namespace dialogs {

PlaceCall::PlaceCall(const QString &callee,
                     const QString &displayName,
                     const QString &roomName,
                     const QString &avatarUrl,
                     QSharedPointer<UserSettings> settings,
                     QWidget *parent)
  : QWidget(parent)
{
        std::string errorMessage;
        if (!WebRTCSession::instance().init(&errorMessage)) {
                emit ChatPage::instance()->showNotification(QString::fromStdString(errorMessage));
                emit close();
                return;
        }
        audioDevices_ = WebRTCSession::instance().getAudioSourceNames(
          settings->defaultAudioSource().toStdString());
        if (audioDevices_.empty()) {
                emit ChatPage::instance()->showNotification("No audio sources found.");
                emit close();
                return;
        }

        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose, true);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        auto buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(15);
        buttonLayout->setMargin(0);

        QFont f;
        f.setPointSizeF(f.pointSizeF());
        auto avatar = new Avatar(this, QFontMetrics(f).height() * 3);
        if (!avatarUrl.isEmpty())
                avatar->setImage(avatarUrl);
        else
                avatar->setLetter(utils::firstChar(roomName));
        const int iconSize = 18;
        voiceBtn_          = new QPushButton(tr("Voice"), this);
        voiceBtn_->setIcon(QIcon(":/icons/icons/ui/place-call.png"));
        voiceBtn_->setIconSize(QSize(iconSize, iconSize));
        voiceBtn_->setDefault(true);
        cancelBtn_ = new QPushButton(tr("Cancel"), this);

        buttonLayout->addWidget(avatar);
        buttonLayout->addStretch();
        buttonLayout->addWidget(voiceBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QString name  = displayName.isEmpty() ? callee : displayName;
        QLabel *label = new QLabel("Place a call to " + name + "?", this);

        auto deviceLayout = new QHBoxLayout;
        auto audioLabel   = new QLabel(this);
        audioLabel->setPixmap(QIcon(":/icons/icons/ui/microphone-unmute.png")
                                .pixmap(QSize(iconSize * 1.2, iconSize * 1.2)));

        auto deviceList = new QComboBox(this);
        for (const auto &d : audioDevices_)
                deviceList->addItem(QString::fromStdString(d));

        deviceLayout->addStretch();
        deviceLayout->addWidget(audioLabel);
        deviceLayout->addWidget(deviceList);

        layout->addWidget(label);
        layout->addLayout(buttonLayout);
        layout->addLayout(deviceLayout);

        connect(voiceBtn_, &QPushButton::clicked, this, [this, deviceList, settings]() {
                WebRTCSession::instance().setAudioSource(deviceList->currentIndex());
                settings->setDefaultAudioSource(
                  QString::fromStdString(audioDevices_[deviceList->currentIndex()]));
                emit voice();
                emit close();
        });
        connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
                emit cancel();
                emit close();
        });
}

}
