#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "Config.h"
#include "Utils.h"
#include "dialogs/PlaceCall.h"
#include "ui/Avatar.h"

namespace dialogs {

PlaceCall::PlaceCall(const QString &callee,
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
        const int iconSize = 24;
        voiceBtn_          = new QPushButton(tr("Voice"), this);
        voiceBtn_->setIcon(QIcon(":/icons/icons/ui/place-call.png"));
        voiceBtn_->setIconSize(QSize(iconSize, iconSize));
        voiceBtn_->setDefault(true);
        cancelBtn_ = new QPushButton(tr("Cancel"), this);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(avatar);
        buttonLayout->addWidget(voiceBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QString name  = displayName.isEmpty() ? callee : displayName;
        QLabel *label = new QLabel("Place a call to " + name + "?", this);

        layout->addWidget(label);
        layout->addLayout(buttonLayout);

        connect(voiceBtn_, &QPushButton::clicked, this, [this]() {
                emit voice();
                emit close();
        });
        connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
                emit cancel();
                emit close();
        });
}

}
