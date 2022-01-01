// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
        if (isComponentComplete())
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
    for (const auto choice : qAsConst(choices_)) {
        const auto &choiceValue = choice->roleValueRef();
        if (choiceValue == roleValue_ || (!choiceValue.isValid() && !roleValue_.isValid())) {
            if (child_) {
                child_->setParentItem(nullptr);
                child_ = nullptr;
            }

            choice->delegate()->create(incubator, QQmlEngine::contextForObject(this));
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

void
DelegateChooser::DelegateIncubator::statusChanged(QQmlIncubator::Status status)
{
    if (status == QQmlIncubator::Ready) {
        chooser.child_ = qobject_cast<QQuickItem *>(object());
        if (chooser.child_ == nullptr) {
            nhlog::ui()->error("Delegate has to be derived of Item!");
            return;
        }

        chooser.child_->setParentItem(&chooser);
        QQmlEngine::setObjectOwnership(chooser.child_,
                                       QQmlEngine::ObjectOwnership::JavaScriptOwnership);
        emit chooser.childChanged();

    } else if (status == QQmlIncubator::Error) {
        auto errors_ = errors();
        for (const auto &e : qAsConst(errors_))
            nhlog::ui()->error("Error instantiating delegate: {}", e.toString().toStdString());
    }
}
