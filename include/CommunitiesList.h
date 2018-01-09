#pragma once

#include <QScrollArea>
#include <QSharedPointer>
#include <QVBoxLayout>
#include <QWidget>

#include "CommunitiesListItem.h"
#include "Community.h"
#include "MatrixClient.h"
#include "ui/Theme.h"

class CommunitiesList : public QWidget
{
        Q_OBJECT

public:
        CommunitiesList(QSharedPointer<MatrixClient> client, QWidget *parent = nullptr);
        ~CommunitiesList();

        void setCommunities(const QMap<QString, QSharedPointer<Community>> &communities);
        void clear();

        void addCommunity(QSharedPointer<Community> community, const QString &community_id);
        void removeCommunity(const QString &community_id);
signals:
        void communityChanged(const QString &community_id);

public slots:
        void updateCommunityAvatar(const QString &community_id, const QPixmap &img);
        void highlightSelectedCommunity(const QString &community_id);

private:
        QVBoxLayout *topLayout_;
        QVBoxLayout *contentsLayout_;
        QWidget *scrollAreaContents_;
        QScrollArea *scrollArea_;

        QMap<QString, QSharedPointer<CommunitiesListItem>> communities_;

        QSharedPointer<MatrixClient> client_;
};
