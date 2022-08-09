// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>

#include <unordered_map>

#include <mtx/responses/sync.hpp>

#include "CacheStructs.h"

class CommunitiesModel;

class FilteredCommunitiesModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FilteredCommunitiesModel(CommunitiesModel *model, QObject *parent = nullptr);
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &) const override;
};

class SpaceItem
{
    Q_GADGET

    Q_PROPERTY(QString roomid MEMBER roomid CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(int treeIndex MEMBER treeIndex CONSTANT)

    Q_PROPERTY(bool childValid MEMBER childValid CONSTANT)
    Q_PROPERTY(bool parentValid MEMBER parentValid CONSTANT)
    Q_PROPERTY(bool canonical MEMBER canonical CONSTANT)

    Q_PROPERTY(bool canEditParent MEMBER canEditParent CONSTANT)
    Q_PROPERTY(bool canEditChild MEMBER canEditChild CONSTANT)

public:
    SpaceItem() {}
    SpaceItem(QString roomid_,
              QString name_,
              int treeIndex_,
              bool childValid_,
              bool parentValid_,
              bool canonical_,
              bool canEditChild_,
              bool canEditParent_)
      : roomid(std::move(roomid_))
      , name(std::move(name_))
      , treeIndex(treeIndex_)
      , childValid(childValid_)
      , parentValid(parentValid_)
      , canonical(canonical_)
      , canEditParent(canEditParent_)
      , canEditChild(canEditChild_)
    {}

    QString roomid, name;
    int treeIndex   = 0;
    bool childValid = false, parentValid = false, canonical = false;
    bool canEditParent = false, canEditChild = false;
};

class CommunitiesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentTagId READ currentTagId WRITE setCurrentTagId NOTIFY
                 currentTagIdChanged RESET resetCurrentTagId)
    Q_PROPERTY(QStringList tags READ tags NOTIFY tagsChanged)
    Q_PROPERTY(QStringList tagsWithDefault READ tagsWithDefault NOTIFY tagsChanged)
    Q_PROPERTY(bool containsSubspaces READ containsSubspaces NOTIFY containsSubspacesChanged)

public:
    enum Roles
    {
        AvatarUrl = Qt::UserRole,
        DisplayName,
        Tooltip,
        Collapsed,
        Collapsible,
        Hidden,
        Parent,
        Depth,
        Id,
        UnreadMessages,
        HasLoudNotification,
        Muted,
        IsDirect,
    };

    struct FlatTree
    {
        struct Elem
        {
            QString id;
            int depth = 0;

            mtx::responses::UnreadNotifications notificationCounts = {0, 0};

            bool collapsed = false;
        };

        std::vector<Elem> tree;

        int size() const { return static_cast<int>(tree.size()); }
        int indexOf(const QString &s) const
        {
            for (int i = 0; i < size(); i++)
                if (tree[i].id == s)
                    return i;
            return -1;
        }
        int lastChild(int index) const
        {
            if (index >= size() || index < 0)
                return index;
            const auto depth = tree[index].depth;
            int i            = index + 1;
            for (; i < size(); i++)
                if (tree[i].depth <= depth)
                    break;
            return i - 1;
        }
        int parent(int index) const
        {
            if (index >= size() || index < 0)
                return -1;
            const auto depth = tree[index].depth;
            if (depth == 0)
                return -1;
            int i = index - 1;
            for (; i >= 0; i--)
                if (tree[i].depth < depth)
                    break;
            return i;
        }

        void storeCollapsed();
        void restoreCollapsed();
    };

    CommunitiesModel(QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        (void)parent;
        return 2 + tags_.size() + spaceOrder_.size();
    }
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool containsSubspaces() const
    {
        for (const auto &e : spaceOrder_.tree)
            if (e.depth > 0)
                return true;
        return false;
    }

    Q_INVOKABLE QVariantList spaceChildrenListFromIndex(QString room, int idx = -1) const;
    Q_INVOKABLE void updateSpaceStatus(QString space,
                                       QString room,
                                       bool setParent,
                                       bool setChild,
                                       bool canonical) const;

public slots:
    void initializeSidebar();
    void sync(const mtx::responses::Sync &sync_);
    void clear();
    QString currentTagId() const { return currentTagId_; }
    void setCurrentTagId(const QString &tagId);
    void resetCurrentTagId()
    {
        currentTagId_.clear();
        emit currentTagIdChanged(currentTagId_);
    }
    QStringList tags() const { return tags_; }
    QStringList tagsWithDefault() const
    {
        QStringList tagsWD = tags_;
        tagsWD.prepend(QStringLiteral("m.lowpriority"));
        tagsWD.prepend(QStringLiteral("m.favourite"));
        tagsWD.removeOne(QStringLiteral("m.server_notice"));
        tagsWD.removeDuplicates();
        return tagsWD;
    }
    void toggleTagId(QString tagId);
    void toggleTagMute(QString tagId);

    FilteredCommunitiesModel *filtered() { return new FilteredCommunitiesModel(this, this); }

signals:
    void currentTagIdChanged(QString tagId);
    void hiddenTagsChanged();
    void tagsChanged();
    void containsSubspacesChanged();

private:
    QStringList tags_;
    QString currentTagId_;
    QStringList hiddenTagIds_;
    QStringList mutedTagIds_;
    FlatTree spaceOrder_;
    std::map<QString, RoomInfo> spaces_;
    std::vector<std::string> directMessages_;

    std::unordered_map<QString, mtx::responses::UnreadNotifications> roomNotificationCache;
    std::unordered_map<QString, mtx::responses::UnreadNotifications> tagNotificationCache;
    mtx::responses::UnreadNotifications globalUnreads{};
    mtx::responses::UnreadNotifications dmUnreads{};

    friend class FilteredCommunitiesModel;
};
