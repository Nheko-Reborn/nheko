#include <QLabel>
#include <QStyleOption>
#include <QVBoxLayout>

#include "Config.h"
#include "FlatButton.h"
#include "Theme.h"

#include "dialogs/LeaveRoom.h"

using namespace dialogs;

LeaveRoom::LeaveRoom(QWidget *parent)
  : QFrame(parent)
{
        setMaximumSize(400, 400);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(30);
        layout->setMargin(20);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setMargin(0);

        confirmBtn_ = new FlatButton("LEAVE", this);
        confirmBtn_->setFontSize(conf::btn::fontSize);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFontSize(conf::btn::fontSize);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QFont font;
        font.setPixelSize(conf::headerFontSize);

        auto label = new QLabel(tr("Are you sure you want to leave?"), this);
        label->setFont(font);

        layout->addWidget(label);
        layout->addLayout(buttonLayout);

        connect(confirmBtn_, &QPushButton::clicked, [this]() { emit closing(true); });
        connect(cancelBtn_, &QPushButton::clicked, [this]() { emit closing(false); });
}

void
LeaveRoom::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
