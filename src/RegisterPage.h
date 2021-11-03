// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

#include <memory>

#include <mtx/user_interactive.hpp>
#include <mtxclient/http/client.hpp>

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

    //! Used to trigger the corresponding slot outside of the main thread.
    void serverError(const QString &err);

    void wellKnownLookup();
    void versionsCheck();
    void registration();

    void registering();
    void registerOk();

private slots:
    void onBackButtonClicked();
    void onRegisterButtonClicked();

    // function for showing different errors
    void showError(const QString &msg);
    void showError(QLabel *label, const QString &msg);

    bool checkOneField(QLabel *label, const TextField *t_field, const QString &msg);
    bool checkUsername();
    bool checkPassword();
    bool checkPasswordConfirmation();
    bool checkServer();

    void doWellKnownLookup();
    void doVersionsCheck();
    void doRegistration();
    mtx::http::Callback<mtx::responses::Register> registrationCb();

private:
    QVBoxLayout *top_layout_;

    QHBoxLayout *back_layout_;
    QHBoxLayout *logo_layout_;
    QHBoxLayout *button_layout_;

    QLabel *logo_;
    QLabel *error_label_;
    QLabel *error_username_label_;
    QLabel *error_password_label_;
    QLabel *error_password_confirmation_label_;
    QLabel *error_server_label_;
    QLabel *error_registration_token_label_;

    FlatButton *back_button_;
    RaisedButton *register_button_;

    QWidget *form_widget_;
    QHBoxLayout *form_wrapper_;
    QVBoxLayout *form_layout_;

    TextField *username_input_;
    TextField *password_input_;
    TextField *password_confirmation_;
    TextField *server_input_;
    TextField *registration_token_input_;
};
