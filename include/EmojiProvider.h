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

#ifndef EMOJI_PROVIDER_H
#define EMOJI_PROVIDER_H

#include <QFile>
#include <QList>
#include <QMap>

struct Emoji {
	// Unicode code.
	QString unicode;
	// Keyboard shortcut e.g :emoji:
	QString shortname;
};

class EmojiProvider
{
public:
	static const QList<Emoji> people;
	static const QList<Emoji> nature;
	static const QList<Emoji> food;
	static const QList<Emoji> activity;
	static const QList<Emoji> travel;
	static const QList<Emoji> objects;
	static const QList<Emoji> symbols;
	static const QList<Emoji> flags;
};

#endif  // EMOJI_PROVIDER_H
