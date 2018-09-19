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
#include <QPainter>
#include <QPushButton>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/Logout.h"

#include "Config.h"
#include "ui/Theme.h"

using namespace dialogs;

Logout::Logout(QWidget *parent)
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

        confirmBtn_ = new QPushButton("Logout", this);
        cancelBtn_  = new QPushButton(tr("Cancel"), this);
        cancelBtn_->setDefault(true);

        buttonLayout->addStretch(1);
        buttonLayout->setSpacing(15);
        buttonLayout->addWidget(cancelBtn_);
        buttonLayout->addWidget(confirmBtn_);

        auto label = new QLabel(tr("Logout. Are you sure?"), this);

        layout->addWidget(label);
        layout->addLayout(buttonLayout);
        layout->addStretch(1);

        connect(confirmBtn_, &QPushButton::clicked, this, &Logout::loggingOut);
        connect(cancelBtn_, &QPushButton::clicked, this, &Logout::close);
}
