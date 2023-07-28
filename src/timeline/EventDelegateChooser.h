// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

// A DelegateChooser like the one, that was added to Qt5.12 (in labs), but compatible with older Qt
// versions see KDE/kquickitemviews see qtdeclarative/qqmldelagatecomponent

#pragma once

#include <QAbstractItemModel>
#include <QQmlComponent>
#include <QQmlIncubator>
#include <QQmlListProperty>
#include <QQuickItem>
#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "TimelineModel.h"

class EventDelegateChoice : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "delegate")

public:
    Q_PROPERTY(QList<int> roleValues READ roleValues WRITE setRoleValues NOTIFY roleValuesChanged
                 REQUIRED FINAL)
    Q_PROPERTY(
      QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged REQUIRED FINAL)

    [[nodiscard]] QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    [[nodiscard]] QList<int> roleValues() const;
    void setRoleValues(const QList<int> &value);

signals:
    void delegateChanged();
    void roleValuesChanged();
    void changed();

private:
    QList<int> roleValues_;
    QQmlComponent *delegate_ = nullptr;
};

class EventDelegateChooser : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "choices")

public:
    Q_PROPERTY(QQmlListProperty<EventDelegateChoice> choices READ choices CONSTANT FINAL)
    Q_PROPERTY(QQuickItem *main READ main NOTIFY mainChanged FINAL)
    Q_PROPERTY(QQuickItem *reply READ reply NOTIFY replyChanged FINAL)
    Q_PROPERTY(QString eventId READ eventId WRITE setEventId NOTIFY eventIdChanged REQUIRED FINAL)
    Q_PROPERTY(QString replyTo READ replyTo WRITE setReplyTo NOTIFY replyToChanged REQUIRED FINAL)
    Q_PROPERTY(TimelineModel *room READ room WRITE setRoom NOTIFY roomChanged REQUIRED FINAL)

    QQmlListProperty<EventDelegateChoice> choices();

    [[nodiscard]] QQuickItem *main() const
    {
        return qobject_cast<QQuickItem *>(eventIncubator.object());
    }
    [[nodiscard]] QQuickItem *reply() const
    {
        return qobject_cast<QQuickItem *>(replyIncubator.object());
    }

    void setRoom(TimelineModel *m)
    {
        if (m != room_) {
            room_ = m;
            emit roomChanged();

            if (isComponentComplete()) {
                eventIncubator.reset(eventId_);
                replyIncubator.reset(replyId);
            }
        }
    }
    [[nodiscard]] TimelineModel *room() { return room_; }

    void setEventId(QString idx)
    {
        eventId_ = idx;
        emit eventIdChanged();

        if (isComponentComplete())
            eventIncubator.reset(eventId_);
    }
    [[nodiscard]] QString eventId() const { return eventId_; }
    void setReplyTo(QString id)
    {
        replyId = id;
        emit replyToChanged();

        if (isComponentComplete())
            replyIncubator.reset(replyId);
    }
    [[nodiscard]] QString replyTo() const { return replyId; }

    void componentComplete() override;

signals:
    void mainChanged();
    void replyChanged();
    void roomChanged();
    void eventIdChanged();
    void replyToChanged();

private:
    struct DelegateIncubator final : public QQmlIncubator
    {
        DelegateIncubator(EventDelegateChooser &parent, bool forReply)
          : QQmlIncubator(QQmlIncubator::AsynchronousIfNested)
          , chooser(parent)
          , forReply(forReply)
        {
        }
        void setInitialState(QObject *object) override;
        void statusChanged(QQmlIncubator::Status status) override;

        void reset(QString id);

        EventDelegateChooser &chooser;
        bool forReply;
        QString currentId;

        QString instantiatedId;
        int instantiatedRole = -1;
        QAbstractItemModel *instantiatedModel = nullptr;
    };

    QVariant roleValue_;
    QList<EventDelegateChoice *> choices_;
    DelegateIncubator eventIncubator{*this, false};
    DelegateIncubator replyIncubator{*this, true};
    TimelineModel *room_{nullptr};
    QString eventId_;
    QString replyId;

    static void appendChoice(QQmlListProperty<EventDelegateChoice> *, EventDelegateChoice *);
    static qsizetype choiceCount(QQmlListProperty<EventDelegateChoice> *);
    static EventDelegateChoice *choice(QQmlListProperty<EventDelegateChoice> *, qsizetype index);
    static void clearChoices(QQmlListProperty<EventDelegateChoice> *);
};
