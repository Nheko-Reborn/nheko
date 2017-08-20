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

#include "JoinRulesEventContent.h"

using namespace matrix::events;

void
JoinRulesEventContent::deserialize(const QJsonValue &data)
{
	if (!data.isObject())
		throw DeserializationException("JoinRulesEventContent is not a JSON object");

	auto object = data.toObject();

	if (object.value("join_rule") == QJsonValue::Undefined)
		throw DeserializationException("join_rule key is missing");

	auto value = object.value("join_rule").toString();

	if (value == "invite")
		join_rule_ = JoinRule::Invite;
	else if (value == "knock")
		join_rule_ = JoinRule::Knock;
	else if (value == "private")
		join_rule_ = JoinRule::Private;
	else if (value == "public")
		join_rule_ = JoinRule::Public;
	else
		throw DeserializationException(QString("Unknown join_rule value: %1").arg(value).toUtf8().constData());
}

QJsonObject
JoinRulesEventContent::serialize() const
{
	QJsonObject object;

	if (join_rule_ == JoinRule::Invite)
		object["join_rule"] = "invite";
	else if (join_rule_ == JoinRule::Knock)
		object["join_rule"] = "knock";
	else if (join_rule_ == JoinRule::Private)
		object["join_rule"] = "private";
	else if (join_rule_ == JoinRule::Public)
		object["join_rule"] = "public";

	return object;
}
