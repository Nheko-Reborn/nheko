// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <set>

#include <mtx/responses/common.hpp>
#include <mtx/responses/register.hpp>
#include <mtx/responses/version.hpp>
#include <mtx/responses/well-known.hpp>
#include <mtxclient/http/client.hpp>

#include "Config.h"
#include "Logging.h"
#include "LoginPage.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "RegisterPage.h"
#include "ui/UIA.h"

RegisterPage::RegisterPage(QObject *parent)
  : QObject(parent)
{
    connect(this, &RegisterPage::registerOk, this, [] { MainWindow::instance()->showChatPage(); });
}

void
RegisterPage::setError(QString err)
{
    registrationError_ = err;
    emit errorChanged();
    registering_ = false;
    emit registeringChanged();
}
void
RegisterPage::setHsError(QString err)
{
    hsError_ = err;
    emit hsErrorChanged();
    lookingUpHs_ = false;
    emit lookingUpHsChanged();
}

QString
RegisterPage::initialDeviceName() const
{
    return QString::fromStdString(LoginPage::initialDeviceName_());
}

void
RegisterPage::setServer(QString server)
{
    if (server == lastServer)
        return;

    lastServer = server;

    http::client()->set_server(server.toStdString());
    http::client()->verify_certificates(!UserSettings::instance()->disableCertificateValidation());

    hsError_.clear();
    emit hsErrorChanged();
    supported_   = false;
    lookingUpHs_ = true;
    emit lookingUpHsChanged();

    http::client()->well_known(
      [this](const mtx::responses::WellKnown &res, mtx::http::RequestErr err) {
          if (err) {
              if (err->status_code == 404) {
                  nhlog::net()->info("Autodiscovery: No .well-known.");
                  // Check that the homeserver can be reached
                  versionsCheck();
                  return;
              }

              if (!err->parse_error.empty()) {
                  setHsError(tr("Autodiscovery failed. Received malformed response."));
                  nhlog::net()->error("Autodiscovery failed. Received malformed response.");
                  emit hsErrorChanged();
                  return;
              }

              setHsError(tr("Autodiscovery failed. Unknown error when requesting .well-known."));
              nhlog::net()->error("Autodiscovery failed. Unknown error when "
                                  "requesting .well-known. {} {}",
                                  err->status_code,
                                  err->error_code);
              return;
          }

          nhlog::net()->info("Autodiscovery: Discovered '" + res.homeserver.base_url + "'");
          http::client()->set_server(res.homeserver.base_url);
          emit hsErrorChanged();
          // Check that the homeserver can be reached
          versionsCheck();
      });
}

void
RegisterPage::versionsCheck()
{
    // Make a request to /_matrix/client/versions to check the address
    // given is a Matrix homeserver.
    http::client()->versions(
      [this](const mtx::responses::Versions &versions, mtx::http::RequestErr err) {
          if (err) {
              if (err->status_code == 404) {
                  setHsError(
                    tr("The required endpoints were not found. Possibly not a Matrix server."));
                  emit hsErrorChanged();
                  return;
              }

              if (!err->parse_error.empty()) {
                  setHsError(
                    tr("Received malformed response. Make sure the homeserver domain is valid."));
                  emit hsErrorChanged();
                  return;
              }

              setHsError(tr("An unknown error occured. Make sure the homeserver domain is valid."));
              emit hsErrorChanged();
              return;
          }

          if (std::find_if(
                versions.versions.cbegin(), versions.versions.cend(), [](const std::string &v) {
                    static const std::set<std::string_view, std::less<>> supported{
                      "v1.1",
                      "v1.2",
                      "v1.3",
                    };
                    return supported.count(v) != 0;
                }) == versions.versions.cend()) {
              emit setHsError(
                tr("The selected server does not support a version of the Matrix protocol, that "
                   "this client understands (v1.1, v1.2 or v1.3). You can't register."));
              emit hsErrorChanged();
              return;
          }

          http::client()->registration(
            [this](const mtx::responses::Register &, mtx::http::RequestErr e) {
                nhlog::net()->debug("Registration check: {}", e);

                if (!e) {
                    setHsError(tr("Server does not support querying registration flows!"));
                    emit hsErrorChanged();
                    return;
                }
                if (e->status_code != 401) {
                    setHsError(tr("Server does not support registration."));
                    emit hsErrorChanged();
                    return;
                }

                supported_   = true;
                lookingUpHs_ = false;
                emit lookingUpHsChanged();
            });
      });
}

void
RegisterPage::checkUsername(QString name)
{
    usernameAvailable_ = usernameUnavailable_ = false;
    usernameError_.clear();
    lookingUpUsername_ = true;
    emit lookingUpUsernameChanged();

    http::client()->register_username_available(
      name.toStdString(),
      [this](const mtx::responses::Available &available, mtx::http::RequestErr e) {
          if (e) {
              if (e->matrix_error.errcode == mtx::errors::ErrorCode::M_INVALID_USERNAME) {
                  usernameError_ = tr("Invalid username.");
              } else if (e->matrix_error.errcode == mtx::errors::ErrorCode::M_USER_IN_USE) {
                  usernameError_ = tr("Name already in use.");
              } else if (e->matrix_error.errcode == mtx::errors::ErrorCode::M_EXCLUSIVE) {
                  usernameError_ = tr("Part of the reserved namespace.");
              } else {
              }

              usernameAvailable_   = false;
              usernameUnavailable_ = true;
          } else {
              usernameAvailable_   = available.available;
              usernameUnavailable_ = !available.available;
          }
          lookingUpUsername_ = false;
          emit lookingUpUsernameChanged();
      });
}

void
RegisterPage::startRegistration(QString username, QString password, QString devicename)
{
    // These inputs should still be alright, but check just in case
    if (!username.isEmpty() && !password.isEmpty() && usernameAvailable_ && supported_) {
        registrationError_.clear();
        emit errorChanged();
        registering_ = true;
        emit registeringChanged();

        connect(UIA::instance(), &UIA::error, this, [this](QString msg) {
            setError(msg);
            disconnect(UIA::instance(), &UIA::error, this, nullptr);
        });
        http::client()->registration(
          username.toStdString(),
          password.toStdString(),
          ::UIA::instance()->genericHandler(QStringLiteral("Registration")),
          [this](const mtx::responses::Register &res, mtx::http::RequestErr err) {
              registering_ = false;
              emit registeringChanged();

              if (!err) {
                  http::client()->set_user(res.user_id);
                  http::client()->set_access_token(res.access_token);
                  emit registerOk();
                  disconnect(UIA::instance(), &UIA::error, this, nullptr);
                  return;
              }

              // The server requires registration flows.
              if (err->status_code == 401 && err->matrix_error.unauthorized.flows.empty()) {
                  nhlog::net()->warn("failed to retrieve registration flows: "
                                     "status_code({}), matrix_error({}) ",
                                     static_cast<int>(err->status_code),
                                     err->matrix_error.error);
                  setError(QString::fromStdString(err->matrix_error.error));
                  disconnect(UIA::instance(), &UIA::error, this, nullptr);
                  return;
              }

              nhlog::net()->error("failed to register: status_code ({}), matrix_error({})",
                                  static_cast<int>(err->status_code),
                                  err->matrix_error.error);

              setError(QString::fromStdString(err->matrix_error.error));
              disconnect(UIA::instance(), &UIA::error, this, nullptr);
          },
          devicename.isEmpty() ? LoginPage::initialDeviceName_() : devicename.toStdString());
    }
}
