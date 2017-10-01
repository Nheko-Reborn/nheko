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

const QRegExp MXID_REGEX("@[A-Za-z0-9._%+-]+:[A-Za-z0-9.-]{1,126}\\.[A-Za-z]{1,63}");
const QRegExp LOCALPART_REGEX("[A-za-z0-9._%+-]{3,}");
const QRegExp PASSWORD_REGEX(".{8,}");
const QRegExp DOMAIN_REGEX("(?!\\-)(?:[a-zA-Z\\d\\-]{0,62}[a-zA-Z\\d]\\.){1,"
                           "126}(?!\\d+)[a-zA-Z\\d]{1,63}");

QRegExpValidator InputValidator::Id(MXID_REGEX);
QRegExpValidator InputValidator::Localpart(LOCALPART_REGEX);
QRegExpValidator InputValidator::Password(PASSWORD_REGEX);
QRegExpValidator InputValidator::Domain(DOMAIN_REGEX);
