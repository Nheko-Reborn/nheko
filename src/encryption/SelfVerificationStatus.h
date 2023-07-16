// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQmlEngine>

class SelfVerificationStatus : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON

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

    Q_INVOKABLE void
    setupCrosssigning(bool useSSSS, const QString &password, bool useOnlineKeyBackup);
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
