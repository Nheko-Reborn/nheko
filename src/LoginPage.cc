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

#include "LoginPage.h"
#include "Config.h"
#include "FlatButton.h"
#include "InputValidator.h"
#include "LoadingIndicator.h"
#include "MatrixClient.h"
#include "OverlayModal.h"
#include "RaisedButton.h"
#include "TextField.h"

LoginPage::LoginPage(QSharedPointer<MatrixClient> client, QWidget *parent)
  : QWidget(parent)
  , inferredServerAddress_()
  , client_{client}
{
        setStyleSheet("background-color: #fff");

        top_layout_ = new QVBoxLayout();

        top_bar_layout_ = new QHBoxLayout();
        top_bar_layout_->setSpacing(0);
        top_bar_layout_->setMargin(0);

        back_button_ = new FlatButton(this);
        back_button_->setMinimumSize(QSize(30, 30));
        back_button_->setForegroundColor("#333333");

        top_bar_layout_->addWidget(back_button_, 0, Qt::AlignLeft | Qt::AlignVCenter);
        top_bar_layout_->addStretch(1);

        QIcon icon;
        icon.addFile(":/icons/icons/ui/angle-pointing-to-left.png");

        back_button_->setIcon(icon);
        back_button_->setIconSize(QSize(32, 32));

        QIcon logo;
        logo.addFile(":/logos/login.png");

        logo_ = new QLabel(this);
        logo_->setPixmap(logo.pixmap(128));

        logo_layout_ = new QHBoxLayout();
        logo_layout_->setContentsMargins(0, 0, 0, 20);
        logo_layout_->addWidget(logo_, 0, Qt::AlignHCenter);

        form_wrapper_ = new QHBoxLayout();
        form_widget_  = new QWidget();
        form_widget_->setMinimumSize(QSize(350, 200));

        form_layout_ = new QVBoxLayout();
        form_layout_->setSpacing(20);
        form_layout_->setContentsMargins(0, 0, 0, 30);
        form_widget_->setLayout(form_layout_);

        form_wrapper_->addStretch(1);
        form_wrapper_->addWidget(form_widget_);
        form_wrapper_->addStretch(1);

        matrixid_input_ = new TextField(this);
        matrixid_input_->setTextColor("#333333");
        matrixid_input_->setLabel(tr("Matrix ID"));
        matrixid_input_->setInkColor("#555459");
        matrixid_input_->setBackgroundColor("#fff");
        matrixid_input_->setPlaceholderText(tr("e.g @joe:matrix.org"));

        spinner_ = new LoadingIndicator(this);
        spinner_->setColor("#333333");
        spinner_->setFixedHeight(40);
        spinner_->setFixedWidth(40);
        spinner_->hide();

        errorIcon_ = new QLabel(this);
        errorIcon_->setPixmap(QPixmap(":/icons/icons/error.png"));
        errorIcon_->hide();

        matrixidLayout_ = new QHBoxLayout();
        matrixidLayout_->addWidget(matrixid_input_, 0, Qt::AlignVCenter);

        password_input_ = new TextField(this);
        password_input_->setTextColor("#333333");
        password_input_->setLabel(tr("Password"));
        password_input_->setInkColor("#555459");
        password_input_->setBackgroundColor("#fff");
        password_input_->setEchoMode(QLineEdit::Password);

        serverInput_ = new TextField(this);
        serverInput_->setTextColor("#333333");
        serverInput_->setLabel("Homeserver address");
        serverInput_->setInkColor("#555459");
        serverInput_->setBackgroundColor("#fff");
        serverInput_->setPlaceholderText("matrix.org");
        serverInput_->hide();

        serverLayout_ = new QHBoxLayout();
        serverLayout_->addWidget(serverInput_, 0, Qt::AlignVCenter);

        form_layout_->addLayout(matrixidLayout_);
        form_layout_->addWidget(password_input_, Qt::AlignHCenter, 0);
        form_layout_->addLayout(serverLayout_);

        button_layout_ = new QHBoxLayout();
        button_layout_->setSpacing(0);
        button_layout_->setContentsMargins(0, 0, 0, 30);

        login_button_ = new RaisedButton(tr("LOGIN"), this);
        login_button_->setBackgroundColor(QColor("#333333"));
        login_button_->setForegroundColor(QColor("white"));
        login_button_->setMinimumSize(350, 65);
        login_button_->setFontSize(20);
        login_button_->setCornerRadius(3);

        button_layout_->addStretch(1);
        button_layout_->addWidget(login_button_);
        button_layout_->addStretch(1);

        QFont font;
        font.setPixelSize(conf::fontSize);

        error_label_ = new QLabel(this);
        error_label_->setFont(font);
        error_label_->setStyleSheet("color: #E22826");

        top_layout_->addLayout(top_bar_layout_);
        top_layout_->addStretch(1);
        top_layout_->addLayout(logo_layout_);
        top_layout_->addLayout(form_wrapper_);
        top_layout_->addStretch(1);
        top_layout_->addLayout(button_layout_);
        top_layout_->addWidget(error_label_, 0, Qt::AlignHCenter);
        top_layout_->addStretch(1);

        setLayout(top_layout_);

        connect(back_button_, SIGNAL(clicked()), this, SLOT(onBackButtonClicked()));
        connect(login_button_, SIGNAL(clicked()), this, SLOT(onLoginButtonClicked()));
        connect(matrixid_input_, SIGNAL(returnPressed()), login_button_, SLOT(click()));
        connect(password_input_, SIGNAL(returnPressed()), login_button_, SLOT(click()));
        connect(serverInput_, SIGNAL(returnPressed()), login_button_, SLOT(click()));
        connect(client_.data(), SIGNAL(loginError(QString)), this, SLOT(loginError(QString)));
        connect(matrixid_input_, SIGNAL(editingFinished()), this, SLOT(onMatrixIdEntered()));
        connect(client_.data(), SIGNAL(versionError(QString)), this, SLOT(versionError(QString)));
        connect(client_.data(), SIGNAL(versionSuccess()), this, SLOT(versionSuccess()));
        connect(serverInput_, SIGNAL(editingFinished()), this, SLOT(onServerAddressEntered()));
}

void
LoginPage::loginError(QString error)
{
        error_label_->setText(error);
}

bool
LoginPage::isMatrixIdValid()
{
        int pos        = 0;
        auto matrix_id = matrixid_input_->text();

        return InputValidator::Id.validate(matrix_id, pos) == QValidator::Acceptable;
}

void
LoginPage::onMatrixIdEntered()
{
        error_label_->setText("");

        if (!isMatrixIdValid()) {
                loginError("You have entered an invalid Matrix ID  e.g @joe:matrix.org");
                return;
        } else if (password_input_->text().isEmpty()) {
                loginError(tr("Empty password"));
        }

        QString homeServer = matrixid_input_->text().split(":").at(1);
        if (homeServer != inferredServerAddress_) {
                serverInput_->hide();
                serverLayout_->removeWidget(errorIcon_);
                errorIcon_->hide();
                if (serverInput_->isVisible()) {
                        matrixidLayout_->removeWidget(spinner_);
                        serverLayout_->addWidget(spinner_, 0, Qt::AlignVCenter | Qt::AlignRight);
                        spinner_->start();
                } else {
                        serverLayout_->removeWidget(spinner_);
                        matrixidLayout_->addWidget(spinner_, 0, Qt::AlignVCenter | Qt::AlignRight);
                        spinner_->start();
                }

                inferredServerAddress_ = homeServer;
                serverInput_->setText(homeServer);
                client_->setServer(homeServer);
                client_->versions();
        }
}

void
LoginPage::onServerAddressEntered()
{
        error_label_->setText("");
        client_->setServer(serverInput_->text());
        client_->versions();

        serverLayout_->removeWidget(errorIcon_);
        errorIcon_->hide();
        serverLayout_->addWidget(spinner_, 0, Qt::AlignVCenter | Qt::AlignRight);
        spinner_->start();
}

void
LoginPage::versionError(QString error)
{
        // Matrix homeservers are often kept on a subdomain called 'matrix'
        // so let's try that next, unless the address was set explicitly or the domain
        // part of the username already points to this subdomain
        QUrl currentServer  = client_->getHomeServer();
        QString mxidAddress = matrixid_input_->text().split(":").at(1);
        if (currentServer.host() == inferredServerAddress_ &&
            !currentServer.host().startsWith("matrix")) {
                error_label_->setText("");
                currentServer.setHost(QString("matrix.") + currentServer.host());
                serverInput_->setText(currentServer.host());
                client_->setServer(currentServer.host());
                client_->versions();
                return;
        }

        error_label_->setText(error);
        serverInput_->show();

        spinner_->stop();
        serverLayout_->removeWidget(spinner_);
        serverLayout_->addWidget(errorIcon_, 0, Qt::AlignVCenter | Qt::AlignRight);
        errorIcon_->show();
        matrixidLayout_->removeWidget(spinner_);
}

void
LoginPage::versionSuccess()
{
        serverLayout_->removeWidget(spinner_);
        matrixidLayout_->removeWidget(spinner_);
        spinner_->stop();

        if (serverInput_->isVisible())
                serverInput_->hide();
}

void
LoginPage::onLoginButtonClicked()
{
        error_label_->setText("");

        if (!isMatrixIdValid()) {
                loginError("You have entered an invalid Matrix ID  e.g @joe:matrix.org");
        } else if (password_input_->text().isEmpty()) {
                loginError("Empty password");
        } else {
                QString user     = matrixid_input_->text().split(":").at(0).split("@").at(1);
                QString password = password_input_->text();
                client_->setServer(serverInput_->text());
                client_->login(user, password);
        }
}

void
LoginPage::reset()
{
        matrixid_input_->clear();
        password_input_->clear();
        serverInput_->clear();

        spinner_->stop();
        errorIcon_->hide();
        serverLayout_->removeWidget(spinner_);
        serverLayout_->removeWidget(errorIcon_);
        matrixidLayout_->removeWidget(spinner_);

        inferredServerAddress_.clear();
}

void
LoginPage::onBackButtonClicked()
{
        emit backButtonClicked();
}

LoginPage::~LoginPage() {}
