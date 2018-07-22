#include <QLabel>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/JoinRoom.h"

#include "Config.h"
#include "ui/FlatButton.h"
#include "ui/TextField.h"
#include "ui/Theme.h"

using namespace dialogs;

JoinRoom::JoinRoom(QWidget *parent)
  : QFrame(parent)
{
        setMinimumWidth(conf::modals::MIN_WIDGET_WIDTH);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setMargin(0);

        QFont buttonFont;
        buttonFont.setPointSizeF(buttonFont.pointSizeF() * conf::modals::BUTTON_TEXT_SIZE_RATIO);

        confirmBtn_ = new FlatButton("JOIN", this);
        confirmBtn_->setFont(buttonFont);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFont(buttonFont);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        roomInput_ = new TextField(this);
        roomInput_->setLabel(tr("Room ID or alias"));

        layout->addWidget(roomInput_);
        layout->addLayout(buttonLayout);
        layout->addStretch(1);

        connect(roomInput_, &QLineEdit::returnPressed, this, &JoinRoom::handleInput);
        connect(confirmBtn_, &QPushButton::clicked, this, &JoinRoom::handleInput);
        connect(cancelBtn_, &QPushButton::clicked, [this]() { emit closing(false, ""); });
}

void
JoinRoom::handleInput()
{
        if (roomInput_->text().isEmpty())
                return;

        // TODO: input validation with error messages.
        emit closing(true, roomInput_->text());
        roomInput_->clear();
}

void
JoinRoom::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
JoinRoom::showEvent(QShowEvent *event)
{
        roomInput_->setFocus();

        QFrame::showEvent(event);
}
