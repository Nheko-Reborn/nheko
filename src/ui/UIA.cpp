// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UIA.h"

#include <algorithm>

#include <QInputDialog>
#include <QTimer>

#include <mtx/responses/common.hpp>

#include "Logging.h"
#include "MatrixClient.h"
#include "dialogs/FallbackAuth.h"
#include "dialogs/ReCaptcha.h"

UIA *
UIA::instance()
{
    static UIA uia;
    return &uia;
}

mtx::http::UIAHandler
UIA::genericHandler(QString context)
{
    return mtx::http::UIAHandler([this, context](const mtx::http::UIAHandler &h,
                                                 const mtx::user_interactive::Unauthorized &u) {
        QTimer::singleShot(0, this, [this, h, u, context]() {
            this->currentHandler = h;
            this->currentStatus  = u;
            this->title_         = context;
            emit titleChanged();

            std::vector<mtx::user_interactive::Flow> flows = u.flows;

            nhlog::ui()->info("Completed stages: {}", u.completed.size());

            if (!u.completed.empty()) {
                // Get rid of all flows which don't start with the sequence of
                // stages that have already been completed.
                flows.erase(std::remove_if(flows.begin(),
                                           flows.end(),
                                           [completed_stages = u.completed](auto flow) {
                                               if (completed_stages.size() > flow.stages.size())
                                                   return true;
                                               for (size_t f = 0; f < completed_stages.size(); f++)
                                                   if (completed_stages[f] != flow.stages[f])
                                                       return true;
                                               return false;
                                           }),
                            flows.end());
            }

            if (flows.empty()) {
                nhlog::ui()->error("No available registration flows!");
                emit error(tr("No available registration flows!"));
                return;
            }

            // sort flows with known stages first
            std::sort(
              flows.begin(),
              flows.end(),
              [](const mtx::user_interactive::Flow &a, const mtx::user_interactive::Flow &b) {
                  auto calcWeight = [](const mtx::user_interactive::Flow &f) {
                      using namespace mtx::user_interactive::auth_types;
                      const static std::map<std::string_view, int> weights{
                        {mtx::user_interactive::auth_types::password, 0},
                        {mtx::user_interactive::auth_types::email_identity, 0},
                        {mtx::user_interactive::auth_types::msisdn, 0},
                        {mtx::user_interactive::auth_types::dummy, 0},
                        {mtx::user_interactive::auth_types::registration_token, 0},
                        // recaptcha is known, but we'd like to avoid it, because it calls out to
                        // the browser
                        {mtx::user_interactive::auth_types::recaptcha, 1},
                      };
                      int weight = 0;
                      for (const auto &s : f.stages) {
                          if (!weights.count(s))
                              weight += 3;
                          else
                              weight += weights.at(s);
                      }
                      return weight;
                  };

                  return calcWeight(a) < calcWeight(b);
              });

            auto current_stage = flows.front().stages.at(u.completed.size());

            if (current_stage == mtx::user_interactive::auth_types::password) {
                emit password();
            } else if (current_stage == mtx::user_interactive::auth_types::email_identity) {
                emit email();
            } else if (current_stage == mtx::user_interactive::auth_types::msisdn) {
                emit phoneNumber();
            } else if (current_stage == mtx::user_interactive::auth_types::recaptcha) {
                auto captchaDialog =
                  new dialogs::ReCaptcha(QString::fromStdString(u.session), nullptr);
                captchaDialog->setWindowTitle(context);

                connect(
                  captchaDialog, &dialogs::ReCaptcha::confirmation, this, [captchaDialog, h, u]() {
                      captchaDialog->close();
                      captchaDialog->deleteLater();
                      h.next(mtx::user_interactive::Auth{u.session,
                                                         mtx::user_interactive::auth::Fallback{}});
                  });

                connect(captchaDialog, &dialogs::ReCaptcha::cancel, this, [this]() {
                    emit error(tr("Registration aborted"));
                });

                QTimer::singleShot(0, this, [captchaDialog]() { captchaDialog->show(); });

            } else if (current_stage == mtx::user_interactive::auth_types::dummy) {
                h.next(
                  mtx::user_interactive::Auth{u.session, mtx::user_interactive::auth::Dummy{}});

            } else if (current_stage == mtx::user_interactive::auth_types::registration_token) {
                bool ok;
                QString token =
                  QInputDialog::getText(nullptr,
                                        context,
                                        tr("Please enter a valid registration token."),
                                        QLineEdit::Normal,
                                        QString(),
                                        &ok);

                if (ok) {
                    h.next(mtx::user_interactive::Auth{
                      u.session,
                      mtx::user_interactive::auth::RegistrationToken{token.toStdString()}});
                } else {
                    emit error(tr("Registration aborted"));
                }
            } else {
                // use fallback
                auto dialog = new dialogs::FallbackAuth(QString::fromStdString(current_stage),
                                                        QString::fromStdString(u.session),
                                                        nullptr);
                dialog->setWindowTitle(context);

                connect(dialog, &dialogs::FallbackAuth::confirmation, this, [h, u, dialog]() {
                    dialog->close();
                    dialog->deleteLater();
                    h.next(mtx::user_interactive::Auth{u.session,
                                                       mtx::user_interactive::auth::Fallback{}});
                });

                connect(dialog, &dialogs::FallbackAuth::cancel, this, [this]() {
                    emit error(tr("Registration aborted"));
                });

                dialog->show();
            }
        });
    });
}

void
UIA::continuePassword(QString password)
{
    mtx::user_interactive::auth::Password p{};
    p.identifier_type = mtx::user_interactive::auth::Password::UserId;
    p.password        = password.toStdString();
    p.identifier_user = http::client()->user_id().to_string();

    if (currentHandler)
        currentHandler->next(mtx::user_interactive::Auth{currentStatus.session, p});
}

void
UIA::continueEmail(QString email)
{
    mtx::requests::RequestEmailToken r{};
    r.client_secret = this->client_secret = mtx::client::utils::random_token(128, false);
    r.email                               = email.toStdString();
    r.send_attempt                        = 0;
    http::client()->register_email_request_token(
      r, [this](const mtx::responses::RequestToken &token, mtx::http::RequestErr e) {
          if (!e) {
              this->sid        = token.sid;
              this->submit_url = token.submit_url;
              this->email_     = true;

              if (submit_url.empty()) {
                  nhlog::ui()->debug("Got no submit url.");
                  emit confirm3pidToken();
              } else {
                  nhlog::ui()->debug("Got submit url: {}", token.submit_url);
                  emit prompt3pidToken();
              }
          } else {
              nhlog::ui()->debug("Registering email failed! ({},{},{},{})",
                                 e->status_code,
                                 e->status_code,
                                 e->parse_error,
                                 e->matrix_error.error);
              emit error(QString::fromStdString(e->matrix_error.error));
          }
      });
}
void
UIA::continuePhoneNumber(QString countryCode, QString phoneNumber)
{
    mtx::requests::RequestMSISDNToken r{};
    r.client_secret = this->client_secret = mtx::client::utils::random_token(128, false);
    r.country                             = countryCode.toStdString();
    r.phone_number                        = phoneNumber.toStdString();
    r.send_attempt                        = 0;
    http::client()->register_phone_request_token(
      r, [this](const mtx::responses::RequestToken &token, mtx::http::RequestErr e) {
          if (!e) {
              this->sid        = token.sid;
              this->submit_url = token.submit_url;
              this->email_     = false;
              if (submit_url.empty()) {
                  nhlog::ui()->debug("Got no submit url.");
                  emit confirm3pidToken();
              } else {
                  nhlog::ui()->debug("Got submit url: {}", token.submit_url);
                  emit prompt3pidToken();
              }
          } else {
              nhlog::ui()->debug("Registering phone number failed! ({},{},{},{})",
                                 e->status_code,
                                 e->status_code,
                                 e->parse_error,
                                 e->matrix_error.error);
              emit error(QString::fromStdString(e->matrix_error.error));
          }
      });
}

void
UIA::continue3pidReceived()
{
    mtx::user_interactive::auth::ThreePIDCred c{};
    c.client_secret = this->client_secret;
    c.sid           = this->sid;

    if (this->email_) {
        mtx::user_interactive::auth::EmailIdentity i{};
        i.threepidCred = c;
        this->currentHandler->next(mtx::user_interactive::Auth{currentStatus.session, i});
    } else {
        mtx::user_interactive::auth::MSISDN i{};
        i.threepidCred = c;
        this->currentHandler->next(mtx::user_interactive::Auth{currentStatus.session, i});
    }
}

void
UIA::submit3pidToken(QString token)
{
    mtx::requests::IdentitySubmitToken t{};
    t.client_secret = this->client_secret;
    t.sid           = this->sid;
    t.token         = token.toStdString();

    http::client()->validate_submit_token(
      submit_url, t, [this](const mtx::responses::Success &success, mtx::http::RequestErr e) {
          if (!e && success.success) {
              mtx::user_interactive::auth::ThreePIDCred c{};
              c.client_secret = this->client_secret;
              c.sid           = this->sid;

              nhlog::ui()->debug("Submit token success");

              if (this->email_) {
                  mtx::user_interactive::auth::EmailIdentity i{};
                  i.threepidCred = c;
                  this->currentHandler->next(mtx::user_interactive::Auth{currentStatus.session, i});
              } else {
                  mtx::user_interactive::auth::MSISDN i{};
                  i.threepidCred = c;
                  this->currentHandler->next(mtx::user_interactive::Auth{currentStatus.session, i});
              }
          } else {
              if (e) {
                  nhlog::ui()->debug("Submit token invalid! ({},{},{},{})",
                                     e->status_code,
                                     e->status_code,
                                     e->parse_error,
                                     e->matrix_error.error);
                  emit error(QString::fromStdString(e->matrix_error.error));
              } else {
                  nhlog::ui()->debug("Submit token invalid!");
                  emit error(tr("Invalid token"));
              }
          }

          this->client_secret.clear();
          this->sid.clear();
          this->submit_url.clear();
      });
}
