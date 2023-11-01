// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EventDelegateChooser.h"
#include "TimelineModel.h"

#include "Logging.h"

#include <QQmlEngine>
#include <QtGlobal>

#include <ranges>

// privat qt headers to access required properties
#include <QtQml/private/qqmlincubator_p.h>
#include <QtQml/private/qqmlobjectcreator_p.h>

QQmlComponent *
EventDelegateChoice::delegate() const
{
    return delegate_;
}

void
EventDelegateChoice::setDelegate(QQmlComponent *delegate)
{
    if (delegate != delegate_) {
        delegate_ = delegate;
        emit delegateChanged();
        emit changed();
    }
}

QList<int>
EventDelegateChoice::roleValues() const
{
    return roleValues_;
}

void
EventDelegateChoice::setRoleValues(const QList<int> &value)
{
    if (value != roleValues_) {
        roleValues_ = value;
        emit roleValuesChanged();
        emit changed();
    }
}

QQmlListProperty<EventDelegateChoice>
EventDelegateChooser::choices()
{
    return QQmlListProperty<EventDelegateChoice>(this,
                                                 this,
                                                 &EventDelegateChooser::appendChoice,
                                                 &EventDelegateChooser::choiceCount,
                                                 &EventDelegateChooser::choice,
                                                 &EventDelegateChooser::clearChoices);
}

void
EventDelegateChooser::appendChoice(QQmlListProperty<EventDelegateChoice> *p, EventDelegateChoice *c)
{
    EventDelegateChooser *dc = static_cast<EventDelegateChooser *>(p->object);
    dc->choices_.append(c);
}

qsizetype
EventDelegateChooser::choiceCount(QQmlListProperty<EventDelegateChoice> *p)
{
    return static_cast<EventDelegateChooser *>(p->object)->choices_.count();
}
EventDelegateChoice *
EventDelegateChooser::choice(QQmlListProperty<EventDelegateChoice> *p, qsizetype index)
{
    return static_cast<EventDelegateChooser *>(p->object)->choices_.at(index);
}
void
EventDelegateChooser::clearChoices(QQmlListProperty<EventDelegateChoice> *p)
{
    static_cast<EventDelegateChooser *>(p->object)->choices_.clear();
}

void
EventDelegateChooser::componentComplete()
{
    QQuickItem::componentComplete();
    eventIncubator.reset(eventId_);
    replyIncubator.reset(replyId);
    // eventIncubator.forceCompletion();
}

void
EventDelegateChooser::DelegateIncubator::setInitialState(QObject *obj)
{
    auto item = qobject_cast<QQuickItem *>(obj);
    if (!item)
        return;

    item->setParentItem(&chooser);
    item->setParent(&chooser);

    auto roleNames = chooser.room_->roleNames();
    QHash<QByteArray, int> nameToRole;
    for (const auto &[k, v] : roleNames.asKeyValueRange()) {
        nameToRole.insert(v, k);
    }

    QHash<int, int> roleToPropIdx;
    std::vector<QModelRoleData> roles;
    // Workaround for https://bugreports.qt.io/browse/QTBUG-98846
    QHash<QString, RequiredPropertyKey> requiredProperties;
    for (const auto &[propKey, prop] :
         QQmlIncubatorPrivate::get(this)->requiredProperties()->asKeyValueRange()) {
        requiredProperties.insert(prop.propertyName, propKey);
    }

    // collect required properties
    auto mo = obj->metaObject();
    for (int i = 0; i < mo->propertyCount(); i++) {
        auto prop = mo->property(i);
        // nhlog::ui()->critical("Found prop {}", prop.name());
        //  See https://bugreports.qt.io/browse/QTBUG-98846
        if (!prop.isRequired() && !requiredProperties.contains(prop.name()))
            continue;

        if (auto role = nameToRole.find(prop.name()); role != nameToRole.end()) {
            roleToPropIdx.insert(*role, i);
            roles.emplace_back(*role);

            // nhlog::ui()->critical("Found prop {}, idx {}, role {}", prop.name(), i, *role);
        } else {
            nhlog::ui()->critical("Required property {} not found in model!", prop.name());
        }
    }

    // nhlog::ui()->debug("Querying data for id {}", currentId.toStdString());
    chooser.room_->multiData(currentId, forReply ? chooser.eventId_ : QString(), roles);

    Qt::beginPropertyUpdateGroup();
    auto attached = qobject_cast<EventDelegateChooserAttachedType *>(
      qmlAttachedPropertiesObject<EventDelegateChooser>(obj));
    Q_ASSERT(attached != nullptr);
    attached->setIsReply(this->forReply || chooser.limitAsReply_);

    for (const auto &role : roles) {
        const auto &roleName = roleNames[role.role()];
        // nhlog::ui()->critical("Setting role {}, {} to {}",
        //                       role.role(),
        //                       roleName.toStdString(),
        //                       role.data().toString().toStdString());

        // nhlog::ui()->critical("Setting {}", mo->property(roleToPropIdx[role.role()]).name());
        mo->property(roleToPropIdx[role.role()]).write(obj, role.data());

        if (const auto &req = requiredProperties.find(roleName); req != requiredProperties.end())
            QQmlIncubatorPrivate::get(this)->requiredProperties()->remove(*req);
    }

    Qt::endPropertyUpdateGroup();

    // setInitialProperties(rolesToSet);

    auto update = [this, obj, roleToPropIdx = std::move(roleToPropIdx)](
                    const QList<int> &changedRoles, TimelineModel *room) {
        if (!room)
            return;

        if (changedRoles.empty() || changedRoles.contains(TimelineModel::Roles::Type)) {
            int type = room
                         ->dataById(currentId,
                                    TimelineModel::Roles::Type,
                                    forReply ? chooser.eventId_ : QString())
                         .toInt();
            if (type != oldType) {
                // nhlog::ui()->debug("Type changed!");
                reset(currentId);
                return;
            }
        }

        std::vector<QModelRoleData> rolesToRequest;

        if (changedRoles.empty()) {
            for (const auto role :
                 std::ranges::subrange(roleToPropIdx.keyBegin(), roleToPropIdx.keyEnd()))
                rolesToRequest.emplace_back(role);
        } else {
            for (auto role : changedRoles) {
                if (roleToPropIdx.contains(role)) {
                    rolesToRequest.emplace_back(role);
                }
            }
        }

        if (rolesToRequest.empty())
            return;

        auto mo = obj->metaObject();
        room->multiData(currentId, forReply ? chooser.eventId_ : QString(), rolesToRequest);

        Qt::beginPropertyUpdateGroup();
        for (const auto &role : rolesToRequest) {
            mo->property(roleToPropIdx[role.role()]).write(obj, role.data());
        }
        Qt::endPropertyUpdateGroup();
    };

    if (!forReply) {
        auto row        = chooser.room_->idToIndex(currentId);
        auto connection = connect(
          chooser.room_,
          &QAbstractItemModel::dataChanged,
          obj,
          [row, update, room = chooser.room_](const QModelIndex &topLeft,
                                              const QModelIndex &bottomRight,
                                              const QList<int> &changedRoles) {
              if (row < topLeft.row() || row > bottomRight.row())
                  return;

              update(changedRoles, room);
          },
          Qt::QueuedConnection);
        connect(
          &this->chooser,
          &EventDelegateChooser::destroyed,
          obj,
          [connection]() { QObject::disconnect(connection); },
          Qt::SingleShotConnection);
        connect(
          &this->chooser,
          &EventDelegateChooser::roomChanged,
          obj,
          [connection]() { QObject::disconnect(connection); },
          Qt::SingleShotConnection);
    }
}

void
EventDelegateChooser::DelegateIncubator::reset(QString id)
{
    if (!chooser.room_ || id.isEmpty())
        return;

    // nhlog::ui()->debug("Reset with id {}, reply {}", id.toStdString(), forReply);

    this->currentId = id;

    auto role =
      chooser.room_
        ->dataById(id, TimelineModel::Roles::Type, forReply ? chooser.eventId_ : QString())
        .toInt();
    this->oldType = role;

    for (const auto choice : std::as_const(chooser.choices_)) {
        const auto &choiceValue = choice->roleValues();
        if (choiceValue.contains(role) || choiceValue.empty()) {
            // nhlog::ui()->debug(
            //   "Instantiating type: {}, c {}", (int)role, choiceValue.contains(role));

            if (auto child = qobject_cast<QQuickItem *>(object())) {
                child->setParentItem(nullptr);
            }

            choice->delegate()->create(*this, QQmlEngine::contextForObject(&chooser));
            return;
        }
    }
}

void
EventDelegateChooser::DelegateIncubator::statusChanged(QQmlIncubator::Status status)
{
    if (status == QQmlIncubator::Ready) {
        auto child = qobject_cast<QQuickItem *>(object());
        if (child == nullptr) {
            nhlog::ui()->error("Delegate has to be derived of Item!");
            return;
        }

        child->setParentItem(&chooser);
        QQmlEngine::setObjectOwnership(child, QQmlEngine::ObjectOwnership::CppOwnership);

        // connect(child, &QQuickItem::parentChanged, child, [child](QQuickItem *) {
        //     // QTBUG-115687
        //     if (child->flags().testFlag(QQuickItem::ItemObservesViewport)) {
        //         nhlog::ui()->critical("SETTING OBSERVES VIEWPORT");
        //         // Re-trigger the parent traversal to get subtreeTransformChangedEnabled turned
        //         on child->setFlag(QQuickItem::ItemObservesViewport);
        //     }
        // });

        if (forReply)
            emit chooser.replyChanged();
        else
            emit chooser.mainChanged();

        chooser.polish();
    } else if (status == QQmlIncubator::Error) {
        auto errors_ = errors();
        for (const auto &e : std::as_const(errors_))
            nhlog::ui()->error("Error instantiating delegate: {}", e.toString().toStdString());
    }
}

void
EventDelegateChooser::updatePolish()
{
    auto mainChild  = qobject_cast<QQuickItem *>(eventIncubator.object());
    auto replyChild = qobject_cast<QQuickItem *>(replyIncubator.object());

    // nhlog::ui()->trace("POLISHING {}", (void *)this);

    auto layoutItem = [this](QQuickItem *item, int inset) {
        if (item) {
            QObject::disconnect(item, &QQuickItem::implicitWidthChanged, this, &QQuickItem::polish);

            auto attached = qobject_cast<EventDelegateChooserAttachedType *>(
              qmlAttachedPropertiesObject<EventDelegateChooser>(item));
            Q_ASSERT(attached != nullptr);

            int maxWidth = maxWidth_ - inset;

            // in theory we could also reset the width, but that doesn't seem to work nicely for
            // text areas because of how they cache it.
            if (attached->maxWidth() > 0)
                item->setWidth(attached->maxWidth());
            else
                item->setWidth(maxWidth);
            item->ensurePolished();
            auto width = item->implicitWidth();

            if (width < 1 || width > maxWidth)
                width = maxWidth;

            if (attached->maxWidth() > 0 && width > attached->maxWidth())
                width = attached->maxWidth();

            if (attached->keepAspectRatio()) {
                auto height = width * attached->aspectRatio();
                if (attached->maxHeight() && height > attached->maxHeight()) {
                    height = attached->maxHeight();
                    width  = height / attached->aspectRatio();
                }

                item->setHeight(height);
            }

            item->setWidth(width);
            item->ensurePolished();

            QObject::connect(item, &QQuickItem::implicitWidthChanged, this, &QQuickItem::polish);
        }
    };

    layoutItem(mainChild, mainInset_);
    layoutItem(replyChild, replyInset_);
}

void
EventDelegateChooserAttachedType::polishChooser()
{
    auto p = parent();
    if (p) {
        auto chooser = qobject_cast<EventDelegateChooser *>(p->parent());
        if (chooser) {
            chooser->polish();
        }
    }
}
