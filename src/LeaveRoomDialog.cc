#include <QLabel>
#include <QVBoxLayout>

#include "Config.h"
#include "LeaveRoomDialog.h"
#include "Theme.h"

LeaveRoomDialog::LeaveRoomDialog(QWidget *parent)
  : QFrame(parent)
{
        setMaximumSize(400, 400);
        setStyleSheet("background-color: #fff");

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
        label->setStyleSheet("color: #333333");

        layout->addWidget(label);
        layout->addLayout(buttonLayout);

        connect(confirmBtn_, &QPushButton::clicked, [=]() { emit closing(true); });
        connect(cancelBtn_, &QPushButton::clicked, [=]() { emit closing(false); });
}
