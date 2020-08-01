#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "Config.h"
#include "Utils.h"
#include "dialogs/AcceptCall.h"
#include "ui/Avatar.h"

namespace dialogs {

AcceptCall::AcceptCall(const QString &caller,
                       const QString &displayName,
                       const QString &roomName,
                       const QString &avatarUrl,
                       QWidget *parent)
  : QWidget(parent)
{
        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose, true);

        setMinimumWidth(conf::modals::MIN_WIDGET_WIDTH);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        QFont f;
        f.setPointSizeF(f.pointSizeF());

        QFont labelFont;
        labelFont.setWeight(QFont::Medium);

        QLabel *displayNameLabel = nullptr;
        if (!displayName.isEmpty() && displayName != caller) {
                displayNameLabel = new QLabel(displayName, this);
                labelFont.setPointSizeF(f.pointSizeF() * 2);
                displayNameLabel->setFont(labelFont);
                displayNameLabel->setAlignment(Qt::AlignCenter);
        }

        QLabel *callerLabel = new QLabel(caller, this);
        labelFont.setPointSizeF(f.pointSizeF() * 1.2);
        callerLabel->setFont(labelFont);
        callerLabel->setAlignment(Qt::AlignCenter);

        auto avatar = new Avatar(this, QFontMetrics(f).height() * 6);
        if (!avatarUrl.isEmpty())
                avatar->setImage(avatarUrl);
        else
                avatar->setLetter(utils::firstChar(roomName));

        const int iconSize        = 24;
        QLabel *callTypeIndicator = new QLabel(this);
        QPixmap callIndicator(":/icons/icons/ui/place-call.png");
        callTypeIndicator->setPixmap(callIndicator.scaled(iconSize * 2, iconSize * 2));

        QLabel *callTypeLabel = new QLabel("Voice Call", this);
        labelFont.setPointSizeF(f.pointSizeF() * 1.1);
        callTypeLabel->setFont(labelFont);
        callTypeLabel->setAlignment(Qt::AlignCenter);

        auto buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(20);
        acceptBtn_ = new QPushButton(tr("Accept"), this);
        acceptBtn_->setDefault(true);
        acceptBtn_->setIcon(QIcon(":/icons/icons/ui/place-call.png"));
        acceptBtn_->setIconSize(QSize(iconSize, iconSize));

        rejectBtn_ = new QPushButton(tr("Reject"), this);
        rejectBtn_->setIcon(QIcon(":/icons/icons/ui/end-call.png"));
        rejectBtn_->setIconSize(QSize(iconSize, iconSize));
        buttonLayout->addWidget(acceptBtn_);
        buttonLayout->addWidget(rejectBtn_);

        if (displayNameLabel)
                layout->addWidget(displayNameLabel, 0, Qt::AlignCenter);
        layout->addWidget(callerLabel, 0, Qt::AlignCenter);
        layout->addWidget(avatar, 0, Qt::AlignCenter);
        layout->addWidget(callTypeIndicator, 0, Qt::AlignCenter);
        layout->addWidget(callTypeLabel, 0, Qt::AlignCenter);
        layout->addLayout(buttonLayout);

        connect(acceptBtn_, &QPushButton::clicked, this, [this]() {
                emit accept();
                emit close();
        });
        connect(rejectBtn_, &QPushButton::clicked, this, [this]() {
                emit reject();
                emit close();
        });
}

}
