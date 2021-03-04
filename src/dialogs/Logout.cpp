// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Config.h"
#include "dialogs/Logout.h"

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

        connect(confirmBtn_, &QPushButton::clicked, this, [this]() {
                emit loggingOut();
                emit close();
        });
        connect(cancelBtn_, &QPushButton::clicked, this, &Logout::close);
}
