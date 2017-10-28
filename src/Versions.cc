/*
 * nheko Copyright (C) 2017  Jan Solanti <jhs@psonet.com>
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

#include <QJsonArray>
#include <QRegExp>

#include "Deserializable.h"
#include "Versions.h"

void
VersionsResponse::deserialize(const QJsonDocument &data)
{
        if (!data.isObject())
                throw DeserializationException("Versions response is not a JSON object");

        QJsonObject object = data.object();

        if (object.value("versions") == QJsonValue::Undefined)
                throw DeserializationException("Versions: missing version list");

        auto versions = object.value("versions").toArray();
        for (auto const &elem : versions) {
                QString str = elem.toString();
                QRegExp rx("r(\\d+)\\.(\\d+)\\.(\\d+)");

                if (rx.indexIn(str) == -1)
                        throw DeserializationException(
                          "Invalid version string in versions response");

                struct Version_ v;
                v.major_ = rx.cap(1).toUInt();
                v.minor_ = rx.cap(2).toUInt();
                v.patch_ = rx.cap(3).toUInt();

                supported_versions_.push_back(v);
        }
}

bool
VersionsResponse::isVersionSupported(unsigned int major, unsigned int minor, unsigned int patch)
{
        for (auto &v : supported_versions_) {
                if (v.major_ == major && v.minor_ == minor && v.patch_ >= patch)
                        return true;
        }

        return false;
}
