// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "InputBar.h"

#include <QBuffer>
#include <QClipboard>
#include <QDropEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QInputMethod>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMimeData>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QTextBoundaryFinder>
#include <QVideoFrame>
#include <QVideoSink>

#include <QRegularExpression>
#include <mtx/responses/common.hpp>
#include <mtx/responses/media.hpp>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "CombinedImagePackModel.h"
#include "Config.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "TimelineModel.h"
#include "TimelineViewManager.h"
#include "UserSettingsPage.h"
#include "Utils.h"

#include "blurhash.hpp"

static constexpr size_t INPUT_HISTORY_SIZE = 10;

std::string
threadFallbackEventId(const std::string &room_id, const std::string &thread_id)
{
    auto event_ids = cache::client()->relatedEvents(room_id, thread_id);

    std::map<uint64_t, std::string_view, std::greater<>> orderedEvents;

    for (const auto &e : event_ids) {
        if (auto index = cache::client()->getTimelineIndex(room_id, e))
            orderedEvents.emplace(*index, e);
    }

    for (const auto &[index, event_id] : orderedEvents) {
        (void)index;
        if (auto event = cache::client()->getEvent(room_id, event_id)) {
            if (mtx::accessors::relations(event.value()).thread() == thread_id)
                return std::string(event_id);
        }
    }
    return thread_id;
}

QUrl
MediaUpload::thumbnailDataUrl() const
{
    if (thumbnail_.isNull())
        return {};

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    thumbnail_.save(&buffer, "PNG");
    QString base64 = QString::fromUtf8(byteArray.toBase64());
    return QString("data:image/png;base64,") + base64;
}

bool
InputBar::tryPasteAttachment(bool fromMouse)
{
    const QMimeData *md = nullptr;

    if (fromMouse && QGuiApplication::clipboard()->supportsSelection()) {
        md = QGuiApplication::clipboard()->mimeData(QClipboard::Selection);
    } else {
        md = QGuiApplication::clipboard()->mimeData(QClipboard::Clipboard);
    }

    if (md)
        return insertMimeData(md);

    return false;
}

bool
InputBar::insertMimeData(const QMimeData *md)
{
    if (!md)
        return false;

    nhlog::ui()->debug("Got mime formats: {}",
                       md->formats().join(QStringLiteral(", ")).toStdString());
    nhlog::ui()->debug("Has image: {}", md->hasImage());
    const auto formats = md->formats().filter(QStringLiteral("/"));
    const auto image   = formats.filter(QStringLiteral("image/"), Qt::CaseInsensitive);
    const auto audio   = formats.filter(QStringLiteral("audio/"), Qt::CaseInsensitive);
    const auto video   = formats.filter(QStringLiteral("video/"), Qt::CaseInsensitive);

    if (md->hasImage()) {
        if (formats.contains(QStringLiteral("image/svg+xml"), Qt::CaseInsensitive)) {
            startUploadFromMimeData(*md, QStringLiteral("image/svg+xml"));
        } else if (formats.contains(QStringLiteral("image/png"), Qt::CaseInsensitive)) {
            startUploadFromMimeData(*md, QStringLiteral("image/png"));
        } else if (image.empty()) {
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            qvariant_cast<QImage>(md->imageData()).save(&buffer, "PNG");
            QMimeData d;
            d.setData(QStringLiteral("image/png"), ba);
            startUploadFromMimeData(d, QStringLiteral("image/png"));
        } else {
            startUploadFromMimeData(*md, image.first());
        }
    } else if (!audio.empty()) {
        startUploadFromMimeData(*md, audio.first());
    } else if (!video.empty()) {
        startUploadFromMimeData(*md, video.first());
    } else if (md->hasUrls() &&
               // NOTE(Nico): Safari, when copying the url, sends a url list. Since we only paste
               // local files, skip remote ones.
               [&md] {
                   for (auto &&u : md->urls()) {
                       if (u.isLocalFile())
                           return true;
                   }
                   return false;
               }()) {
        // Generic file path for any platform.
        for (auto &&u : md->urls()) {
            if (u.isLocalFile()) {
                startUploadFromPath(u.toLocalFile());
            }
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
            return false;
        }

        for (int i = 1; i < data.size(); ++i) {
            QUrl url{data[i]};
            if (url.isLocalFile()) {
                startUploadFromPath(url.toLocalFile());
            }
        }
    } else if (md->hasText()) {
        return false;
    } else {
        nhlog::ui()->debug("formats: {}", md->formats().join(QStringLiteral(", ")).toStdString());
        return false;
    }

    return true;
}

void
InputBar::updateTextContentProperties(const QString &t)
{
    // check for @room
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

    // check for invalid commands
    auto commandName = getCommandAndArgs(t).first;
    static const QSet<QString> validCommands{QStringLiteral("me"),
                                             QStringLiteral("react"),
                                             QStringLiteral("join"),
                                             QStringLiteral("knock"),
                                             QStringLiteral("part"),
                                             QStringLiteral("leave"),
                                             QStringLiteral("invite"),
                                             QStringLiteral("kick"),
                                             QStringLiteral("ban"),
                                             QStringLiteral("unban"),
                                             QStringLiteral("redact"),
                                             QStringLiteral("roomnick"),
                                             QStringLiteral("shrug"),
                                             QStringLiteral("fliptable"),
                                             QStringLiteral("unfliptable"),
                                             QStringLiteral("sovietflip"),
                                             QStringLiteral("clear-timeline"),
                                             QStringLiteral("reset-state"),
                                             QStringLiteral("rotate-megolm-session"),
                                             QStringLiteral("md"),
                                             QStringLiteral("cmark"),
                                             QStringLiteral("plain"),
                                             QStringLiteral("rainbow"),
                                             QStringLiteral("rainbowme"),
                                             QStringLiteral("notice"),
                                             QStringLiteral("rainbownotice"),
                                             QStringLiteral("confetti"),
                                             QStringLiteral("rainbowconfetti"),
                                             QStringLiteral("rainfall"),
                                             QStringLiteral("msgtype"),
                                             QStringLiteral("goto"),
                                             QStringLiteral("converttodm"),
                                             QStringLiteral("converttoroom")};
    bool hasInvalidCommand    = !commandName.isNull() && !validCommands.contains(commandName);
    bool hasIncompleteCommand = hasInvalidCommand && '/' + commandName == t;

    bool signalsChanged{false};
    if (containsInvalidCommand_ != hasInvalidCommand) {
        containsInvalidCommand_ = hasInvalidCommand;
        signalsChanged          = true;
    }
    if (containsIncompleteCommand_ != hasIncompleteCommand) {
        containsIncompleteCommand_ = hasIncompleteCommand;
        signalsChanged             = true;
    }
    if (currentCommand_ != commandName) {
        currentCommand_ = commandName;
        signalsChanged  = true;
    }
    if (signalsChanged) {
        emit currentCommandChanged();
        emit containsInvalidCommandChanged();
        emit containsIncompleteCommandChanged();
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

    updateTextContentProperties(QLatin1String(""));
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

        updateTextContentProperties(text_);
        // disabled, as it moves the cursor to the end
        // emit textChanged(text_);
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

    updateTextContentProperties(text());
    return text();
}

QString
InputBar::nextText()
{
    history_index_--;
    if (history_index_ >= INPUT_HISTORY_SIZE)
        history_index_ = 0;

    updateTextContentProperties(text());
    return text();
}

void
InputBar::send()
{
    QInputMethod *im = QGuiApplication::inputMethod();
    im->commit();
    if (text().trimmed().isEmpty()) {
        acceptUploads();
        return;
    }

    nhlog::ui()->debug("Send: {}", text().toStdString());

    auto wasEdit = !room->edit().isEmpty();

    auto [commandName, args] = getCommandAndArgs();
    updateTextContentProperties(text());
    if (containsIncompleteCommand_)
        return;
    if (commandName.isEmpty() || !command(commandName, args))
        message(text());

    if (!wasEdit) {
        history_.push_front(QLatin1String(""));
        setText(QLatin1String(""));
    }
}

void
InputBar::openFileSelection()
{
    const QString homeFolder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QStringList fileNames =
      QFileDialog::getOpenFileNames(nullptr, tr("Select file(s)"), homeFolder, tr("All Files (*)"));

    if (fileNames.isEmpty())
        return;

    ChatPage::instance()->timelineManager()->focusMessageInput();
    for (const auto &fileName : fileNames)
        startUploadFromPath(fileName);
}

QString
replaceMatrixToMarkdownLink(QString input)
{
    bool replaced = false;
    do {
        replaced = false;

        int endOfName = input.indexOf("](https://matrix.to/#/");
        int startOfName;
        int nestingCount = 0;
        for (startOfName = endOfName - 1; startOfName > 0; startOfName--) {
            // skip escaped chars
            if (startOfName > 0 && input[startOfName - 1] == '\\')
                continue;

            if (input[startOfName] == '[') {
                if (nestingCount <= 0)
                    break;
                else
                    nestingCount--;
            }
            if (input[startOfName] == ']')
                nestingCount++;
        }
        if (startOfName < 0 || nestingCount > 0)
            break;

        int endOfLink = input.indexOf(')', endOfName);
        int newline   = input.indexOf('\n', endOfName);
        if (endOfLink > endOfName && (newline == -1 || endOfLink < newline)) {
            auto name = input.mid(startOfName + 1, endOfName - startOfName - 1);
            name.replace("\\[", "[");
            name.replace("\\]", "]");
            input.replace(startOfName, endOfLink - startOfName + 1, name);
            replaced = true;
        }
    } while (replaced);

    return input;
}

mtx::common::Relations
InputBar::generateRelations() const
{
    mtx::common::Relations relations;
    if (!room->thread().isEmpty()) {
        relations.relations.push_back(
          {mtx::common::RelationType::Thread, room->thread().toStdString()});
        if (room->reply().isEmpty())
            relations.relations.push_back(
              {mtx::common::RelationType::InReplyTo,
               threadFallbackEventId(room->roomId().toStdString(), room->thread().toStdString()),
               std::nullopt,
               true});
    }
    if (!room->reply().isEmpty()) {
        relations.relations.push_back(
          {mtx::common::RelationType::InReplyTo, room->reply().toStdString()});
    }
    if (!room->edit().isEmpty()) {
        relations.relations.push_back(
          {mtx::common::RelationType::Replace, room->edit().toStdString()});
    }
    return relations;
}

void
InputBar::message(const QString &msg, MarkdownOverride useMarkdown, bool rainbowify)
{
    auto trimmed                = msg.trimmed();
    mtx::events::msg::Text text = {};
    text.body                   = trimmed.toStdString();

    // we don't want to have unexpected behavior if somebody was genuinely trying to edit a message
    // to be a regex
    if (UserSettings::instance()->sedEditing() && room->edit().isEmpty()) [[unlikely]] {
        // The bulk of this logic was shamelessly stolen from Neochat. Thanks to Carl Schwan for
        // letting us have this :)
        static const QRegularExpression sed("^s/([^/]*)/([^/]*)(/g)?$");
        auto match = sed.match(trimmed);

        if (match.hasMatch()) {
            const QString regex       = match.captured(1);
            const QString replacement = match.captured(2).toHtmlEscaped();
            const QString flags       = match.captured(3);

            std::optional<mtx::events::collections::TimelineEvents> event;
            if (!room->reply().isEmpty()) {
                auto ev = room->eventById(room->reply());
                if (ev && room->data(*ev, TimelineModel::IsEditable).toBool())
                    event = ev;
            } else {
                for (int i = 0; i < room->rowCount(); ++i) {
                    const auto idx = room->index(i);
                    if (room->data(idx, TimelineModel::IsEditable).toBool()) {
                        event = room->eventById(room->data(idx, TimelineModel::EventId).toString());
                        break;
                    }
                }
            }

            if (event) {
                auto other = room->data(*event, TimelineModel::FormattedBody).toString();
                const auto original{other};
                if (flags == "/g")
                    other.replace(regex, replacement);
                else
                    other.replace(other.indexOf(regex), regex.size(), replacement);

                // don't bother sending a pointless edit
                if (original != other) {
                    text.formatted_body = other.toStdString();
                    text.format         = "org.matrix.custom.html";
                    text.relations.relations.push_back(
                      {.rel_type = mtx::common::RelationType::Replace,
                       .event_id =
                         room->data(*event, TimelineModel::EventId).toString().toStdString()});
                    room->sendMessageEvent(text, mtx::events::EventType::RoomMessage);
                    return;
                }
            }
        }
    }

    if ((ChatPage::instance()->userSettings()->markdown() &&
         useMarkdown == MarkdownOverride::NOT_SPECIFIED) ||
        useMarkdown == MarkdownOverride::ON) {
        text.formatted_body = utils::markdownToHtml(msg, rainbowify).toStdString();
        // Remove markdown links by completer
        text.body = replaceMatrixToMarkdownLink(trimmed).toStdString();

        // Don't send formatted_body, when we don't need to
        // Specifically, if it includes no html tag and no newlines (which behave differently in
        // formatted bodies). Probably we forgot something, so this might need to expand at some
        // point.
        if (text.formatted_body.find('<') == std::string::npos &&
            text.body.find('\n') == std::string::npos)
            text.formatted_body = "";
        else
            text.format = "org.matrix.custom.html";
    } else if (useMarkdown == MarkdownOverride::CMARK) {
        // disable all markdown extensions
        text.formatted_body = utils::markdownToHtml(msg, rainbowify, true).toStdString();
        // keep everything as it was
        text.body = trimmed.toStdString();

        // always send formatted
        text.format = "org.matrix.custom.html";
    }

    text.relations = generateRelations();
    if (!room->reply().isEmpty() && room->thread().isEmpty() && room->edit().isEmpty()) {
        auto related = room->relatedInfo(room->reply());

        // Skip reply fallbacks to users who would cause a room ping with the fallback.
        // This should be fine, since in some cases the reply fallback can be omitted now and
        // the alternative is worse! On Element Android this applies to any substring, but that
        // is their bug to fix.
        if (!related.quoted_user.startsWith("@room:")) {
            QString body;
            bool firstLine = true;
            auto lines     = QStringView(related.quoted_body).split(u'\n');
            for (auto line : qAsConst(lines)) {
                if (firstLine) {
                    firstLine = false;
                    body      = QStringLiteral("> <%1> %2\n").arg(related.quoted_user, line);
                } else {
                    body += QStringLiteral("> %1\n").arg(line);
                }
            }

            text.body =
              QStringLiteral("%1\n%2").arg(body, QString::fromStdString(text.body)).toStdString();

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
        }
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
        emote.body = replaceMatrixToMarkdownLink(msg.trimmed()).toStdString();
    }

    emote.relations = generateRelations();

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
        notice.body = replaceMatrixToMarkdownLink(msg.trimmed()).toStdString();
    }

    notice.relations = generateRelations();

    room->sendMessageEvent(notice, mtx::events::EventType::RoomMessage);
}

void
InputBar::confetti(const QString &body, bool rainbowify)
{
    auto html = utils::markdownToHtml(body, rainbowify);

    mtx::events::msg::ElementEffect confetti;
    confetti.msgtype = "nic.custom.confetti";
    confetti.body    = body.trimmed().toStdString();

    if (html != body.trimmed().toHtmlEscaped() &&
        ChatPage::instance()->userSettings()->markdown()) {
        confetti.formatted_body = html.toStdString();
        confetti.format         = "org.matrix.custom.html";
        // Remove markdown links by completer
        confetti.body = replaceMatrixToMarkdownLink(body.trimmed()).toStdString();
    }

    confetti.relations = generateRelations();

    room->sendMessageEvent(confetti, mtx::events::EventType::RoomMessage);
}

void
InputBar::rainfall(const QString &body)
{
    auto html = utils::markdownToHtml(body);

    mtx::events::msg::Unknown rain;
    rain.msgtype = "io.element.effect.rainfall";
    rain.body    = body.trimmed().toStdString();

    if (html != body.trimmed().toHtmlEscaped() &&
        ChatPage::instance()->userSettings()->markdown()) {
        nlohmann::json j;
        j["formatted_body"] = html.toStdString();
        j["format"]         = "org.matrix.custom.html";
        rain.content        = j.dump();
        // Remove markdown links by completer
        rain.body = replaceMatrixToMarkdownLink(body.trimmed()).toStdString();
    }

    rain.relations = generateRelations();

    room->sendMessageEvent(rain, mtx::events::EventType::RoomMessage);
}

void
InputBar::customMsgtype(const QString &msgtype, const QString &body)
{
    auto html = utils::markdownToHtml(body);

    mtx::events::msg::Unknown msg;
    msg.msgtype = msgtype.toStdString();
    msg.body    = body.trimmed().toStdString();

    if (html != body.trimmed().toHtmlEscaped() &&
        ChatPage::instance()->userSettings()->markdown()) {
        nlohmann::json j;
        j["formatted_body"] = html.toStdString();
        j["format"]         = "org.matrix.custom.html";
        msg.content         = j.dump();
        // Remove markdown links by completer
        msg.body = replaceMatrixToMarkdownLink(body.trimmed()).toStdString();
    }

    msg.relations = generateRelations();

    room->sendMessageEvent(msg, mtx::events::EventType::RoomMessage);
}

void
InputBar::image(const QString &filename,
                const std::optional<mtx::crypto::EncryptedFile> &file,
                const QString &url,
                const QString &mime,
                uint64_t dsize,
                const QSize &dimensions,
                const std::optional<mtx::crypto::EncryptedFile> &thumbnailEncryptedFile,
                const QString &thumbnailUrl,
                uint64_t thumbnailSize,
                const QSize &thumbnailDimensions,
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

    if (!thumbnailUrl.isEmpty()) {
        if (thumbnailEncryptedFile)
            image.info.thumbnail_file = thumbnailEncryptedFile;
        else
            image.info.thumbnail_url = thumbnailUrl.toStdString();

        image.info.thumbnail_info.h        = thumbnailDimensions.height();
        image.info.thumbnail_info.w        = thumbnailDimensions.width();
        image.info.thumbnail_info.size     = thumbnailSize;
        image.info.thumbnail_info.mimetype = "image/png";
    }

    image.relations = generateRelations();

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

    file.relations = generateRelations();

    room->sendMessageEvent(file, mtx::events::EventType::RoomMessage);
}

void
InputBar::audio(const QString &filename,
                const std::optional<mtx::crypto::EncryptedFile> &file,
                const QString &url,
                const QString &mime,
                uint64_t dsize,
                uint64_t duration)
{
    mtx::events::msg::Audio audio;
    audio.info.mimetype = mime.toStdString();
    audio.info.size     = dsize;
    audio.body          = filename.toStdString();
    audio.url           = url.toStdString();

    if (duration > 0)
        audio.info.duration = duration;

    if (file)
        audio.file = file;
    else
        audio.url = url.toStdString();

    audio.relations = generateRelations();

    room->sendMessageEvent(audio, mtx::events::EventType::RoomMessage);
}

void
InputBar::video(const QString &filename,
                const std::optional<mtx::crypto::EncryptedFile> &file,
                const QString &url,
                const QString &mime,
                uint64_t dsize,
                uint64_t duration,
                const QSize &dimensions,
                const std::optional<mtx::crypto::EncryptedFile> &thumbnailEncryptedFile,
                const QString &thumbnailUrl,
                uint64_t thumbnailSize,
                const QSize &thumbnailDimensions,
                const QString &blurhash)
{
    mtx::events::msg::Video video;
    video.info.mimetype = mime.toStdString();
    video.info.size     = dsize;
    video.info.blurhash = blurhash.toStdString();
    video.body          = filename.toStdString();

    if (duration > 0)
        video.info.duration = duration;
    if (dimensions.isValid()) {
        video.info.h = dimensions.height();
        video.info.w = dimensions.width();
    }

    if (file)
        video.file = file;
    else
        video.url = url.toStdString();

    if (!thumbnailUrl.isEmpty()) {
        if (thumbnailEncryptedFile)
            video.info.thumbnail_file = thumbnailEncryptedFile;
        else
            video.info.thumbnail_url = thumbnailUrl.toStdString();

        video.info.thumbnail_info.h        = thumbnailDimensions.height();
        video.info.thumbnail_info.w        = thumbnailDimensions.width();
        video.info.thumbnail_info.size     = thumbnailSize;
        video.info.thumbnail_info.mimetype = "image/png";
    }

    video.relations = generateRelations();

    room->sendMessageEvent(video, mtx::events::EventType::RoomMessage);
}

QPair<QString, QString>
InputBar::getCommandAndArgs(const QString &currentText) const
{
    if (!currentText.startsWith('/'))
        return {{}, currentText};

    static QRegularExpression spaceRegex(QStringLiteral("\\s"));

    int command_end = currentText.indexOf(spaceRegex);
    if (command_end == -1)
        command_end = currentText.size();
    auto name = currentText.mid(1, command_end - 1);
    auto args = currentText.mid(command_end + 1);
    if (name.isEmpty() || name == QLatin1String("/")) {
        return {{}, currentText};
    } else {
        return {name, args};
    }
}

void
InputBar::sticker(QStringList descriptor)
{
    if (descriptor.size() != 3)
        return;

    auto originalPacks = cache::client()->getImagePacks(room->roomId().toStdString(), true);

    auto source_room = descriptor[0].toStdString();
    auto state_key   = descriptor[1].toStdString();
    auto short_code  = descriptor[2].toStdString();

    for (auto &pack : originalPacks) {
        if (pack.source_room == source_room && pack.state_key == state_key &&
            pack.pack.images.contains(short_code)) {
            auto img = pack.pack.images.at(short_code);

            mtx::events::msg::StickerImage sticker{};
            sticker.info = img.info.value_or(mtx::common::ImageInfo{});
            sticker.url  = img.url;
            sticker.body = img.body.empty() ? short_code : img.body;

            // workaround for https://github.com/vector-im/element-ios/issues/2353
            sticker.info.thumbnail_url           = sticker.url;
            sticker.info.thumbnail_info.mimetype = sticker.info.mimetype;
            sticker.info.thumbnail_info.size     = sticker.info.size;
            sticker.info.thumbnail_info.h        = sticker.info.h;
            sticker.info.thumbnail_info.w        = sticker.info.w;

            sticker.relations = generateRelations();

            room->sendMessageEvent(sticker, mtx::events::EventType::Sticker);
            break;
        }
    }
}

bool
InputBar::command(const QString &command, QString args)
{
    if (command == QLatin1String("me")) {
        emote(args, false);
    } else if (command == QLatin1String("react")) {
        auto eventId = room->reply();
        if (!eventId.isEmpty())
            reaction(eventId, args.trimmed());
    } else if (command == QLatin1String("join")) {
        ChatPage::instance()->joinRoom(args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("knock")) {
        ChatPage::instance()->knockRoom(args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("part") || command == QLatin1String("leave")) {
        ChatPage::instance()->timelineManager()->openLeaveRoomDialog(room->roomId(), args);
    } else if (command == QLatin1String("invite")) {
        ChatPage::instance()->inviteUser(
          room->roomId(), args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("kick")) {
        ChatPage::instance()->kickUser(
          room->roomId(), args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("ban")) {
        ChatPage::instance()->banUser(
          room->roomId(), args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("unban")) {
        ChatPage::instance()->unbanUser(
          room->roomId(), args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("redact")) {
        if (args.startsWith('@')) {
            room->redactAllFromUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (args.startsWith('$')) {
            room->redactEvent(args.section(' ', 0, 0), args.section(' ', 1, -1));
        }
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
    } else if (command == QLatin1String("cmark")) {
        message(args, MarkdownOverride::CMARK);
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
    } else if (command == QLatin1String("confetti")) {
        confetti(args, false);
    } else if (command == QLatin1String("rainbowconfetti")) {
        confetti(args, true);
    } else if (command == QLatin1String("rainfall")) {
        rainfall(args);
    } else if (command == QLatin1String("msgtype")) {
        customMsgtype(args.section(' ', 0, 0), args.section(' ', 1, -1));
    } else if (command == QLatin1String("goto")) {
        // Goto has three different modes:
        // 1 - Going directly to a given event ID
        if (args[0] == '$') {
            room->showEvent(args);
            return true;
        }
        // 2 - Going directly to a given message index
        if (args[0] >= '0' && args[0] <= '9') {
            room->showEvent(args);
            return true;
        }
        // 3 - Matrix URI handler, as if you clicked the URI
        if (ChatPage::instance()->handleMatrixUri(args)) {
            return true;
        }
        nhlog::net()->error("Could not resolve goto: {}", args.toStdString());
    } else if (command == QLatin1String("converttodm")) {
        utils::markRoomAsDirect(this->room->roomId(),
                                cache::getMembers(this->room->roomId().toStdString(), 0, -1));
    } else if (command == QLatin1String("converttoroom")) {
        utils::removeDirectFromRoom(this->room->roomId());
    } else {
        return false;
    }

    return true;
}

MediaUpload::MediaUpload(std::unique_ptr<QIODevice> source_,
                         const QString &mimetype,
                         const QString &originalFilename,
                         bool encrypt,
                         QObject *parent)
  : QObject(parent)
  , source(std::move(source_))
  , mimetype_(std::move(mimetype))
  , originalFilename_(QFileInfo(originalFilename).fileName())
  , encrypt_(encrypt)
{
    mimeClass_ = mimetype_.left(mimetype_.indexOf(u'/'));

    if (!source->isOpen())
        source->open(QIODevice::ReadOnly);

    data = source->readAll();
    source->reset();

    if (!data.size()) {
        nhlog::ui()->warn("Attempted to upload zero-byte file?! Mimetype {}, filename {}",
                          mimetype_.toStdString(),
                          originalFilename_.toStdString());
        emit uploadFailed(this);
        return;
    }

    nhlog::ui()->debug("Mime: {}", mimetype_.toStdString());
    if (mimeClass_ == u"image") {
        QImage img = utils::readImage(data);
        setThumbnail(img.scaled(
          std::min(800, img.width()), std::min(800, img.height()), Qt::KeepAspectRatioByExpanding));

        dimensions_ = img.size();
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
        blurhash_ =
          QString::fromStdString(blurhash::encode(data_.data(), img.width(), img.height(), 4, 3));
    } else if (mimeClass_ == u"video" || mimeClass_ == u"audio") {
        auto mediaPlayer = new QMediaPlayer(this);
        mediaPlayer->setAudioOutput(nullptr);

        if (mimeClass_ == u"video") {
            auto newSurface = new QVideoSink(this);
            connect(newSurface,
                    &QVideoSink::videoFrameChanged,
                    this,
                    [this, mediaPlayer](const QVideoFrame &frame) {
                        QImage img = frame.toImage();
                        if (img.size().isEmpty())
                            return;

                        mediaPlayer->stop();

                        auto orientation =
                          mediaPlayer->metaData().value(QMediaMetaData::Orientation).toInt();
                        if (orientation == 90 || orientation == 270 || orientation == 180) {
                            img = img.transformed(QTransform().rotate(orientation),
                                                  Qt::SmoothTransformation);
                        }

                        nhlog::ui()->debug("Got image {}x{}", img.width(), img.height());

                        this->setThumbnail(img);

                        if (!dimensions_.isValid())
                            this->dimensions_ = img.size();

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
                        blurhash_ = QString::fromStdString(
                          blurhash::encode(data_.data(), img.width(), img.height(), 4, 3));
                    });
            mediaPlayer->setVideoOutput(newSurface);
        }

        connect(mediaPlayer,
                &QMediaPlayer::errorOccurred,
                this,
                [](QMediaPlayer::Error error, QString errorString) {
                    nhlog::ui()->debug("Media player error {} and errorStr {}",
                                       static_cast<int>(error),
                                       errorString.toStdString());
                });
        connect(mediaPlayer,
                &QMediaPlayer::mediaStatusChanged,
                [mediaPlayer](QMediaPlayer::MediaStatus status) {
                    nhlog::ui()->debug("Media player status {} and error {}",
                                       static_cast<int>(status),
                                       static_cast<int>(mediaPlayer->error()));
                });
        connect(mediaPlayer, &QMediaPlayer::metaDataChanged, this, [this, mediaPlayer]() {
            nhlog::ui()->debug("Got metadata {}");

            if (mediaPlayer->duration() > 0)
                this->duration_ = mediaPlayer->duration();

            auto dimensions = mediaPlayer->metaData().value(QMediaMetaData::Resolution).toSize();
            if (!dimensions.isEmpty()) {
                dimensions_ = dimensions;
                auto orientation =
                  mediaPlayer->metaData().value(QMediaMetaData::Orientation).toInt();
                if (orientation == 90 || orientation == 270) {
                    dimensions_.transpose();
                }
            }
        });
        connect(
          mediaPlayer, &QMediaPlayer::durationChanged, this, [this, mediaPlayer](qint64 duration) {
              if (duration > 0) {
                  this->duration_ = mediaPlayer->duration();
                  if (mimeClass_ == u"audio")
                      mediaPlayer->stop();
              }
              nhlog::ui()->debug("Duration changed {}", duration);
          });

        auto originalFile = qobject_cast<QFile *>(source.get());

        mediaPlayer->setSourceDevice(
          source.get(), QUrl(originalFile ? originalFile->fileName() : originalFilename_));

        mediaPlayer->play();
    }
}

void
MediaUpload::startUpload()
{
    if (!thumbnail_.isNull() && thumbnailUrl_.isEmpty()) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        thumbnail_.save(&buffer, "PNG", 0);
        if (ba.size() >= (data.size() - data.size() / 10)) {
            nhlog::ui()->info(
              "Thumbnail is not a lot smaller than original image, not uploading it");
            nhlog::ui()->debug(
              "\n    Image size: {:9d}\nThumbnail size: {:9d}", data.size(), ba.size());
        } else {
            auto payload = std::string(ba.data(), ba.size());
            if (encrypt_) {
                mtx::crypto::BinaryBuf buf;
                std::tie(buf, thumbnailEncryptedFile) =
                  mtx::crypto::encrypt_file(std::move(payload));
                payload = mtx::crypto::to_string(buf);
            }
            thumbnailSize_ = payload.size();

            http::client()->upload(
              payload,
              encryptedFile ? "application/octet-stream" : "image/png",
              "",
              [this](const mtx::responses::ContentURI &res, mtx::http::RequestErr err) mutable {
                  if (err) {
                      emit ChatPage::instance()->showNotification(
                        tr("Failed to upload media. Please try again."));
                      nhlog::net()->warn("failed to upload media: {} {} ({})",
                                         err->matrix_error.error,
                                         to_string(err->matrix_error.errcode),
                                         static_cast<int>(err->status_code));
                      thumbnail_ = QImage();
                      startUpload();
                      return;
                  }

                  thumbnailUrl_ = QString::fromStdString(res.content_uri);
                  if (thumbnailEncryptedFile)
                      thumbnailEncryptedFile->url = res.content_uri;

                  startUpload();
              });
            return;
        }
    }

    auto payload = std::string(data.data(), data.size());
    if (encrypt_) {
        mtx::crypto::BinaryBuf buf;
        std::tie(buf, encryptedFile) = mtx::crypto::encrypt_file(std::move(payload));
        payload                      = mtx::crypto::to_string(buf);
    }
    size_ = payload.size();

    http::client()->upload(
      payload,
      encryptedFile ? "application/octet-stream" : mimetype_.toStdString(),
      encrypt_ ? "" : originalFilename_.toStdString(),
      [this](const mtx::responses::ContentURI &res, mtx::http::RequestErr err) mutable {
          if (err) {
              emit ChatPage::instance()->showNotification(
                tr("Failed to upload media. Please try again."));
              nhlog::net()->warn("failed to upload media: {} {} ({})",
                                 err->matrix_error.error,
                                 to_string(err->matrix_error.errcode),
                                 static_cast<int>(err->status_code));
              emit uploadFailed(this);
              return;
          }

          auto url = QString::fromStdString(res.content_uri);
          if (encryptedFile)
              encryptedFile->url = res.content_uri;

          emit uploadComplete(this, std::move(url));
      });
}

void
InputBar::finalizeUpload(MediaUpload *upload, const QString &url)
{
    auto mime          = upload->mimetype();
    auto filename      = upload->filename();
    auto mimeClass     = upload->mimeClass();
    auto size          = upload->size();
    auto encryptedFile = upload->encryptedFile_();
    if (mimeClass == u"image")
        image(filename,
              encryptedFile,
              url,
              mime,
              size,
              upload->dimensions(),
              upload->thumbnailEncryptedFile_(),
              upload->thumbnailUrl(),
              upload->thumbnailSize(),
              upload->thumbnailImg().size(),
              upload->blurhash());
    else if (mimeClass == u"audio")
        audio(filename, encryptedFile, url, mime, size, upload->duration());
    else if (mimeClass == u"video")
        video(filename,
              encryptedFile,
              url,
              mime,
              size,
              upload->duration(),
              upload->dimensions(),
              upload->thumbnailEncryptedFile_(),
              upload->thumbnailUrl(),
              upload->thumbnailSize(),
              upload->thumbnailImg().size(),
              upload->blurhash());
    else
        file(filename, encryptedFile, url, mime, size);

    removeRunUpload(upload);
}

void
InputBar::removeRunUpload(MediaUpload *upload)
{
    auto it = std::find_if(runningUploads.begin(),
                           runningUploads.end(),
                           [upload](const UploadHandle &h) { return h.get() == upload; });
    if (it != runningUploads.end())
        runningUploads.erase(it);

    if (runningUploads.empty())
        setUploading(false);
    else
        runningUploads.front()->startUpload();
}

void
InputBar::startUploadFromPath(const QString &path)
{
    if (path.isEmpty())
        return;

    auto file = std::make_unique<QFile>(path);

    if (!file->open(QIODevice::ReadOnly)) {
        nhlog::ui()->warn(
          "Failed to open file ({}): {}", path.toStdString(), file->errorString().toStdString());
        return;
    }

    QMimeDatabase db;
    auto mime = db.mimeTypeForFileNameAndData(path, file.get());

    startUpload(std::move(file), path, mime.name());
}

void
InputBar::startUploadFromMimeData(const QMimeData &source, const QString &format)
{
    auto file = std::make_unique<QBuffer>();
    file->setData(source.data(format));

    if (!file->open(QIODevice::ReadOnly)) {
        nhlog::ui()->warn("Failed to open buffer: {}", file->errorString().toStdString());
        return;
    }

    QMimeDatabase db;
    auto mime        = db.mimeTypeForName(format);
    auto suffix      = mime.preferredSuffix();
    QString filename = QStringLiteral("clipboard");

    startUpload(std::move(file), suffix.isEmpty() ? filename : (filename + "." + suffix), format);
}
void
InputBar::startUpload(std::unique_ptr<QIODevice> dev, const QString &orgPath, const QString &format)
{
    auto upload =
      UploadHandle(new MediaUpload(std::move(dev), format, orgPath, room->isEncrypted(), this));
    connect(upload.get(), &MediaUpload::uploadComplete, this, &InputBar::finalizeUpload);
    // TODO(Nico): Show a retry option
    connect(upload.get(), &MediaUpload::uploadFailed, this, [this](MediaUpload *up) {
        ChatPage::instance()->showNotification(tr("Upload of '%1' failed").arg(up->filename()));
        removeRunUpload(up);
    });

    unconfirmedUploads.push_back(std::move(upload));

    nhlog::ui()->debug("Uploads {}", unconfirmedUploads.size());
    emit uploadsChanged();
}

void
InputBar::acceptUploads()
{
    if (unconfirmedUploads.empty())
        return;

    bool wasntRunning = runningUploads.empty();
    runningUploads.insert(runningUploads.end(),
                          std::make_move_iterator(unconfirmedUploads.begin()),
                          std::make_move_iterator(unconfirmedUploads.end()));
    unconfirmedUploads.clear();
    emit uploadsChanged();

    if (wasntRunning) {
        setUploading(true);
        runningUploads.front()->startUpload();
    }
}

void
InputBar::declineUploads()
{
    unconfirmedUploads.clear();
    emit uploadsChanged();
}

QVariantList
InputBar::uploads() const
{
    QVariantList l;
    l.reserve((int)unconfirmedUploads.size());

    for (auto &e : unconfirmedUploads)
        l.push_back(QVariant::fromValue(e.get()));
    return l;
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
