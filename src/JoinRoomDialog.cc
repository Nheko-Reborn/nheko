#include <QLabel>
#include <QVBoxLayout>

#include "Config.h"
#include "JoinRoomDialog.h"
#include "Theme.h"

JoinRoomDialog::JoinRoomDialog(QWidget *parent)
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

        confirmBtn_ = new FlatButton("JOIN", this);
        confirmBtn_->setFontSize(conf::btn::fontSize);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFontSize(conf::btn::fontSize);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QFont font;
        font.setPixelSize(conf::headerFontSize);

        auto label = new QLabel(tr("Room alias to join:"), this);
        label->setFont(font);
        label->setStyleSheet("color: #333333");

        roomAliasEdit_ = new QLineEdit(this);

        layout->addWidget(label);
        layout->addWidget(roomAliasEdit_);
        layout->addLayout(buttonLayout);

        connect(confirmBtn_, &QPushButton::clicked, [=]() {
                emit closing(true, roomAliasEdit_->text());
        });
        connect(cancelBtn_, &QPushButton::clicked, [=]() { emit closing(false, nullptr); });
}
