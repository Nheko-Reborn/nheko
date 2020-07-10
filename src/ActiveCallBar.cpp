#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QString>

#include "ActiveCallBar.h"
#include "WebRTCSession.h"
#include "ui/FlatButton.h"

ActiveCallBar::ActiveCallBar(QWidget *parent)
  : QWidget(parent)
{
        setAutoFillBackground(true);
        auto p = palette();
        p.setColor(backgroundRole(), Qt::green);
        setPalette(p);

        QFont f;
        f.setPointSizeF(f.pointSizeF());

        const int fontHeight    = QFontMetrics(f).height();
        const int widgetMargin  = fontHeight / 3;
        const int contentHeight = fontHeight * 3;

        setFixedHeight(contentHeight + widgetMargin);

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(widgetMargin);
        topLayout_->setContentsMargins(
          2 * widgetMargin, widgetMargin, 2 * widgetMargin, widgetMargin);
        topLayout_->setSizeConstraint(QLayout::SetMinimumSize);

        QFont labelFont;
        labelFont.setPointSizeF(labelFont.pointSizeF() * 1.2);
        labelFont.setWeight(QFont::Medium);

        callPartyLabel_ = new QLabel(this);
        callPartyLabel_->setFont(labelFont);

        // TODO microphone mute/unmute icons
        muteBtn_ = new FlatButton(this);
        QIcon muteIcon;
        muteIcon.addFile(":/icons/icons/ui/do-not-disturb-rounded-sign.png");
        muteBtn_->setIcon(muteIcon);
        muteBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));
        muteBtn_->setToolTip(tr("Mute Mic"));
        muteBtn_->setFixedSize(buttonSize_, buttonSize_);
        muteBtn_->setCornerRadius(buttonSize_ / 2);
        connect(muteBtn_, &FlatButton::clicked, this, [this]() {
                if (WebRTCSession::instance().toggleMuteAudioSrc(muted_)) {
                        QIcon icon;
                        if (muted_) {
                                muteBtn_->setToolTip("Unmute Mic");
                                icon.addFile(":/icons/icons/ui/round-remove-button.png");
                        } else {
                                muteBtn_->setToolTip("Mute Mic");
                                icon.addFile(":/icons/icons/ui/do-not-disturb-rounded-sign.png");
                        }
                        muteBtn_->setIcon(icon);
                }
        });

        topLayout_->addWidget(callPartyLabel_, 0, Qt::AlignLeft);
        topLayout_->addWidget(muteBtn_, 0, Qt::AlignRight);
}

void
ActiveCallBar::setCallParty(const QString &userid, const QString &displayName)
{
        if (!displayName.isEmpty() && displayName != userid)
                callPartyLabel_->setText("Active Call: " + displayName + " (" + userid + ")");
        else
                callPartyLabel_->setText("Active Call: " + userid);
}
