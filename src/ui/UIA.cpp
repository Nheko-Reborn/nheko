// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UIA.h"

#include <algorithm>

#include <QInputDialog>
#include <QTimer>

#include "Logging.h"
#include "MainWindow.h"
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
                return;
            }

            auto current_stage = flows.front().stages.at(u.completed.size());

            if (current_stage == mtx::user_interactive::auth_types::password) {
                emit password();
            } else if (current_stage == mtx::user_interactive::auth_types::recaptcha) {
                auto captchaDialog =
                  new dialogs::ReCaptcha(QString::fromStdString(u.session), MainWindow::instance());
                captchaDialog->setWindowTitle(context);

                connect(
                  captchaDialog, &dialogs::ReCaptcha::confirmation, this, [captchaDialog, h, u]() {
                      captchaDialog->close();
                      captchaDialog->deleteLater();
                      h.next(mtx::user_interactive::Auth{u.session,
                                                         mtx::user_interactive::auth::Fallback{}});
                  });

                // connect(
                //  captchaDialog, &dialogs::ReCaptcha::cancel, this, &RegisterPage::errorOccurred);

                QTimer::singleShot(0, this, [captchaDialog]() { captchaDialog->show(); });

            } else if (current_stage == mtx::user_interactive::auth_types::dummy) {
                h.next(
                  mtx::user_interactive::Auth{u.session, mtx::user_interactive::auth::Dummy{}});

            } else if (current_stage == mtx::user_interactive::auth_types::registration_token) {
                bool ok;
                QString token =
                  QInputDialog::getText(MainWindow::instance(),
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
                    // emit errorOccurred();
                }
            } else {
                // use fallback
                auto dialog = new dialogs::FallbackAuth(QString::fromStdString(current_stage),
                                                        QString::fromStdString(u.session),
                                                        MainWindow::instance());
                dialog->setWindowTitle(context);

                connect(dialog, &dialogs::FallbackAuth::confirmation, this, [h, u, dialog]() {
                    dialog->close();
                    dialog->deleteLater();
                    h.next(mtx::user_interactive::Auth{u.session,
                                                       mtx::user_interactive::auth::Fallback{}});
                });

                // connect(dialog, &dialogs::FallbackAuth::cancel, this,
                // &RegisterPage::errorOccurred);

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
