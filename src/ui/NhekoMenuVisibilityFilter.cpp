// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoMenuVisibilityFilter.h"

#include <QQmlListReference>
#include <QQuickItem>
#include <QTimer>

#include "Logging.h"

QQmlListProperty<QQmlComponent>
NhekoMenuVisibilityFilter::items()
{
    return QQmlListProperty<QQmlComponent>(this,
                                           this,
                                           &NhekoMenuVisibilityFilter::appendItem,
                                           &NhekoMenuVisibilityFilter::itemCount,
                                           &NhekoMenuVisibilityFilter::getItem,
                                           &NhekoMenuVisibilityFilter::clearItems,
                                           &NhekoMenuVisibilityFilter::replaceItem,
                                           &NhekoMenuVisibilityFilter::removeLast);
}

void
NhekoMenuVisibilityFilter::appendItem(QQmlListProperty<QQmlComponent> *p, QQmlComponent *c)
{
    NhekoMenuVisibilityFilter *dc = static_cast<NhekoMenuVisibilityFilter *>(p->object);
    dc->items_.append(c);
    // dc->updateTarget();

    // QQmlProperty prop(c, "visible");
    // prop.connectNotifySignal(dc, SLOT(updateTarget()));
}
qsizetype
NhekoMenuVisibilityFilter::itemCount(QQmlListProperty<QQmlComponent> *p)
{
    return static_cast<NhekoMenuVisibilityFilter *>(p->object)->items_.count();
}
QQmlComponent *
NhekoMenuVisibilityFilter::getItem(QQmlListProperty<QQmlComponent> *p, qsizetype index)
{
    return static_cast<NhekoMenuVisibilityFilter *>(p->object)->items_.at(index);
}
void
NhekoMenuVisibilityFilter::clearItems(QQmlListProperty<QQmlComponent> *p)
{
    static_cast<NhekoMenuVisibilityFilter *>(p->object)->items_.clear();
    // static_cast<NhekoMenuVisibilityFilter *>(p->object)->updateTarget();
}
void
NhekoMenuVisibilityFilter::replaceItem(QQmlListProperty<QQmlComponent> *p,
                                       qsizetype index,
                                       QQmlComponent *c)
{
    static_cast<NhekoMenuVisibilityFilter *>(p->object)->items_.replace(index, c);
    // static_cast<NhekoMenuVisibilityFilter *>(p->object)->updateTarget();
}
void
NhekoMenuVisibilityFilter::removeLast(QQmlListProperty<QQmlComponent> *p)
{
    static_cast<NhekoMenuVisibilityFilter *>(p->object)->items_.pop_back();
    // static_cast<NhekoMenuVisibilityFilter *>(p->object)->updateTarget();
}

void
NhekoMenuVisibilityFilter::setTarget(const QQmlProperty &prop)
{
    if (prop.propertyTypeCategory() != QQmlProperty::List) {
        nhlog::ui()->warn("Target prop of NhekoMenuVisibilityFilter set to non list property");
        return;
    }

    targetProperty = prop;
    // updateTarget();
}

void
NhekoMenuVisibilityFilter::updateTarget()
{
    if (!targetProperty.isValid())
        return;

    auto newItems = qvariant_cast<QQmlListReference>(targetProperty.read());
    // newItems.clear(); <- does not remove the visual items

    for (qsizetype i = newItems.size(); i > 0; i--) {
        // only remove items, not other random stuff in there!
        if (auto item = qobject_cast<QQuickItem *>(newItems.at(i - 1))) {
            // emit removeItem(item); <- easier to "automagic" this by using invokeMethod
            QMetaObject::invokeMethod(
              targetProperty.object(), "removeItem", Qt::DirectConnection, item);
        }
    }

    for (const auto &item : std::as_const(items_)) {
        auto newItem = item->create(QQmlEngine::contextForObject(this));

        if (auto prop = newItem->property("visible"); !prop.isValid() || prop.toBool()) {
            // targetProperty.write(QVariant::fromValue(newItem)); <- appends but breaks removal
            newItems.append(newItem);
            // You might think this should be JS Ownership, but no! The menu deletes stuff
            // explicitly!
            // Inb4 this causes a leak...
            //
            // QQmlEngine::setObjectOwnership(newItem, QQmlEngine::JavaScriptOwnership);
            //  emit addItem(newItem); <- would work, but manual code, ew
        } else {
            newItem->deleteLater();
        }
    }

    if (delegate) {
        for (const auto &modelData : std::as_const(model)) {
            QVariantMap initial;
            initial["modelData"] = modelData;

            auto newItem =
              delegate->createWithInitialProperties(initial, QQmlEngine::contextForObject(this));

            //  targetProperty.write(QVariant::fromValue(newItem)); <- appends but breaks
            //  removal
            newItems.append(newItem);
            // You might think this should be JS Ownership, but no! The menu deletes stuff
            // explicitly!
            // Inb4 this causes a leak...
            //
            // QQmlEngine::setObjectOwnership(newItem, QQmlEngine::JavaScriptOwnership);
            //  emit addItem(newItem); <- would work, but manual code, ew
        }
    }

    QTimer::singleShot(0, this, [this] {
        auto createdItems = qvariant_cast<QQmlListReference>(targetProperty.read());
        // newItems.clear(); <- does not remove the visual items

        for (qsizetype i = createdItems.size(); i > 0; i--) {
            // only remove items, not other random stuff in there!
            if (auto item = qobject_cast<QQuickItem *>(createdItems.at(i - 1))) {
                item->enabledChanged();
            }
        }
    });

    // targetProperty.write(QVariant::fromValue(std::move(newItems)));
}

#include "moc_NhekoMenuVisibilityFilter.cpp"
