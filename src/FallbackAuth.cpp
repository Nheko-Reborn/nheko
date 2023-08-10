// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FallbackAuth.h"

#include <QDesktopServices>
#include <QUrl>

#include "MatrixClient.h"

FallbackAuth::FallbackAuth(const QString &session, const QString &authType, QObject *parent)
  : QObject{parent}
  , m_session{session}
  , m_authType{authType}
{
}

void
FallbackAuth::openFallbackAuth()
{
    const auto url = QString("https://%1:%2/_matrix/client/r0/auth/%4/"
                             "fallback/web?session=%3")
                       .arg(QString::fromStdString(http::client()->server()))
                       .arg(http::client()->port())
                       .arg(m_session, m_authType);

    QDesktopServices::openUrl(url);
}
