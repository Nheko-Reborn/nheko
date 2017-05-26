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
#include <QSettings>

#include "RoomState.h"

namespace events = matrix::events;

void RoomState::resolveName()
{
	name_ = "Empty Room";
	userAvatar_.clear();

	if (!name.content().name().isEmpty()) {
		name_ = name.content().name().simplified();
		return;
	}

	if (!canonical_alias.content().alias().isEmpty()) {
		name_ = canonical_alias.content().alias().simplified();
		return;
	}

	// FIXME: Doesn't follow the spec guidelines.
	if (aliases.content().aliases().size() != 0) {
		name_ = aliases.content().aliases()[0].simplified();
		return;
	}

	QSettings settings;
	auto user_id = settings.value("auth/user_id");

	// TODO: Display names should be sorted alphabetically.
	for (const auto membership : memberships) {
		if (membership.stateKey() == user_id)
			continue;

		if (membership.content().membershipState() == events::Membership::Join) {
			userAvatar_ = membership.stateKey();

			if (membership.content().displayName().isEmpty())
				name_ = membership.stateKey();
			else
				name_ = membership.content().displayName();

			break;
		}
	}

	// TODO: pluralization
	if (memberships.size() > 2)
		name_ = QString("%1 and %2 others").arg(name_).arg(memberships.size());
}

void RoomState::resolveAvatar()
{
	if (userAvatar_.isEmpty()) {
		avatar_ = avatar.content().url();
		return;
	}

	if (memberships.contains(userAvatar_)) {
		avatar_ = memberships[userAvatar_].content().avatarUrl();
	} else {
		qWarning() << "Setting room avatar from unknown user id" << userAvatar_;
	}
}

// Should be used only after initial sync.
void RoomState::removeLeaveMemberships()
{
	for (auto it = memberships.begin(); it != memberships.end();) {
		if (it.value().content().membershipState() == events::Membership::Leave) {
			it = memberships.erase(it);
		} else {
			++it;
		}
	}
}

void RoomState::update(const RoomState &state)
{
	bool needsNameCalculation = false;
	bool needsAvatarCalculation = false;

	if (aliases.eventId() != state.aliases.eventId()) {
		aliases = state.aliases;
	}

	if (avatar.eventId() != state.avatar.eventId()) {
		avatar = state.avatar;
		needsAvatarCalculation = true;
	}

	if (canonical_alias.eventId() != state.canonical_alias.eventId()) {
		canonical_alias = state.canonical_alias;
		needsNameCalculation = true;
	}

	if (create.eventId() != state.create.eventId())
		create = state.create;
	if (history_visibility.eventId() != state.history_visibility.eventId())
		history_visibility = state.history_visibility;
	if (join_rules.eventId() != state.join_rules.eventId())
		join_rules = state.join_rules;

	if (name.eventId() != state.name.eventId()) {
		name = state.name;
		needsNameCalculation = true;
	}

	if (power_levels.eventId() != state.power_levels.eventId())
		power_levels = state.power_levels;
	if (topic.eventId() != state.topic.eventId())
		topic = state.topic;

	for (auto it = state.memberships.constBegin(); it != state.memberships.constEnd(); ++it) {
		auto membershipState = it.value().content().membershipState();

		if (it.key() == userAvatar_) {
			needsNameCalculation = true;
			needsAvatarCalculation = true;
		}

		if (membershipState == events::Membership::Leave)
			this->memberships.remove(it.key());
		else
			this->memberships.insert(it.key(), it.value());
	}

	if (needsNameCalculation)
		resolveName();

	if (needsAvatarCalculation)
		resolveAvatar();
}
