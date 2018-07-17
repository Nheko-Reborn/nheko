#pragma once

#include <QGraphicsDropShadowEffect>
#include <QState>
#include <QStateMachine>

#include "FlatButton.h"

class RaisedButton : public FlatButton
{
        Q_OBJECT

public:
        explicit RaisedButton(QWidget *parent = 0);
        explicit RaisedButton(const QString &text, QWidget *parent = 0);
        ~RaisedButton();

protected:
        bool event(QEvent *event) override;

private:
        void init();

        QStateMachine *shadow_state_machine_;
        QState *normal_state_;
        QState *pressed_state_;
        QGraphicsDropShadowEffect *effect_;
};
