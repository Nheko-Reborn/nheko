/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QLabel>
#include <QPaintEvent>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/Logout.h"

#include "Config.h"
#include "ui/FlatButton.h"
#include "ui/Theme.h"

using namespace dialogs;

Logout::Logout(QWidget *parent)
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

        confirmBtn_ = new FlatButton("OK", this);
        confirmBtn_->setFont(buttonFont);
        confirmBtn_->setRippleStyle(ui::RippleStyle::NoRipple);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFont(buttonFont);
        cancelBtn_->setRippleStyle(ui::RippleStyle::NoRipple);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QFont font;
        font.setPointSizeF(font.pointSizeF() * conf::modals::LABEL_MEDIUM_SIZE_RATIO);

        auto label = new QLabel(tr("Logout. Are you sure?"), this);
        label->setFont(font);

        layout->addWidget(label);
        layout->addLayout(buttonLayout);
        layout->addStretch(1);

        connect(confirmBtn_, &QPushButton::clicked, [this]() { emit closing(true); });
        connect(cancelBtn_, &QPushButton::clicked, [this]() { emit closing(false); });
}

void
Logout::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
