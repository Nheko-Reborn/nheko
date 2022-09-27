// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Utils.h"

#include <QApplication>
#include <QBuffer>
#include <QComboBox>
#include <QCryptographicHash>
#include <QGuiApplication>
#include <QImageReader>
#include <QProcessEnvironment>
#include <QScreen>
#include <QSettings>
#include <QStringBuilder>
#include <QTextBoundaryFinder>
#include <QTextDocument>
#include <QWindow>
#include <QXmlStreamReader>

#include <array>
#include <cmath>
#include <variant>

#include <cmark.h>

#include "Cache.h"
#include "Cache_p.h"
#include "Config.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"

using TimelineEvent = mtx::events::collections::TimelineEvents;

template<class T, class Event>
static DescInfo
createDescriptionInfo(const Event &event, const QString &localUser, const QString &displayName)
{
    const auto msg    = std::get<T>(event);
    const auto sender = QString::fromStdString(msg.sender);

    const auto username = displayName;
    const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);
    auto body           = utils::event_body(event).trimmed();
    if (mtx::accessors::relations(event).reply_to())
        body = QString::fromStdString(utils::stripReplyFromBody(body.toStdString()));

    return DescInfo{QString::fromStdString(msg.event_id),
                    sender,
                    utils::messageDescription<T>(username, body, sender == localUser),
                    utils::descriptiveTime(ts),
                    msg.origin_server_ts,
                    ts};
}

std::string
utils::stripReplyFromBody(const std::string &bodyi)
{
    QString body = QString::fromStdString(bodyi);
    if (body.startsWith(QLatin1String("> <"))) {
        auto segments = body.split('\n');
        while (!segments.isEmpty() && segments.begin()->startsWith('>'))
            segments.erase(segments.begin());
        if (!segments.empty() && segments.first().isEmpty())
            segments.erase(segments.begin());
        body = segments.join('\n');
    }

    body.replace(QLatin1String("@room"), QString::fromUtf8("@\u2060room"));
    return body.toStdString();
}

std::string
utils::stripReplyFromFormattedBody(const std::string &formatted_bodyi)
{
    QString formatted_body = QString::fromStdString(formatted_bodyi);
    formatted_body.remove(QRegularExpression(QStringLiteral("<mx-reply>.*</mx-reply>"),
                                             QRegularExpression::DotMatchesEverythingOption));
    formatted_body.replace(QLatin1String("@room"), QString::fromUtf8("@\u2060room"));
    return formatted_body.toStdString();
}

RelatedInfo
utils::stripReplyFallbacks(const TimelineEvent &event, std::string id, QString room_id_)
{
    RelatedInfo related   = {};
    related.quoted_user   = QString::fromStdString(mtx::accessors::sender(event));
    related.related_event = std::move(id);
    related.type          = mtx::accessors::msg_type(event);

    // get body, strip reply fallback, then transform the event to text, if it is a media event
    // etc
    related.quoted_body = QString::fromStdString(mtx::accessors::body(event));
    related.quoted_body =
      QString::fromStdString(stripReplyFromBody(related.quoted_body.toStdString()));
    related.quoted_body = utils::getQuoteBody(related);

    // get quoted body and strip reply fallback
    related.quoted_formatted_body = mtx::accessors::formattedBodyWithFallback(event);
    related.quoted_formatted_body = QString::fromStdString(
      stripReplyFromFormattedBody(related.quoted_formatted_body.toStdString()));
    related.room = room_id_;

    return related;
}

QString
utils::localUser()
{
    return QString::fromStdString(http::client()->user_id().to_string());
}

bool
utils::codepointIsEmoji(uint code)
{
    // TODO: Be more precise here.
    return (code >= 0x2600 && code <= 0x27bf) || (code >= 0x2b00 && code <= 0x2bff) ||
           (code >= 0x1f000 && code <= 0x1faff) || code == 0x200d || code == 0xfe0f;
}

QString
utils::replaceEmoji(const QString &body)
{
    QString fmtBody;
    fmtBody.reserve(body.size());

    QVector<uint> utf32_string = body.toUcs4();

    bool insideFontBlock = false;
    for (auto &code : utf32_string) {
        if (utils::codepointIsEmoji(code)) {
            if (!insideFontBlock) {
                fmtBody += QStringLiteral("<font face=\"") % UserSettings::instance()->emojiFont() %
                           QStringLiteral("\">");
                insideFontBlock = true;
            } else if (code == 0xfe0f) {
                // BUG(Nico):
                // Workaround https://bugreports.qt.io/browse/QTBUG-97401
                // See also https://github.com/matrix-org/matrix-react-sdk/pull/1458/files
                // Nheko bug: https://github.com/Nheko-Reborn/nheko/issues/439
                continue;
            }
        } else {
            if (insideFontBlock) {
                fmtBody += QStringLiteral("</font>");
                insideFontBlock = false;
            }
        }
        if (QChar::requiresSurrogates(code)) {
            QChar emoji[] = {static_cast<ushort>(QChar::highSurrogate(code)),
                             static_cast<ushort>(QChar::lowSurrogate(code))};
            fmtBody.append(emoji, 2);
        } else {
            fmtBody.append(QChar(static_cast<ushort>(code)));
        }
    }
    if (insideFontBlock) {
        fmtBody += QStringLiteral("</font>");
    }

    return fmtBody;
}

void
utils::setScaleFactor(float factor)
{
    if (factor < 1 || factor > 3)
        return;

    QSettings settings;
    settings.setValue(QStringLiteral("settings/scale_factor"), factor);
}

float
utils::scaleFactor()
{
    QSettings settings;
    return settings.value(QStringLiteral("settings/scale_factor"), -1).toFloat();
}

QString
utils::descriptiveTime(const QDateTime &then)
{
    const auto now  = QDateTime::currentDateTime();
    const auto days = then.daysTo(now);

    if (days == 0)
        return QLocale::system().toString(then.time(), QLocale::ShortFormat);
    else if (days < 2)
        return QString(QCoreApplication::translate("descriptiveTime", "Yesterday"));
    else if (days < 7)
        return then.toString(QStringLiteral("dddd"));

    return QLocale::system().toString(then.date(), QLocale::ShortFormat);
}

DescInfo
utils::getMessageDescription(const TimelineEvent &event,
                             const QString &localUser,
                             const QString &displayName)
{
    using Audio      = mtx::events::RoomEvent<mtx::events::msg::Audio>;
    using Emote      = mtx::events::RoomEvent<mtx::events::msg::Emote>;
    using File       = mtx::events::RoomEvent<mtx::events::msg::File>;
    using Image      = mtx::events::RoomEvent<mtx::events::msg::Image>;
    using Notice     = mtx::events::RoomEvent<mtx::events::msg::Notice>;
    using Text       = mtx::events::RoomEvent<mtx::events::msg::Text>;
    using Video      = mtx::events::RoomEvent<mtx::events::msg::Video>;
    using CallInvite = mtx::events::RoomEvent<mtx::events::voip::CallInvite>;
    using CallAnswer = mtx::events::RoomEvent<mtx::events::voip::CallAnswer>;
    using CallHangUp = mtx::events::RoomEvent<mtx::events::voip::CallHangUp>;
    using Encrypted  = mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>;

    if (std::holds_alternative<Audio>(event)) {
        return createDescriptionInfo<Audio>(event, localUser, displayName);
    } else if (std::holds_alternative<Emote>(event)) {
        return createDescriptionInfo<Emote>(event, localUser, displayName);
    } else if (std::holds_alternative<File>(event)) {
        return createDescriptionInfo<File>(event, localUser, displayName);
    } else if (std::holds_alternative<Image>(event)) {
        return createDescriptionInfo<Image>(event, localUser, displayName);
    } else if (std::holds_alternative<Notice>(event)) {
        return createDescriptionInfo<Notice>(event, localUser, displayName);
    } else if (std::holds_alternative<Text>(event)) {
        return createDescriptionInfo<Text>(event, localUser, displayName);
    } else if (std::holds_alternative<Video>(event)) {
        return createDescriptionInfo<Video>(event, localUser, displayName);
    } else if (std::holds_alternative<CallInvite>(event)) {
        return createDescriptionInfo<CallInvite>(event, localUser, displayName);
    } else if (std::holds_alternative<CallAnswer>(event)) {
        return createDescriptionInfo<CallAnswer>(event, localUser, displayName);
    } else if (std::holds_alternative<CallHangUp>(event)) {
        return createDescriptionInfo<CallHangUp>(event, localUser, displayName);
    } else if (std::holds_alternative<mtx::events::Sticker>(event)) {
        return createDescriptionInfo<mtx::events::Sticker>(event, localUser, displayName);
    } else if (auto msg = std::get_if<Encrypted>(&event); msg != nullptr) {
        const auto sender = QString::fromStdString(msg->sender);

        const auto username = displayName;
        const auto ts       = QDateTime::fromMSecsSinceEpoch(msg->origin_server_ts);

        DescInfo info;
        info.userid = sender;
        info.body   = QStringLiteral(" %1").arg(
          messageDescription<Encrypted>(username, QLatin1String(""), sender == localUser));
        info.timestamp       = msg->origin_server_ts;
        info.descriptiveTime = utils::descriptiveTime(ts);
        info.event_id        = QString::fromStdString(msg->event_id);
        info.datetime        = ts;

        return info;
    }

    return DescInfo{};
}

QString
utils::firstChar(const QString &input)
{
    if (input.isEmpty())
        return input;

    for (auto const &c : input.toStdU32String()) {
        if (QString::fromUcs4(&c, 1) != QStringLiteral("#"))
            return QString::fromUcs4(&c, 1).toUpper();
    }

    return QString::fromUcs4(&input.toUcs4().at(0), 1).toUpper();
}

QString
utils::humanReadableFileSize(uint64_t bytes)
{
    constexpr static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    constexpr static const int length    = sizeof(units) / sizeof(units[0]);

    int u       = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && u < length) {
        ++u;
        size /= 1024.0;
    }

    return QString::number(size, 'g', 4) + ' ' + units[u];
}

int
utils::levenshtein_distance(const std::string &s1, const std::string &s2)
{
    const auto nlen = s1.size();
    const auto hlen = s2.size();

    if (hlen == 0)
        return -1;
    if (nlen == 1)
        return (int)s2.find(s1);

    std::vector<int> row1(hlen + 1, 0);

    for (size_t i = 0; i < nlen; ++i) {
        std::vector<int> row2(1, (int)i + 1);

        for (size_t j = 0; j < hlen; ++j) {
            const int cost = s1[i] != s2[j];
            row2.push_back(std::min(row1[j + 1] + 1, std::min(row2[j] + 1, row1[j] + cost)));
        }

        row1.swap(row2);
    }

    return *std::min_element(row1.begin(), row1.end());
}

QString
utils::event_body(const mtx::events::collections::TimelineEvents &e)
{
    using namespace mtx::events;
    if (auto ev = std::get_if<RoomEvent<msg::Audio>>(&e); ev != nullptr)
        return QString::fromStdString(ev->content.body);
    if (auto ev = std::get_if<RoomEvent<msg::Emote>>(&e); ev != nullptr)
        return QString::fromStdString(ev->content.body);
    if (auto ev = std::get_if<RoomEvent<msg::File>>(&e); ev != nullptr)
        return QString::fromStdString(ev->content.body);
    if (auto ev = std::get_if<RoomEvent<msg::Image>>(&e); ev != nullptr)
        return QString::fromStdString(ev->content.body);
    if (auto ev = std::get_if<RoomEvent<msg::Notice>>(&e); ev != nullptr)
        return QString::fromStdString(ev->content.body);
    if (auto ev = std::get_if<RoomEvent<msg::Text>>(&e); ev != nullptr)
        return QString::fromStdString(ev->content.body);
    if (auto ev = std::get_if<RoomEvent<msg::Video>>(&e); ev != nullptr)
        return QString::fromStdString(ev->content.body);

    return QString();
}

QPixmap
utils::scaleImageToPixmap(const QImage &img, int size)
{
    if (img.isNull())
        return QPixmap();

    // Deprecated in 5.13: const double sz =
    //  std::ceil(QApplication::desktop()->screen()->devicePixelRatioF() * (double)size);
    const double sz =
      std::ceil(QGuiApplication::primaryScreen()->devicePixelRatio() * (double)size);
    return QPixmap::fromImage(img.scaled(sz, sz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

QPixmap
utils::scaleDown(uint64_t maxWidth, uint64_t maxHeight, const QPixmap &source)
{
    if (source.isNull())
        return QPixmap();

    const double widthRatio     = (double)maxWidth / (double)source.width();
    const double heightRatio    = (double)maxHeight / (double)source.height();
    const double minAspectRatio = std::min(widthRatio, heightRatio);

    // Size of the output image.
    int w, h = 0;

    if (minAspectRatio > 1) {
        w = source.width();
        h = source.height();
    } else {
        w = source.width() * minAspectRatio;
        h = source.height() * minAspectRatio;
    }

    return source.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QString
utils::mxcToHttp(const QUrl &url, const QString &server, int port)
{
    auto mxcParts = mtx::client::utils::parse_mxc_url(url.toString().toStdString());

    return QStringLiteral("https://%1:%2/_matrix/media/r0/download/%3/%4")
      .arg(server)
      .arg(port)
      .arg(QString::fromStdString(mxcParts.server), QString::fromStdString(mxcParts.media_id));
}

QString
utils::humanReadableFingerprint(const std::string &ed25519)
{
    return humanReadableFingerprint(QString::fromStdString(ed25519));
}
QString
utils::humanReadableFingerprint(const QString &ed25519)
{
    QString fingerprint;
    for (int i = 0; i < ed25519.length(); i = i + 4) {
        fingerprint.append(QStringView(ed25519).mid(i, 4));
        if (i > 0 && i == 20)
            fingerprint.append('\n');
        else if (i < ed25519.length())
            fingerprint.append(' ');
    }
    return fingerprint;
}

QString
utils::linkifyMessage(const QString &body)
{
    // Convert to valid XML.
    auto doc = body;
    doc.replace(conf::strings::url_regex, conf::strings::url_html);
    doc.replace(
      QRegularExpression(QStringLiteral("\\b(?<![\"'])(?>(matrix:[\\S]{5,}))(?![\"'])\\b")),
      conf::strings::url_html);

    return doc;
}

QString
utils::escapeBlacklistedHtml(const QString &rawStr)
{
    static const std::set<QByteArray> allowedTags = {
      "font",       "/font",       "del",     "/del",    "h1",    "/h1",    "h2",     "/h2",
      "h3",         "/h3",         "h4",      "/h4",     "h5",    "/h5",    "h6",     "/h6",
      "blockquote", "/blockquote", "p",       "/p",      "a",     "/a",     "ul",     "/ul",
      "ol",         "/ol",         "sup",     "/sup",    "sub",   "/sub",   "li",     "/li",
      "b",          "/b",          "i",       "/i",      "u",     "/u",     "strong", "/strong",
      "em",         "/em",         "strike",  "/strike", "code",  "/code",  "hr",     "/hr",
      "br",         "br/",         "div",     "/div",    "table", "/table", "thead",  "/thead",
      "tbody",      "/tbody",      "tr",      "/tr",     "th",    "/th",    "td",     "/td",
      "caption",    "/caption",    "pre",     "/pre",    "span",  "/span",  "img",    "/img",
      "details",    "/details",    "summary", "/summary"};
    constexpr static const std::array tagNameEnds   = {' ', '>'};
    constexpr static const std::array attrNameEnds  = {' ', '>', '=', '\t', '\r', '\n', '/', '\f'};
    constexpr static const std::array attrValueEnds = {' ', '\t', '\r', '\n', '\f', '>'};
    constexpr static const std::array spaceChars    = {' ', '\t', '\r', '\n', '\f'};

    QByteArray data = rawStr.toUtf8();
    QByteArray buffer;
    const int length = data.size();
    buffer.reserve(length);
    const auto end = data.cend();
    for (auto pos = data.cbegin(); pos < end;) {
        auto tagStart = std::find(pos, end, '<');
        buffer.append(pos, tagStart - pos);
        if (tagStart == end)
            break;

        const auto tagNameStart = tagStart + 1;
        const auto tagNameEnd =
          std::find_first_of(tagNameStart, end, tagNameEnds.begin(), tagNameEnds.end());

        if (allowedTags.find(QByteArray(tagNameStart, tagNameEnd - tagNameStart).toLower()) ==
            allowedTags.end()) {
            // not allowed -> escape
            buffer.append("&lt;");
            pos = tagNameStart;
            continue;
        } else {
            buffer.append(tagStart, tagNameEnd - tagStart);

            pos = tagNameEnd;

            if (tagNameEnd != end) {
                auto attrStart = tagNameEnd;
                auto attrsEnd  = std::find(attrStart, end, '>');
                if (*(attrsEnd - 1) == '/')
                    attrsEnd -= 1;

                pos = attrsEnd;

                auto consumeSpaces = [attrsEnd](auto p) {
                    while (p < attrsEnd &&
                           std::find(spaceChars.begin(), spaceChars.end(), *p) != spaceChars.end())
                        p++;
                    return p;
                };

                attrStart = consumeSpaces(attrStart);

                while (attrStart < attrsEnd) {
                    auto attrEnd = std::find_first_of(
                      attrStart, attrsEnd, attrNameEnds.begin(), attrNameEnds.end());

                    auto attrName = QByteArray(attrStart, attrEnd - attrStart).toLower();

                    auto sanitizeValue = [&attrName](QByteArray val) {
                        if (attrName == QByteArrayLiteral("src") && !val.startsWith("mxc://"))
                            return QByteArray();
                        else
                            return val;
                    };

                    attrStart = consumeSpaces(attrEnd);

                    if (attrName.isEmpty()) {
                        buffer.append(QUrl::toPercentEncoding(QString(QByteArray(attrStart, 1))));
                        attrStart++;
                        continue;
                    } else if (attrStart < attrsEnd) {
                        if (*attrStart == '=') {
                            attrStart = consumeSpaces(attrStart + 1);

                            if (attrStart < attrsEnd) {
                                // we fall through here if the value is empty to transform attr=""
                                // into attr, because otherwise we can't style it
                                if (*attrStart == '"') {
                                    attrStart += 1;
                                    auto valueEnd = std::find(attrStart, attrsEnd, '"');
                                    if (valueEnd == attrsEnd)
                                        break;

                                    auto val =
                                      sanitizeValue(QByteArray(attrStart, valueEnd - attrStart));
                                    attrStart = consumeSpaces(valueEnd + 1);
                                    if (!val.isEmpty()) {
                                        buffer.append(' ');
                                        buffer.append(attrName);
                                        buffer.append("=\"");
                                        buffer.append(val);
                                        buffer.append('"');
                                        continue;
                                    }
                                } else if (*attrStart == '\'') {
                                    attrStart += 1;
                                    auto valueEnd = std::find(attrStart, attrsEnd, '\'');
                                    if (valueEnd == attrsEnd)
                                        break;

                                    auto val =
                                      sanitizeValue(QByteArray(attrStart, valueEnd - attrStart));
                                    attrStart = consumeSpaces(valueEnd + 1);
                                    if (!val.isEmpty()) {
                                        buffer.append(' ');
                                        buffer.append(attrName);
                                        buffer.append("=\'");
                                        buffer.append(val);
                                        buffer.append('\'');
                                        continue;
                                    }
                                } else {
                                    auto valueEnd = std::find_first_of(attrStart,
                                                                       attrsEnd,
                                                                       attrValueEnds.begin(),
                                                                       attrValueEnds.end());
                                    auto val =
                                      sanitizeValue(QByteArray(attrStart, valueEnd - attrStart));
                                    attrStart = consumeSpaces(valueEnd);

                                    if (val.contains('"'))
                                        continue;

                                    buffer.append(' ');
                                    buffer.append(attrName);
                                    buffer.append("=\"");
                                    buffer.append(val);
                                    buffer.append('"');
                                    continue;
                                }
                            }
                        }
                    }

                    buffer.append(' ');
                    buffer.append(attrName);
                }
            }
        }
    }

    return QString::fromUtf8(buffer);
}

QString
utils::markdownToHtml(const QString &text, bool rainbowify)
{
    const auto str         = text.toUtf8();
    cmark_node *const node = cmark_parse_document(str.constData(), str.size(), CMARK_OPT_UNSAFE);

    if (rainbowify) {
        // create iterator over node
        cmark_iter *iter = cmark_iter_new(node);

        // First loop to get total text length
        int textLen = 0;
        while (cmark_iter_next(iter) != CMARK_EVENT_DONE) {
            cmark_node *cur = cmark_iter_get_node(iter);
            // only text nodes (no code or semilar)
            if (cmark_node_get_type(cur) != CMARK_NODE_TEXT)
                continue;
            // count up by length of current node's text
            QTextBoundaryFinder tbf(QTextBoundaryFinder::BoundaryType::Grapheme,
                                    QString(cmark_node_get_literal(cur)));
            while (tbf.toNextBoundary() != -1)
                textLen++;
        }

        // create new iter to start over
        cmark_iter_free(iter);
        iter = cmark_iter_new(node);

        // Second loop to rainbowify
        int charIdx = 0;
        while (cmark_iter_next(iter) != CMARK_EVENT_DONE) {
            cmark_node *cur = cmark_iter_get_node(iter);
            // only text nodes (no code or semilar)
            if (cmark_node_get_type(cur) != CMARK_NODE_TEXT)
                continue;

            // get text in current node
            QString nodeText(cmark_node_get_literal(cur));
            // create buffer to append rainbow text to
            QString buf;
            int boundaryStart = 0;
            int boundaryEnd   = 0;
            // use QTextBoundaryFinder to iterate ofer graphemes
            QTextBoundaryFinder tbf(QTextBoundaryFinder::BoundaryType::Grapheme, nodeText);
            while ((boundaryEnd = tbf.toNextBoundary()) != -1) {
                charIdx++;
                // Split text to get current char
                auto curChar =
                  QStringView(nodeText).mid(boundaryStart, boundaryEnd - boundaryStart);
                boundaryStart = boundaryEnd;
                // Don't rainbowify whitespaces
                if (curChar.trimmed().isEmpty() || codepointIsEmoji(curChar.toUcs4().at(0))) {
                    buf.append(curChar);
                    continue;
                }

                // get correct color for char index
                // Use colors as described here:
                // https://shark.comfsm.fm/~dleeling/cis/hsl_rainbow.html
                auto color = QColor::fromHslF((charIdx - 1.0) / textLen * (5. / 6.), 0.9, 0.5);
                // format color for HTML
                auto colorString = color.name(QColor::NameFormat::HexRgb);
                // create HTML element for current char
                auto curCharColored =
                  QStringLiteral("<font color=\"%0\">%1</font>").arg(colorString).arg(curChar);
                // append colored HTML element to buffer
                buf.append(curCharColored);
            }

            // create HTML_INLINE node to prevent HTML from being escaped
            auto htmlNode = cmark_node_new(CMARK_NODE_HTML_INLINE);
            // set content of HTML node to buffer contents
            cmark_node_set_literal(htmlNode, buf.toUtf8().data());
            // replace current node with HTML node
            cmark_node_replace(cur, htmlNode);
            // free memory of old node
            cmark_node_free(cur);
        }

        cmark_iter_free(iter);
    }

    const char *tmp_buf = cmark_render_html(node, CMARK_OPT_UNSAFE);
    // Copy the null terminated output buffer.
    std::string html(tmp_buf);

    // The buffer is no longer needed.
    free((char *)tmp_buf);

    auto result = linkifyMessage(escapeBlacklistedHtml(QString::fromStdString(html))).trimmed();

    if (result.count(QStringLiteral("<p>")) == 1 && result.startsWith(QLatin1String("<p>")) &&
        result.endsWith(QLatin1String("</p>"))) {
        result = result.mid(3, result.size() - 3 - 4);
    }

    return result;
}

QString
utils::getFormattedQuoteBody(const RelatedInfo &related, const QString &html)
{
    auto getFormattedBody = [related]() -> QString {
        using MsgType = mtx::events::MessageType;

        switch (related.type) {
        case MsgType::File: {
            return QStringLiteral("sent a file.");
        }
        case MsgType::Image: {
            return QStringLiteral("sent an image.");
        }
        case MsgType::Audio: {
            return QStringLiteral("sent an audio file.");
        }
        case MsgType::Video: {
            return QStringLiteral("sent a video");
        }
        default: {
            return related.quoted_formatted_body;
        }
        }
    };
    return QString("<mx-reply><blockquote><a "
                   "href=\"https://matrix.to/#/%1/%2\">In reply "
                   "to</a> <a href=\"https://matrix.to/#/%3\">%4</a><br"
                   "/>%5</blockquote></mx-reply>")
             .arg(related.room,
                  QString::fromStdString(related.related_event),
                  related.quoted_user,
                  related.quoted_user,
                  getFormattedBody()) +
           html;
}

QString
utils::getQuoteBody(const RelatedInfo &related)
{
    using MsgType = mtx::events::MessageType;

    switch (related.type) {
    case MsgType::File: {
        return QStringLiteral("sent a file.");
    }
    case MsgType::Image: {
        return QStringLiteral("sent an image.");
    }
    case MsgType::Audio: {
        return QStringLiteral("sent an audio file.");
    }
    case MsgType::Video: {
        return QStringLiteral("sent a video");
    }
    default: {
        return related.quoted_body;
    }
    }
}

QString
utils::linkColor()
{
    const auto theme = UserSettings::instance()->theme();

    if (theme == QLatin1String("light")) {
        return QStringLiteral("#0077b5");
    } else if (theme == QLatin1String("dark")) {
        return QStringLiteral("#38A3D8");
    } else {
        return QPalette().color(QPalette::Link).name();
    }
}

uint32_t
utils::hashQString(const QString &input)
{
    auto h = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha1);

    return (static_cast<uint32_t>(h[0]) << 24) ^ (static_cast<uint32_t>(h[1]) << 16) ^
           (static_cast<uint32_t>(h[2]) << 8) ^ static_cast<uint32_t>(h[3]);
}

QColor
utils::generateContrastingHexColor(const QString &input, const QColor &backgroundCol)
{
    const qreal backgroundLum = luminance(backgroundCol);

    // Create a color for the input
    auto hash = hashQString(input);
    // create a hue value based on the hash of the input.
    // Adapted to make Nico blue
    auto userHue =
      static_cast<int>(static_cast<double>(hash - static_cast<uint32_t>(0x60'00'00'00)) /
                       std::numeric_limits<uint32_t>::max() * 360.);
    // start with moderate saturation and lightness values.
    auto sat       = 230;
    auto lightness = 125;

    // converting to a QColor makes the luminance calc easier.
    QColor inputColor = QColor::fromHsl(userHue, sat, lightness);

    // calculate the initial luminance and contrast of the
    // generated color.  It's possible that no additional
    // work will be necessary.
    auto lum      = luminance(inputColor);
    auto contrast = computeContrast(lum, backgroundLum);

    // If the contrast doesn't meet our criteria,
    // try again and again until they do by modifying first
    // the lightness and then the saturation of the color.
    int iterationCount = 9;
    while (contrast < 4.5) {
        // if our lightness is at it's bounds, try changing
        // saturation instead.
        if (lightness >= 242 || lightness <= 13) {
            qreal newSat = qBound(26.0, sat * 1.25, 242.0);

            inputColor.setHsl(userHue, qFloor(newSat), lightness);
            auto tmpLum         = luminance(inputColor);
            auto higherContrast = computeContrast(tmpLum, backgroundLum);
            if (higherContrast > contrast) {
                contrast = higherContrast;
                sat      = newSat;
            } else {
                newSat = qBound(26.0, sat / 1.25, 242.0);
                inputColor.setHsl(userHue, qFloor(newSat), lightness);
                tmpLum             = luminance(inputColor);
                auto lowerContrast = computeContrast(tmpLum, backgroundLum);
                if (lowerContrast > contrast) {
                    contrast = lowerContrast;
                    sat      = newSat;
                }
            }
        } else {
            qreal newLightness = qBound(13.0, lightness * 1.25, 242.0);

            inputColor.setHsl(userHue, sat, qFloor(newLightness));

            auto tmpLum         = luminance(inputColor);
            auto higherContrast = computeContrast(tmpLum, backgroundLum);

            // Check to make sure we have actually improved contrast
            if (higherContrast > contrast) {
                contrast  = higherContrast;
                lightness = newLightness;
                // otherwise, try going the other way instead.
            } else {
                newLightness = qBound(13.0, lightness / 1.25, 242.0);
                inputColor.setHsl(userHue, sat, qFloor(newLightness));
                tmpLum             = luminance(inputColor);
                auto lowerContrast = computeContrast(tmpLum, backgroundLum);
                if (lowerContrast > contrast) {
                    contrast  = lowerContrast;
                    lightness = newLightness;
                }
            }
        }

        // don't loop forever, just give up at some point!
        // Someone smart may find a better solution
        if (--iterationCount < 0)
            break;
    }

    // get the hex value of the generated color.
    auto colorHex = inputColor.name();

    return colorHex;
}

qreal
utils::computeContrast(const qreal &one, const qreal &two)
{
    auto ratio = (one + 0.05) / (two + 0.05);

    if (two > one) {
        ratio = 1 / ratio;
    }

    return ratio;
}

qreal
utils::luminance(const QColor &col)
{
    int colRgb[3] = {col.red(), col.green(), col.blue()};
    qreal lumRgb[3];

    for (int i = 0; i < 3; i++) {
        qreal v   = colRgb[i] / 255.0;
        lumRgb[i] = v <= 0.03928 ? v / 12.92 : qPow((v + 0.055) / 1.055, 2.4);
    }

    auto lum = lumRgb[0] * 0.2126 + lumRgb[1] * 0.7152 + lumRgb[2] * 0.0722;

    return lum;
}

void
utils::centerWidget(QWidget *widget, QWindow *parent)
{
    if (parent) {
        widget->window()->windowHandle()->setTransientParent(parent);
        return;
    }

    auto findCenter = [childRect = widget->rect()](QRect hostRect) -> QPoint {
        return QPoint(hostRect.center().x() - (childRect.width() * 0.5),
                      hostRect.center().y() - (childRect.height() * 0.5));
    };
    widget->move(findCenter(QGuiApplication::primaryScreen()->geometry()));
}

void
utils::restoreCombobox(QComboBox *combo, const QString &value)
{
    for (auto i = 0; i < combo->count(); ++i) {
        if (value == combo->itemText(i)) {
            combo->setCurrentIndex(i);
            break;
        }
    }
}

QImage
utils::readImageFromFile(const QString &filename)
{
    QImageReader reader(filename);
    reader.setAutoTransform(true);
    return reader.read();
}
QImage
utils::readImage(const QByteArray &data)
{
    QBuffer buf;
    buf.setData(data);
    QImageReader reader(&buf);
    reader.setAutoTransform(true);
    return reader.read();
}

bool
utils::isReply(const mtx::events::collections::TimelineEvents &e)
{
    return mtx::accessors::relations(e).reply_to().has_value();
}

void
utils::removeDirectFromRoom(QString roomid)
{
    http::client()->get_account_data<mtx::events::account_data::Direct>(
      [roomid](mtx::events::account_data::Direct ev, mtx::http::RequestErr e) {
          if (e && e->status_code == 404)
              ev = {};
          else if (e) {
              nhlog::net()->error("Failed to retrieve m.direct: {}", *e);
              return;
          }

          auto r = roomid.toStdString();

          for (auto it = ev.user_to_rooms.begin(); it != ev.user_to_rooms.end();) {
              for (auto rit = it->second.begin(); rit != it->second.end();) {
                  if (r == *rit)
                      rit = it->second.erase(rit);
                  else
                      ++rit;
              }

              if (it->second.empty())
                  it = ev.user_to_rooms.erase(it);
              else
                  ++it;
          }

          http::client()->put_account_data(ev, [r](mtx::http::RequestErr e) {
              if (e)
                  nhlog::net()->error("Failed to update m.direct: {}", *e);
          });
      });
}
void
utils::markRoomAsDirect(QString roomid, std::vector<RoomMember> members)
{
    http::client()->get_account_data<mtx::events::account_data::Direct>(
      [roomid, members](mtx::events::account_data::Direct ev, mtx::http::RequestErr e) {
          if (e && e->status_code == 404)
              ev = {};
          else if (e) {
              nhlog::net()->error("Failed to retrieve m.direct: {}", *e);
              return;
          }

          auto local = utils::localUser();
          auto r     = roomid.toStdString();

          for (const auto &m : members) {
              if (m.user_id != local) {
                  ev.user_to_rooms[m.user_id.toStdString()].push_back(r);
              }
          }

          http::client()->put_account_data(ev, [r](mtx::http::RequestErr e) {
              if (e)
                  nhlog::net()->error("Failed to update m.direct: {}", *e);
          });
      });
}

std::vector<std::string>
utils::roomVias(const std::string &roomid)
{
    std::vector<std::string> vias;

    {
        auto members = cache::getMembers(roomid, 0, 100);
        if (!members.empty()) {
            vias.push_back(http::client()->user_id().hostname());
            for (const auto &m : members) {
                if (vias.size() >= 4)
                    break;

                auto user_id =
                  mtx::identifiers::parse<mtx::identifiers::User>(m.user_id.toStdString());

                auto server = user_id.hostname();
                if (std::find(begin(vias), end(vias), server) == vias.end())
                    vias.push_back(server);
            }
        }
    }

    if (vias.empty()) {
        auto members = cache::getMembersFromInvite(roomid, 0, 100);
        if (!members.empty()) {
            vias.push_back(http::client()->user_id().hostname());
            for (const auto &m : members) {
                if (vias.size() >= 4)
                    break;

                auto user_id =
                  mtx::identifiers::parse<mtx::identifiers::User>(m.user_id.toStdString());

                auto server = user_id.hostname();
                if (std::find(begin(vias), end(vias), server) == vias.end())
                    vias.push_back(server);
            }
        }
    }

    if (vias.empty()) {
        auto parents = cache::client()->getParentRoomIds(roomid);
        for (const auto &p : parents) {
            auto child =
              cache::client()->getStateEvent<mtx::events::state::space::Child>(p, roomid);
            if (child && child->content.via)
                vias.insert(vias.end(), child->content.via->begin(), child->content.via->end());
        }

        std::sort(begin(vias), end(vias));
        auto last = std::unique(begin(vias), end(vias));
        vias.erase(last, end(vias));

        // if (vias.size()> 3)
        //     vias.erase(begin(vias)+3, end(vias));
    }
    return vias;
}
