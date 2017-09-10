#include <QEventTransition>
#include <QPropertyAnimation>

#include "RaisedButton.h"

void
RaisedButton::init()
{
        shadow_state_machine_ = new QStateMachine(this);
        normal_state_         = new QState;
        pressed_state_        = new QState;
        effect_               = new QGraphicsDropShadowEffect;

        effect_->setBlurRadius(7);
        effect_->setOffset(QPointF(0, 2));
        effect_->setColor(QColor(0, 0, 0, 75));

        setBackgroundMode(Qt::OpaqueMode);
        setMinimumHeight(42);
        setGraphicsEffect(effect_);
        setBaseOpacity(0.3);

        shadow_state_machine_->addState(normal_state_);
        shadow_state_machine_->addState(pressed_state_);

        normal_state_->assignProperty(effect_, "offset", QPointF(0, 2));
        normal_state_->assignProperty(effect_, "blurRadius", 7);

        pressed_state_->assignProperty(effect_, "offset", QPointF(0, 5));
        pressed_state_->assignProperty(effect_, "blurRadius", 29);

        QAbstractTransition *transition;

        transition = new QEventTransition(this, QEvent::MouseButtonPress);
        transition->setTargetState(pressed_state_);
        normal_state_->addTransition(transition);

        transition = new QEventTransition(this, QEvent::MouseButtonDblClick);
        transition->setTargetState(pressed_state_);
        normal_state_->addTransition(transition);

        transition = new QEventTransition(this, QEvent::MouseButtonRelease);
        transition->setTargetState(normal_state_);
        pressed_state_->addTransition(transition);

        QPropertyAnimation *animation;

        animation = new QPropertyAnimation(effect_, "offset", this);
        animation->setDuration(100);
        shadow_state_machine_->addDefaultAnimation(animation);

        animation = new QPropertyAnimation(effect_, "blurRadius", this);
        animation->setDuration(100);
        shadow_state_machine_->addDefaultAnimation(animation);

        shadow_state_machine_->setInitialState(normal_state_);
        shadow_state_machine_->start();
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

RaisedButton::~RaisedButton()
{
}

bool
RaisedButton::event(QEvent *event)
{
        if (QEvent::EnabledChange == event->type()) {
                if (isEnabled()) {
                        shadow_state_machine_->start();
                        effect_->setEnabled(true);
                } else {
                        shadow_state_machine_->stop();
                        effect_->setEnabled(false);
                }
        }

        return FlatButton::event(event);
}
