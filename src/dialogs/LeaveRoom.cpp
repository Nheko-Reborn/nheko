#include <QLabel>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/LeaveRoom.h"

#include "Config.h"
#include "ui/FlatButton.h"
#include "ui/Theme.h"

using namespace dialogs;

LeaveRoom::LeaveRoom(QWidget *parent)
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

        confirmBtn_ = new FlatButton("LEAVE", this);
        confirmBtn_->setFont(buttonFont);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFont(buttonFont);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QFont font;
        font.setPointSizeF(font.pointSizeF() * conf::modals::LABEL_MEDIUM_SIZE_RATIO);

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
