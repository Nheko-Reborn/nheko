// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EventDelegateChooser.h"
#include "TimelineModel.h"

#include "Logging.h"

#include <QQmlEngine>
#include <QtGlobal>

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

    auto roleNames = chooser.room_->roleNames();
    QHash<QByteArray, int> nameToRole;
    for (const auto &[k, v] : roleNames.asKeyValueRange()) {
        nameToRole.insert(v, k);
    }

    QHash<int, int> roleToPropIdx;
    std::vector<QModelRoleData> roles;
    bool isReplyNeeded = false;

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

        if (prop.name() == std::string_view("isReply")) {
            isReplyNeeded = true;
            roleToPropIdx.insert(-1, i);
        } else if (auto role = nameToRole.find(prop.name()); role != nameToRole.end()) {
            roleToPropIdx.insert(*role, i);
            roles.emplace_back(*role);

            // nhlog::ui()->critical("Found prop {}, idx {}, role {}", prop.name(), i, *role);
        } else {
            nhlog::ui()->critical("Required property {} not found in model!", prop.name());
        }
    }

    nhlog::ui()->debug("Querying data for id {}", currentId.toStdString());
    chooser.room_->multiData(currentId, forReply ? chooser.eventId_ : QString(), roles);

    Qt::beginPropertyUpdateGroup();
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

    if (isReplyNeeded) {
        const auto roleName = QByteArray("isReply");
        // nhlog::ui()->critical("Setting role {} to {}", roleName.toStdString(), forReply);

        // nhlog::ui()->critical("Setting {}", mo->property(roleToPropIdx[-1]).name());
        mo->property(roleToPropIdx[-1]).write(obj, forReply);

        if (const auto &req = requiredProperties.find(roleName); req != requiredProperties.end())
            QQmlIncubatorPrivate::get(this)->requiredProperties()->remove(*req);
    }
    Qt::endPropertyUpdateGroup();

    // setInitialProperties(rolesToSet);

    auto update =
      [this, obj, roleToPropIdx = std::move(roleToPropIdx)](const QList<int> &changedRoles) {
          std::vector<QModelRoleData> rolesToRequest;

          if (changedRoles.empty()) {
              for (auto role : roleToPropIdx.keys())
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
          chooser.room_->multiData(
            currentId, forReply ? chooser.eventId_ : QString(), rolesToRequest);

          Qt::beginPropertyUpdateGroup();
          for (const auto &role : rolesToRequest) {
              mo->property(roleToPropIdx[role.role()]).write(obj, role.data());
          }
          Qt::endPropertyUpdateGroup();
      };

    if (!forReply) {
        auto row = chooser.room_->idToIndex(currentId);
        auto connection = connect(
          chooser.room_,
          &QAbstractItemModel::dataChanged,
          obj,
          [row, update](const QModelIndex &topLeft,
                        const QModelIndex &bottomRight,
                        const QList<int> &changedRoles) {
              if (row < topLeft.row() || row > bottomRight.row())
                  return;

              update(changedRoles);
          },
          Qt::QueuedConnection);
        connect(&this->chooser, &EventDelegateChooser::destroyed, obj, [connection]() {
            QObject::disconnect(connection);
        });
    }
}

void
EventDelegateChooser::DelegateIncubator::reset(QString id)
{
    if (!chooser.room_ || id.isEmpty())
        return;

    nhlog::ui()->debug("Reset with id {}, reply {}", id.toStdString(), forReply);

    this->currentId = id;

    auto role =
      chooser.room_
        ->dataById(id, TimelineModel::Roles::Type, forReply ? chooser.eventId_ : QString())
        .toInt();

    for (const auto choice : qAsConst(chooser.choices_)) {
        const auto &choiceValue = choice->roleValues();
        if (choiceValue.contains(role) || choiceValue.empty()) {
            nhlog::ui()->debug(
              "Instantiating type: {}, c {}", (int)role, choiceValue.contains(role));

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
        QQmlEngine::setObjectOwnership(child, QQmlEngine::ObjectOwnership::JavaScriptOwnership);

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
        for (const auto &e : qAsConst(errors_))
            nhlog::ui()->error("Error instantiating delegate: {}", e.toString().toStdString());
    }
}

void
EventDelegateChooser::updatePolish()
{
    auto mainChild = qobject_cast<QQuickItem *>(eventIncubator.object());
    auto replyChild = qobject_cast<QQuickItem *>(replyIncubator.object());

    nhlog::ui()->critical("POLISHING {}", (void *)this);

    if (mainChild) {
        auto attached = qobject_cast<EventDelegateChooserAttachedType *>(
          qmlAttachedPropertiesObject<EventDelegateChooser>(mainChild));
        Q_ASSERT(attached != nullptr);

        // in theory we could also reset the width, but that doesn't seem to work nicely for text
        // areas because of how they cache it.
        mainChild->setWidth(maxWidth_);
        mainChild->ensurePolished();
        auto width = mainChild->implicitWidth();

        if (width > maxWidth_ || attached->fillWidth())
            width = maxWidth_;

        nhlog::ui()->debug(
          "Made event delegate width: {}, {}", width, mainChild->metaObject()->className());
        mainChild->setWidth(width);
        mainChild->ensurePolished();
    }

    if (replyChild) {
        auto attached = qobject_cast<EventDelegateChooserAttachedType *>(
          qmlAttachedPropertiesObject<EventDelegateChooser>(replyChild));
        Q_ASSERT(attached != nullptr);

        // in theory we could also reset the width, but that doesn't seem to work nicely for text
        // areas because of how they cache it.
        replyChild->setWidth(maxWidth_);
        replyChild->ensurePolished();
        auto width = replyChild->implicitWidth();

        if (width > maxWidth_ || attached->fillWidth())
            width = maxWidth_;

        replyChild->setWidth(width);
        replyChild->ensurePolished();
    }
}
