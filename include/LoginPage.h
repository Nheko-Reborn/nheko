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

#pragma once

#include <QLabel>
#include <QLayout>
#include <QSharedPointer>
#include <QWidget>

class FlatButton;
class LoadingIndicator;
class MatrixClient;
class OverlayModal;
class RaisedButton;
class TextField;

class LoginPage : public QWidget
{
        Q_OBJECT

public:
        LoginPage(QSharedPointer<MatrixClient> client, QWidget *parent = 0);
        ~LoginPage();

        void reset();

signals:
        void backButtonClicked();

public slots:
        // Displays errors produced during the login.
        void loginError(QString error_message);

private slots:
        // Callback for the back button.
        void onBackButtonClicked();

        // Callback for the login button.
        void onLoginButtonClicked();

        // Callback for probing the server found in the mxid
        void onMatrixIdEntered();

        // Callback for probing the manually entered server
        void onServerAddressEntered();

        // Callback for errors produced during server probing
        void versionError(QString error_message);

        // Callback for successful server probing
        void versionSuccess();

private:
        bool isMatrixIdValid();

        QVBoxLayout *top_layout_;

        QHBoxLayout *top_bar_layout_;
        QHBoxLayout *logo_layout_;
        QHBoxLayout *button_layout_;

        QLabel *logo_;
        QLabel *error_label_;

        QHBoxLayout *serverLayout_;
        QHBoxLayout *matrixidLayout_;
        LoadingIndicator *spinner_;
        QLabel *errorIcon_;
        QString inferredServerAddress_;

        FlatButton *back_button_;
        RaisedButton *login_button_;

        QWidget *form_widget_;
        QHBoxLayout *form_wrapper_;
        QVBoxLayout *form_layout_;

        TextField *matrixid_input_;
        TextField *password_input_;
        TextField *serverInput_;

        // Matrix client API provider.
        QSharedPointer<MatrixClient> client_;
};
