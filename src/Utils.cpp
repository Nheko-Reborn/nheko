// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Utils.h"

#include <QApplication>
#include <QBuffer>
#include <QComboBox>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QImageReader>
#include <QProcessEnvironment>
#include <QScreen>
#include <QSettings>
#include <QTextDocument>
#include <QXmlStreamReader>

#include <cmath>
#include <variant>

#include <cmark.h>

#include "Cache.h"
#include "Config.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"

using TimelineEvent = mtx::events::collections::TimelineEvents;

QHash<QString, QString> authorColors_;

template<class T, class Event>
static DescInfo
createDescriptionInfo(const Event &event, const QString &localUser, const QString &displayName)
{
        const auto msg    = std::get<T>(event);
        const auto sender = QString::fromStdString(msg.sender);

        const auto username = displayName;
        const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

        return DescInfo{QString::fromStdString(msg.event_id),
                        sender,
                        utils::messageDescription<T>(
                          username, utils::event_body(event).trimmed(), sender == localUser),
                        utils::descriptiveTime(ts),
                        msg.origin_server_ts,
                        ts};
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
        QString fmtBody = "";

        QVector<uint> utf32_string = body.toUcs4();

        bool insideFontBlock = false;
        for (auto &code : utf32_string) {
                if (utils::codepointIsEmoji(code)) {
                        if (!insideFontBlock) {
                                fmtBody += QString("<font face=\"" +
                                                   UserSettings::instance()->emojiFont() + "\">");
                                insideFontBlock = true;
                        }

                } else {
                        if (insideFontBlock) {
                                fmtBody += "</font>";
                                insideFontBlock = false;
                        }
                }
                fmtBody += QString::fromUcs4(&code, 1);
        }
        if (insideFontBlock) {
                fmtBody += "</font>";
        }

        return fmtBody;
}

void
utils::setScaleFactor(float factor)
{
        if (factor < 1 || factor > 3)
                return;

        QSettings settings;
        settings.setValue("settings/scale_factor", factor);
}

float
utils::scaleFactor()
{
        QSettings settings;
        return settings.value("settings/scale_factor", -1).toFloat();
}

bool
utils::respondsToKeyRequests(const std::string &roomId)
{
        return respondsToKeyRequests(QString::fromStdString(roomId));
}

bool
utils::respondsToKeyRequests(const QString &roomId)
{
        if (roomId.isEmpty())
                return false;

        QSettings settings;
        return settings.value("rooms/respond_to_key_requests/" + roomId, false).toBool();
}

void
utils::setKeyRequestsPreference(QString roomId, bool value)
{
        if (roomId.isEmpty())
                return;

        QSettings settings;
        settings.setValue("rooms/respond_to_key_requests/" + roomId, value);
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
                return then.toString("dddd");

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
        using CallInvite = mtx::events::RoomEvent<mtx::events::msg::CallInvite>;
        using CallAnswer = mtx::events::RoomEvent<mtx::events::msg::CallAnswer>;
        using CallHangUp = mtx::events::RoomEvent<mtx::events::msg::CallHangUp>;
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
                info.body   = QString(" %1").arg(
                  messageDescription<Encrypted>(username, "", sender == localUser));
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

        for (auto const &c : input.toUcs4()) {
                if (QString::fromUcs4(&c, 1) != QString("#"))
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
                        row2.push_back(
                          std::min(row1[j + 1] + 1, std::min(row2[j] + 1, row1[j] + cost)));
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

        return "";
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
        return QPixmap::fromImage(
          img.scaled(sz, sz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
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

        return QString("https://%1:%2/_matrix/media/r0/download/%3/%4")
          .arg(server)
          .arg(port)
          .arg(QString::fromStdString(mxcParts.server))
          .arg(QString::fromStdString(mxcParts.media_id));
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
                fingerprint.append(ed25519.midRef(i, 4));
                if (i > 0 && i % 16 == 12)
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

        return doc;
}

QString
utils::escapeBlacklistedHtml(const QString &rawStr)
{
        static const std::array allowedTags = {
          "font",       "/font",       "del",    "/del",    "h1",    "/h1",    "h2",     "/h2",
          "h3",         "/h3",         "h4",     "/h4",     "h5",    "/h5",    "h6",     "/h6",
          "blockquote", "/blockquote", "p",      "/p",      "a",     "/a",     "ul",     "/ul",
          "ol",         "/ol",         "sup",    "/sup",    "sub",   "/sub",   "li",     "/li",
          "b",          "/b",          "i",      "/i",      "u",     "/u",     "strong", "/strong",
          "em",         "/em",         "strike", "/strike", "code",  "/code",  "hr",     "/hr",
          "br",         "br/",         "div",    "/div",    "table", "/table", "thead",  "/thead",
          "tbody",      "/tbody",      "tr",     "/tr",     "th",    "/th",    "td",     "/td",
          "caption",    "/caption",    "pre",    "/pre",    "span",  "/span",  "img",    "/img"};
        QByteArray data = rawStr.toUtf8();
        QByteArray buffer;
        const int length = data.size();
        buffer.reserve(length);
        bool escapingTag = false;
        for (int pos = 0; pos != length; ++pos) {
                switch (data.at(pos)) {
                case '<': {
                        bool oneTagMatched = false;
                        const int endPos =
                          static_cast<int>(std::min(static_cast<size_t>(data.indexOf('>', pos)),
                                                    static_cast<size_t>(data.indexOf(' ', pos))));

                        auto mid = data.mid(pos + 1, endPos - pos - 1);
                        for (const auto &tag : allowedTags) {
                                // TODO: Check src and href attribute
                                if (mid.toLower() == tag) {
                                        oneTagMatched = true;
                                }
                        }
                        if (oneTagMatched)
                                buffer.append('<');
                        else {
                                escapingTag = true;
                                buffer.append("&lt;");
                        }
                        break;
                }
                case '>':
                        if (escapingTag) {
                                buffer.append("&gt;");
                                escapingTag = false;
                        } else
                                buffer.append('>');
                        break;
                default:
                        buffer.append(data.at(pos));
                        break;
                }
        }
        return QString::fromUtf8(buffer);
}

QString
utils::markdownToHtml(const QString &text)
{
        const auto str      = text.toUtf8();
        const char *tmp_buf = cmark_markdown_to_html(str.constData(), str.size(), CMARK_OPT_UNSAFE);

        // Copy the null terminated output buffer.
        std::string html(tmp_buf);

        // The buffer is no longer needed.
        free((char *)tmp_buf);

        auto result = linkifyMessage(escapeBlacklistedHtml(QString::fromStdString(html))).trimmed();

        if (result.count("<p>") == 1 && result.startsWith("<p>") && result.endsWith("</p>")) {
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
                        return "sent a file.";
                }
                case MsgType::Image: {
                        return "sent an image.";
                }
                case MsgType::Audio: {
                        return "sent an audio file.";
                }
                case MsgType::Video: {
                        return "sent a video";
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
                return "sent a file.";
        }
        case MsgType::Image: {
                return "sent an image.";
        }
        case MsgType::Audio: {
                return "sent an audio file.";
        }
        case MsgType::Video: {
                return "sent a video";
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

        if (theme == "light") {
                return "#0077b5";
        } else if (theme == "dark") {
                return "#38A3D8";
        } else {
                return QPalette().color(QPalette::Link).name();
        }
}

uint32_t
utils::hashQString(const QString &input)
{
        uint32_t hash = 0;

        for (int i = 0; i < input.length(); i++) {
                hash = input.at(i).digitValue() + ((hash << 5) - hash);
        }

        return hash;
}

QString
utils::generateContrastingHexColor(const QString &input, const QString &background)
{
        const QColor backgroundCol(background);
        const qreal backgroundLum = luminance(background);

        // Create a color for the input
        auto hash = hashQString(input);
        // create a hue value based on the hash of the input.
        auto userHue = static_cast<int>(hash % 360);
        // start with moderate saturation and lightness values.
        auto sat       = 220;
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
        while (contrast < 5) {
                // if our lightness is at it's bounds, try changing
                // saturation instead.
                if (lightness == 242 || lightness == 13) {
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
utils::centerWidget(QWidget *widget, QWidget *parent)
{
        auto findCenter = [childRect = widget->rect()](QRect hostRect) -> QPoint {
                return QPoint(hostRect.center().x() - (childRect.width() * 0.5),
                              hostRect.center().y() - (childRect.height() * 0.5));
        };

        if (parent) {
                widget->move(parent->window()->frameGeometry().topLeft() +
                             parent->window()->rect().center() - widget->rect().center());
                return;
        }

        // Deprecated in 5.13: widget->move(findCenter(QApplication::desktop()->screenGeometry()));
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
utils::readImage(const QByteArray *data)
{
        QBuffer buf;
        buf.setData(*data);
        QImageReader reader(&buf);
        reader.setAutoTransform(true);
        return reader.read();
}
