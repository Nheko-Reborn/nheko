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

#include <QJsonDocument>
#include <QVector>

#include "Deserializable.h"

class VersionsResponse : public Deserializable
{
public:
        void deserialize(const QJsonDocument &data) override;

        bool isVersionSupported(unsigned int major, unsigned int minor, unsigned int patch);

private:
        struct Version_
        {
                unsigned int major_;
                unsigned int minor_;
                unsigned int patch_;
        };

        QVector<Version_> supported_versions_;
};
