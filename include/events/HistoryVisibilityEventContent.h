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

#ifndef HISTORY_VISIBILITY_EVENT_CONTENT_H
#define HISTORY_VISIBILITY_EVENT_CONTENT_H

#include <QJsonValue>

#include "Deserializable.h"

namespace matrix
{
namespace events
{
enum HistoryVisibility {
	Invited,
	Joined,
	Shared,
	WorldReadable,
};

class HistoryVisibilityEventContent : public Deserializable
{
public:
	inline HistoryVisibility historyVisibility() const;

	void deserialize(const QJsonValue &data) override;

private:
	HistoryVisibility history_visibility_;
};

inline HistoryVisibility HistoryVisibilityEventContent::historyVisibility() const
{
	return history_visibility_;
}
}  // namespace events
}  // namespace matrix

#endif  // HISTORY_VISIBILITY_EVENT_CONTENT_H
