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

#include <QDebug>

#include "MemberEventContent.h"

void MemberEventContent::deserialize(const QJsonValue &data)
{
	if (!data.isObject())
		throw DeserializationException("MemberEventContent is not a JSON object");

	auto object = data.toObject();

	if (!object.contains("membership"))
		throw DeserializationException("membership key is missing");

	auto value = object.value("membership").toString();

	if (value == "ban")
		membership_state_ = Membership::BanState;
	else if (value == "invite")
		membership_state_ = Membership::InviteState;
	else if (value == "join")
		membership_state_ = Membership::JoinState;
	else if (value == "knock")
		membership_state_ = Membership::KnockState;
	else if (value == "leave")
		membership_state_ = Membership::LeaveState;
	else
		throw DeserializationException(QString("Unknown membership value: %1").arg(value).toUtf8().constData());

	if (object.contains("avatar_url"))
		avatar_url_ = QUrl(object.value("avatar_url").toString());

	if (!avatar_url_.toString().isEmpty() && !avatar_url_.isValid())
		qWarning() << "Invalid avatar url" << avatar_url_;

	if (object.contains("displayname"))
		display_name_ = object.value("displayname").toString();
}
