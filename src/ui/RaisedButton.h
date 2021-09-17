// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QGraphicsDropShadowEffect>
#include <QState>
#include <QStateMachine>

#include "FlatButton.h"

class RaisedButton : public FlatButton
{
    Q_OBJECT

public:
    explicit RaisedButton(QWidget *parent = nullptr);
    explicit RaisedButton(const QString &text, QWidget *parent = nullptr);
    ~RaisedButton() override;

protected:
    bool event(QEvent *event) override;

private:
    void init();

    QStateMachine *shadow_state_machine_;
    QState *normal_state_;
    QState *pressed_state_;
    QGraphicsDropShadowEffect *effect_;
};
