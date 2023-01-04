// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <mtxclient/http/client.hpp>

class UIA final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title NOTIFY titleChanged)

public:
    static UIA *instance();

    UIA(QObject *parent = nullptr)
      : QObject(parent)
    {
    }

    mtx::http::UIAHandler genericHandler(QString context);

    QString title() const { return title_; }

public slots:
    void continuePassword(const QString &password);
    void continueEmail(const QString &email);
    void continuePhoneNumber(const QString &countryCode, const QString &phoneNumber);
    void submit3pidToken(const QString &token);
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
    std::optional<mtx::http::UIAHandler> currentHandler;
    mtx::user_interactive::Unauthorized currentStatus;
    QString title_;

    // for 3pids like email and phone number
    std::string client_secret;
    std::string sid;
    std::string submit_url;
    bool email_ = true;
};
