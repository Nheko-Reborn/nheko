#include <QLabel>
#include <QPushButton>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/LeaveRoom.h"

#include "Config.h"
#include "ui/Theme.h"

using namespace dialogs;

LeaveRoom::LeaveRoom(QWidget *parent)
  : QFrame(parent)
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

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setMargin(0);

        confirmBtn_ = new QPushButton("Leave", this);
        cancelBtn_  = new QPushButton(tr("Cancel"), this);
        cancelBtn_->setDefault(true);

        buttonLayout->addStretch(1);
        buttonLayout->setSpacing(15);
        buttonLayout->addWidget(cancelBtn_);
        buttonLayout->addWidget(confirmBtn_);

        auto label = new QLabel(tr("Are you sure you want to leave?"), this);

        layout->addWidget(label);
        layout->addLayout(buttonLayout);

        connect(confirmBtn_, &QPushButton::clicked, this, &LeaveRoom::leaving);
        connect(cancelBtn_, &QPushButton::clicked, this, &LeaveRoom::close);
}
