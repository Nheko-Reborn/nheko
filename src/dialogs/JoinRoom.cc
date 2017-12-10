#include <QLabel>
#include <QStyleOption>
#include <QVBoxLayout>

#include "Config.h"
#include "FlatButton.h"
#include "TextField.h"
#include "Theme.h"

#include "dialogs/JoinRoom.h"

using namespace dialogs;

JoinRoom::JoinRoom(QWidget *parent)
  : QFrame(parent)
{
        setMaximumSize(400, 400);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(30);
        layout->setMargin(20);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setMargin(0);

        confirmBtn_ = new FlatButton("JOIN", this);
        confirmBtn_->setFontSize(conf::btn::fontSize);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFontSize(conf::btn::fontSize);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QFont font;
        font.setPixelSize(conf::headerFontSize);

        roomInput_ = new TextField(this);
        roomInput_->setLabel(tr("Room ID or alias"));

        layout->addWidget(roomInput_);
        layout->addLayout(buttonLayout);

        // TODO: input validation with error messages.
        connect(confirmBtn_, &QPushButton::clicked, [=]() {
                emit closing(true, roomInput_->text());
                roomInput_->clear();
        });
        connect(cancelBtn_, &QPushButton::clicked, [=]() { emit closing(false, ""); });
}

void
JoinRoom::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
