#include "TimelineModel.h"

#include "Utils.h"

QHash<int, QByteArray>
TimelineModel::roleNames() const
{
        return {
          {Type, "type"},
          {Body, "body"},
          {FormattedBody, "formattedBody"},
          {UserId, "userId"},
          {UserName, "userName"},
          {Timestamp, "timestamp"},
        };
}
int
TimelineModel::rowCount(const QModelIndex &parent) const
{
        Q_UNUSED(parent);
        return (int)this->eventOrder.size();
}

QVariant
TimelineModel::data(const QModelIndex &index, int role) const
{
        if (index.row() < 0 && index.row() >= (int)eventOrder.size())
                return QVariant();

        QString id = eventOrder[index.row()];

        switch (role) {
        case UserId:
                return QVariant(QString(""));
        default:
                return QVariant();
        }
}

QColor
TimelineModel::userColor(QString id, QColor background)
{
        if (!userColors.count(id))
                userColors.insert(
                  {id, QColor(utils::generateContrastingHexColor(id, background.name()))});
        return userColors.at(id);
}
