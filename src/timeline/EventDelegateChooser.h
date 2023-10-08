// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QQmlComponent>
#include <QQmlIncubator>
#include <QQmlListProperty>
#include <QQuickItem>
#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "TimelineModel.h"

class EventDelegateChooserAttachedType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth NOTIFY fillWidthChanged)
    QML_ANONYMOUS
public:
    EventDelegateChooserAttachedType(QObject *parent)
      : QObject(parent)
    {
    }
    bool fillWidth() const { return fillWidth_; }
    void setFillWidth(bool fill)
    {
        fillWidth_ = fill;
        emit fillWidthChanged();
    }
signals:
    void fillWidthChanged();

private:
    bool fillWidth_ = false, keepAspectRatio = false;
    double aspectRatio = 1.;
    int maxWidth       = -1;
    int maxHeight      = -1;
};

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

    QML_ATTACHED(EventDelegateChooserAttachedType)

    Q_PROPERTY(QQmlListProperty<EventDelegateChoice> choices READ choices CONSTANT FINAL)
    Q_PROPERTY(QQuickItem *main READ main NOTIFY mainChanged FINAL)
    Q_PROPERTY(QQuickItem *reply READ reply NOTIFY replyChanged FINAL)
    Q_PROPERTY(QString eventId READ eventId WRITE setEventId NOTIFY eventIdChanged REQUIRED FINAL)
    Q_PROPERTY(QString replyTo READ replyTo WRITE setReplyTo NOTIFY replyToChanged REQUIRED FINAL)
    Q_PROPERTY(TimelineModel *room READ room WRITE setRoom NOTIFY roomChanged REQUIRED FINAL)
    Q_PROPERTY(bool sameWidth READ sameWidth WRITE setSameWidth NOTIFY sameWidthChanged)
    Q_PROPERTY(int maxWidth READ maxWidth WRITE setMaxWidth NOTIFY maxWidthChanged)

public:
    QQmlListProperty<EventDelegateChoice> choices();

    [[nodiscard]] QQuickItem *main() const
    {
        return qobject_cast<QQuickItem *>(eventIncubator.object());
    }
    [[nodiscard]] QQuickItem *reply() const
    {
        return qobject_cast<QQuickItem *>(replyIncubator.object());
    }

    bool sameWidth() const { return sameWidth_; }
    void setSameWidth(bool width)
    {
        sameWidth_ = width;
        emit sameWidthChanged();
    }
    bool maxWidth() const { return maxWidth_; }
    void setMaxWidth(int width)
    {
        maxWidth_ = width;
        emit maxWidthChanged();
        polish();
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

    static EventDelegateChooserAttachedType *qmlAttachedProperties(QObject *object)
    {
        return new EventDelegateChooserAttachedType(object);
    }

    void updatePolish() override;

signals:
    void mainChanged();
    void replyChanged();
    void roomChanged();
    void eventIdChanged();
    void replyToChanged();
    void sameWidthChanged();
    void maxWidthChanged();

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
    bool sameWidth_ = false;
    int maxWidth_   = 400;

    static void appendChoice(QQmlListProperty<EventDelegateChoice> *, EventDelegateChoice *);
    static qsizetype choiceCount(QQmlListProperty<EventDelegateChoice> *);
    static EventDelegateChoice *choice(QQmlListProperty<EventDelegateChoice> *, qsizetype index);
    static void clearChoices(QQmlListProperty<EventDelegateChoice> *);
};
