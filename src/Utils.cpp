// SPDX-FileCopyrightText: Nheko Contributors
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
#include <QTimer>
#include <QWindow>
#include <QXmlStreamReader>

#include <array>
#include <cmath>
#include <mtx/responses/messages.hpp>
#include <unordered_set>
#include <variant>

#include <cmark.h>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Config.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "timeline/Permissions.h"

template<class T, class Event>
static DescInfo
createDescriptionInfo(const Event &event, const QString &localUser, const QString &displayName)
{
    const auto msg    = std::get<T>(event);
    const auto sender = QString::fromStdString(msg.sender);

    const auto username = displayName;
    const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);
    auto body           = mtx::accessors::body(event);
    if (mtx::accessors::relations(event).reply_to())
        body = utils::stripReplyFromBody(body);

    return DescInfo{
      QString::fromStdString(msg.event_id),
      sender,
      utils::messageDescription<T>(username, QString::fromStdString(body), sender == localUser),
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
            segments.erase(segments.cbegin());
        if (!segments.empty() && segments.first().isEmpty())
            segments.erase(segments.cbegin());
        body = segments.join('\n');
    }

    body.replace(QLatin1String("@room"), QString::fromUtf8("@\u2060room"));
    return body.toStdString();
}

std::string
utils::stripReplyFromFormattedBody(const std::string &formatted_bodyi)
{
    QString formatted_body = QString::fromStdString(formatted_bodyi);
    static QRegularExpression replyRegex(QStringLiteral("<mx-reply>.*</mx-reply>"),
                                         QRegularExpression::DotMatchesEverythingOption);
    formatted_body.remove(replyRegex);
    formatted_body.replace(QLatin1String("@room"), QString::fromUtf8("@\u2060room"));
    return formatted_body.toStdString();
}

RelatedInfo
utils::stripReplyFallbacks(const mtx::events::collections::TimelineEvents &event,
                           std::string id,
                           QString room_id_)
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
    bool insideTag       = false;
    for (auto &code : utf32_string) {
        if (code == U'<')
            insideTag = true;
        else if (code == U'>')
            insideTag = false;

        if (!insideTag && utils::codepointIsEmoji(code)) {
            if (!insideFontBlock) {
                fmtBody += QStringLiteral("<font face=\"") % UserSettings::instance()->emojiFont() %
                           (UserSettings::instance()->enlargeEmojiOnlyMessages()
                              ? QStringLiteral("\" size=\"4\">")
                              : QStringLiteral("\">"));
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
utils::getMessageDescription(const mtx::events::collections::TimelineEvents &event,
                             const QString &localUser,
                             const QString &displayName)
{
    using Audio         = mtx::events::RoomEvent<mtx::events::msg::Audio>;
    using Emote         = mtx::events::RoomEvent<mtx::events::msg::Emote>;
    using File          = mtx::events::RoomEvent<mtx::events::msg::File>;
    using Image         = mtx::events::RoomEvent<mtx::events::msg::Image>;
    using Notice        = mtx::events::RoomEvent<mtx::events::msg::Notice>;
    using Text          = mtx::events::RoomEvent<mtx::events::msg::Text>;
    using Unknown       = mtx::events::RoomEvent<mtx::events::msg::Unknown>;
    using Video         = mtx::events::RoomEvent<mtx::events::msg::Video>;
    using ElementEffect = mtx::events::RoomEvent<mtx::events::msg::ElementEffect>;
    using CallInvite    = mtx::events::RoomEvent<mtx::events::voip::CallInvite>;
    using CallAnswer    = mtx::events::RoomEvent<mtx::events::voip::CallAnswer>;
    using CallHangUp    = mtx::events::RoomEvent<mtx::events::voip::CallHangUp>;
    using CallReject    = mtx::events::RoomEvent<mtx::events::voip::CallReject>;
    using Encrypted     = mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>;

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
    } else if (std::holds_alternative<Unknown>(event)) {
        return createDescriptionInfo<Unknown>(event, localUser, displayName);
    } else if (std::holds_alternative<Video>(event)) {
        return createDescriptionInfo<Video>(event, localUser, displayName);
    } else if (std::holds_alternative<ElementEffect>(event)) {
        return createDescriptionInfo<ElementEffect>(event, localUser, displayName);
    } else if (std::holds_alternative<CallInvite>(event)) {
        return createDescriptionInfo<CallInvite>(event, localUser, displayName);
    } else if (std::holds_alternative<CallAnswer>(event)) {
        return createDescriptionInfo<CallAnswer>(event, localUser, displayName);
    } else if (std::holds_alternative<CallHangUp>(event)) {
        return createDescriptionInfo<CallHangUp>(event, localUser, displayName);
    } else if (std::holds_alternative<CallReject>(event)) {
        return createDescriptionInfo<CallReject>(event, localUser, displayName);
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

    return QString::fromUcs4(&input.toStdU32String().at(0), 1).toUpper();
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

QPixmap
utils::scaleImageToPixmap(const QImage &img, int size)
{
    if (img.isNull())
        return QPixmap();

    // Deprecated in 5.13: const double sz =
    //  std::ceil(QApplication::desktop()->screen()->devicePixelRatioF() * (double)size);
    const int sz = static_cast<int>(
      std::ceil(QGuiApplication::primaryScreen()->devicePixelRatio() * (double)size));
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
        w = static_cast<int>(static_cast<double>(source.width()) * minAspectRatio);
        h = static_cast<int>(static_cast<double>(source.height()) * minAspectRatio);
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

    static QRegularExpression matrixURIRegex(
      QStringLiteral("\\b(?<![\"'])(?>(matrix:[\\S]{5,}))(?![\"'])\\b"));
    doc.replace(matrixURIRegex, conf::strings::url_html);

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
      "em",         "/em",         "strike",  "/strike", "code",  "/code",  "hr",     "hr/",
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
        buffer.append(pos, static_cast<int>(tagStart - pos));
        if (tagStart == end)
            break;

        const auto tagNameStart = tagStart + 1;
        const auto tagNameEnd =
          std::find_first_of(tagNameStart, end, tagNameEnds.begin(), tagNameEnds.end());

        if (allowedTags.find(
              QByteArray(tagNameStart, static_cast<int>(tagNameEnd - tagNameStart)).toLower()) ==
            allowedTags.end()) {
            // not allowed -> escape
            buffer.append("&lt;");
            pos = tagNameStart;
            continue;
        } else {
            buffer.append(tagStart, static_cast<int>(tagNameEnd - tagStart));

            pos = tagNameEnd;

            if (tagNameEnd != end) {
                auto attrStart = tagNameEnd;
                auto attrsEnd  = std::find(attrStart, end, '>');
                // we don't want to consume the slash of self closing tags as part of an attribute.
                // However, obviously we don't want to move backwards, if there are no attributes.
                if (*(attrsEnd - 1) == '/' && attrStart < attrsEnd)
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

                    auto attrName =
                      QByteArray(attrStart, static_cast<int>(attrEnd - attrStart)).toLower();

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

                                    auto val  = sanitizeValue(QByteArray(
                                      attrStart, static_cast<int>(valueEnd - attrStart)));
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

                                    auto val  = sanitizeValue(QByteArray(
                                      attrStart, static_cast<int>(valueEnd - attrStart)));
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
                                    auto val      = sanitizeValue(QByteArray(
                                      attrStart, static_cast<int>(valueEnd - attrStart)));
                                    attrStart     = consumeSpaces(valueEnd);

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

static void
rainbowify(cmark_node *node)
{
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
        // only text nodes (no code or similar)
        if (cmark_node_get_type(cur) != CMARK_NODE_TEXT)
            continue;

        // get text in current node
        QString nodeText(cmark_node_get_literal(cur));
        // create buffer to append rainbow text to
        QString buf;
        int boundaryStart = 0;
        int boundaryEnd   = 0;
        // use QTextBoundaryFinder to iterate over graphemes
        QTextBoundaryFinder tbf(QTextBoundaryFinder::BoundaryType::Grapheme, nodeText);
        while ((boundaryEnd = tbf.toNextBoundary()) != -1) {
            charIdx++;
            // Split text to get current char
            auto curChar  = QStringView(nodeText).mid(boundaryStart, boundaryEnd - boundaryStart);
            boundaryStart = boundaryEnd;
            // Don't rainbowify whitespaces
            if (curChar.trimmed().isEmpty() || utils::codepointIsEmoji(curChar.toUcs4().at(0))) {
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

static std::string
extract_spoiler_warning(std::string &inside_spoiler)
{
    std::string spoiler_text;
    if (auto spoilerTextEnd = inside_spoiler.find("|"); spoilerTextEnd != std::string::npos) {
        spoiler_text   = inside_spoiler.substr(0, spoilerTextEnd);
        inside_spoiler = inside_spoiler.substr(spoilerTextEnd + 1);
    }
    return QString::fromStdString(spoiler_text).replace('"', "&quot;").toStdString();
}

// TODO(Nico): Add tests :D
static void
process_spoilers(cmark_node *node)
{
    auto iter = cmark_iter_new(node);

    while (cmark_iter_next(iter) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);

        // only text nodes (no code or similar)
        if (cmark_node_get_type(cur) != CMARK_NODE_TEXT) {
            continue;
        }

        std::string_view content = cmark_node_get_literal(cur);

        if (auto posStart = content.find("||"); posStart != std::string::npos) {
            // we have the start of the spoiler
            if (auto posEnd = content.find("||", posStart + 2); posEnd != std::string::npos) {
                // we have the end of the spoiler in the same node

                std::string before_spoiler = std::string(content.substr(0, posStart));
                std::string inside_spoiler =
                  std::string(content.substr(posStart + 2, posEnd - 2 - posStart));
                std::string after_spoiler = std::string(content.substr(posEnd + 2));

                std::string spoiler_text = extract_spoiler_warning(inside_spoiler);

                // create the new nodes
                auto before_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                cmark_node_set_literal(before_node, before_spoiler.c_str());
                auto after_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                cmark_node_set_literal(after_node, after_spoiler.c_str());

                auto block = cmark_node_new(cmark_node_type::CMARK_NODE_CUSTOM_INLINE);
                cmark_node_set_on_enter(
                  block, ("<span data-mx-spoiler=\"" + spoiler_text + "\">").c_str());
                cmark_node_set_on_exit(block, "</span>");
                auto child_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                cmark_node_set_literal(child_node, inside_spoiler.c_str());
                cmark_node_append_child(block, child_node);

                // insert the new nodes into the tree
                cmark_node_replace(cur, block);
                cmark_node_insert_before(block, before_node);
                cmark_node_insert_after(block, after_node);

                // cleanup the replaced node
                cmark_node_free(cur);

                // fixup the iterator
                cmark_iter_reset(iter, block, CMARK_EVENT_EXIT);

            } else {
                // no end found, but lets try sibling nodes
                for (auto next = cmark_node_next(cur); next != nullptr;
                     next      = cmark_node_next(next)) {
                    // only text nodes again
                    if (cmark_node_get_type(next) != CMARK_NODE_TEXT)
                        continue;

                    std::string_view next_content = cmark_node_get_literal(next);
                    if (auto posEndNext = next_content.find("||");
                        posEndNext != std::string_view::npos) {
                        // We found the end of the spoiler
                        std::string before_spoiler = std::string(content.substr(0, posStart));
                        std::string after_spoiler =
                          std::string(next_content.substr(posEndNext + 2));

                        std::string inside_spoiler_start =
                          std::string(content.substr(posStart + 2));
                        std::string inside_spoiler_end =
                          std::string(next_content.substr(0, posEndNext));

                        std::string spoiler_text = extract_spoiler_warning(inside_spoiler_start);

                        // save all the nodes inside the spoiler for later
                        std::vector<cmark_node *> child_nodes;
                        for (auto kid = cmark_node_next(cur); kid != nullptr && kid != next;
                             kid      = cmark_node_next(kid)) {
                            child_nodes.push_back(kid);
                        }

                        // create the new nodes
                        auto before_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                        cmark_node_set_literal(before_node, before_spoiler.c_str());
                        auto after_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                        cmark_node_set_literal(after_node, after_spoiler.c_str());

                        auto block = cmark_node_new(cmark_node_type::CMARK_NODE_CUSTOM_INLINE);
                        cmark_node_set_on_enter(
                          block, ("<span data-mx-spoiler=\"" + spoiler_text + "\">").c_str());
                        cmark_node_set_on_exit(block, "</span>");

                        // create the content inside the spoiler by adding the old text at the start
                        // and the end as well as all the existing children
                        auto child_node_start = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                        cmark_node_set_literal(child_node_start, inside_spoiler_start.c_str());
                        cmark_node_append_child(block, child_node_start);
                        for (auto &child : child_nodes)
                            cmark_node_append_child(block, child);
                        auto child_node_end = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                        cmark_node_set_literal(child_node_end, inside_spoiler_end.c_str());
                        cmark_node_append_child(block, child_node_end);

                        // insert the new nodes into the tree
                        cmark_node_replace(cur, block);
                        cmark_node_insert_before(block, before_node);
                        cmark_node_insert_after(block, after_node);

                        // cleanup removed nodes
                        cmark_node_free(cur);
                        cmark_node_free(next);

                        // fixup the iterator
                        cmark_iter_reset(iter, block, CMARK_EVENT_EXIT);

                        break;
                    }
                }
            }
        }
    }

    cmark_iter_free(iter);
}

static void
process_strikethrough(cmark_node *node)
{
    auto iter = cmark_iter_new(node);

    while (cmark_iter_next(iter) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);

        // only text nodes (no code or similar)
        if (cmark_node_get_type(cur) != CMARK_NODE_TEXT) {
            continue;
        }

        std::string_view content = cmark_node_get_literal(cur);

        if (auto posStart = content.find("~~"); posStart != std::string::npos) {
            // we have the start of the strikethrough
            if (auto posEnd = content.find("~~", posStart + 2); posEnd != std::string::npos) {
                // we have the end of the strikethrough in the same node

                std::string before_strike = std::string(content.substr(0, posStart));
                std::string inside_strike =
                  std::string(content.substr(posStart + 2, posEnd - 2 - posStart));
                std::string after_strike = std::string(content.substr(posEnd + 2));

                // create the new nodes
                auto before_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                cmark_node_set_literal(before_node, before_strike.c_str());
                auto after_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                cmark_node_set_literal(after_node, after_strike.c_str());

                auto block = cmark_node_new(cmark_node_type::CMARK_NODE_CUSTOM_INLINE);
                cmark_node_set_on_enter(block, "<del>");
                cmark_node_set_on_exit(block, "</del>");
                auto child_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                cmark_node_set_literal(child_node, inside_strike.c_str());
                cmark_node_append_child(block, child_node);

                // insert the new nodes into the tree
                cmark_node_replace(cur, block);
                cmark_node_insert_before(block, before_node);
                cmark_node_insert_after(block, after_node);

                // cleanup the replaced node
                cmark_node_free(cur);

                // fixup the iterator
                cmark_iter_reset(iter, block, CMARK_EVENT_EXIT);

            } else {
                // no end found, but lets try sibling nodes
                for (auto next = cmark_node_next(cur); next != nullptr;
                     next      = cmark_node_next(next)) {
                    // only text nodes again
                    if (cmark_node_get_type(next) != CMARK_NODE_TEXT)
                        continue;

                    std::string_view next_content = cmark_node_get_literal(next);
                    if (auto posEndNext = next_content.find("~~");
                        posEndNext != std::string_view::npos) {
                        // We found the end of the strikethrough
                        std::string before_strike = std::string(content.substr(0, posStart));
                        std::string after_strike = std::string(next_content.substr(posEndNext + 2));

                        std::string inside_strike_start = std::string(content.substr(posStart + 2));
                        std::string inside_strike_end =
                          std::string(next_content.substr(0, posEndNext));

                        // save all the nodes inside the strikethrough for later
                        std::vector<cmark_node *> child_nodes;
                        for (auto kid = cmark_node_next(cur); kid != nullptr && kid != next;
                             kid      = cmark_node_next(kid)) {
                            child_nodes.push_back(kid);
                        }

                        // create the new nodes
                        auto before_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                        cmark_node_set_literal(before_node, before_strike.c_str());
                        auto after_node = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                        cmark_node_set_literal(after_node, after_strike.c_str());

                        auto block = cmark_node_new(cmark_node_type::CMARK_NODE_CUSTOM_INLINE);
                        cmark_node_set_on_enter(block, "<del>");
                        cmark_node_set_on_exit(block, "</del>");

                        // create the content inside the strikethrough by adding the old text at the
                        // start and the end as well as all the existing children
                        auto child_node_start = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                        cmark_node_set_literal(child_node_start, inside_strike_start.c_str());
                        cmark_node_append_child(block, child_node_start);
                        for (auto &child : child_nodes)
                            cmark_node_append_child(block, child);
                        auto child_node_end = cmark_node_new(cmark_node_type::CMARK_NODE_TEXT);
                        cmark_node_set_literal(child_node_end, inside_strike_end.c_str());
                        cmark_node_append_child(block, child_node_end);

                        // insert the new nodes into the tree
                        cmark_node_replace(cur, block);
                        cmark_node_insert_before(block, before_node);
                        cmark_node_insert_after(block, after_node);

                        // cleanup removed nodes
                        cmark_node_free(cur);
                        cmark_node_free(next);

                        // fixup the iterator
                        cmark_iter_reset(iter, block, CMARK_EVENT_EXIT);

                        break;
                    }
                }
            }
        }
    }

    cmark_iter_free(iter);
}
QString
utils::markdownToHtml(const QString &text, bool rainbowify_, bool noExtensions)
{
    const auto str         = text.toUtf8();
    cmark_node *const node = cmark_parse_document(str.constData(), str.size(), CMARK_OPT_UNSAFE);

    if (!noExtensions) {
        process_strikethrough(node);
        process_spoilers(node);

        if (rainbowify_) {
            rainbowify(node);
        }
    }

    const char *tmp_buf = cmark_render_html(
      node,
      // by default make single linebreaks <br> tags
      noExtensions ? CMARK_OPT_UNSAFE : (CMARK_OPT_UNSAFE | CMARK_OPT_HARDBREAKS));
    // Copy the null terminated output buffer.
    std::string html(tmp_buf);

    // The buffer is no longer needed.
    free((char *)tmp_buf);
    cmark_node_free(node);

    auto result = escapeBlacklistedHtml(QString::fromStdString(html)).trimmed();

    if (!noExtensions) {
        result = linkifyMessage(std::move(result)).trimmed();
    }

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
    auto userHue = static_cast<double>(hash - static_cast<uint32_t>(0x60'00'00'00)) /
                   std::numeric_limits<uint32_t>::max() * 360.;
    // start with moderate saturation and lightness values.
    auto sat       = 230.;
    auto lightness = 125.;

    // converting to a QColor makes the luminance calc easier.
    QColor inputColor = QColor::fromHsl(
      static_cast<int>(userHue), static_cast<int>(sat), static_cast<int>(lightness));

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

            inputColor.setHsl(static_cast<int>(userHue),
                              static_cast<int>(qFloor(newSat)),
                              static_cast<int>(lightness));
            auto tmpLum         = luminance(inputColor);
            auto higherContrast = computeContrast(tmpLum, backgroundLum);
            if (higherContrast > contrast) {
                contrast = higherContrast;
                sat      = newSat;
            } else {
                newSat = qBound(26.0, sat / 1.25, 242.0);
                inputColor.setHsl(static_cast<int>(userHue),
                                  static_cast<int>(qFloor(newSat)),
                                  static_cast<int>(lightness));
                tmpLum             = luminance(inputColor);
                auto lowerContrast = computeContrast(tmpLum, backgroundLum);
                if (lowerContrast > contrast) {
                    contrast = lowerContrast;
                    sat      = newSat;
                }
            }
        } else {
            qreal newLightness = qBound(13.0, lightness * 1.25, 242.0);

            inputColor.setHsl(static_cast<int>(userHue),
                              static_cast<int>(sat),
                              static_cast<int>(qFloor(newLightness)));

            auto tmpLum         = luminance(inputColor);
            auto higherContrast = computeContrast(tmpLum, backgroundLum);

            // Check to make sure we have actually improved contrast
            if (higherContrast > contrast) {
                contrast  = higherContrast;
                lightness = newLightness;
                // otherwise, try going the other way instead.
            } else {
                newLightness = qBound(13.0, lightness / 1.25, 242.0);
                inputColor.setHsl(static_cast<int>(userHue),
                                  static_cast<int>(sat),
                                  static_cast<int>(qFloor(newLightness)));
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
        return QPoint(static_cast<int>(hostRect.center().x() - (childRect.width() * 0.5)),
                      static_cast<int>(hostRect.center().y() - (childRect.height() * 0.5)));
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

    // for joined rooms
    {
        // see https://spec.matrix.org/v1.6/appendices/#routing for the algorithm

        auto members = cache::roomMembers(roomid);
        if (!members.empty()) {
            auto powerlevels =
              cache::client()->getStateEvent<mtx::events::state::PowerLevels>(roomid).value_or(
                mtx::events::StateEvent<mtx::events::state::PowerLevels>{});
            auto acls = cache::client()->getStateEvent<mtx::events::state::ServerAcl>(roomid);

            std::vector<QRegularExpression> allowedServers;
            std::vector<QRegularExpression> deniedServers;

            if (acls) {
                auto globToRegexp = [](const std::string &globExp) {
                    auto rawReg = QRegularExpression::escape(QString::fromStdString(globExp))
                                    .replace("\\*", ".*")
                                    .replace("\\?", ".");
                    return QRegularExpression(QRegularExpression::anchoredPattern(rawReg),
                                              QRegularExpression::DotMatchesEverythingOption |
                                                QRegularExpression::DontCaptureOption);
                };

                allowedServers.reserve(acls->content.allow.size());
                for (const auto &s : acls->content.allow)
                    allowedServers.push_back(globToRegexp(s));
                deniedServers.reserve(acls->content.deny.size());
                for (const auto &s : acls->content.deny)
                    allowedServers.push_back(globToRegexp(s));
            }

            auto isHostAllowed = [&acls, &allowedServers, &deniedServers](const std::string &host) {
                if (!acls)
                    return true;

                auto url = QUrl::fromEncoded(
                  "https://" + QByteArray::fromRawData(host.data(), host.size()), QUrl::StrictMode);
                if (url.hasQuery() || url.hasFragment())
                    return false;

                auto hostname = url.host();

                for (const auto &d : deniedServers)
                    if (d.match(hostname).hasMatch())
                        return false;
                for (const auto &a : allowedServers)
                    if (a.match(hostname).hasMatch())
                        return true;

                return false;
            };

            std::unordered_set<std::string> users_with_high_pl;
            std::set<std::string> users_with_high_pl_in_room;
            // we should pick PL > 50, but imo that is broken, so we just pick users who have admins
            // perm
            for (const auto &user : powerlevels.content.users) {
                if (user.second >= powerlevels.content.events_default &&
                    user.second >= powerlevels.content.state_default) {
                    auto host =
                      mtx::identifiers::parse<mtx::identifiers::User>(user.first).hostname();
                    if (isHostAllowed(host))
                        users_with_high_pl.insert(user.first);
                }
            }

            std::unordered_map<std::string, std::size_t> usercount_by_server;
            for (const auto &m : members) {
                auto user_id = mtx::identifiers::parse<mtx::identifiers::User>(m);
                usercount_by_server[user_id.hostname()] += 1;

                if (users_with_high_pl.count(m))
                    users_with_high_pl_in_room.insert(m);
            }

            std::erase_if(usercount_by_server, [&isHostAllowed](const auto &item) {
                return !isHostAllowed(item.first);
            });

            // add the highest powerlevel user
            auto max_pl_user = std::max_element(
              users_with_high_pl_in_room.begin(),
              users_with_high_pl_in_room.end(),
              [&pl_content = powerlevels.content](const std::string &a, const std::string &b) {
                  return pl_content.user_level(a) < pl_content.user_level(b);
              });
            if (max_pl_user != users_with_high_pl_in_room.end()) {
                auto host =
                  mtx::identifiers::parse<mtx::identifiers::User>(*max_pl_user).hostname();
                vias.push_back(host);
                usercount_by_server.erase(host);
            }

            // add up to 3 users, by usercount size from that server
            std::vector<std::pair<std::size_t, std::string>> servers_sorted_by_usercount;
            servers_sorted_by_usercount.reserve(usercount_by_server.size());
            for (const auto &[server, count] : usercount_by_server)
                servers_sorted_by_usercount.emplace_back(count, server);

            std::sort(servers_sorted_by_usercount.begin(),
                      servers_sorted_by_usercount.end(),
                      [](const auto &a, const auto &b) {
                          if (a.first == b.first)
                              // same pl, sort lex smaller server first
                              return a.second < b.second;

                          // sort high user count first
                          return a.first > b.first;
                      });

            for (const auto &server : servers_sorted_by_usercount) {
                if (vias.size() >= 3)
                    break;

                vias.push_back(server.second);
            }

            return vias;
        }
    }

    // for invites
    {
        auto members = cache::getMembersFromInvite(roomid, 0, 100);
        if (!members.empty()) {
            vias.push_back(http::client()->user_id().hostname());
            for (const auto &m : members) {
                if (vias.size() >= 3)
                    break;

                auto user_id =
                  mtx::identifiers::parse<mtx::identifiers::User>(m.user_id.toStdString());

                auto server = user_id.hostname();
                if (std::find(begin(vias), end(vias), server) == vias.end())
                    vias.push_back(server);
            }

            return vias;
        }
    }

    // for space previews
    auto parents = cache::client()->getParentRoomIds(roomid);
    for (const auto &p : parents) {
        auto child = cache::client()->getStateEvent<mtx::events::state::space::Child>(p, roomid);
        if (child && child->content.via)
            vias.insert(vias.end(), child->content.via->begin(), child->content.via->end());
    }

    std::sort(begin(vias), end(vias));
    auto last = std::unique(begin(vias), end(vias));
    vias.erase(last, end(vias));

    return vias;
}

void
utils::updateSpaceVias()
{
    if (!UserSettings::instance()->updateSpaceVias())
        return;

    nhlog::net()->info("update space vias called");

    auto rooms = cache::roomInfo(false);

    auto us = http::client()->user_id().to_string();

    auto weekAgo = (uint64_t)QDateTime::currentDateTime().addDays(-7).toMSecsSinceEpoch();

    struct ApplySpaceUpdatesState
    {
        std::vector<mtx::events::StateEvent<mtx::events::state::space::Child>> childrenToUpdate;
        std::vector<mtx::events::StateEvent<mtx::events::state::space::Parent>> parentsToUpdate;

        static void next(std::shared_ptr<ApplySpaceUpdatesState> state)
        {
            if (!state->childrenToUpdate.empty()) {
                const auto &child = state->childrenToUpdate.back();

                http::client()->send_state_event(
                  child.room_id,
                  child.state_key,
                  child.content,
                  [state = std::move(state)](const mtx::responses::EventId &,
                                             mtx::http::RequestErr e) mutable {
                      const auto &child_ = state->childrenToUpdate.back();
                      if (e) {
                          if (e->status_code == 429 && e->matrix_error.retry_after.count() != 0) {
                              ChatPage::instance()->callFunctionOnGuiThread(
                                [state    = std::move(state),
                                 interval = e->matrix_error.retry_after]() {
                                    QTimer::singleShot(interval * 3,
                                                       ChatPage::instance(),
                                                       [self = std::move(state)]() mutable {
                                                           next(std::move(self));
                                                       });
                                });
                              return;
                          }

                          nhlog::net()->error("Failed to update space child {} -> {}: {}",
                                              child_.room_id,
                                              child_.state_key,
                                              *e);
                      }
                      nhlog::net()->info(
                        "Updated space child {} -> {}", child_.room_id, child_.state_key);
                      state->childrenToUpdate.pop_back();
                      next(std::move(state));
                  });
                return;
            } else if (!state->parentsToUpdate.empty()) {
                const auto &parent = state->parentsToUpdate.back();

                http::client()->send_state_event(
                  parent.room_id,
                  parent.state_key,
                  parent.content,
                  [state = std::move(state)](const mtx::responses::EventId &,
                                             mtx::http::RequestErr e) mutable {
                      const auto &parent_ = state->parentsToUpdate.back();
                      if (e) {
                          if (e->status_code == 429 && e->matrix_error.retry_after.count() != 0) {
                              ChatPage::instance()->callFunctionOnGuiThread(
                                [state    = std::move(state),
                                 interval = e->matrix_error.retry_after]() {
                                    QTimer::singleShot(interval * 3,
                                                       ChatPage::instance(),
                                                       [self = std::move(state)]() mutable {
                                                           next(std::move(self));
                                                       });
                                });
                              return;
                          }

                          nhlog::net()->error("Failed to update space parent {} -> {}: {}",
                                              parent_.room_id,
                                              parent_.state_key,
                                              *e);
                      }
                      nhlog::net()->info(
                        "Updated space parent {} -> {}", parent_.room_id, parent_.state_key);
                      state->parentsToUpdate.pop_back();
                      next(std::move(state));
                  });
                return;
            }
        }
    };

    auto asus = std::make_shared<ApplySpaceUpdatesState>();

    for (const auto &[roomid, info] : rooms.toStdMap()) {
        if (!info.is_space)
            continue;

        auto spaceid = roomid.toStdString();

        if (auto pl = cache::client()
                        ->getStateEvent<mtx::events::state::PowerLevels>(spaceid)
                        .value_or(mtx::events::StateEvent<mtx::events::state::PowerLevels>{})
                        .content;
            pl.user_level(us) < pl.state_level(to_string(mtx::events::EventType::SpaceChild)))
            continue;

        auto children = cache::client()->getChildRoomIds(spaceid);

        for (const auto &childid : children) {
            // only update children we are joined to
            if (!rooms.contains(QString::fromStdString(childid)))
                continue;

            auto child =
              cache::client()->getStateEvent<mtx::events::state::space::Child>(spaceid, childid);
            if (child &&
                // don't update too often
                child->origin_server_ts < weekAgo &&
                // ignore unset spaces
                (child->content.via && !child->content.via->empty())) {
                auto newVias = utils::roomVias(childid);

                if (!newVias.empty() && newVias != child->content.via) {
                    nhlog::net()->info("Will update {} -> {} child relation from {} to {}",
                                       spaceid,
                                       childid,
                                       fmt::join(*child->content.via, ","),
                                       fmt::join(newVias, ","));

                    child->content.via = std::move(newVias);
                    child->room_id     = spaceid;
                    asus->childrenToUpdate.push_back(*std::move(child));
                }
            }

            auto parent =
              cache::client()->getStateEvent<mtx::events::state::space::Parent>(childid, spaceid);
            if (parent &&
                // don't update too often
                parent->origin_server_ts < weekAgo &&
                // ignore unset spaces
                (parent->content.via && !parent->content.via->empty())) {
                if (auto pl =
                      cache::client()
                        ->getStateEvent<mtx::events::state::PowerLevels>(childid)
                        .value_or(mtx::events::StateEvent<mtx::events::state::PowerLevels>{})
                        .content;
                    pl.user_level(us) <
                    pl.state_level(to_string(mtx::events::EventType::SpaceParent)))
                    continue;

                auto newVias = utils::roomVias(spaceid);

                if (!newVias.empty() && newVias != parent->content.via) {
                    nhlog::net()->info("Will update {} -> {} parent relation from {} to {}",
                                       childid,
                                       spaceid,
                                       fmt::join(*parent->content.via, ","),
                                       fmt::join(newVias, ","));

                    parent->content.via = std::move(newVias);
                    parent->room_id     = childid;
                    asus->parentsToUpdate.push_back(*std::move(parent));
                }
            }
        }
    }

    ApplySpaceUpdatesState::next(std::move(asus));
}

std::atomic<bool> event_expiration_running = false;
void
utils::removeExpiredEvents()
{
    if (!UserSettings::instance()->expireEvents())
        return;

    if (event_expiration_running.exchange(true)) {
        nhlog::net()->info("Event expiration still running, not starting second job.");
        return;
    }

    nhlog::net()->info("Remove expired events starting.");

    auto rooms = cache::roomInfo(false);

    auto us = http::client()->user_id().to_string();

    using ExpType =
      mtx::events::AccountDataEvent<mtx::events::account_data::nheko_extensions::EventExpiry>;
    static auto getExpEv = [](const std::string &room = "") -> std::optional<ExpType> {
        if (auto accountEvent =
              cache::client()->getAccountData(mtx::events::EventType::NhekoEventExpiry, room))
            if (auto ev = std::get_if<ExpType>(&*accountEvent);
                ev && (ev->content.expire_after_ms || ev->content.keep_only_latest))
                return std::optional{*ev};
        return std::nullopt;
    };

    struct ApplyEventExpiration
    {
        std::optional<ExpType> globalExpiry;
        std::vector<std::string> roomsToUpdate;
        std::string filter;

        std::string currentRoom;
        bool firstMessagesCall         = true;
        std::uint64_t currentRoomCount = 0;

        // batch token for pagination
        std::string currentRoomPrevToken;
        // event id of an event redacted in a previous run
        std::string currentRoomStopAt;
        // event id of first event redacted in the current run, hoping that the order stays the
        // same.
        std::string currentRoomFirstRedactedEvent;
        // (evtype,state_key) tuple to keep the latest state event of each.
        std::set<std::pair<std::string, std::string>> currentRoomStateEvents;
        // event ids pending redaction
        std::vector<std::string> currentRoomRedactionQueue;

        mtx::events::account_data::nheko_extensions::EventExpiry currentExpiry;

        static void next(std::shared_ptr<ApplyEventExpiration> state)
        {
            if (!state->currentRoomRedactionQueue.empty()) {
                auto evid = state->currentRoomRedactionQueue.back();
                auto room = state->currentRoom;
                http::client()->redact_event(
                  room,
                  evid,
                  [state = std::move(state), evid](const mtx::responses::EventId &,
                                                   mtx::http::RequestErr e) mutable {
                      if (e) {
                          if (e->status_code == 429 && e->matrix_error.retry_after.count() != 0) {
                              ChatPage::instance()->callFunctionOnGuiThread(
                                [state    = std::move(state),
                                 interval = e->matrix_error.retry_after]() {
                                    // triple interval to allow other traffic as well
                                    QTimer::singleShot(interval * 3,
                                                       ChatPage::instance(),
                                                       [self = std::move(state)]() mutable {
                                                           next(std::move(self));
                                                       });
                                });
                              return;
                          } else {
                              nhlog::net()->error("Failed to redact event {} in {}: {}",
                                                  evid,
                                                  state->currentRoom,
                                                  *e);
                              state->currentRoomRedactionQueue.pop_back();
                              next(std::move(state));
                          }
                      } else {
                          nhlog::net()->info("Redacted event {} in {}", evid, state->currentRoom);

                          if (state->currentRoomFirstRedactedEvent.empty())
                              state->currentRoomFirstRedactedEvent = evid;

                          state->currentRoomRedactionQueue.pop_back();
                          next(std::move(state));
                      }
                  });
            } else if (!state->currentRoom.empty()) {
                if (state->currentRoomPrevToken.empty() && !state->firstMessagesCall) {
                    nhlog::net()->info("Finished room {}", state->currentRoom);

                    if (!state->currentRoomFirstRedactedEvent.empty())
                        cache::client()->storeEventExpirationProgress(
                          state->currentRoom,
                          nlohmann::json(state->currentExpiry).dump(),
                          state->currentRoomFirstRedactedEvent);

                    state->currentRoom.clear();
                    next(std::move(state));
                    return;
                }

                mtx::http::MessagesOpts opts{};
                opts.dir     = mtx::http::PaginationDirection::Backwards;
                opts.from    = state->currentRoomPrevToken;
                opts.limit   = 1000;
                opts.filter  = state->filter;
                opts.room_id = state->currentRoom;

                state->firstMessagesCall = false;

                http::client()->messages(
                  opts,
                  [state = std::move(state)](const mtx::responses::Messages &msgs,
                                             mtx::http::RequestErr error) mutable {
                      if (error) {
                          // skip success handler
                          nhlog::net()->warn(
                            "Finished room {} with error {}", state->currentRoom, *error);
                          state->currentRoom.clear();
                      } else if (msgs.chunk.empty()) {
                          state->currentRoomPrevToken.clear();
                      } else {
                          state->currentRoomPrevToken = msgs.end;

                          auto now = (uint64_t)QDateTime::currentMSecsSinceEpoch();
                          auto us  = http::client()->user_id().to_string();

                          for (const auto &e : msgs.chunk) {
                              if (std::holds_alternative<
                                    mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(e))
                                  continue;

                              if (std::holds_alternative<
                                    mtx::events::RoomEvent<mtx::events::msg::Redacted>>(e) ||
                                  std::holds_alternative<
                                    mtx::events::StateEvent<mtx::events::msg::Redacted>>(e)) {
                                  if (!state->currentRoomStopAt.empty() &&
                                      mtx::accessors::event_id(e) == state->currentRoomStopAt) {
                                      // There is no filter to remove redacted events from
                                      // pagination, so we try to stop early by caching what event
                                      // we redacted last if we reached the end of a room.
                                      nhlog::net()->info(
                                        "Found previous redaction marker, stopping early: {}",
                                        state->currentRoom);
                                      state->currentRoomPrevToken.clear();
                                      break;
                                  }
                                  continue;
                              }

                              if (std::holds_alternative<
                                    mtx::events::StateEvent<mtx::events::msg::Redacted>>(e))
                                  continue;

                              // synapse protects these 2 against redaction
                              if (std::holds_alternative<
                                    mtx::events::StateEvent<mtx::events::state::Create>>(e))
                                  continue;

                              if (std::holds_alternative<
                                    mtx::events::StateEvent<mtx::events::state::ServerAcl>>(e))
                                  continue;

                              // skip events we don't know to protect us from mistakes.
                              if (std::holds_alternative<
                                    mtx::events::RoomEvent<mtx::events::Unknown>>(e))
                                  continue;

                              if (mtx::accessors::sender(e) != us)
                                  continue;

                              state->currentRoomCount++;
                              if (state->currentRoomCount <= state->currentExpiry.protect_latest) {
                                  continue;
                              }

                              if (state->currentExpiry.exclude_state_events &&
                                  mtx::accessors::is_state_event(e))
                                  continue;

                              if (mtx::accessors::is_state_event(e)) {
                                  // skip the first state event of a type
                                  if (std::visit(
                                        [&state](const auto &se) {
                                            if constexpr (requires { se.state_key; })
                                                return state->currentRoomStateEvents
                                                  .emplace(to_string(se.type), se.state_key)
                                                  .second;
                                            else
                                                return true;
                                        },
                                        e))
                                      continue;
                              }

                              if (state->currentExpiry.keep_only_latest &&
                                  state->currentRoomCount > state->currentExpiry.keep_only_latest) {
                                  state->currentRoomRedactionQueue.push_back(
                                    mtx::accessors::event_id(e));
                              } else if (state->currentExpiry.expire_after_ms &&
                                         (state->currentExpiry.expire_after_ms +
                                          mtx::accessors::origin_server_ts(e).toMSecsSinceEpoch()) <
                                           now) {
                                  state->currentRoomRedactionQueue.push_back(
                                    mtx::accessors::event_id(e));
                              }
                          }
                      }

                      next(std::move(state));
                  });
            } else if (!state->roomsToUpdate.empty()) {
                const auto &room = state->roomsToUpdate.back();

                auto localExp = getExpEv(room);
                if (localExp) {
                    state->currentRoom   = room;
                    state->currentExpiry = localExp->content;
                } else if (state->globalExpiry) {
                    state->currentRoom   = room;
                    state->currentExpiry = state->globalExpiry->content;
                }
                state->firstMessagesCall    = true;
                state->currentRoomCount     = 0;
                state->currentRoomPrevToken = "";
                state->currentRoomRedactionQueue.clear();
                state->currentRoomStateEvents.clear();

                state->currentRoomStopAt = cache::client()->loadEventExpirationProgress(
                  state->currentRoom, nlohmann::json(state->currentExpiry).dump());

                state->roomsToUpdate.pop_back();
                next(std::move(state));
            } else {
                nhlog::net()->info("Finished event expiry");
                event_expiration_running = false;
            }
        }
    };

    auto asus = std::make_shared<ApplyEventExpiration>();

    nlohmann::json filter;
    filter["timeline"]["senders"]   = nlohmann::json::array({us});
    filter["timeline"]["not_types"] = nlohmann::json::array({"m.room.redaction"});

    asus->filter = filter.dump();

    asus->globalExpiry = getExpEv();

    for (const auto &[roomid_, info] : rooms.toStdMap()) {
        auto roomid = roomid_.toStdString();

        if (!asus->globalExpiry && !getExpEv(roomid))
            continue;

        if (auto pl = cache::client()
                        ->getStateEvent<mtx::events::state::PowerLevels>(roomid)
                        .value_or(mtx::events::StateEvent<mtx::events::state::PowerLevels>{})
                        .content;
            pl.user_level(us) < pl.event_level(to_string(mtx::events::EventType::RoomRedaction))) {
            nhlog::net()->warn("Can't react events in {}, not running expiration.", roomid);
            continue;
        }

        asus->roomsToUpdate.push_back(roomid);
    }

    nhlog::db()->info("Running expiration in {} rooms", asus->roomsToUpdate.size());

    ApplyEventExpiration::next(std::move(asus));
}
