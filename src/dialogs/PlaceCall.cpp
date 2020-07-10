#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "Config.h"
#include "dialogs/PlaceCall.h"

namespace dialogs {

PlaceCall::PlaceCall(const QString &callee, const QString &displayName, QWidget *parent)
  : QWidget(parent)
{
        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose, true);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(15);
        buttonLayout->setMargin(0);

        voiceBtn_ = new QPushButton(tr("Voice Call"), this);
        voiceBtn_->setDefault(true);
        videoBtn_  = new QPushButton(tr("Video Call"), this);
        cancelBtn_ = new QPushButton(tr("Cancel"), this);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(voiceBtn_);
        buttonLayout->addWidget(videoBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QLabel *label;
        if (!displayName.isEmpty() && displayName != callee)
                label = new QLabel("Place a call to " + displayName + " (" + callee + ")?", this);
        else
                label = new QLabel("Place a call to " + callee + "?", this);

        layout->addWidget(label);
        layout->addLayout(buttonLayout);

        connect(voiceBtn_, &QPushButton::clicked, this, [this]() {
                emit voice();
                emit close();
        });
        connect(videoBtn_, &QPushButton::clicked, this, [this]() {
                emit video();
                emit close();
        });
        connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
                emit cancel();
                emit close();
        });
}

}
