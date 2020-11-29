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

#include <QWidget>

#include <memory>

#include <mtx/user_interactive.hpp>

class FlatButton;
class RaisedButton;
class TextField;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

class RegisterPage : public QWidget
{
        Q_OBJECT

public:
        RegisterPage(QWidget *parent = nullptr);

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void backButtonClicked();
        void errorOccurred();
        void registering();
        void registerOk();
        void registerErrorCb(const QString &msg);
        void registrationFlow(const std::string &user,
                              const std::string &pass,
                              const mtx::user_interactive::Unauthorized &unauthorized);
        void registerAuth(const std::string &user,
                          const std::string &pass,
                          const mtx::user_interactive::Auth &auth);

private slots:
        void onBackButtonClicked();
        void onRegisterButtonClicked();

        // Display registration specific errors to the user.
        void registerError(const QString &msg);
        // Display username hint, if it's not valid
        void usernameError(const QString &msg);

private:
        bool isUsernameValid(const QString &username);
        bool checkUsername();

        QVBoxLayout *top_layout_;

        QHBoxLayout *back_layout_;
        QHBoxLayout *logo_layout_;
        QHBoxLayout *button_layout_;

        QLabel *logo_;
        QLabel *error_label_;
        QLabel *error_username_label_;

        FlatButton *back_button_;
        RaisedButton *register_button_;

        QWidget *form_widget_;
        QHBoxLayout *form_wrapper_;
        QVBoxLayout *form_layout_;

        TextField *username_input_;
        TextField *password_input_;
        TextField *password_confirmation_;
        TextField *server_input_;
};
