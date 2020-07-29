#include <cstdio>

#include <QDateTime>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QString>
#include <QTimer>

#include "ActiveCallBar.h"
#include "ChatPage.h"
#include "Utils.h"
#include "WebRTCSession.h"
#include "ui/Avatar.h"
#include "ui/FlatButton.h"

ActiveCallBar::ActiveCallBar(QWidget *parent)
  : QWidget(parent)
{
        setAutoFillBackground(true);
        auto p = palette();
        p.setColor(backgroundRole(), QColorConstants::Svg::limegreen);
        setPalette(p);

        QFont f;
        f.setPointSizeF(f.pointSizeF());

        const int fontHeight    = QFontMetrics(f).height();
        const int widgetMargin  = fontHeight / 3;
        const int contentHeight = fontHeight * 3;

        setFixedHeight(contentHeight + widgetMargin);

        layout_ = new QHBoxLayout(this);
        layout_->setSpacing(widgetMargin);
        layout_->setContentsMargins(
          2 * widgetMargin, widgetMargin, 2 * widgetMargin, widgetMargin);

        QFont labelFont;
        labelFont.setPointSizeF(labelFont.pointSizeF() * 1.1);
        labelFont.setWeight(QFont::Medium);

        avatar_ = new Avatar(this, QFontMetrics(f).height() * 2.5);

        callPartyLabel_ = new QLabel(this);
        callPartyLabel_->setFont(labelFont);

        stateLabel_ = new QLabel(this);
        stateLabel_->setFont(labelFont);

        durationLabel_ = new QLabel(this);
        durationLabel_->setFont(labelFont);
        durationLabel_->hide();

        muteBtn_ = new FlatButton(this);
        setMuteIcon(false);
        muteBtn_->setFixedSize(buttonSize_, buttonSize_);
        muteBtn_->setCornerRadius(buttonSize_ / 2);
        connect(muteBtn_, &FlatButton::clicked, this, [this](){
                if (WebRTCSession::instance().toggleMuteAudioSrc(muted_))
                    setMuteIcon(muted_);
        });

        layout_->addWidget(avatar_, 0, Qt::AlignLeft);
        layout_->addWidget(callPartyLabel_, 0, Qt::AlignLeft);
        layout_->addWidget(stateLabel_, 0, Qt::AlignLeft);
        layout_->addWidget(durationLabel_, 0, Qt::AlignLeft);
        layout_->addStretch();
        layout_->addWidget(muteBtn_, 0, Qt::AlignCenter);
        layout_->addSpacing(18);

        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this,
            [this](){
              auto seconds = QDateTime::currentSecsSinceEpoch() - callStartTime_;
              int s = seconds % 60;
              int m = (seconds / 60) % 60;
              int h = seconds / 3600;
              char buf[12];
              if (h)
                snprintf(buf, sizeof(buf), "%.2d:%.2d:%.2d", h, m, s);
              else
                snprintf(buf, sizeof(buf), "%.2d:%.2d", m, s);
              durationLabel_->setText(buf);
        });

        connect(&WebRTCSession::instance(), &WebRTCSession::stateChanged, this, &ActiveCallBar::update);
}

void
ActiveCallBar::setMuteIcon(bool muted)
{
        QIcon icon;
        if (muted) {
                muteBtn_->setToolTip("Unmute Mic");
                icon.addFile(":/icons/icons/ui/microphone-unmute.png");
        } else {
                muteBtn_->setToolTip("Mute Mic");
                icon.addFile(":/icons/icons/ui/microphone-mute.png");
        }
        muteBtn_->setIcon(icon);
        muteBtn_->setIconSize(QSize(buttonSize_, buttonSize_));
}

void
ActiveCallBar::setCallParty(
    const QString &userid,
    const QString &displayName,
    const QString &roomName,
    const QString &avatarUrl)
{
        callPartyLabel_->setText("  " +
            (displayName.isEmpty() ? userid : displayName) + " ");

        if (!avatarUrl.isEmpty())
          avatar_->setImage(avatarUrl);
        else
          avatar_->setLetter(utils::firstChar(roomName));
}

void
ActiveCallBar::update(WebRTCSession::State state)
{
        switch (state) {
          case WebRTCSession::State::INITIATING:
            show();
            stateLabel_->setText("Initiating call...");
            break;
          case WebRTCSession::State::INITIATED:
            show();
            stateLabel_->setText("Call initiated...");
            break;
          case WebRTCSession::State::OFFERSENT:
            show();
            stateLabel_->setText("Calling...");
            break;
          case WebRTCSession::State::CONNECTING:
            show();
            stateLabel_->setText("Connecting...");
            break;
          case WebRTCSession::State::CONNECTED:
            show();
            callStartTime_ = QDateTime::currentSecsSinceEpoch();
            timer_->start(1000);
            stateLabel_->setPixmap(QIcon(":/icons/icons/ui/place-call.png").
                pixmap(QSize(buttonSize_, buttonSize_)));
            durationLabel_->setText("00:00");
            durationLabel_->show();
            break;
          case WebRTCSession::State::ICEFAILED:
          case WebRTCSession::State::DISCONNECTED:
            hide();
            timer_->stop();
            callPartyLabel_->setText(QString());
            stateLabel_->setText(QString());
            durationLabel_->setText(QString());
            durationLabel_->hide();
            setMuteIcon(false);
            break;
          default:
            break;
        }
}
