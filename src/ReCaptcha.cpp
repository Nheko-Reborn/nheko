// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReCaptcha.h"

#include <QDesktopServices>
#include <QUrl>

#include "MatrixClient.h"

ReCaptcha::ReCaptcha(const QString &session, const QString &context, QObject *parent)
  : QObject{parent}
  , m_session{session}
  , m_context{context}
{
}

void
ReCaptcha::openReCaptcha()
{
    const auto url = QString("https://%1:%2/_matrix/client/r0/auth/m.login.recaptcha/"
                             "fallback/web?session=%3")
                       .arg(QString::fromStdString(http::client()->server()))
                       .arg(http::client()->port())
                       .arg(m_session);

    QDesktopServices::openUrl(url);
}
