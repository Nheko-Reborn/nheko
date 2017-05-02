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

#ifndef MEMBER_EVENT_CONTENT_H
#define MEMBER_EVENT_CONTENT_H

#include <QJsonValue>
#include <QUrl>

#include "Deserializable.h"

enum Membership {
	// The user is banned.
	BanState,

	// The user has been invited.
	InviteState,

	// The user has joined.
	JoinState,

	// The user has requested to join.
	KnockState,

	// The user has left.
	LeaveState,
};

/*
 * The current membership state of a user in the room.
 */

class MemberEventContent : public Deserializable
{
public:
	void deserialize(const QJsonValue &data) override;

	inline QUrl avatarUrl() const;
	inline QString displayName() const;
	inline Membership membershipState() const;

private:
	QUrl avatar_url_;
	QString display_name_;
	Membership membership_state_;
};

inline QUrl MemberEventContent::avatarUrl() const
{
	return avatar_url_;
}

inline QString MemberEventContent::displayName() const
{
	return display_name_;
}

inline Membership MemberEventContent::membershipState() const
{
	return membership_state_;
}

#endif  // MEMBER_EVENT_CONTENT_H
