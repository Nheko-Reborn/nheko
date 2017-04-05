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

#include "InputValidator.h"

// FIXME: Maybe change the regex to match the real Matrix ID format and not email.
InputValidator::InputValidator(QObject *parent)
    : matrix_id_("[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,6}")
    , matrix_localpart_("[A-za-z0-9._%+-]{3,}")
    , matrix_password_(".{8,}")
    , server_domain_("(?!\\-)(?:[a-zA-Z\\d\\-]{0,62}[a-zA-Z\\d]\\.){1,126}(?!\\d+)[a-zA-Z\\d]{1,63}")
{
	id_ = new QRegExpValidator(matrix_id_, parent);
	localpart_ = new QRegExpValidator(matrix_localpart_, parent);
	password_ = new QRegExpValidator(matrix_password_, parent);
	domain_ = new QRegExpValidator(server_domain_, parent);
}
