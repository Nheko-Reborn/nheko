// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QInputDialog>
#include <QLabel>
#include <QMetaType>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>
#include <QtMath>

#include <mtx/responses/register.hpp>
#include <mtx/responses/well-known.hpp>
#include <mtxclient/http/client.hpp>

#include "Config.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "RegisterPage.h"
#include "ui/FlatButton.h"
#include "ui/RaisedButton.h"
#include "ui/TextField.h"
#include "ui/UIA.h"

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
    icon.addFile(":/icons/icons/ui/angle-arrow-left.svg");

    back_button_->setIcon(icon);
    back_button_->setIconSize(QSize(32, 32));

    back_layout_->addWidget(back_button_, 0, Qt::AlignLeft | Qt::AlignVCenter);
    back_layout_->addStretch(1);

    QIcon logo;
    logo.addFile(":/logos/register.png");

    logo_ = new QLabel(this);
    logo_->setPixmap(logo.pixmap(128));

    logo_layout_ = new QHBoxLayout();
    logo_layout_->setContentsMargins(0, 0, 0, 0);
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
    username_input_->setRegexp(QRegularExpression("[a-z0-9._=/-]+"));
    username_input_->setToolTip(tr("The username must not be empty, and must contain only the "
                                   "characters a-z, 0-9, ., _, =, -, and /."));

    password_input_ = new TextField();
    password_input_->setLabel(tr("Password"));
    password_input_->setRegexp(QRegularExpression("^.{8,}$"));
    password_input_->setEchoMode(QLineEdit::Password);
    password_input_->setToolTip(tr("Please choose a secure password. The exact requirements "
                                   "for password strength may depend on your server."));

    password_confirmation_ = new TextField();
    password_confirmation_->setLabel(tr("Password confirmation"));
    password_confirmation_->setEchoMode(QLineEdit::Password);

    server_input_ = new TextField();
    server_input_->setLabel(tr("Homeserver"));
    server_input_->setRegexp(QRegularExpression(".+"));
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

    error_server_label_ = new QLabel(this);
    error_server_label_->setWordWrap(true);
    error_server_label_->hide();

    form_layout_->addWidget(username_input_, Qt::AlignHCenter);
    form_layout_->addWidget(error_username_label_, Qt::AlignHCenter);
    form_layout_->addWidget(password_input_, Qt::AlignHCenter);
    form_layout_->addWidget(error_password_label_, Qt::AlignHCenter);
    form_layout_->addWidget(password_confirmation_, Qt::AlignHCenter);
    form_layout_->addWidget(error_password_confirmation_label_, Qt::AlignHCenter);
    form_layout_->addWidget(server_input_, Qt::AlignHCenter);
    form_layout_->addWidget(error_server_label_, Qt::AlignHCenter);

    button_layout_ = new QHBoxLayout();
    button_layout_->setSpacing(0);
    button_layout_->setContentsMargins(0, 0, 0, 0);

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
    setLayout(top_layout_);

    connect(back_button_, SIGNAL(clicked()), this, SLOT(onBackButtonClicked()));
    connect(register_button_, SIGNAL(clicked()), this, SLOT(onRegisterButtonClicked()));

    connect(username_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
    connect(username_input_, &TextField::editingFinished, this, &RegisterPage::checkUsername);
    connect(password_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
    connect(password_input_, &TextField::editingFinished, this, &RegisterPage::checkPassword);
    connect(password_confirmation_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
    connect(password_confirmation_,
            &TextField::editingFinished,
            this,
            &RegisterPage::checkPasswordConfirmation);
    connect(server_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
    connect(server_input_, &TextField::editingFinished, this, &RegisterPage::checkServer);

    connect(
      this,
      &RegisterPage::serverError,
      this,
      [this](const QString &msg) {
          server_input_->setValid(false);
          showError(error_server_label_, msg);
      },
      Qt::QueuedConnection);

    connect(this, &RegisterPage::wellKnownLookup, this, &RegisterPage::doWellKnownLookup);
    connect(this, &RegisterPage::versionsCheck, this, &RegisterPage::doVersionsCheck);
    connect(this, &RegisterPage::registration, this, &RegisterPage::doRegistration);
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
    label->show();
}

bool
RegisterPage::checkOneField(QLabel *label, const TextField *t_field, const QString &msg)
{
    if (t_field->isValid()) {
        label->hide();
        return true;
    } else {
        showError(label, msg);
        return false;
    }
}

bool
RegisterPage::checkUsername()
{
    return checkOneField(error_username_label_,
                         username_input_,
                         tr("The username must not be empty, and must contain only the "
                            "characters a-z, 0-9, ., _, =, -, and /."));
}

bool
RegisterPage::checkPassword()
{
    return checkOneField(
      error_password_label_, password_input_, tr("Password is not long enough (min 8 chars)"));
}

bool
RegisterPage::checkPasswordConfirmation()
{
    if (password_input_->text() == password_confirmation_->text()) {
        error_password_confirmation_label_->hide();
        password_confirmation_->setValid(true);
        return true;
    } else {
        showError(error_password_confirmation_label_, tr("Passwords don't match"));
        password_confirmation_->setValid(false);
        return false;
    }
}

bool
RegisterPage::checkServer()
{
    // This doesn't check that the server is reachable,
    // just that the input is not obviously wrong.
    return checkOneField(error_server_label_, server_input_, tr("Invalid server name"));
}

void
RegisterPage::onRegisterButtonClicked()
{
    if (checkUsername() && checkPassword() && checkPasswordConfirmation() && checkServer()) {
        auto server = server_input_->text().toStdString();

        http::client()->set_server(server);
        http::client()->verify_certificates(
          !UserSettings::instance()->disableCertificateValidation());

        // This starts a chain of `emit`s which ends up doing the
        // registration. Signals are used rather than normal function
        // calls so that the dialogs used in UIA work correctly.
        //
        // The sequence of events looks something like this:
        //
        // doKnownLookup
        //   v
        // doVersionsCheck
        //   v
        // doRegistration -> loops the UIAHandler until complete

        emit wellKnownLookup();

        emit registering();
    }
}

void
RegisterPage::doWellKnownLookup()
{
    http::client()->well_known(
      [this](const mtx::responses::WellKnown &res, mtx::http::RequestErr err) {
          if (err) {
              if (err->status_code == 404) {
                  nhlog::net()->info("Autodiscovery: No .well-known.");
                  // Check that the homeserver can be reached
                  emit versionsCheck();
                  return;
              }

              if (!err->parse_error.empty()) {
                  emit serverError(tr("Autodiscovery failed. Received malformed response."));
                  nhlog::net()->error("Autodiscovery failed. Received malformed response.");
                  return;
              }

              emit serverError(tr("Autodiscovery failed. Unknown error when "
                                  "requesting .well-known."));
              nhlog::net()->error("Autodiscovery failed. Unknown error when "
                                  "requesting .well-known. {} {}",
                                  err->status_code,
                                  err->error_code);
              return;
          }

          nhlog::net()->info("Autodiscovery: Discovered '" + res.homeserver.base_url + "'");
          http::client()->set_server(res.homeserver.base_url);
          // Check that the homeserver can be reached
          emit versionsCheck();
      });
}

void
RegisterPage::doVersionsCheck()
{
    // Make a request to /_matrix/client/versions to check the address
    // given is a Matrix homeserver.
    http::client()->versions([this](const mtx::responses::Versions &, mtx::http::RequestErr err) {
        if (err) {
            if (err->status_code == 404) {
                emit serverError(tr("The required endpoints were not found. Possibly "
                                    "not a Matrix server."));
                return;
            }

            if (!err->parse_error.empty()) {
                emit serverError(tr("Received malformed response. Make sure the homeserver "
                                    "domain is valid."));
                return;
            }

            emit serverError(tr("An unknown error occured. Make sure the "
                                "homeserver domain is valid."));
            return;
        }

        // Attempt registration without an `auth` dict
        emit registration();
    });
}

void
RegisterPage::doRegistration()
{
    // These inputs should still be alright, but check just in case
    if (checkUsername() && checkPassword() && checkPasswordConfirmation()) {
        auto username = username_input_->text().toStdString();
        auto password = password_input_->text().toStdString();
        connect(UIA::instance(), &UIA::error, this, [this](QString msg) {
            showError(msg);
            disconnect(UIA::instance(), &UIA::error, this, nullptr);
        });
        http::client()->registration(
          username, password, ::UIA::instance()->genericHandler("Registration"), registrationCb());
    }
}

mtx::http::Callback<mtx::responses::Register>
RegisterPage::registrationCb()
{
    // Return a function to be used as the callback when an attempt at
    // registration is made.
    return [this](const mtx::responses::Register &res, mtx::http::RequestErr err) {
        if (!err) {
            http::client()->set_user(res.user_id);
            http::client()->set_access_token(res.access_token);
            emit registerOk();
            disconnect(UIA::instance(), &UIA::error, this, nullptr);
            return;
        }

        // The server requires registration flows.
        if (err->status_code == 401) {
            if (err->matrix_error.unauthorized.flows.empty()) {
                nhlog::net()->warn("failed to retrieve registration flows: "
                                   "status_code({}), matrix_error({}) ",
                                   static_cast<int>(err->status_code),
                                   err->matrix_error.error);
                showError(QString::fromStdString(err->matrix_error.error));
            }
            return;
        }

        nhlog::net()->error("failed to register: status_code ({}), matrix_error({})",
                            static_cast<int>(err->status_code),
                            err->matrix_error.error);

        showError(QString::fromStdString(err->matrix_error.error));
    };
}

void
RegisterPage::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
