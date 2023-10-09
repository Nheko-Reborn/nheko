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
    Q_PROPERTY(bool keepAspectRatio READ keepAspectRatio WRITE setKeepAspectRatio NOTIFY
                 keepAspectRatioChanged)
    Q_PROPERTY(double aspectRatio READ aspectRatio WRITE setAspectRatio NOTIFY aspectRatioChanged)
    Q_PROPERTY(int maxWidth READ maxWidth WRITE setMaxWidth NOTIFY maxWidthChanged)
    Q_PROPERTY(int maxHeight READ maxHeight WRITE setMaxHeight NOTIFY maxHeightChanged)
    Q_PROPERTY(bool isReply READ isReply WRITE setIsReply NOTIFY isReplyChanged)

    QML_ANONYMOUS
public:
    EventDelegateChooserAttachedType(QObject *parent)
      : QObject(parent)
    {
    }

    bool keepAspectRatio() const { return keepAspectRatio_; }
    void setKeepAspectRatio(bool fill)
    {
        if (fill != keepAspectRatio_) {
            keepAspectRatio_ = fill;
            emit keepAspectRatioChanged();
            polishChooser();
        }
    }

    double aspectRatio() const { return aspectRatio_; }
    void setAspectRatio(double fill)
    {
        aspectRatio_ = fill;
        emit aspectRatioChanged();
        polishChooser();
    }

    int maxWidth() const { return maxWidth_; }
    void setMaxWidth(int fill)
    {
        maxWidth_ = fill;
        emit maxWidthChanged();
        polishChooser();
    }

    int maxHeight() const { return maxHeight_; }
    void setMaxHeight(int fill)
    {
        maxHeight_ = fill;
        emit maxHeightChanged();
    }

    bool isReply() const { return isReply_; }
    void setIsReply(bool fill)
    {
        if (fill != isReply_) {
            isReply_ = fill;
            emit isReplyChanged();
            polishChooser();
        }
    }

signals:
    void keepAspectRatioChanged();
    void aspectRatioChanged();
    void maxWidthChanged();
    void maxHeightChanged();
    void isReplyChanged();

private:
    void polishChooser();

    double aspectRatio_   = 1.;
    int maxWidth_         = -1;
    int maxHeight_        = -1;
    bool keepAspectRatio_ = false;
    bool isReply_         = false;
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
    Q_PROPERTY(int replyInset READ replyInset WRITE setReplyInset NOTIFY replyInsetChanged)
    Q_PROPERTY(int mainInset READ mainInset WRITE setMainInset NOTIFY mainInsetChanged)

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
    int maxWidth() const { return maxWidth_; }
    void setMaxWidth(int width)
    {
        maxWidth_ = width;
        emit maxWidthChanged();
        polish();
    }

    int replyInset() const { return replyInset_; }
    void setReplyInset(int width)
    {
        replyInset_ = width;
        emit replyInsetChanged();
        polish();
    }

    int mainInset() const { return mainInset_; }
    void setMainInset(int width)
    {
        mainInset_ = width;
        emit mainInsetChanged();
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
    void replyInsetChanged();
    void mainInsetChanged();

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
        int instantiatedRole                  = -1;
        QAbstractItemModel *instantiatedModel = nullptr;
        int oldType                           = -1;
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
    int replyInset_ = 0;
    int mainInset_  = 0;

    static void appendChoice(QQmlListProperty<EventDelegateChoice> *, EventDelegateChoice *);
    static qsizetype choiceCount(QQmlListProperty<EventDelegateChoice> *);
    static EventDelegateChoice *choice(QQmlListProperty<EventDelegateChoice> *, qsizetype index);
    static void clearChoices(QQmlListProperty<EventDelegateChoice> *);
};
