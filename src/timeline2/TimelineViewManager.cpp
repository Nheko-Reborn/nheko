#include "TimelineViewManager.h"

#include <QMetaType>
#include <QQmlContext>

#include "Logging.h"
#include "MxcImageProvider.h"

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
        view->engine()->addImageProvider("MxcImage", new MxcImageProvider());
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

void
TimelineViewManager::queueTextMessage(const QString &msg)
{
        mtx::events::msg::Text text = {};
        text.body                   = msg.trimmed().toStdString();
        text.format                 = "org.matrix.custom.html";
        text.formatted_body         = utils::markdownToHtml(msg).toStdString();

        if (timeline_)
                timeline_->sendMessage(text);
}

void
TimelineViewManager::queueReplyMessage(const QString &reply, const RelatedInfo &related)
{
        mtx::events::msg::Text text = {};

        QString body;
        bool firstLine = true;
        for (const auto &line : related.quoted_body.split("\n")) {
                if (firstLine) {
                        firstLine = false;
                        body      = QString("> <%1> %2\n").arg(related.quoted_user).arg(line);
                } else {
                        body = QString("%1\n> %2\n").arg(body).arg(line);
                }
        }

        text.body   = QString("%1\n%2").arg(body).arg(reply).toStdString();
        text.format = "org.matrix.custom.html";
        text.formatted_body =
          utils::getFormattedQuoteBody(related, utils::markdownToHtml(reply)).toStdString();
        text.relates_to.in_reply_to.event_id = related.related_event;

        if (timeline_)
                timeline_->sendMessage(text);
}

void
TimelineViewManager::queueEmoteMessage(const QString &msg)
{
        auto html = utils::markdownToHtml(msg);

        mtx::events::msg::Emote emote;
        emote.body = msg.trimmed().toStdString();

        if (html != msg.trimmed().toHtmlEscaped())
                emote.formatted_body = html.toStdString();

        if (timeline_)
                timeline_->sendMessage(emote);
}

void
TimelineViewManager::queueImageMessage(const QString &roomid,
                                       const QString &filename,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize,
                                       const QSize &dimensions)
{
        mtx::events::msg::Image image;
        image.info.mimetype = mime.toStdString();
        image.info.size     = dsize;
        image.body          = filename.toStdString();
        image.url           = url.toStdString();
        image.info.h        = dimensions.height();
        image.info.w        = dimensions.width();
        models.value(roomid)->sendMessage(image);
}

void
TimelineViewManager::queueFileMessage(const QString &roomid,
                                      const QString &filename,
                                      const QString &url,
                                      const QString &mime,
                                      uint64_t dsize)
{
        mtx::events::msg::File file;
        file.info.mimetype = mime.toStdString();
        file.info.size     = dsize;
        file.body          = filename.toStdString();
        file.url           = url.toStdString();
        models.value(roomid)->sendMessage(file);
}

void
TimelineViewManager::queueAudioMessage(const QString &roomid,
                                       const QString &filename,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize)
{
        mtx::events::msg::Audio audio;
        audio.info.mimetype = mime.toStdString();
        audio.info.size     = dsize;
        audio.body          = filename.toStdString();
        audio.url           = url.toStdString();
        models.value(roomid)->sendMessage(audio);
}

void
TimelineViewManager::queueVideoMessage(const QString &roomid,
                                       const QString &filename,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize)
{
        mtx::events::msg::Video video;
        video.info.mimetype = mime.toStdString();
        video.info.size     = dsize;
        video.body          = filename.toStdString();
        video.url           = url.toStdString();
        models.value(roomid)->sendMessage(video);
}
