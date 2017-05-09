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

#ifndef LOGINPAGE_H
#define LOGINPAGE_H

#include <QHBoxLayout>
#include <QLabel>
#include <QSharedPointer>
#include <QVBoxLayout>
#include <QWidget>

#include "FlatButton.h"
#include "InputValidator.h"
#include "LoginSettings.h"
#include "MatrixClient.h"
#include "OverlayModal.h"
#include "RaisedButton.h"
#include "TextField.h"

class LoginPage : public QWidget
{
	Q_OBJECT

public:
	LoginPage(QSharedPointer<MatrixClient> client, QWidget *parent = 0);
	~LoginPage();

	void reset();

signals:
	void backButtonClicked();

private slots:
	// Callback for the back button.
	void onBackButtonClicked();

	// Callback for the login button.
	void onLoginButtonClicked();

	// Displays errors produced during the login.
	void loginError(QString error_message);

	// Manipulate settings modal.
	void showSettingsModal();
	void closeSettingsModal(const QString &server);

private:
	QVBoxLayout *top_layout_;

	QHBoxLayout *top_bar_layout_;
	QHBoxLayout *logo_layout_;
	QHBoxLayout *button_layout_;

	QLabel *logo_;
	QLabel *error_label_;

	FlatButton *back_button_;
	FlatButton *advanced_settings_button_;
	RaisedButton *login_button_;

	QWidget *form_widget_;
	QHBoxLayout *form_wrapper_;
	QVBoxLayout *form_layout_;

	TextField *matrixid_input_;
	TextField *password_input_;

	OverlayModal *settings_modal_;
	LoginSettings *login_settings_;
	QString custom_domain_;

	InputValidator *matrix_id_validator_;

	// Matrix client API provider.
	QSharedPointer<MatrixClient> client_;
};

#endif  // LOGINPAGE_H
