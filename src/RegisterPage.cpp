/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QLabel>
#include <QMetaType>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>
#include <QtMath>

#include <mtx/responses/register.hpp>

#include "Config.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "RegisterPage.h"
#include "ui/FlatButton.h"
#include "ui/RaisedButton.h"
#include "ui/TextField.h"

#include "dialogs/FallbackAuth.h"
#include "dialogs/ReCaptcha.h"

Q_DECLARE_METATYPE(mtx::user_interactive::Unauthorized)
Q_DECLARE_METATYPE(mtx::user_interactive::Auth)

RegisterPage::RegisterPage(QWidget *parent)
  : QWidget(parent)
{
        qRegisterMetaType<mtx::user_interactive::Unauthorized>();
        qRegisterMetaType<mtx::user_interactive::Auth>();
        top_layout_ = new QVBoxLayout();

        back_layout_ = new QHBoxLayout();
        back_layout_->setSpacing(0);
        back_layout_->setContentsMargins(5, 5, -1, -1);

        back_button_ = new FlatButton(this);
        back_button_->setMinimumSize(QSize(30, 30));

        QIcon icon;
        icon.addFile(":/icons/icons/ui/angle-pointing-to-left.png");

        back_button_->setIcon(icon);
        back_button_->setIconSize(QSize(32, 32));

        back_layout_->addWidget(back_button_, 0, Qt::AlignLeft | Qt::AlignVCenter);
        back_layout_->addStretch(1);

        QIcon logo;
        logo.addFile(":/logos/register.png");

        logo_ = new QLabel(this);
        logo_->setPixmap(logo.pixmap(128));

        logo_layout_ = new QHBoxLayout();
        logo_layout_->setMargin(0);
        logo_layout_->addWidget(logo_, 0, Qt::AlignHCenter);

        form_wrapper_ = new QHBoxLayout();
        form_widget_  = new QWidget();
        form_widget_->setMinimumSize(QSize(350, 300));

        form_layout_ = new QVBoxLayout();
        form_layout_->setSpacing(20);
        form_layout_->setContentsMargins(0, 0, 0, 40);
        form_widget_->setLayout(form_layout_);

        form_wrapper_->addStretch(1);
        form_wrapper_->addWidget(form_widget_);
        form_wrapper_->addStretch(1);

        username_input_ = new TextField();
        username_input_->setLabel(tr("Username"));
        username_input_->setRegexp("[a-z0-9._=/-]+");
        username_input_->setToolTip(tr("The username must not be empty, and must contain only the "
                                       "characters a-z, 0-9, ., _, =, -, and /."));

        password_input_ = new TextField();
        password_input_->setLabel(tr("Password"));
        password_input_->setRegexp("^.{8,}$");
        password_input_->setEchoMode(QLineEdit::Password);
        password_input_->setToolTip(tr("Please choose a secure password. The exact requirements "
                                       "for password strength may depend on your server."));

        password_confirmation_ = new TextField();
        password_confirmation_->setLabel(tr("Password confirmation"));
        password_confirmation_->setEchoMode(QLineEdit::Password);

        server_input_ = new TextField();
        server_input_->setLabel(tr("Homeserver"));
        server_input_->setToolTip(
          tr("A server that allows registration. Since matrix is decentralized, you need to first "
             "find a server you can register on or host your own."));

        error_username_label_ = new QLabel(this);
        error_username_label_->setWordWrap(true);
        error_username_label_->hide();

        error_password_label_ = new QLabel(this);
        error_password_label_->setWordWrap(true);
        error_password_label_->hide();

        error_password_confirmation_label_ = new QLabel(this);
        error_password_confirmation_label_->setWordWrap(true);
        error_password_confirmation_label_->hide();

        form_layout_->addWidget(username_input_, Qt::AlignHCenter);
        form_layout_->addWidget(error_username_label_, Qt::AlignHCenter);
        form_layout_->addWidget(password_input_, Qt::AlignHCenter);
        form_layout_->addWidget(error_password_label_, Qt::AlignHCenter);
        form_layout_->addWidget(password_confirmation_, Qt::AlignHCenter);
        form_layout_->addWidget(error_password_confirmation_label_, Qt::AlignHCenter);
        form_layout_->addWidget(server_input_, Qt::AlignHCenter);

        button_layout_ = new QHBoxLayout();
        button_layout_->setSpacing(0);
        button_layout_->setMargin(0);

        error_label_ = new QLabel(this);
        error_label_->setWordWrap(true);

        register_button_ = new RaisedButton(tr("REGISTER"), this);
        register_button_->setMinimumSize(350, 65);
        register_button_->setFontSize(conf::btn::fontSize);
        register_button_->setCornerRadius(conf::btn::cornerRadius);

        button_layout_->addStretch(1);
        button_layout_->addWidget(register_button_);
        button_layout_->addStretch(1);

        top_layout_->addLayout(back_layout_);
        top_layout_->addLayout(logo_layout_);
        top_layout_->addLayout(form_wrapper_);
        top_layout_->addStretch(1);
        top_layout_->addLayout(button_layout_);
        top_layout_->addWidget(error_label_, 0, Qt::AlignHCenter);
        top_layout_->addStretch(1);

        connect(back_button_, SIGNAL(clicked()), this, SLOT(onBackButtonClicked()));
        connect(register_button_, SIGNAL(clicked()), this, SLOT(onRegisterButtonClicked()));

        connect(username_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
        connect(username_input_, &TextField::editingFinished, this, &RegisterPage::checkFields);
        connect(password_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
        connect(password_input_, &TextField::editingFinished, this, &RegisterPage::checkFields);
        connect(password_confirmation_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
        connect(
          password_confirmation_, &TextField::editingFinished, this, &RegisterPage::checkFields);
        connect(server_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
        connect(server_input_, &TextField::editingFinished, this, &RegisterPage::checkFields);
        connect(this, &RegisterPage::registerErrorCb, this, [this](const QString &msg) {
                showError(msg);
        });
        connect(
          this,
          &RegisterPage::registrationFlow,
          this,
          [this](const std::string &user,
                 const std::string &pass,
                 const mtx::user_interactive::Unauthorized &unauthorized) {
                  auto completed_stages = unauthorized.completed;
                  auto flows            = unauthorized.flows;
                  auto session = unauthorized.session.empty() ? http::client()->generate_txn_id()
                                                              : unauthorized.session;

                  nhlog::ui()->info("Completed stages: {}", completed_stages.size());

                  if (!completed_stages.empty())
                          flows.erase(std::remove_if(
                                        flows.begin(),
                                        flows.end(),
                                        [completed_stages](auto flow) {
                                                if (completed_stages.size() > flow.stages.size())
                                                        return true;
                                                for (size_t f = 0; f < completed_stages.size(); f++)
                                                        if (completed_stages[f] != flow.stages[f])
                                                                return true;
                                                return false;
                                        }),
                                      flows.end());

                  if (flows.empty()) {
                          nhlog::net()->error("No available registration flows!");
                          emit registerErrorCb(tr("No supported registration flows!"));
                          return;
                  }

                  auto current_stage = flows.front().stages.at(completed_stages.size());

                  if (current_stage == mtx::user_interactive::auth_types::recaptcha) {
                          auto captchaDialog =
                            new dialogs::ReCaptcha(QString::fromStdString(session), this);

                          connect(captchaDialog,
                                  &dialogs::ReCaptcha::confirmation,
                                  this,
                                  [this, user, pass, session, captchaDialog]() {
                                          captchaDialog->close();
                                          captchaDialog->deleteLater();

                                          emit registerAuth(
                                            user,
                                            pass,
                                            mtx::user_interactive::Auth{
                                              session, mtx::user_interactive::auth::Fallback{}});
                                  });
                          connect(captchaDialog,
                                  &dialogs::ReCaptcha::cancel,
                                  this,
                                  &RegisterPage::errorOccurred);

                          QTimer::singleShot(
                            1000, this, [captchaDialog]() { captchaDialog->show(); });
                  } else if (current_stage == mtx::user_interactive::auth_types::dummy) {
                          emit registerAuth(user,
                                            pass,
                                            mtx::user_interactive::Auth{
                                              session, mtx::user_interactive::auth::Dummy{}});
                  } else {
                          // use fallback
                          auto dialog =
                            new dialogs::FallbackAuth(QString::fromStdString(current_stage),
                                                      QString::fromStdString(session),
                                                      this);

                          connect(dialog,
                                  &dialogs::FallbackAuth::confirmation,
                                  this,
                                  [this, user, pass, session, dialog]() {
                                          dialog->close();
                                          dialog->deleteLater();

                                          emit registerAuth(
                                            user,
                                            pass,
                                            mtx::user_interactive::Auth{
                                              session, mtx::user_interactive::auth::Fallback{}});
                                  });
                          connect(dialog,
                                  &dialogs::FallbackAuth::cancel,
                                  this,
                                  &RegisterPage::errorOccurred);

                          dialog->show();
                  }
          });

        connect(
          this,
          &RegisterPage::registerAuth,
          this,
          [this](const std::string &user,
                 const std::string &pass,
                 const mtx::user_interactive::Auth &auth) {
                  http::client()->registration(
                    user,
                    pass,
                    auth,
                    [this, user, pass](const mtx::responses::Register &res,
                                       mtx::http::RequestErr err) {
                            if (!err) {
                                    http::client()->set_user(res.user_id);
                                    http::client()->set_access_token(res.access_token);

                                    emit registerOk();
                                    return;
                            }

                            // The server requires registration flows.
                            if (err->status_code == boost::beast::http::status::unauthorized) {
                                    if (err->matrix_error.unauthorized.flows.empty()) {
                                            nhlog::net()->warn(
                                              "failed to retrieve registration flows: ({}) "
                                              "{}",
                                              static_cast<int>(err->status_code),
                                              err->matrix_error.error);
                                            emit registerErrorCb(
                                              QString::fromStdString(err->matrix_error.error));
                                            return;
                                    }

                                    emit registrationFlow(
                                      user, pass, err->matrix_error.unauthorized);
                                    return;
                            }

                            nhlog::net()->warn("failed to register: status_code ({}), "
                                               "matrix_error: ({}), parser error ({})",
                                               static_cast<int>(err->status_code),
                                               err->matrix_error.error,
                                               err->parse_error);

                            emit registerErrorCb(QString::fromStdString(err->matrix_error.error));
                    });
          });

        setLayout(top_layout_);
}

void
RegisterPage::onBackButtonClicked()
{
        emit backButtonClicked();
}

void
RegisterPage::showError(const QString &msg)
{
        emit errorOccurred();
        auto rect  = QFontMetrics(font()).boundingRect(msg);
        int width  = rect.width();
        int height = rect.height();
        error_label_->setFixedHeight(qCeil(width / 200.0) * height);
        error_label_->setText(msg);
}

void
RegisterPage::showError(QLabel *label, const QString &msg)
{
        emit errorOccurred();
        auto rect  = QFontMetrics(font()).boundingRect(msg);
        int width  = rect.width();
        int height = rect.height();
        label->setFixedHeight((int)qCeil(width / 200.0) * height);
        label->setText(msg);
}

bool
RegisterPage::checkOneField(QLabel *label, const TextField *t_field, const QString &msg)
{
        if (t_field->isValid()) {
                label->setText("");
                label->hide();
                return true;
        } else {
                label->show();
                showError(label, msg);
                return false;
        }
}

bool
RegisterPage::checkFields()
{
        error_label_->setText("");
        error_username_label_->setText("");
        error_password_label_->setText("");
        error_password_confirmation_label_->setText("");

        error_username_label_->hide();
        error_password_label_->hide();
        error_password_confirmation_label_->hide();

        password_confirmation_->setValid(true);
        server_input_->setValid(true);

        bool all_fields_good = true;
        if (!checkOneField(error_username_label_,
                           username_input_,
                           tr("The username must not be empty, and must contain only the "
                              "characters a-z, 0-9, ., _, =, -, and /."))) {
                all_fields_good = false;
        } else if (!checkOneField(error_password_label_,
                                  password_input_,
                                  tr("Password is not long enough (min 8 chars)"))) {
                all_fields_good = false;
        } else if (password_input_->text() != password_confirmation_->text()) {
                error_password_confirmation_label_->show();
                showError(error_password_confirmation_label_, tr("Passwords don't match"));
                password_confirmation_->setValid(false);
                all_fields_good = false;
        } else if (!server_input_->hasAcceptableInput() || server_input_->text().isEmpty()) {
                showError(tr("Invalid server name"));
                server_input_->setValid(false);
                all_fields_good = false;
        }
        return all_fields_good;
}

void
RegisterPage::onRegisterButtonClicked()
{
        if (!checkFields()) {
                showError(error_label_, tr("Regisration Failed"));
                return;
        } else {
                auto username = username_input_->text().toStdString();
                auto password = password_input_->text().toStdString();
                auto server   = server_input_->text().toStdString();

                http::client()->set_server(server);
                http::client()->registration(
                  username,
                  password,
                  [this, username, password](const mtx::responses::Register &res,
                                             mtx::http::RequestErr err) {
                          if (!err) {
                                  http::client()->set_user(res.user_id);
                                  http::client()->set_access_token(res.access_token);

                                  emit registerOk();
                                  return;
                          }

                          // The server requires registration flows.
                          if (err->status_code == boost::beast::http::status::unauthorized) {
                                  if (err->matrix_error.unauthorized.flows.empty()) {
                                          nhlog::net()->warn(
                                            "failed to retrieve registration flows1: ({}) "
                                            "{}",
                                            static_cast<int>(err->status_code),
                                            err->matrix_error.error);
                                          emit errorOccurred();
                                          emit registerErrorCb(
                                            QString::fromStdString(err->matrix_error.error));
                                          return;
                                  }

                                  emit registrationFlow(
                                    username, password, err->matrix_error.unauthorized);
                                  return;
                          }

                          nhlog::net()->error(
                            "failed to register: status_code ({}), matrix_error({})",
                            static_cast<int>(err->status_code),
                            err->matrix_error.error);

                          emit registerErrorCb(QString::fromStdString(err->matrix_error.error));
                          emit errorOccurred();
                  });

                emit registering();
        }
}

void
RegisterPage::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
