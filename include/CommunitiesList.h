#pragma once

#include <QScrollArea>
#include <QSharedPointer>
#include <QVBoxLayout>

#include "CommunitiesListItem.h"
#include "Community.h"
#include "ui/Theme.h"

class CommunitiesList : public QWidget
{
        Q_OBJECT

public:
        CommunitiesList(QWidget *parent = nullptr);

        void setCommunities(const std::map<QString, QSharedPointer<Community>> &communities);
        void clear() { communities_.clear(); }

        void addCommunity(QSharedPointer<Community> community, const QString &id);
        void removeCommunity(const QString &id) { communities_.erase(id); };

signals:
        void communityChanged(const QString &id);

public slots:
        void updateCommunityAvatar(const QString &id, const QPixmap &img);
        void highlightSelectedCommunity(const QString &id);

private:
        void addGlobalItem() { addCommunity(QSharedPointer<Community>(new Community), "world"); }

        //! Check whether or not a community id is currently managed.
        bool communityExists(const QString &id)
        {
                return communities_.find(id) != communities_.end();
        }

        QVBoxLayout *topLayout_;
        QVBoxLayout *contentsLayout_;
        QWidget *scrollAreaContents_;
        QScrollArea *scrollArea_;

        std::map<QString, QSharedPointer<CommunitiesListItem>> communities_;
};
