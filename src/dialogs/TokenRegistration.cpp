// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "dialogs/TokenRegistration.h"

#include "Config.h"
#include "MatrixClient.h"
#include "ui/TextField.h"

using namespace dialogs;

TokenRegistration::TokenRegistration(QWidget *parent)
  : QWidget(parent)
{
        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose, true);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(8);
        buttonLayout->setMargin(0);

        cancelBtn_  = new QPushButton(tr("Cancel"), this);
        confirmBtn_ = new QPushButton(tr("Confirm"), this);
        confirmBtn_->setDefault(true);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(cancelBtn_);
        buttonLayout->addWidget(confirmBtn_);

        tokenInput_ = new TextField(this);
        tokenInput_->setLabel(tr("Registration token"));

        QFont font;
        font.setPointSizeF(font.pointSizeF() * conf::modals::LABEL_MEDIUM_SIZE_RATIO);

        auto label = new QLabel(tr("Please enter a valid registration token."), this);
        label->setFont(font);

        layout->addWidget(label);
        layout->addWidget(tokenInput_);
        layout->addLayout(buttonLayout);

        connect(confirmBtn_, &QPushButton::clicked, this, [this]() {
                emit confirmation(tokenInput_->text().toStdString());
                emit close();
        });
        connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
                emit cancel();
                emit close();
        });
}
