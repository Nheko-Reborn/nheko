// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDesktopServices>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "dialogs/ReCaptcha.h"

#include "Config.h"
#include "MatrixClient.h"

using namespace dialogs;

ReCaptcha::ReCaptcha(const QString &session, QWidget *parent)
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
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);

    openCaptchaBtn_ = new QPushButton(tr("Open reCAPTCHA"), this);
    cancelBtn_      = new QPushButton(tr("Cancel"), this);
    confirmBtn_     = new QPushButton(tr("Confirm"), this);
    confirmBtn_->setDefault(true);

    buttonLayout->addStretch(1);
    buttonLayout->addWidget(openCaptchaBtn_);
    buttonLayout->addWidget(cancelBtn_);
    buttonLayout->addWidget(confirmBtn_);

    QFont font;
    font.setPointSizeF(font.pointSizeF() * conf::modals::LABEL_MEDIUM_SIZE_RATIO);

    auto label = new QLabel(tr("Solve the reCAPTCHA and press the confirm button"), this);
    label->setFont(font);

    layout->addWidget(label);
    layout->addLayout(buttonLayout);

    connect(openCaptchaBtn_, &QPushButton::clicked, [session]() {
        const auto url = QString("https://%1:%2/_matrix/client/r0/auth/m.login.recaptcha/"
                                 "fallback/web?session=%3")
                           .arg(QString::fromStdString(http::client()->server()))
                           .arg(http::client()->port())
                           .arg(session);

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
