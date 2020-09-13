#pragma once

#include <QObject>
#include <QString>

struct Reaction
{
        Q_GADGET
        Q_PROPERTY(QString key READ key)
        Q_PROPERTY(QString users READ users)
        Q_PROPERTY(QString selfReactedEvent READ selfReactedEvent)
        Q_PROPERTY(int count READ count)

public:
        QString key() const { return key_; }
        QString users() const { return users_; }
        QString selfReactedEvent() const { return selfReactedEvent_; }
        int count() const { return count_; }

        QString key_;
        QString users_;
        QString selfReactedEvent_;
        int count_;
};
