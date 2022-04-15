// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>

#include <MatrixClient.h>

class UIA : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString title READ title NOTIFY titleChanged)

public:
    static UIA *instance();
    static UIA *create(QQmlEngine *qmlEngine, QJSEngine *)
    {
        // The instance has to exist before it is used. We cannot replace it.
        Q_ASSERT(instance());

        // The engine has to have the same thread affinity as the singleton.
        Q_ASSERT(qmlEngine->thread() == instance()->thread());

        // There can only be one engine accessing the singleton.
        static QJSEngine *s_engine = nullptr;
        if (s_engine)
            Q_ASSERT(qmlEngine == s_engine);
        else
            s_engine = qmlEngine;

        QJSEngine::setObjectOwnership(instance(), QJSEngine::CppOwnership);
        return instance();
    }

    mtx::http::UIAHandler genericHandler(QString context);

    QString title() const { return title_; }

public slots:
    void continuePassword(QString password);
    void continueEmail(QString email);
    void continuePhoneNumber(QString countryCode, QString phoneNumber);
    void submit3pidToken(QString token);
    void continue3pidReceived();

signals:
    void password();
    void email();
    void phoneNumber();

    void confirm3pidToken();
    void prompt3pidToken();
    void tokenAccepted();

    void titleChanged();
    void error(QString msg);

private:
    UIA(QObject *parent = nullptr)
      : QObject(parent)
    {}

    std::optional<mtx::http::UIAHandler> currentHandler;
    mtx::user_interactive::Unauthorized currentStatus;
    QString title_;

    // for 3pids like email and phone number
    std::string client_secret;
    std::string sid;
    std::string submit_url;
    bool email_ = true;
};
