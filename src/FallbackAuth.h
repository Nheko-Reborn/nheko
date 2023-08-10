// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQmlEngine>

class FallbackAuth : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString authType MEMBER m_authType CONSTANT)
    Q_PROPERTY(QString session MEMBER m_session CONSTANT)

public:
    FallbackAuth(const QString &session, const QString &authType, QObject *parent = nullptr);

    Q_INVOKABLE void openFallbackAuth();
    Q_INVOKABLE void confirm() { emit confirmation(); }
    Q_INVOKABLE void cancel() { emit cancelled(); }

signals:
    void confirmation();
    void cancelled();

private:
    const QString m_session;
    const QString m_authType;
};
