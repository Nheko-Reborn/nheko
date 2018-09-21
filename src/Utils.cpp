#include "Utils.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>
#include <QTextDocument>
#include <QXmlStreamReader>
#include <cmath>

#include <boost/variant.hpp>
#include <cmark.h>

#include "Config.h"

using TimelineEvent = mtx::events::collections::TimelineEvents;

QString
utils::localUser()
{
        QSettings settings;
        return settings.value("auth/user_id").toString();
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
                return then.toString("HH:mm");
        else if (days < 2)
                return QString("Yesterday");
        else if (days < 365)
                return then.toString("dd/MM");

        return then.toString("dd/MM/yy");
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
                if (sender == localUser)
                        info.username = "You";
                else
                        info.username = username;

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

        const double sz =
          std::ceil(QApplication::desktop()->screen()->devicePixelRatioF() * (double)size);
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

        doc.replace("<mx-reply>", "");
        doc.replace("</mx-reply>", "");
        doc.replace("<br>", "<br></br>");

        QXmlStreamReader xml{doc};

        QString textString;
        while (!xml.atEnd() && !xml.hasError()) {
                auto t = xml.readNext();

                switch (t) {
                case QXmlStreamReader::Characters: {
                        auto text = xml.text().toString();
                        text.replace(conf::strings::url_regex, conf::strings::url_html);

                        textString += text;
                        break;
                }
                case QXmlStreamReader::StartDocument:
                case QXmlStreamReader::EndDocument:
                        break;
                case QXmlStreamReader::StartElement: {
                        if (xml.name() == "html")
                                break;

                        textString += QString("<%1").arg(xml.name().toString());

                        const auto attrs = xml.attributes();
                        for (const auto &e : attrs)
                                textString += QString(" %1=\"%2\"")
                                                .arg(e.name().toString())
                                                .arg(e.value().toString());

                        textString += ">";

                        break;
                }
                case QXmlStreamReader::EndElement: {
                        if (xml.name() == "html")
                                break;

                        textString += QString("</%1>").arg(xml.name().toString());
                        break;
                }
                default: {
                        break;
                }
                }
        }

        if (xml.hasError()) {
                qWarning() << "error while parsing xml" << xml.errorString() << doc;
                doc.replace("<html>", "");
                doc.replace("</html>", "");
                return doc;
        }

        return textString;
}

QString
utils::markdownToHtml(const QString &text)
{
        const auto str = text.toUtf8();
        const char *tmp_buf =
          cmark_markdown_to_html(str.constData(), str.size(), CMARK_OPT_DEFAULT);

        // Copy the null terminated output buffer.
        std::string html(tmp_buf);

        // The buffer is no longer needed.
        free((char *)tmp_buf);

        auto result = QString::fromStdString(html).trimmed();

        // Strip paragraph tags.
        result.replace("<p>", "");
        result.replace("</p>", "");

        return result;
}

QString
utils::linkColor()
{
        QSettings settings;
        const auto theme = settings.value("user/theme", "light").toString();

        if (theme == "light")
                return "#0077b5";
        else if (theme == "dark")
                return "#38A3D8";

        return QPalette().color(QPalette::Link).name();
}

void
utils::centerWidget(QWidget *widget, QWidget *parent)
{
        if (parent) {
                widget->move(parent->geometry().center() - widget->rect().center());
                return;
        }

        const QRect screenGeometry = QApplication::desktop()->screenGeometry();
        const int x                = (screenGeometry.width() - widget->width()) / 2;
        const int y                = (screenGeometry.height() - widget->height()) / 2;

        widget->move(x, y);
}
