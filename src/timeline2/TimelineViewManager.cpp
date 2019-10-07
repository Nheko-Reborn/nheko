#include "TimelineViewManager.h"

#include <QFileDialog>
#include <QMetaType>
#include <QMimeDatabase>
#include <QQmlContext>

#include "Logging.h"
#include "MxcImageProvider.h"
#include "dialogs/ImageOverlay.h"

TimelineViewManager::TimelineViewManager(QWidget *parent)
  : imgProvider(new MxcImageProvider())
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
        view->engine()->addImageProvider("MxcImage", imgProvider);
        view->setSource(QUrl("qrc:///qml/TimelineView.qml"));
}

void
TimelineViewManager::sync(const mtx::responses::Rooms &rooms)
{
        for (auto it = rooms.join.cbegin(); it != rooms.join.cend(); ++it) {
                // addRoom will only add the room, if it doesn't exist
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
TimelineViewManager::openImageOverlay(QString mxcUrl,
                                      QString originalFilename,
                                      QString mimeType,
                                      qml_mtx_events::EventType eventType) const
{
        QQuickImageResponse *imgResponse =
          imgProvider->requestImageResponse(mxcUrl.remove("mxc://"), QSize());
        connect(imgResponse,
                &QQuickImageResponse::finished,
                this,
                [this, mxcUrl, originalFilename, mimeType, eventType, imgResponse]() {
                        if (!imgResponse->errorString().isEmpty()) {
                                nhlog::ui()->error("Error when retrieving image for overlay: {}",
                                                   imgResponse->errorString().toStdString());
                                return;
                        }
                        auto pixmap = QPixmap::fromImage(imgResponse->textureFactory()->image());

                        auto imgDialog = new dialogs::ImageOverlay(pixmap);
                        imgDialog->show();
                        connect(imgDialog,
                                &dialogs::ImageOverlay::saving,
                                this,
                                [this, mxcUrl, originalFilename, mimeType, eventType]() {
                                        saveMedia(mxcUrl, originalFilename, mimeType, eventType);
                                });
                });
}

void
TimelineViewManager::saveMedia(QString mxcUrl,
                               QString originalFilename,
                               QString mimeType,
                               qml_mtx_events::EventType eventType) const
{
        QString dialogTitle;
        if (eventType == qml_mtx_events::EventType::ImageMessage) {
                dialogTitle = tr("Save image");
        } else if (eventType == qml_mtx_events::EventType::VideoMessage) {
                dialogTitle = tr("Save video");
        } else if (eventType == qml_mtx_events::EventType::AudioMessage) {
                dialogTitle = tr("Save audio");
        } else {
                dialogTitle = tr("Save file");
        }

        QString filterString = QMimeDatabase().mimeTypeForName(mimeType).filterString();

        auto filename =
          QFileDialog::getSaveFileName(container, dialogTitle, originalFilename, filterString);

        if (filename.isEmpty())
                return;

        const auto url = mxcUrl.toStdString();

        http::client()->download(
          url,
          [filename, url](const std::string &data,
                          const std::string &,
                          const std::string &,
                          mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to retrieve image {}: {} {}",
                                             url,
                                             err->matrix_error.error,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  try {
                          QFile file(filename);

                          if (!file.open(QIODevice::WriteOnly))
                                  return;

                          file.write(QByteArray(data.data(), data.size()));
                          file.close();
                  } catch (const std::exception &e) {
                          nhlog::ui()->warn("Error while saving file to: {}", e.what());
                  }
          });
}

void
TimelineViewManager::updateReadReceipts(const QString &room_id,
                                        const std::vector<QString> &event_ids)
{
        auto room = models.find(room_id);
        if (room != models.end()) {
                room.value()->markEventsAsRead(event_ids);
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
