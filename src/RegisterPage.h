// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

#include <mtx/user_interactive.hpp>
#include <mtxclient/http/client.hpp>

class RegisterPage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString hsError READ hsError NOTIFY hsErrorChanged)
    Q_PROPERTY(QString usernameError READ usernameError NOTIFY lookingUpUsernameChanged)
    Q_PROPERTY(bool registering READ registering NOTIFY registeringChanged)
    Q_PROPERTY(bool supported READ supported NOTIFY lookingUpHsChanged)
    Q_PROPERTY(bool lookingUpHs READ lookingUpHs NOTIFY lookingUpHsChanged)
    Q_PROPERTY(bool lookingUpUsername READ lookingUpUsername NOTIFY lookingUpUsernameChanged)
    Q_PROPERTY(bool usernameAvailable READ usernameAvailable NOTIFY lookingUpUsernameChanged)
    Q_PROPERTY(bool usernameUnavailable READ usernameUnavailable NOTIFY lookingUpUsernameChanged)

public:
    RegisterPage(QObject *parent = nullptr);

    Q_INVOKABLE void setServer(QString server);
    Q_INVOKABLE void checkUsername(QString name);
    Q_INVOKABLE void startRegistration(QString username, QString password, QString deviceName);
    Q_INVOKABLE QString initialDeviceName() const;

    bool registering() const { return registering_; }
    bool supported() const { return supported_; }
    bool lookingUpHs() const { return lookingUpHs_; }
    bool lookingUpUsername() const { return lookingUpUsername_; }
    bool usernameAvailable() const { return usernameAvailable_; }
    bool usernameUnavailable() const { return usernameUnavailable_; }

    QString error() const { return registrationError_; }
    QString usernameError() const { return usernameError_; }
    QString hsError() const { return hsError_; }

signals:
    void errorChanged();
    void hsErrorChanged();

    void registeringChanged();
    void lookingUpHsChanged();
    void lookingUpUsernameChanged();

    void registerOk();

private:
    void versionsCheck();

    void setHsError(QString err);
    void setError(QString err);

    QString registrationError_, hsError_, usernameError_;

    bool registering_         = false;
    bool supported_           = false;
    bool lookingUpHs_         = false;
    bool lookingUpUsername_   = false;
    bool usernameAvailable_   = false;
    bool usernameUnavailable_ = false;

    QString lastServer;
};
