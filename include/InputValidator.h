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

#ifndef MATRIXIDVALIDATOR_H
#define MATRIXIDVALIDATOR_H

#include <QRegExp>
#include <QRegExpValidator>

class InputValidator
{
public:
	InputValidator(QObject *parent = 0);

	// Validators for the different types of input.
	QRegExpValidator *id_;
	QRegExpValidator *localpart_;
	QRegExpValidator *password_;
	QRegExpValidator *domain_;

private:
	// Regular expression used to validate the whole matrix id.
	const QRegExp matrix_id_;

	// Regular expressino to validate the matrix localpart.
	const QRegExp matrix_localpart_;

	// Regular expression to validate a password for a matrix account.
	const QRegExp matrix_password_;

	// Regular expression to validate a domain name.
	const QRegExp server_domain_;
};

#endif  // MATRIXIDVALIDATOR_H
