// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class SelfVerificationStatus : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool hasSSSS READ hasSSSS NOTIFY hasSSSSChanged)

public:
    SelfVerificationStatus(QObject *o = nullptr);
    enum Status
    {
        AllVerified,
        NoMasterKey,
        UnverifiedMasterKey,
        UnverifiedDevices,
    };
    Q_ENUM(Status)

    Q_INVOKABLE void setupCrosssigning(bool useSSSS, QString password, bool useOnlineKeyBackup);
    Q_INVOKABLE void verifyMasterKey();
    Q_INVOKABLE void verifyMasterKeyWithPassphrase();
    Q_INVOKABLE void verifyUnverifiedDevices();

    Status status() const { return status_; }
    bool hasSSSS() const { return hasSSSS_; }

signals:
    void statusChanged();
    void hasSSSSChanged();
    void setupCompleted();
    void showRecoveryKey(QString key);
    void setupFailed(QString message);

public slots:
    void invalidate();

private:
    Status status_ = AllVerified;
    bool hasSSSS_  = true;
};
