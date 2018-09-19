#include <QLabel>
#include <QPushButton>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/JoinRoom.h"

#include "Config.h"
#include "ui/TextField.h"
#include "ui/Theme.h"

using namespace dialogs;

JoinRoom::JoinRoom(QWidget *parent)
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
        buttonLayout->setSpacing(15);

        confirmBtn_ = new QPushButton(tr("Join"), this);
        cancelBtn_  = new QPushButton(tr("Cancel"), this);
        cancelBtn_->setDefault(true);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(cancelBtn_);
        buttonLayout->addWidget(confirmBtn_);

        roomInput_ = new TextField(this);
        roomInput_->setLabel(tr("Room ID or alias"));

        layout->addWidget(roomInput_);
        layout->addLayout(buttonLayout);
        layout->addStretch(1);

        connect(roomInput_, &QLineEdit::returnPressed, this, &JoinRoom::handleInput);
        connect(confirmBtn_, &QPushButton::clicked, this, &JoinRoom::handleInput);
        connect(cancelBtn_, &QPushButton::clicked, this, &JoinRoom::close);
}

void
JoinRoom::handleInput()
{
        if (roomInput_->text().isEmpty())
                return;

        // TODO: input validation with error messages.
        emit joinRoom(roomInput_->text());
        roomInput_->clear();
}

void
JoinRoom::showEvent(QShowEvent *event)
{
        roomInput_->setFocus();

        QFrame::showEvent(event);
}
