#include "TimelineViewManager.h"

#include <QMetaType>
#include <QQmlContext>

#include "Logging.h"

TimelineViewManager::TimelineViewManager(QWidget *parent)
{
        view      = new QQuickView();
        container = QWidget::createWindowContainer(view, parent);
        container->setMinimumSize(200, 200);
        view->setSource(QUrl("qrc:///qml/TimelineView.qml"));
}

void
TimelineViewManager::initialize(const mtx::responses::Rooms &rooms)
{
        for (auto it = rooms.join.cbegin(); it != rooms.join.cend(); ++it) {
                addRoom(QString::fromStdString(it->first));
        }

        sync(rooms);
}

void
TimelineViewManager::addRoom(const QString &room_id)
{
        if (!models.contains(room_id))
                models.insert(room_id, QSharedPointer<TimelineModel>(new TimelineModel()));
}

void
TimelineViewManager::setHistoryView(const QString &room_id)
{
        nhlog::ui()->info("Trying to activate room {}", room_id.toStdString());

        auto room = models.find(room_id);
        if (room != models.end()) {
                view->rootContext()->setContextProperty("timeline",
                                                        QVariant::fromValue(room.value().data()));
                nhlog::ui()->info("Activated room {}", room_id.toStdString());
        }
}

void
TimelineViewManager::initWithMessages(const std::map<QString, mtx::responses::Timeline> &msgs)
{
        for (const auto &e : msgs) {
                addRoom(e.first);
        }
}
