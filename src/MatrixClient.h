#pragma once

#include <QMetaType>
#include <QObject>
#include <QString>

#include "nlohmann/json.hpp"
#include <mtx/responses.hpp>
#include <mtxclient/http/client.hpp>

Q_DECLARE_METATYPE(mtx::responses::Login)
Q_DECLARE_METATYPE(mtx::responses::Messages)
Q_DECLARE_METATYPE(mtx::responses::Notifications)
Q_DECLARE_METATYPE(mtx::responses::Rooms)
Q_DECLARE_METATYPE(mtx::responses::Sync)
Q_DECLARE_METATYPE(mtx::responses::JoinedGroups)
Q_DECLARE_METATYPE(mtx::responses::GroupProfile)
Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(nlohmann::json)
Q_DECLARE_METATYPE(std::vector<std::string>)
Q_DECLARE_METATYPE(std::vector<QString>)

class MediaProxy : public QObject
{
        Q_OBJECT

signals:
        void imageDownloaded(const QPixmap &);
        void imageSaved(const QString &, const QByteArray &);
        void fileDownloaded(const QByteArray &);
};

namespace http {
mtx::http::Client *
client();

bool
is_logged_in();

//! Initialize the http module
void
init();
}
