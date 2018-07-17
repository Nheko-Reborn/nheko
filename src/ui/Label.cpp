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

#include "Label.h"
#include <QMouseEvent>

Label::Label(QWidget *parent, Qt::WindowFlags f)
  : QLabel(parent, f)
{}

Label::Label(const QString &text, QWidget *parent, Qt::WindowFlags f)
  : QLabel(text, parent, f)
{}

void
Label::mousePressEvent(QMouseEvent *e)
{
        pressPosition_ = e->pos();
        emit pressed(e);
        QLabel::mousePressEvent(e);
}

void
Label::mouseReleaseEvent(QMouseEvent *e)
{
        emit released(e);
        if (pressPosition_ == e->pos())
                emit clicked(e);
        QLabel::mouseReleaseEvent(e);
}
