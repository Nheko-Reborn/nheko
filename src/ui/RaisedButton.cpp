// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RaisedButton.h"

void
RaisedButton::init()
{
    setBackgroundMode(Qt::OpaqueMode);
    setMinimumHeight(42);
    setBaseOpacity(0.3);
}

RaisedButton::RaisedButton(QWidget *parent)
  : FlatButton(parent)
{
    init();
}

RaisedButton::RaisedButton(const QString &text, QWidget *parent)
  : FlatButton(parent)
{
    init();
    setText(text);
}

bool
RaisedButton::event(QEvent *event)
{
    return FlatButton::event(event);
}
