#include "TimelineViewManager.h"

#include <QFileDialog>
#include <QMetaType>
#include <QMimeDatabase>
#include <QPalette>
#include <QQmlContext>
#include <QStandardPaths>

#include "ChatPage.h"
#include "ColorImageProvider.h"
#include "DelegateChooser.h"
#include "Logging.h"
#include "MxcImageProvider.h"
#include "UserSettingsPage.h"
#include "dialogs/ImageOverlay.h"

void
TimelineViewManager::updateColorPalette()
{
        UserSettings settings;
        if (settings.theme() == "light") {
                QPalette lightActive(/*windowText*/ QColor("#333"),
                                     /*button*/ QColor("#333"),
                                     /*light*/ QColor(),
                                     /*dark*/ QColor(220, 220, 220, 120),
                                     /*mid*/ QColor(),
                                     /*text*/ QColor("#333"),
                                     /*bright_text*/ QColor(),
                                     /*base*/ QColor("white"),
                                     /*window*/ QColor("white"));
                view->rootContext()->setContextProperty("currentActivePalette", lightActive);
                view->rootContext()->setContextProperty("currentInactivePalette", lightActive);
        } else if (settings.theme() == "dark") {
                QPalette darkActive(/*windowText*/ QColor("#caccd1"),
                                    /*button*/ QColor("#caccd1"),
                                    /*light*/ QColor(),
                                    /*dark*/ QColor(45, 49, 57, 120),
                                    /*mid*/ QColor(),
                                    /*text*/ QColor("#caccd1"),
                                    /*bright_text*/ QColor(),
                                    /*base*/ QColor("#202228"),
                                    /*window*/ QColor("#202228"));
                darkActive.setColor(QPalette::Highlight, QColor("#e7e7e9"));
                view->rootContext()->setContextProperty("currentActivePalette", darkActive);
                view->rootContext()->setContextProperty("currentInactivePalette", darkActive);
        } else {
                view->rootContext()->setContextProperty("currentActivePalette", QPalette());
                view->rootContext()->setContextProperty("currentInactivePalette", nullptr);
        }
}

TimelineViewManager::TimelineViewManager(QWidget *parent)
  : imgProvider(new MxcImageProvider())
  , colorImgProvider(new ColorImageProvider())
{
        qmlRegisterUncreatableMetaObject(qml_mtx_events::staticMetaObject,
                                         "com.github.nheko",
                                         1,
                                         0,
                                         "MtxEvent",
                                         "Can't instantiate enum!");
        qmlRegisterType<DelegateChoice>("com.github.nheko", 1, 0, "DelegateChoice");
        qmlRegisterType<DelegateChooser>("com.github.nheko", 1, 0, "DelegateChooser");

#ifdef USE_QUICK_VIEW
        view      = new QQuickView();
        container = QWidget::createWindowContainer(view, parent);
#else
        view      = new QQuickWidget(parent);
        container = view;
        view->setResizeMode(QQuickWidget::SizeRootObjectToView);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        connect(view, &QQuickWidget::statusChanged, this, [](QQuickWidget::Status status) {
                nhlog::ui()->debug("Status changed to {}", status);
        });
#endif
        container->setMinimumSize(200, 200);
        view->rootContext()->setContextProperty("timelineManager", this);
        updateColorPalette();
        view->engine()->addImageProvider("MxcImage", imgProvider);
        view->engine()->addImageProvider("colorimage", colorImgProvider);
        view->setSource(QUrl("qrc:///qml/TimelineView.qml"));

        connect(dynamic_cast<ChatPage *>(parent),
                &ChatPage::themeChanged,
                this,
                &TimelineViewManager::updateColorPalette);
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
                models.insert(room_id,
                              QSharedPointer<TimelineModel>(new TimelineModel(this, room_id)));
}

void
TimelineViewManager::setHistoryView(const QString &room_id)
{
        nhlog::ui()->info("Trying to activate room {}", room_id.toStdString());

        auto room = models.find(room_id);
        if (room != models.end()) {
                timeline_ = room.value().data();
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
TimelineViewManager::cacheMedia(QString mxcUrl, QString mimeType)
{
        // If the message is a link to a non mxcUrl, don't download it
        if (!mxcUrl.startsWith("mxc://")) {
                emit mediaCached(mxcUrl, mxcUrl);
                return;
        }

        QString suffix = QMimeDatabase().mimeTypeForName(mimeType).preferredSuffix();

        const auto url = mxcUrl.toStdString();
        QFileInfo filename(QString("%1/media_cache/%2.%3")
                             .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                             .arg(QString(mxcUrl).remove("mxc://"))
                             .arg(suffix));
        if (QDir::cleanPath(filename.path()) != filename.path()) {
                nhlog::net()->warn("mxcUrl '{}' is not safe, not downloading file", url);
                return;
        }

        QDir().mkpath(filename.path());

        if (filename.isReadable()) {
                emit mediaCached(mxcUrl, filename.filePath());
                return;
        }

        http::client()->download(
          url,
          [this, mxcUrl, filename, url](const std::string &data,
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
                          QFile file(filename.filePath());

                          if (!file.open(QIODevice::WriteOnly))
                                  return;

                          file.write(QByteArray(data.data(), data.size()));
                          file.close();
                  } catch (const std::exception &e) {
                          nhlog::ui()->warn("Error while saving file to: {}", e.what());
                  }

                  emit mediaCached(mxcUrl, filename.filePath());
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
