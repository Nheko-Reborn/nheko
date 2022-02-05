// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "InputBar.h"

#include <QClipboard>
#include <QDropEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMimeData>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QTextBoundaryFinder>
#include <QUrl>

#include <QRegularExpression>
#include <mtx/responses/common.hpp>
#include <mtx/responses/media.hpp>

#include "Cache.h"
#include "ChatPage.h"
#include "CombinedImagePackModel.h"
#include "Config.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "TimelineModel.h"
#include "TimelineViewManager.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "dialogs/PreviewUploadOverlay.h"

#include "blurhash.hpp"

static constexpr size_t INPUT_HISTORY_SIZE = 10;

void
InputBar::paste(bool fromMouse)
{
    const QMimeData *md = nullptr;

    if (fromMouse && QGuiApplication::clipboard()->supportsSelection()) {
        md = QGuiApplication::clipboard()->mimeData(QClipboard::Selection);
    } else {
        md = QGuiApplication::clipboard()->mimeData(QClipboard::Clipboard);
    }

    if (md)
        insertMimeData(md);
}

void
InputBar::insertMimeData(const QMimeData *md)
{
    if (!md)
        return;

    nhlog::ui()->debug("Got mime formats: {}",
                       md->formats().join(QStringLiteral(", ")).toStdString());
    const auto formats = md->formats().filter(QStringLiteral("/"));
    const auto image   = formats.filter(QStringLiteral("image/"), Qt::CaseInsensitive);
    const auto audio   = formats.filter(QStringLiteral("audio/"), Qt::CaseInsensitive);
    const auto video   = formats.filter(QStringLiteral("video/"), Qt::CaseInsensitive);

    if (md->hasImage()) {
        if (formats.contains(QStringLiteral("image/svg+xml"), Qt::CaseInsensitive)) {
            showPreview(*md, QLatin1String(""), QStringList(QStringLiteral("image/svg+xml")));
        } else {
            showPreview(*md, QLatin1String(""), image);
        }
    } else if (!audio.empty()) {
        showPreview(*md, QLatin1String(""), audio);
    } else if (!video.empty()) {
        showPreview(*md, QLatin1String(""), video);
    } else if (md->hasUrls()) {
        // Generic file path for any platform.
        QString path;
        for (auto &&u : md->urls()) {
            if (u.isLocalFile()) {
                path = u.toLocalFile();
                break;
            }
        }

        if (!path.isEmpty() && QFileInfo::exists(path)) {
            showPreview(*md, path, formats);
        } else {
            nhlog::ui()->warn("Clipboard does not contain any valid file paths.");
        }
    } else if (md->hasFormat(QStringLiteral("x-special/gnome-copied-files"))) {
        // Special case for X11 users. See "Notes for X11 Users" in md.
        // Source: http://doc.qt.io/qt-5/qclipboard.html

        // This MIME type returns a string with multiple lines separated by '\n'. The first
        // line is the command to perform with the clipboard (not useful to us). The
        // following lines are the file URIs.
        //
        // Source: the nautilus source code in file 'src/nautilus-clipboard.c' in function
        // nautilus_clipboard_get_uri_list_from_selection_data()
        // https://github.com/GNOME/nautilus/blob/master/src/nautilus-clipboard.c

        auto data = md->data(QStringLiteral("x-special/gnome-copied-files")).split('\n');
        if (data.size() < 2) {
            nhlog::ui()->warn("MIME format is malformed, cannot perform paste.");
            return;
        }

        QString path;
        for (int i = 1; i < data.size(); ++i) {
            QUrl url{data[i]};
            if (url.isLocalFile()) {
                path = url.toLocalFile();
                break;
            }
        }

        if (!path.isEmpty()) {
            showPreview(*md, path, formats);
        } else {
            nhlog::ui()->warn("Clipboard does not contain any valid file paths: {}",
                              data.join(", ").toStdString());
        }
    } else if (md->hasText()) {
        emit insertText(md->text());
    } else {
        nhlog::ui()->debug("formats: {}", md->formats().join(QStringLiteral(", ")).toStdString());
    }
}

void
InputBar::updateAtRoom(const QString &t)
{
    bool roomMention = false;

    if (t.size() > 4) {
        QTextBoundaryFinder finder(QTextBoundaryFinder::BoundaryType::Word, t);

        finder.toStart();
        do {
            auto start = finder.position();
            finder.toNextBoundary();
            auto end = finder.position();
            if (start > 0 && end - start >= 4 &&
                t.mid(start, end - start) == QLatin1String("room") &&
                t.at(start - 1) == QChar('@')) {
                roomMention = true;
                break;
            }
        } while (finder.position() < t.size());
    }

    if (roomMention != this->containsAtRoom_) {
        this->containsAtRoom_ = roomMention;
        emit containsAtRoomChanged();
    }
}

void
InputBar::setText(const QString &newText)
{
    if (history_.empty())
        history_.push_front(newText);
    else
        history_.front() = newText;
    history_index_ = 0;

    if (history_.size() == INPUT_HISTORY_SIZE)
        history_.pop_back();

    updateAtRoom(QLatin1String(""));
    emit textChanged(newText);
}
void
InputBar::updateState(int selectionStart_,
                      int selectionEnd_,
                      int cursorPosition_,
                      const QString &text_)
{
    if (text_.isEmpty())
        stopTyping();
    else
        startTyping();

    if (text_ != text()) {
        if (history_.empty())
            history_.push_front(text_);
        else
            history_.front() = text_;
        history_index_ = 0;

        updateAtRoom(text_);
    }

    selectionStart = selectionStart_;
    selectionEnd   = selectionEnd_;
    cursorPosition = cursorPosition_;
}

QString
InputBar::text() const
{
    if (history_index_ < history_.size())
        return history_.at(history_index_);

    return QString();
}

QString
InputBar::previousText()
{
    history_index_++;
    if (history_index_ >= INPUT_HISTORY_SIZE)
        history_index_ = INPUT_HISTORY_SIZE;
    else if (text().isEmpty())
        history_index_--;

    updateAtRoom(text());
    return text();
}

QString
InputBar::nextText()
{
    history_index_--;
    if (history_index_ >= INPUT_HISTORY_SIZE)
        history_index_ = 0;

    updateAtRoom(text());
    return text();
}

void
InputBar::send()
{
    if (text().trimmed().isEmpty())
        return;

    nhlog::ui()->debug("Send: {}", text().toStdString());

    auto wasEdit = !room->edit().isEmpty();

    if (text().startsWith('/')) {
        int command_end = text().indexOf(QRegularExpression(QStringLiteral("\\s")));
        if (command_end == -1)
            command_end = text().size();
        auto name = text().mid(1, command_end - 1);
        auto args = text().mid(command_end + 1);
        if (name.isEmpty() || name == QLatin1String("/")) {
            message(args);
        } else {
            command(name, args);
        }
    } else {
        message(text());
    }

    if (!wasEdit) {
        history_.push_front(QLatin1String(""));
        setText(QLatin1String(""));
    }
}

void
InputBar::openFileSelection()
{
    const QString homeFolder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const auto fileName =
      QFileDialog::getOpenFileName(nullptr, tr("Select a file"), homeFolder, tr("All Files (*)"));

    if (fileName.isEmpty())
        return;

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);

    QFile file{fileName};

    if (!file.open(QIODevice::ReadOnly)) {
        emit ChatPage::instance()->showNotification(
          QStringLiteral("Error while reading media: %1").arg(file.errorString()));
        return;
    }

    setUploading(true);

    auto bin = file.readAll();

    QMimeData data;
    data.setData(mime.name(), bin);

    showPreview(data, fileName, QStringList{mime.name()});
}

void
InputBar::message(const QString &msg, MarkdownOverride useMarkdown, bool rainbowify)
{
    mtx::events::msg::Text text = {};
    text.body                   = msg.trimmed().toStdString();

    if ((ChatPage::instance()->userSettings()->markdown() &&
         useMarkdown == MarkdownOverride::NOT_SPECIFIED) ||
        useMarkdown == MarkdownOverride::ON) {
        text.formatted_body = utils::markdownToHtml(msg, rainbowify).toStdString();
        // Remove markdown links by completer
        text.body = msg.trimmed().replace(conf::strings::matrixToMarkdownLink, "\\1").toStdString();

        // Don't send formatted_body, when we don't need to
        if (text.formatted_body.find('<') == std::string::npos)
            text.formatted_body = "";
        else
            text.format = "org.matrix.custom.html";
    }

    if (!room->edit().isEmpty()) {
        if (!room->reply().isEmpty()) {
            text.relations.relations.push_back(
              {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
        }

        text.relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});

    } else if (!room->reply().isEmpty()) {
        auto related = room->relatedInfo(room->reply());

        QString body;
        bool firstLine = true;
        auto lines     = related.quoted_body.splitRef(u'\n');
        for (auto line : qAsConst(lines)) {
            if (firstLine) {
                firstLine = false;
                body      = QStringLiteral("> <%1> %2\n").arg(related.quoted_user, line);
            } else {
                body += QStringLiteral("> %1\n").arg(line);
            }
        }

        text.body = QStringLiteral("%1\n%2").arg(body, msg).toStdString();

        // NOTE(Nico): rich replies always need a formatted_body!
        text.format = "org.matrix.custom.html";
        if ((ChatPage::instance()->userSettings()->markdown() &&
             useMarkdown == MarkdownOverride::NOT_SPECIFIED) ||
            useMarkdown == MarkdownOverride::ON)
            text.formatted_body =
              utils::getFormattedQuoteBody(related, utils::markdownToHtml(msg, rainbowify))
                .toStdString();
        else
            text.formatted_body =
              utils::getFormattedQuoteBody(related, msg.toHtmlEscaped()).toStdString();

        text.relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, related.related_event});
    }

    room->sendMessageEvent(text, mtx::events::EventType::RoomMessage);
}

void
InputBar::emote(const QString &msg, bool rainbowify)
{
    auto html = utils::markdownToHtml(msg, rainbowify);

    mtx::events::msg::Emote emote;
    emote.body = msg.trimmed().toStdString();

    if (html != msg.trimmed().toHtmlEscaped() && ChatPage::instance()->userSettings()->markdown()) {
        emote.formatted_body = html.toStdString();
        emote.format         = "org.matrix.custom.html";
        // Remove markdown links by completer
        emote.body =
          msg.trimmed().replace(conf::strings::matrixToMarkdownLink, "\\1").toStdString();
    }

    if (!room->reply().isEmpty()) {
        emote.relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
    }
    if (!room->edit().isEmpty()) {
        emote.relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});
    }

    room->sendMessageEvent(emote, mtx::events::EventType::RoomMessage);
}

void
InputBar::notice(const QString &msg, bool rainbowify)
{
    auto html = utils::markdownToHtml(msg, rainbowify);

    mtx::events::msg::Notice notice;
    notice.body = msg.trimmed().toStdString();

    if (html != msg.trimmed().toHtmlEscaped() && ChatPage::instance()->userSettings()->markdown()) {
        notice.formatted_body = html.toStdString();
        notice.format         = "org.matrix.custom.html";
        // Remove markdown links by completer
        notice.body =
          msg.trimmed().replace(conf::strings::matrixToMarkdownLink, "\\1").toStdString();
    }

    if (!room->reply().isEmpty()) {
        notice.relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
    }
    if (!room->edit().isEmpty()) {
        notice.relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});
    }

    room->sendMessageEvent(notice, mtx::events::EventType::RoomMessage);
}

void
InputBar::image(const QString &filename,
                const std::optional<mtx::crypto::EncryptedFile> &file,
                const QString &url,
                const QString &mime,
                uint64_t dsize,
                const QSize &dimensions,
                const QString &blurhash)
{
    mtx::events::msg::Image image;
    image.info.mimetype = mime.toStdString();
    image.info.size     = dsize;
    image.info.blurhash = blurhash.toStdString();
    image.body          = filename.toStdString();
    image.info.h        = dimensions.height();
    image.info.w        = dimensions.width();

    if (file)
        image.file = file;
    else
        image.url = url.toStdString();

    if (!room->reply().isEmpty()) {
        image.relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
    }
    if (!room->edit().isEmpty()) {
        image.relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});
    }

    room->sendMessageEvent(image, mtx::events::EventType::RoomMessage);
}

void
InputBar::file(const QString &filename,
               const std::optional<mtx::crypto::EncryptedFile> &encryptedFile,
               const QString &url,
               const QString &mime,
               uint64_t dsize)
{
    mtx::events::msg::File file;
    file.info.mimetype = mime.toStdString();
    file.info.size     = dsize;
    file.body          = filename.toStdString();

    if (encryptedFile)
        file.file = encryptedFile;
    else
        file.url = url.toStdString();

    if (!room->reply().isEmpty()) {
        file.relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
    }
    if (!room->edit().isEmpty()) {
        file.relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});
    }

    room->sendMessageEvent(file, mtx::events::EventType::RoomMessage);
}

void
InputBar::audio(const QString &filename,
                const std::optional<mtx::crypto::EncryptedFile> &file,
                const QString &url,
                const QString &mime,
                uint64_t dsize)
{
    mtx::events::msg::Audio audio;
    audio.info.mimetype = mime.toStdString();
    audio.info.size     = dsize;
    audio.body          = filename.toStdString();
    audio.url           = url.toStdString();

    if (file)
        audio.file = file;
    else
        audio.url = url.toStdString();

    if (!room->reply().isEmpty()) {
        audio.relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
    }
    if (!room->edit().isEmpty()) {
        audio.relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});
    }

    room->sendMessageEvent(audio, mtx::events::EventType::RoomMessage);
}

void
InputBar::video(const QString &filename,
                const std::optional<mtx::crypto::EncryptedFile> &file,
                const QString &url,
                const QString &mime,
                uint64_t dsize)
{
    mtx::events::msg::Video video;
    video.info.mimetype = mime.toStdString();
    video.info.size     = dsize;
    video.body          = filename.toStdString();

    if (file)
        video.file = file;
    else
        video.url = url.toStdString();

    if (!room->reply().isEmpty()) {
        video.relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
    }
    if (!room->edit().isEmpty()) {
        video.relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});
    }

    room->sendMessageEvent(video, mtx::events::EventType::RoomMessage);
}

void
InputBar::sticker(CombinedImagePackModel *model, int row)
{
    if (!model || row < 0)
        return;

    auto img = model->imageAt(row);

    mtx::events::msg::StickerImage sticker{};
    sticker.info = img.info.value_or(mtx::common::ImageInfo{});
    sticker.url  = img.url;
    sticker.body = img.body;

    // workaround for https://github.com/vector-im/element-ios/issues/2353
    sticker.info.thumbnail_url           = sticker.url;
    sticker.info.thumbnail_info.mimetype = sticker.info.mimetype;
    sticker.info.thumbnail_info.size     = sticker.info.size;
    sticker.info.thumbnail_info.h        = sticker.info.h;
    sticker.info.thumbnail_info.w        = sticker.info.w;

    if (!room->reply().isEmpty()) {
        sticker.relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
    }
    if (!room->edit().isEmpty()) {
        sticker.relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});
    }

    room->sendMessageEvent(sticker, mtx::events::EventType::Sticker);
}

void
InputBar::command(const QString &command, QString args)
{
    if (command == QLatin1String("me")) {
        emote(args, false);
    } else if (command == QLatin1String("react")) {
        auto eventId = room->reply();
        if (!eventId.isEmpty())
            reaction(eventId, args.trimmed());
    } else if (command == QLatin1String("join")) {
        ChatPage::instance()->joinRoom(args);
    } else if (command == QLatin1String("part") || command == QLatin1String("leave")) {
        ChatPage::instance()->timelineManager()->openLeaveRoomDialog(room->roomId());
    } else if (command == QLatin1String("invite")) {
        ChatPage::instance()->inviteUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("kick")) {
        ChatPage::instance()->kickUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("ban")) {
        ChatPage::instance()->banUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("unban")) {
        ChatPage::instance()->unbanUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("roomnick")) {
        mtx::events::state::Member member;
        member.display_name = args.toStdString();
        member.avatar_url =
          cache::avatarUrl(room->roomId(),
                           QString::fromStdString(http::client()->user_id().to_string()))
            .toStdString();
        member.membership = mtx::events::state::Membership::Join;

        http::client()->send_state_event(
          room->roomId().toStdString(),
          http::client()->user_id().to_string(),
          member,
          [](const mtx::responses::EventId &, mtx::http::RequestErr err) {
              if (err)
                  nhlog::net()->error("Failed to set room displayname: {}",
                                      err->matrix_error.error);
          });
    } else if (command == QLatin1String("shrug")) {
        message("¯\\_(ツ)_/¯" + (args.isEmpty() ? QLatin1String("") : " " + args));
    } else if (command == QLatin1String("fliptable")) {
        message(QStringLiteral("(╯°□°)╯︵ ┻━┻"));
    } else if (command == QLatin1String("unfliptable")) {
        message(QStringLiteral(" ┯━┯╭( º _ º╭)"));
    } else if (command == QLatin1String("sovietflip")) {
        message(QStringLiteral("ノ┬─┬ノ ︵ ( \\o°o)\\"));
    } else if (command == QLatin1String("clear-timeline")) {
        room->clearTimeline();
    } else if (command == QLatin1String("reset-state")) {
        room->resetState();
    } else if (command == QLatin1String("rotate-megolm-session")) {
        cache::dropOutboundMegolmSession(room->roomId().toStdString());
    } else if (command == QLatin1String("md")) {
        message(args, MarkdownOverride::ON);
    } else if (command == QLatin1String("plain")) {
        message(args, MarkdownOverride::OFF);
    } else if (command == QLatin1String("rainbow")) {
        message(args, MarkdownOverride::ON, true);
    } else if (command == QLatin1String("rainbowme")) {
        emote(args, true);
    } else if (command == QLatin1String("notice")) {
        notice(args, false);
    } else if (command == QLatin1String("rainbownotice")) {
        notice(args, true);
    } else if (command == QLatin1String("goto")) {
        // Goto has three different modes:
        // 1 - Going directly to a given event ID
        if (args[0] == '$') {
            room->showEvent(args);
            return;
        }
        // 2 - Going directly to a given message index
        if (args[0] >= '0' && args[0] <= '9') {
            room->showEvent(args);
            return;
        }
        // 3 - Matrix URI handler, as if you clicked the URI
        if (ChatPage::instance()->handleMatrixUri(args)) {
            return;
        }
        nhlog::net()->error("Could not resolve goto: {}", args.toStdString());
    } else if (command == QLatin1String("converttodm")) {
        utils::markRoomAsDirect(this->room->roomId(),
                                cache::getMembers(this->room->roomId().toStdString(), 0, -1));
    } else if (command == QLatin1String("converttoroom")) {
        utils::removeDirectFromRoom(this->room->roomId());
    }
}

void
InputBar::showPreview(const QMimeData &source, const QString &path, const QStringList &formats)
{
    auto *previewDialog_ = new dialogs::PreviewUploadOverlay(nullptr);
    previewDialog_->setAttribute(Qt::WA_DeleteOnClose);

    // Force SVG to _not_ be handled as an image, but as raw data
    if (source.hasImage() &&
        (formats.empty() || formats.front() != QLatin1String("image/svg+xml"))) {
        if (!formats.empty() && formats.front().startsWith(QLatin1String("image/"))) {
            // known format, keep as-is
            previewDialog_->setPreview(qvariant_cast<QImage>(source.imageData()), formats.front());
        } else {
            // unknown image format, default to image/png
            previewDialog_->setPreview(qvariant_cast<QImage>(source.imageData()),
                                       QStringLiteral("image/png"));
        }
    } else if (!path.isEmpty())
        previewDialog_->setPreview(path);
    else if (!formats.isEmpty()) {
        const auto &mime = formats.first();
        previewDialog_->setPreview(source.data(mime), mime);
    } else {
        setUploading(false);
        previewDialog_->deleteLater();
        return;
    }

    connect(previewDialog_, &dialogs::PreviewUploadOverlay::aborted, this, [this]() {
        setUploading(false);
    });

    connect(
      previewDialog_,
      &dialogs::PreviewUploadOverlay::confirmUpload,
      this,
      [this](const QByteArray &data, const QString &mime, const QString &fn) {
          if (!data.size()) {
              nhlog::ui()->warn("Attempted to upload zero-byte file?! Mimetype {}, filename {}",
                                mime.toStdString(),
                                fn.toStdString());
              return;
          }
          setUploading(true);

          setText(QLatin1String(""));

          auto payload = std::string(data.data(), data.size());
          std::optional<mtx::crypto::EncryptedFile> encryptedFile;
          if (cache::isRoomEncrypted(room->roomId().toStdString())) {
              mtx::crypto::BinaryBuf buf;
              std::tie(buf, encryptedFile) = mtx::crypto::encrypt_file(payload);
              payload                      = mtx::crypto::to_string(buf);
          }

          QSize dimensions;
          QString blurhash;
          auto mimeClass = mime.left(mime.indexOf(u'/'));
          nhlog::ui()->debug("Mime: {}", mime.toStdString());
          if (mimeClass == u"image") {
              QImage img = utils::readImage(data);

              dimensions = img.size();
              if (img.height() > 200 && img.width() > 360)
                  img = img.scaled(360, 200, Qt::KeepAspectRatioByExpanding);
              std::vector<unsigned char> data_;
              for (int y = 0; y < img.height(); y++) {
                  for (int x = 0; x < img.width(); x++) {
                      auto p = img.pixel(x, y);
                      data_.push_back(static_cast<unsigned char>(qRed(p)));
                      data_.push_back(static_cast<unsigned char>(qGreen(p)));
                      data_.push_back(static_cast<unsigned char>(qBlue(p)));
                  }
              }
              blurhash = QString::fromStdString(
                blurhash::encode(data_.data(), img.width(), img.height(), 4, 3));
          }

          http::client()->upload(
            payload,
            encryptedFile ? "application/octet-stream" : mime.toStdString(),
            QFileInfo(fn).fileName().toStdString(),
            [this,
             filename      = fn,
             encryptedFile = std::move(encryptedFile),
             mimeClass,
             mime,
             size = payload.size(),
             dimensions,
             blurhash](const mtx::responses::ContentURI &res, mtx::http::RequestErr err) mutable {
                if (err) {
                    emit ChatPage::instance()->showNotification(
                      tr("Failed to upload media. Please try again."));
                    nhlog::net()->warn("failed to upload media: {} {} ({})",
                                       err->matrix_error.error,
                                       to_string(err->matrix_error.errcode),
                                       static_cast<int>(err->status_code));
                    setUploading(false);
                    return;
                }

                auto url = QString::fromStdString(res.content_uri);
                if (encryptedFile)
                    encryptedFile->url = res.content_uri;

                if (mimeClass == u"image")
                    image(filename, encryptedFile, url, mime, size, dimensions, blurhash);
                else if (mimeClass == u"audio")
                    audio(filename, encryptedFile, url, mime, size);
                else if (mimeClass == u"video")
                    video(filename, encryptedFile, url, mime, size);
                else
                    file(filename, encryptedFile, url, mime, size);

                setUploading(false);
            });
      });
}

void
InputBar::startTyping()
{
    if (!typingRefresh_.isActive()) {
        typingRefresh_.start();

        if (ChatPage::instance()->userSettings()->typingNotifications()) {
            http::client()->start_typing(
              room->roomId().toStdString(), 10'000, [](mtx::http::RequestErr err) {
                  if (err) {
                      nhlog::net()->warn("failed to send typing notification: {}",
                                         err->matrix_error.error);
                  }
              });
        }
    }
    typingTimeout_.start();
}
void
InputBar::stopTyping()
{
    typingRefresh_.stop();
    typingTimeout_.stop();

    if (!ChatPage::instance()->userSettings()->typingNotifications())
        return;

    http::client()->stop_typing(room->roomId().toStdString(), [](mtx::http::RequestErr err) {
        if (err) {
            nhlog::net()->warn("failed to stop typing notifications: {}", err->matrix_error.error);
        }
    });
}

void
InputBar::reaction(const QString &reactedEvent, const QString &reactionKey)
{
    auto reactions = room->reactions(reactedEvent.toStdString());

    QString selfReactedEvent;
    for (const auto &reaction : reactions) {
        if (reactionKey == reaction.key_) {
            selfReactedEvent = reaction.selfReactedEvent_;
            break;
        }
    }

    if (selfReactedEvent.startsWith(QLatin1String("m")))
        return;

    // If selfReactedEvent is empty, that means we haven't previously reacted
    if (selfReactedEvent.isEmpty()) {
        mtx::events::msg::Reaction reaction;
        mtx::common::Relation rel;
        rel.rel_type = mtx::common::RelationType::Annotation;
        rel.event_id = reactedEvent.toStdString();
        rel.key      = reactionKey.toStdString();
        reaction.relations.relations.push_back(rel);

        room->sendMessageEvent(reaction, mtx::events::EventType::Reaction);

        auto recents = UserSettings::instance()->recentReactions();
        if (recents.contains(reactionKey))
            recents.removeOne(reactionKey);
        else if (recents.size() >= 6)
            recents.removeLast();
        recents.push_front(reactionKey);
        UserSettings::instance()->setRecentReactions(recents);
        // Otherwise, we have previously reacted and the reaction should be redacted
    } else {
        room->redactEvent(selfReactedEvent);
    }
}
