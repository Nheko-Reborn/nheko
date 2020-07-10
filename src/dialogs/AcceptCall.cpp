#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Config.h"
#include "dialogs/AcceptCall.h"

namespace dialogs {

AcceptCall::AcceptCall(const QString &caller, const QString &displayName, QWidget *parent)
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

        acceptBtn_ = new QPushButton(tr("Accept"), this);
        acceptBtn_->setDefault(true);
        rejectBtn_ = new QPushButton(tr("Reject"), this);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(acceptBtn_);
        buttonLayout->addWidget(rejectBtn_);

        QLabel *label;
        if (!displayName.isEmpty() && displayName != caller)
                label = new QLabel("Accept call from " + displayName + " (" + caller + ")?", this);
        else
                label = new QLabel("Accept call from " + caller + "?", this);

        layout->addWidget(label);
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
