// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDesktopServices>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "dialogs/FallbackAuth.h"

#include "Config.h"
#include "MatrixClient.h"

using namespace dialogs;

FallbackAuth::FallbackAuth(const QString &authType, const QString &session, QWidget *parent)
  : QWidget(parent)
{
    setAutoFillBackground(true);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_DeleteOnClose, true);

    auto layout = new QVBoxLayout(this);
    layout->setSpacing(conf::modals::WIDGET_SPACING);
    layout->setContentsMargins(conf::modals::WIDGET_MARGIN,
                               conf::modals::WIDGET_MARGIN,
                               conf::modals::WIDGET_MARGIN,
                               conf::modals::WIDGET_MARGIN);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    openBtn_    = new QPushButton(tr("Open Fallback in Browser"), this);
    cancelBtn_  = new QPushButton(tr("Cancel"), this);
    confirmBtn_ = new QPushButton(tr("Confirm"), this);
    confirmBtn_->setDefault(true);

    buttonLayout->addStretch(1);
    buttonLayout->addWidget(openBtn_);
    buttonLayout->addWidget(cancelBtn_);
    buttonLayout->addWidget(confirmBtn_);

    QFont font;
    font.setPointSizeF(font.pointSizeF() * conf::modals::LABEL_MEDIUM_SIZE_RATIO);

    auto label = new QLabel(
      tr("Open the fallback, follow the steps and confirm after completing them."), this);
    label->setFont(font);

    layout->addWidget(label);
    layout->addLayout(buttonLayout);

    connect(openBtn_, &QPushButton::clicked, [session, authType]() {
        const auto url = QString("https://%1:%2/_matrix/client/r0/auth/%4/"
                                 "fallback/web?session=%3")
                           .arg(QString::fromStdString(http::client()->server()))
                           .arg(http::client()->port())
                           .arg(session, authType);

        QDesktopServices::openUrl(url);
    });

    connect(confirmBtn_, &QPushButton::clicked, this, [this]() {
        emit confirmation();
        emit close();
    });
    connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
        emit cancel();
        emit close();
    });
}
