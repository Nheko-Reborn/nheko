// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
