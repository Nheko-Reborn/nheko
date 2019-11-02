#include "Utils.h"

#include <QApplication>
#include <QComboBox>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QProcessEnvironment>
#include <QScreen>
#include <QSettings>
#include <QTextDocument>
#include <QXmlStreamReader>
#include <cmath>

#include <boost/variant.hpp>
#include <cmark.h>

#include "Config.h"

using TimelineEvent = mtx::events::collections::TimelineEvents;

QHash<QString, QString> authorColors_;

QString
utils::localUser()
{
        QSettings settings;
        return settings.value("auth/user_id").toString();
}

QString
utils::replaceEmoji(const QString &body)
{
        QString fmtBody = "";

        QVector<uint> utf32_string = body.toUcs4();

        QSettings settings;
        QString userFontFamily = settings.value("user/emoji_font_family", "emoji").toString();

        for (auto &code : utf32_string) {
                // TODO: Be more precise here.
                if (code > 9000)
                        fmtBody += QString("<font face=\"" + userFontFamily + "\">") +
                                   QString::fromUcs4(&code, 1) + "</font>";
                else
                        fmtBody += QString::fromUcs4(&code, 1);
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
        QSettings settings("nheko", "nheko");
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
                return then.time().toString(Qt::DefaultLocaleShortDate);
        else if (days < 2)
                return QString(QCoreApplication::translate("descriptiveTime", "Yesterday"));
        else if (days < 7)
                return then.toString("dddd");

        return then.date().toString(Qt::DefaultLocaleShortDate);
}

DescInfo
utils::getMessageDescription(const TimelineEvent &event,
                             const QString &localUser,
                             const QString &room_id)
{
        using Audio     = mtx::events::RoomEvent<mtx::events::msg::Audio>;
        using Emote     = mtx::events::RoomEvent<mtx::events::msg::Emote>;
        using File      = mtx::events::RoomEvent<mtx::events::msg::File>;
        using Image     = mtx::events::RoomEvent<mtx::events::msg::Image>;
        using Notice    = mtx::events::RoomEvent<mtx::events::msg::Notice>;
        using Text      = mtx::events::RoomEvent<mtx::events::msg::Text>;
        using Video     = mtx::events::RoomEvent<mtx::events::msg::Video>;
        using Encrypted = mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>;

        if (boost::get<Audio>(&event) != nullptr) {
                return createDescriptionInfo<Audio>(event, localUser, room_id);
        } else if (boost::get<Emote>(&event) != nullptr) {
                return createDescriptionInfo<Emote>(event, localUser, room_id);
        } else if (boost::get<File>(&event) != nullptr) {
                return createDescriptionInfo<File>(event, localUser, room_id);
        } else if (boost::get<Image>(&event) != nullptr) {
                return createDescriptionInfo<Image>(event, localUser, room_id);
        } else if (boost::get<Notice>(&event) != nullptr) {
                return createDescriptionInfo<Notice>(event, localUser, room_id);
        } else if (boost::get<Text>(&event) != nullptr) {
                return createDescriptionInfo<Text>(event, localUser, room_id);
        } else if (boost::get<Video>(&event) != nullptr) {
                return createDescriptionInfo<Video>(event, localUser, room_id);
        } else if (boost::get<mtx::events::Sticker>(&event) != nullptr) {
                return createDescriptionInfo<mtx::events::Sticker>(event, localUser, room_id);
        } else if (boost::get<Encrypted>(&event) != nullptr) {
                const auto msg    = boost::get<Encrypted>(event);
                const auto sender = QString::fromStdString(msg.sender);

                const auto username = Cache::displayName(room_id, sender);
                const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

                DescInfo info;
                info.userid    = sender;
                info.body      = QString(" %1").arg(messageDescription<Encrypted>());
                info.timestamp = utils::descriptiveTime(ts);
                info.event_id  = QString::fromStdString(msg.event_id);
                info.datetime  = ts;

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
        const int nlen = s1.size();
        const int hlen = s2.size();

        if (hlen == 0)
                return -1;
        if (nlen == 1)
                return s2.find(s1);

        std::vector<int> row1(hlen + 1, 0);

        for (int i = 0; i < nlen; ++i) {
                std::vector<int> row2(1, i + 1);

                for (int j = 0; j < hlen; ++j) {
                        const int cost = s1[i] != s2[j];
                        row2.push_back(
                          std::min(row1[j + 1] + 1, std::min(row2[j] + 1, row1[j] + cost)));
                }

                row1.swap(row2);
        }

        return *std::min_element(row1.begin(), row1.end());
}

QString
utils::event_body(const mtx::events::collections::TimelineEvents &event)
{
        using namespace mtx::events;
        using namespace mtx::events::msg;

        if (boost::get<RoomEvent<Audio>>(&event) != nullptr) {
                return message_body<RoomEvent<Audio>>(event);
        } else if (boost::get<RoomEvent<Emote>>(&event) != nullptr) {
                return message_body<RoomEvent<Emote>>(event);
        } else if (boost::get<RoomEvent<File>>(&event) != nullptr) {
                return message_body<RoomEvent<File>>(event);
        } else if (boost::get<RoomEvent<Image>>(&event) != nullptr) {
                return message_body<RoomEvent<Image>>(event);
        } else if (boost::get<RoomEvent<Notice>>(&event) != nullptr) {
                return message_body<RoomEvent<Notice>>(event);
        } else if (boost::get<Sticker>(&event) != nullptr) {
                return message_body<Sticker>(event);
        } else if (boost::get<RoomEvent<Text>>(&event) != nullptr) {
                return message_body<RoomEvent<Text>>(event);
        } else if (boost::get<RoomEvent<Video>>(&event) != nullptr) {
                return message_body<RoomEvent<Video>>(event);
        }

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
        QStringList fingerprintList;
        for (int i = 0; i < ed25519.length(); i = i + 4) {
                fingerprintList << ed25519.mid(i, 4);
        }
        return fingerprintList.join(" ");
}

QString
utils::linkifyMessage(const QString &body)
{
        // Convert to valid XML.
        auto doc = QString("<html>%1</html>").arg(body);
        doc.replace(conf::strings::url_regex, conf::strings::url_html);

        return doc;
}

QByteArray
escapeRawHtml(const QByteArray &data)
{
        QByteArray buffer;
        const size_t length = data.size();
        buffer.reserve(length);
        for (size_t pos = 0; pos != length; ++pos) {
                switch (data.at(pos)) {
                case '&':
                        buffer.append("&amp;");
                        break;
                case '<':
                        buffer.append("&lt;");
                        break;
                case '>':
                        buffer.append("&gt;");
                        break;
                default:
                        buffer.append(data.at(pos));
                        break;
                }
        }
        return buffer;
}

QString
utils::markdownToHtml(const QString &text)
{
        const auto str = escapeRawHtml(text.toUtf8());
        const char *tmp_buf =
          cmark_markdown_to_html(str.constData(), str.size(), CMARK_OPT_DEFAULT);

        // Copy the null terminated output buffer.
        std::string html(tmp_buf);

        // The buffer is no longer needed.
        free((char *)tmp_buf);

        auto result = QString::fromStdString(html).trimmed();

        return result;
}

QString
utils::getFormattedQuoteBody(const RelatedInfo &related, const QString &html)
{
        return QString("<mx-reply><blockquote><a "
                       "href=\"https://matrix.to/#/%1/%2\">In reply "
                       "to</a>* <a href=\"https://matrix.to/#/%3\">%4</a><br "
                       "/>%5</blockquote></mx-reply>")
                 .arg(related.room,
                      QString::fromStdString(related.related_event),
                      related.quoted_user,
                      related.quoted_user,
                      getQuoteBody(related)) +
               html;
}

QString
utils::getQuoteBody(const RelatedInfo &related)
{
        using MsgType = mtx::events::MessageType;

        switch (related.type) {
        case MsgType::Text: {
                return markdownToHtml(related.quoted_body);
        }
        case MsgType::File: {
                return QString(QCoreApplication::translate("utils", "sent a file."));
        }
        case MsgType::Image: {
                return QString(QCoreApplication::translate("utils", "sent an image."));
        }
        case MsgType::Audio: {
                return QString(QCoreApplication::translate("utils", "sent an audio file."));
        }
        case MsgType::Video: {
                return QString(QCoreApplication::translate("utils", "sent a video"));
        }
        default: {
                return related.quoted_body;
        }
        }
}

QString
utils::linkColor()
{
        QSettings settings;
        // Default to system theme if QT_QPA_PLATFORMTHEME var is set.
        QString defaultTheme =
          QProcessEnvironment::systemEnvironment().value("QT_QPA_PLATFORMTHEME", "").isEmpty()
            ? "light"
            : "system";
        const auto theme = settings.value("user/theme", defaultTheme).toString();

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
        auto userHue = static_cast<int>(qAbs(hash % 360));
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
                qreal v                  = colRgb[i] / 255.0;
                v <= 0.03928 ? lumRgb[i] = v / 12.92 : lumRgb[i] = qPow((v + 0.055) / 1.055, 2.4);
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
                widget->move(findCenter(parent->geometry()));
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

utils::SideBarSizes
utils::calculateSidebarSizes(const QFont &f)
{
        const auto height = static_cast<double>(QFontMetrics{f}.lineSpacing());

        SideBarSizes sz;
        sz.small         = std::ceil(3.5 * height + height / 4.0);
        sz.normal        = std::ceil(16 * height);
        sz.groups        = std::ceil(3 * height);
        sz.collapsePoint = 2 * sz.normal;

        return sz;
}
