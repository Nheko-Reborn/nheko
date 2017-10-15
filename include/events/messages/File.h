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

#pragma once

#include <QJsonObject>

#include "Deserializable.h"
#include "MessageEvent.h"

namespace matrix {
namespace events {
namespace messages {
struct FileInfo
{
        int size;

        QString mimetype;
        QString thumbnail_url;
        ThumbnailInfo thumbnail_info;
};

class File : public Deserializable
{
public:
        inline QString url() const;
        inline QString filename() const;

        inline FileInfo info() const;

        void deserialize(const QJsonObject &object) override;

private:
        QString url_;
        QString filename_;

        FileInfo info_;
};

inline QString
File::filename() const
{
        return filename_;
}

inline QString
File::url() const
{
        return url_;
}

inline FileInfo
File::info() const
{
        return info_;
}

} // namespace messages
} // namespace events
} // namespace matrix
