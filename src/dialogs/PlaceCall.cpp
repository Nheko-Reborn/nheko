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
        WebRTCSession *session = &WebRTCSession::instance();
        if (!session->havePlugins(false, &errorMessage)) {
                emit ChatPage::instance()->showNotification(QString::fromStdString(errorMessage));
                emit close();
                return;
        }
        session->refreshDevices();
        microphones_ = session->getDeviceNames(false, settings->microphone().toStdString());
        if (microphones_.empty()) {
                emit ChatPage::instance()->showNotification(tr("No microphone found."));
                emit close();
                return;
        }
        cameras_ = session->getDeviceNames(true, settings->camera().toStdString());

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

        voiceBtn_ = new QPushButton(tr("Voice"), this);
        voiceBtn_->setIcon(QIcon(":/icons/icons/ui/place-call.png"));
        voiceBtn_->setIconSize(QSize(iconSize_, iconSize_));
        voiceBtn_->setDefault(true);

        if (!cameras_.empty()) {
                videoBtn_ = new QPushButton(tr("Video"), this);
                videoBtn_->setIcon(QIcon(":/icons/icons/ui/video-call.png"));
                videoBtn_->setIconSize(QSize(iconSize_, iconSize_));
        }
        cancelBtn_ = new QPushButton(tr("Cancel"), this);

        buttonLayout->addWidget(avatar);
        buttonLayout->addStretch();
        buttonLayout->addWidget(voiceBtn_);
        if (videoBtn_)
                buttonLayout->addWidget(videoBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QString name  = displayName.isEmpty() ? callee : displayName;
        QLabel *label = new QLabel(tr("Place a call to ") + name + "?", this);

        microphoneCombo_ = new QComboBox(this);
        for (const auto &m : microphones_)
                microphoneCombo_->addItem(QIcon(":/icons/icons/ui/microphone-unmute.png"),
                                          QString::fromStdString(m));

        if (videoBtn_) {
                cameraCombo_ = new QComboBox(this);
                for (const auto &c : cameras_)
                        cameraCombo_->addItem(QIcon(":/icons/icons/ui/video-call.png"),
                                              QString::fromStdString(c));
        }

        layout->addWidget(label);
        layout->addLayout(buttonLayout);
        layout->addStretch();
        layout->addWidget(microphoneCombo_);
        if (videoBtn_)
                layout->addWidget(cameraCombo_);

        connect(voiceBtn_, &QPushButton::clicked, this, [this, settings]() {
                settings->setMicrophone(
                  QString::fromStdString(microphones_[microphoneCombo_->currentIndex()]));
                emit voice();
                emit close();
        });
        if (videoBtn_)
                connect(videoBtn_, &QPushButton::clicked, this, [this, settings, session]() {
                        std::string error;
                        if (!session->havePlugins(true, &error)) {
                                emit ChatPage::instance()->showNotification(
                                  QString::fromStdString(error));
                                emit close();
                                return;
                        }
                        settings->setMicrophone(
                          QString::fromStdString(microphones_[microphoneCombo_->currentIndex()]));
                        settings->setCamera(
                          QString::fromStdString(cameras_[cameraCombo_->currentIndex()]));
                        emit video();
                        emit close();
                });
        connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
                emit cancel();
                emit close();
        });
}

}
