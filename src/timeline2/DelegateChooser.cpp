#include "DelegateChooser.h"

#include "Logging.h"

// uses private API, which moved between versions
#include <QQmlEngine>
#include <QtGlobal>

QQmlComponent *
DelegateChoice::delegate() const
{
	return delegate_;
}

void
DelegateChoice::setDelegate(QQmlComponent *delegate)
{
        if (delegate != delegate_) {
                delegate_ = delegate;
                emit delegateChanged();
                emit changed();
        }
}

QVariant
DelegateChoice::roleValue() const
{
        return roleValue_;
}

void
DelegateChoice::setRoleValue(const QVariant &value)
{
        if (value != roleValue_) {
                roleValue_ = value;
                emit roleValueChanged();
                emit changed();
        }
}

QVariant
DelegateChooser::roleValue() const
{
        return roleValue_;
}

void
DelegateChooser::setRoleValue(const QVariant &value)
{
        if (value != roleValue_) {
                roleValue_ = value;
                recalcChild();
                emit roleValueChanged();
        }
}

QQmlListProperty<DelegateChoice>
DelegateChooser::choices()
{
        return QQmlListProperty<DelegateChoice>(this,
                                                this,
                                                &DelegateChooser::appendChoice,
                                                &DelegateChooser::choiceCount,
                                                &DelegateChooser::choice,
                                                &DelegateChooser::clearChoices);
}

void
DelegateChooser::appendChoice(QQmlListProperty<DelegateChoice> *p, DelegateChoice *c)
{
        DelegateChooser *dc = static_cast<DelegateChooser *>(p->object);
        dc->choices_.append(c);
}

int
DelegateChooser::choiceCount(QQmlListProperty<DelegateChoice> *p)
{
        return static_cast<DelegateChooser *>(p->object)->choices_.count();
}
DelegateChoice *
DelegateChooser::choice(QQmlListProperty<DelegateChoice> *p, int index)
{
        return static_cast<DelegateChooser *>(p->object)->choices_.at(index);
}
void
DelegateChooser::clearChoices(QQmlListProperty<DelegateChoice> *p)
{
        static_cast<DelegateChooser *>(p->object)->choices_.clear();
}

void
DelegateChooser::recalcChild()
{
        for (const auto choice : choices_) {
                auto choiceValue = choice->roleValue();
                if (!roleValue_.isValid() || !choiceValue.isValid() || choiceValue == roleValue_) {
                        if (child) {
                                // delete child;
                                child = nullptr;
                        }

                        child = dynamic_cast<QQuickItem *>(
                          choice->delegate()->create(QQmlEngine::contextForObject(this)));
                        child->setParentItem(this);
                        connect(this->child, &QQuickItem::heightChanged, this, [this]() {
                                this->setHeight(this->child->height());
                        });
                        this->setHeight(this->child->height());
                        return;
                }
        }
}

void
DelegateChooser::componentComplete()
{
        QQuickItem::componentComplete();
        recalcChild();
}

