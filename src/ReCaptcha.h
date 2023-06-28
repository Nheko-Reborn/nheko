// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQmlEngine>

class ReCaptcha : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString context MEMBER m_context CONSTANT)
    Q_PROPERTY(QString session MEMBER m_session CONSTANT)

public:
    ReCaptcha(const QString &session, const QString &context, QObject *parent = nullptr);

    Q_INVOKABLE void openReCaptcha();
    Q_INVOKABLE void confirm() { emit confirmation(); }
    Q_INVOKABLE void cancel() { emit cancelled(); }

signals:
    void confirmation();
    void cancelled();

private:
    QString m_session;
    QString m_context;
};
