#include "TimelineModel.h"

#include "Logging.h"
#include "Utils.h"

namespace {
template<class T>
QString
eventId(const T &event)
{
        return QString::fromStdString(event.event_id);
}
template<class T>
QString
roomId(const T &event)
{
        return QString::fromStdString(event.room_id);
}
template<class T>
QString
senderId(const T &event)
{
        return QString::fromStdString(event.sender);
}
}

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
        nhlog::ui()->info("current order size: {}", eventOrder.size());
        return (int)this->eventOrder.size();
}

QVariant
TimelineModel::data(const QModelIndex &index, int role) const
{
        nhlog::ui()->info("data");
        if (index.row() < 0 && index.row() >= (int)eventOrder.size())
                return QVariant();

        QString id = eventOrder[index.row()];

        switch (role) {
        case UserId:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> QString { return senderId(e); }, events.value(id)));
        default:
                return QVariant();
        }
}

void
TimelineModel::addEvents(const mtx::responses::Timeline &events)
{
        nhlog::ui()->info("add {} events", events.events.size());
        std::vector<QString> ids;
        for (const auto &e : events.events) {
                QString id =
                  boost::apply_visitor([](const auto &e) -> QString { return eventId(e); }, e);

                this->events.insert(id, e);
                ids.push_back(id);
                nhlog::ui()->info("add event {}", id.toStdString());
        }

        beginInsertRows(QModelIndex(),
                        static_cast<int>(this->events.size()),
                        static_cast<int>(this->events.size() + ids.size() - 1));
        this->eventOrder.insert(this->eventOrder.end(), ids.begin(), ids.end());
        endInsertRows();
}

QColor
TimelineModel::userColor(QString id, QColor background)
{
        if (!userColors.contains(id))
                userColors.insert(
                  id, QColor(utils::generateContrastingHexColor(id, background.name())));
        return userColors.value(id);
}
