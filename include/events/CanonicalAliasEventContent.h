
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

#ifndef CANONICAL_ALIAS_EVENT_CONTENT_H
#define CANONICAL_ALIAS_EVENT_CONTENT_H

#include <QJsonValue>

#include "CanonicalAliasEventContent.h"
#include "Deserializable.h"

/*
 * This event is used to inform the room about which alias should be considered
 * the canonical one. This could be for display purposes or as suggestion to
 * users which alias to use to advertise the room.
 */

class CanonicalAliasEventContent : public Deserializable
{
public:
	void deserialize(const QJsonValue &data) override;

	inline QString alias() const;

private:
	QString alias_;
};

inline QString CanonicalAliasEventContent::alias() const
{
	return alias_;
}

#endif  // CANONICAL_ALIAS_EVENT_CONTENT_H
