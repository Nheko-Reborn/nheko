// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QStringList>

#include <mtx/responses/sync.hpp>

#include "CacheStructs.h"

class CommunitiesModel : public QAbstractListModel
{
        Q_OBJECT
        Q_PROPERTY(QString currentTagId READ currentTagId WRITE setCurrentTagId NOTIFY
                     currentTagIdChanged RESET resetCurrentTagId)
        Q_PROPERTY(QStringList tags READ tags NOTIFY tagsChanged)
        Q_PROPERTY(QStringList tagsWithDefault READ tagsWithDefault NOTIFY tagsChanged)

public:
        enum Roles
        {
                AvatarUrl = Qt::UserRole,
                DisplayName,
                Tooltip,
                ChildrenHidden,
                Hidden,
                Id,
        };

        CommunitiesModel(QObject *parent = nullptr);
        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
                (void)parent;
                return 1 + tags_.size();
        }
        QVariant data(const QModelIndex &index, int role) const override;

public slots:
        void initializeSidebar();
        void sync(const mtx::responses::Rooms &rooms);
        void clear();
        QString currentTagId() const { return currentTagId_; }
        void setCurrentTagId(QString tagId);
        void resetCurrentTagId()
        {
                currentTagId_.clear();
                emit currentTagIdChanged(currentTagId_);
        }
        QStringList tags() const { return tags_; }
        QStringList tagsWithDefault() const
        {
                QStringList tagsWD = tags_;
                tagsWD.prepend("m.lowpriority");
                tagsWD.prepend("m.favourite");
                tagsWD.removeOne("m.server_notice");
                tagsWD.removeDuplicates();
                return tagsWD;
        }
        void toggleTagId(QString tagId);

signals:
        void currentTagIdChanged(QString tagId);
        void hiddenTagsChanged();
        void tagsChanged();

private:
        QStringList tags_;
        QString currentTagId_;
        QStringList hiddentTagIds_;
        QStringList spaceOrder_;
        std::map<QString, RoomInfo> spaces_;
};
