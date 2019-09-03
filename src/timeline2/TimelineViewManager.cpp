#include "TimelineViewManager.h"

#include <QMetaType>
#include <QQmlContext>

#include "Logging.h"

TimelineViewManager::TimelineViewManager(QWidget *parent)
{
        qmlRegisterUncreatableMetaObject(qml_mtx_events::staticMetaObject,
                                         "com.github.nheko",
                                         1,
                                         0,
                                         "MtxEvent",
                                         "Can't instantiate enum!");

        view      = new QQuickView();
        container = QWidget::createWindowContainer(view, parent);
        container->setMinimumSize(200, 200);
        view->rootContext()->setContextProperty("timelineManager", this);
        view->setSource(QUrl("qrc:///qml/TimelineView.qml"));
}

void
TimelineViewManager::initialize(const mtx::responses::Rooms &rooms)
{
        for (auto it = rooms.join.cbegin(); it != rooms.join.cend(); ++it) {
                addRoom(QString::fromStdString(it->first));
                models.value(QString::fromStdString(it->first))->addEvents(it->second.timeline);
        }
}

void
TimelineViewManager::addRoom(const QString &room_id)
{
        if (!models.contains(room_id))
                models.insert(room_id, QSharedPointer<TimelineModel>(new TimelineModel(room_id)));
}

void
TimelineViewManager::setHistoryView(const QString &room_id)
{
        nhlog::ui()->info("Trying to activate room {}", room_id.toStdString());

        auto room = models.find(room_id);
        if (room != models.end()) {
                timeline_ = room.value().data();
                timeline_->fetchHistory();
                emit activeTimelineChanged(timeline_);
                nhlog::ui()->info("Activated room {}", room_id.toStdString());
        }
}

void
TimelineViewManager::initWithMessages(const std::map<QString, mtx::responses::Timeline> &msgs)
{
        for (const auto &e : msgs) {
                addRoom(e.first);

                models.value(e.first)->addEvents(e.second);
        }
}
