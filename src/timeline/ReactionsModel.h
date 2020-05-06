#pragma once

#include <QAbstractListModel>
#include <QHash>

#include <utility>
#include <vector>

#include <mtx/events/collections.hpp>

class ReactionsModel : public QAbstractListModel
{
        Q_OBJECT
public:
        explicit ReactionsModel(QObject *parent = nullptr) { Q_UNUSED(parent); }
        enum Roles
        {
                Key,
                Count,
                Users,
                SelfReacted,
        };

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
        void addReaction(const mtx::events::RoomEvent<mtx::events::msg::Reaction> &reaction);
        void removeReaction(const mtx::events::RoomEvent<mtx::events::msg::Reaction> &reaction);

private:
        struct KeyReaction
        {
                std::string key;
                std::map<std::string, mtx::events::RoomEvent<mtx::events::msg::Reaction>> reactions;
        };
        std::vector<KeyReaction> reactions;
};
