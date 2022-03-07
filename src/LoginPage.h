// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QVariantList>

namespace mtx {
namespace responses {
struct Login;
}
}

struct SSOProvider
{
    Q_GADGET
    Q_PROPERTY(QString avatarUrl READ avatarUrl CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)

public:
    [[nodiscard]] QString avatarUrl() const { return avatarUrl_; }
    [[nodiscard]] QString name() const { return name_.toHtmlEscaped(); }
    [[nodiscard]] QString id() const { return id_; }

    QString avatarUrl_;
    QString name_;
    QString id_;
};

class LoginPage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString mxid READ mxid WRITE setMxid NOTIFY matrixIdChanged)
    Q_PROPERTY(QString homeserver READ homeserver WRITE setHomeserver NOTIFY homeserverChanged)

    Q_PROPERTY(QString mxidError READ mxidError NOTIFY mxidErrorChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorOccurred)
    Q_PROPERTY(bool lookingUpHs READ lookingUpHs NOTIFY lookingUpHsChanged)
    Q_PROPERTY(bool homeserverValid READ homeserverValid NOTIFY lookingUpHsChanged)
    Q_PROPERTY(bool loggingIn READ loggingIn NOTIFY loggingInChanged)
    Q_PROPERTY(bool passwordSupported READ passwordSupported NOTIFY versionLookedUp)
    Q_PROPERTY(bool ssoSupported READ ssoSupported NOTIFY versionLookedUp)
    Q_PROPERTY(bool homeserverNeeded READ homeserverNeeded NOTIFY versionLookedUp)

    Q_PROPERTY(QVariantList identityProviders READ identityProviders NOTIFY versionLookedUp)

public:
    enum class LoginMethod
    {
        Password,
        SSO,
    };
    Q_ENUM(LoginMethod)

    LoginPage(QObject *parent = nullptr);

    Q_INVOKABLE QString initialDeviceName() const
    {
        return QString::fromStdString(initialDeviceName_());
    }

    bool lookingUpHs() const { return lookingUpHs_; }
    bool loggingIn() const { return loggingIn_; }
    bool passwordSupported() const { return passwordSupported_; }
    bool ssoSupported() const { return ssoSupported_; }
    bool homeserverNeeded() const { return homeserverNeeded_; }
    bool homeserverValid() const { return homeserverValid_; }
    QVariantList identityProviders() const { return identityProviders_; }

    QString homeserver() { return homeserver_; }
    QString mxid() { return mxid_; }

    QString error() { return error_; }
    QString mxidError() { return mxidError_; }

    void setHomeserver(QString hs);
    void setMxid(QString id)
    {
        if (id != mxid_) {
            mxid_ = id;
            emit matrixIdChanged();
            onMatrixIdEntered();
        }
    }

    static std::string initialDeviceName_()
    {
#if defined(Q_OS_MAC)
        return "Nheko on macOS";
#elif defined(Q_OS_LINUX)
        return "Nheko on Linux";
#elif defined(Q_OS_WIN)
        return "Nheko on Windows";
#elif defined(Q_OS_FREEBSD)
        return "Nheko on FreeBSD";
#else
        return "Nheko";
#endif
    }

signals:
    void loggingInChanged();
    void errorOccurred();

    //! Used to trigger the corresponding slot outside of the main thread.
    void versionErrorCb(const QString &err);
    void versionOkCb(bool passwordSupported, bool ssoSupported, QVariantList identityProviders);

    void loginOk(const mtx::responses::Login &res);

    void onServerAddressEntered();

    void matrixIdChanged();
    void homeserverChanged();

    void mxidErrorChanged();
    void lookingUpHsChanged();
    void versionLookedUp();
    void versionLookupFinished();

public slots:
    // Displays errors produced during the login.
    void showError(const QString &msg);

    // Callback for the login button.
    void onLoginButtonClicked(LoginMethod loginMethod,
                              QString userid,
                              QString password,
                              QString deviceName);

    // Callback for errors produced during server probing
    void versionError(const QString &error_message);
    // Callback for successful server probing
    void versionOk(bool passwordSupported, bool ssoSupported, QVariantList identityProviders);

private:
    void checkHomeserverVersion();
    void onMatrixIdEntered();
    void clearErrors()
    {
        error_.clear();
        mxidError_.clear();
        emit errorOccurred();
        emit mxidErrorChanged();
    }

    QString inferredServerAddress_;

    QString mxid_;
    QString homeserver_;

    QString mxidError_;
    QString error_;

    QVariantList identityProviders_;

    bool passwordSupported_ = true;
    bool ssoSupported_      = false;

    bool lookingUpHs_      = false;
    bool loggingIn_        = false;
    bool homeserverNeeded_ = false;
    bool homeserverValid_  = false;
};
