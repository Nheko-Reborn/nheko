// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef GSTREAMER_AVAILABLE

#include "ScreenCastPortal.h"
#include "ChatPage.h"
#include "Logging.h"
#include "UserSettingsPage.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <mtxclient/utils.hpp>
#include <random>

static QString
make_token()
{
    return QString::fromStdString("nheko" + mtx::client::utils::random_token(64, false));
}

static QString
handle_path(QString handle_token)
{
    QString sender = QDBusConnection::sessionBus().baseService();
    if (sender[0] == ':')
        sender.remove(0, 1);
    sender.replace(".", "_");
    return QStringLiteral("/org/freedesktop/portal/desktop/request/") + sender +
           QStringLiteral("/") + handle_token;
}

void
ScreenCastPortal::init()
{
    switch (state) {
    case State::Closed:
        state = State::Starting;
        createSession();
        break;
    case State::Starting:
        nhlog::ui()->warn("ScreenCastPortal already starting");
        break;
    case State::Started:
        close(true);
        break;
    case State::Closing:
        nhlog::ui()->warn("ScreenCastPortal still closing");
        break;
    }
}

const ScreenCastPortal::Stream *
ScreenCastPortal::getStream() const
{
    if (state != State::Started)
        return nullptr;
    else
        return &stream;
}

bool
ScreenCastPortal::ready() const
{
    return state == State::Started;
}

void
ScreenCastPortal::close(bool reinit)
{
    switch (state) {
    case State::Closed:
        if (reinit)
            init();
        break;
    case State::Starting:
        if (!reinit) {
            // Remaining handler will abort.
            state = State::Closed;
        }
        break;
    case State::Started: {
        state = State::Closing;
        emit readyChanged();
        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                  sessionHandle.path(),
                                                  QStringLiteral("org.freedesktop.portal.Session"),
                                                  QStringLiteral("Close"));

        QDBusPendingCall pendingCall     = QDBusConnection::sessionBus().asyncCall(msg);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
        connect(watcher,
                &QDBusPendingCallWatcher::finished,
                this,
                [this, reinit](QDBusPendingCallWatcher *self) {
                    QDBusPendingReply reply = *self;
                    if (!reply.isValid()) {
                        nhlog::ui()->warn("org.freedesktop.portal.ScreenCast (Close): {}",
                                          reply.error().message().toStdString());
                    }
                    state = State::Closed;
                    if (reinit)
                        init();
                });
    } break;
    case State::Closing:
        nhlog::ui()->warn("ScreenCastPortal already closing");
        break;
    }
}

void
ScreenCastPortal::closedHandler(uint response, const QVariantMap &)
{
    if (response != 0) {
        nhlog::ui()->error("org.freedekstop.portal.ScreenCast (Closed): {}", response);
    }

    nhlog::ui()->debug("org.freedesktop.portal.ScreenCast: Connection closed");
    state = State::Closed;
    emit readyChanged();
}

void
ScreenCastPortal::createSession()
{
    // Connect before sending the request to avoid missing the reply
    QString handle_token = make_token();
    QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.portal.Desktop"),
                                          handle_path(handle_token),
                                          QStringLiteral("org.freedesktop.portal.Request"),
                                          QStringLiteral("Response"),
                                          this,
                                          SLOT(createSessionHandler(uint, QVariantMap)));

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.portal.ScreenCast"),
                                              QStringLiteral("CreateSession"));
    msg << QVariantMap{{QStringLiteral("handle_token"), handle_token},
                       {QStringLiteral("session_handle_token"), make_token()}};

    QDBusPendingCall pendingCall     = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(
      watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
          QDBusPendingReply<QDBusObjectPath> reply = *self;
          self->deleteLater();

          if (!reply.isValid()) {
              nhlog::ui()->error("org.freedesktop.portal.ScreenCast (CreateSession): {}",
                                 reply.error().message().toStdString());
              close();
          }
      });
}

void
ScreenCastPortal::createSessionHandler(uint response, const QVariantMap &results)
{
    switch (state) {
    case State::Closed:
        nhlog::ui()->warn("ScreenCastPortal not starting");
        break;
    case State::Starting: {
        if (response != 0) {
            nhlog::ui()->error("org.freedekstop.portal.ScreenCast (CreateSession Response): {}",
                               response);
            close();
            return;
        }

        sessionHandle = QDBusObjectPath(results.value(QStringLiteral("session_handle")).toString());

        nhlog::ui()->debug("org.freedesktop.portal.ScreenCast: sessionHandle = {}",
                           sessionHandle.path().toStdString());

        getAvailableSourceTypes();
    } break;
    case State::Started:
        nhlog::ui()->warn("ScreenCastPortal already started");
        break;
    case State::Closing:
        break;
    }
}

void
ScreenCastPortal::getAvailableSourceTypes()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.DBus.Properties"),
                                              QStringLiteral("Get"));
    msg << QStringLiteral("org.freedesktop.portal.ScreenCast")
        << QStringLiteral("AvailableSourceTypes");

    QDBusPendingCall pendingCall     = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(
      watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
          QDBusPendingReply<QDBusVariant> reply = *self;
          self->deleteLater();

          if (!reply.isValid()) {
              nhlog::ui()->error("org.freedesktop.DBus.Properties (Get AvailableSourceTypes): {}",
                                 reply.error().message().toStdString());
              close();
              return;
          }

          switch (state) {
          case State::Closed:
              nhlog::ui()->warn("ScreenCastPortal not starting");
              break;
          case State::Starting: {
              const auto &value = reply.value().variant();
              if (value.canConvert<uint>()) {
                  availableSourceTypes = value.value<uint>();
              } else {
                  nhlog::ui()->error("Invalid reply from org.freedesktop.DBus.Properties (Get "
                                     "AvailableSourceTypes)");
                  close();
                  return;
              }

              getAvailableCursorModes();
          } break;
          case State::Started:
              nhlog::ui()->warn("ScreenCastPortal already started");
              break;
          case State::Closing:
              break;
          }
      });
}

void
ScreenCastPortal::getAvailableCursorModes()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.DBus.Properties"),
                                              QStringLiteral("Get"));
    msg << QStringLiteral("org.freedesktop.portal.ScreenCast")
        << QStringLiteral("AvailableCursorModes");

    QDBusPendingCall pendingCall     = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(
      watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
          QDBusPendingReply<QDBusVariant> reply = *self;
          self->deleteLater();

          if (!reply.isValid()) {
              nhlog::ui()->error("org.freedesktop.DBus.Properties (Get AvailableCursorModes): {}",
                                 reply.error().message().toStdString());
              close();
              return;
          }

          switch (state) {
          case State::Closed:
              nhlog::ui()->warn("ScreenCastPortal not starting");
              break;
          case State::Starting: {
              const auto &value = reply.value().variant();
              if (value.canConvert<uint>()) {
                  availableCursorModes = value.value<uint>();
              } else {
                  nhlog::ui()->error("Invalid reply from org.freedesktop.DBus.Properties (Get "
                                     "AvailableCursorModes)");
                  close();
                  return;
              }

              selectSources();
          } break;
          case State::Started:
              nhlog::ui()->warn("ScreenCastPortal already started");
              break;
          case State::Closing:
              break;
          }
      });
}

void
ScreenCastPortal::selectSources()
{
    // Connect before sending the request to avoid missing the reply
    auto handle_token = make_token();
    QDBusConnection::sessionBus().connect(QString(),
                                          handle_path(handle_token),
                                          QStringLiteral("org.freedesktop.portal.Request"),
                                          QStringLiteral("Response"),
                                          this,
                                          SLOT(selectSourcesHandler(uint, QVariantMap)));

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.portal.ScreenCast"),
                                              QStringLiteral("SelectSources"));

    QVariantMap options{{QStringLiteral("multiple"), false},
                        {QStringLiteral("types"), availableSourceTypes},
                        {QStringLiteral("handle_token"), handle_token}};

    auto settings = ChatPage::instance()->userSettings();
    if (settings->screenShareHideCursor() && (availableCursorModes & (uint)1) != 0) {
        options["cursor_mode"] = (uint)1;
    }

    msg << QVariant::fromValue(sessionHandle) << options;

    QDBusPendingCall pendingCall     = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(
      watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
          QDBusPendingReply<QDBusObjectPath> reply = *self;
          if (!reply.isValid()) {
              nhlog::ui()->error("org.freedesktop.portal.ScreenCast (SelectSources): {}",
                                 reply.error().message().toStdString());
              close();
          }
      });
}

void
ScreenCastPortal::selectSourcesHandler(uint response, const QVariantMap &)
{
    switch (state) {
    case State::Closed:
        nhlog::ui()->warn("ScreenCastPortal not starting");
        break;
    case State::Starting: {
        if (response != 0) {
            nhlog::ui()->error("org.freedekstop.portal.ScreenCast (SelectSources Response): {}",
                               response);
            close();
            return;
        }
        start();
    } break;
    case State::Started:
        nhlog::ui()->warn("ScreenCastPortal already started");
        break;
    case State::Closing:
        break;
    }
}

void
ScreenCastPortal::start()
{
    // Connect before sending the request to avoid missing the reply
    auto handle_token = make_token();
    QDBusConnection::sessionBus().connect(QString(),
                                          handle_path(handle_token),
                                          QStringLiteral("org.freedesktop.portal.Request"),
                                          QStringLiteral("Response"),
                                          this,
                                          SLOT(startHandler(uint, QVariantMap)));

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.portal.ScreenCast"),
                                              QStringLiteral("Start"));
    msg << QVariant::fromValue(sessionHandle) << QString()
        << QVariantMap{{QStringLiteral("handle_token"), handle_token}};

    QDBusPendingCall pendingCall     = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(
      watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
          QDBusPendingReply<QDBusObjectPath> reply = *self;
          if (!reply.isValid()) {
              nhlog::ui()->error("org.freedesktop.portal.ScreenCast (Start): {}",
                                 reply.error().message().toStdString());
          } else {
          }
      });
}

struct PipeWireStream
{
    quint32 nodeId = 0;
    QVariantMap map;
};

Q_DECLARE_METATYPE(PipeWireStream)

const QDBusArgument &
operator>>(const QDBusArgument &argument, PipeWireStream &stream)
{
    argument.beginStructure();
    argument >> stream.nodeId;
    argument.beginMap();
    while (!argument.atEnd()) {
        QString key;
        QVariant map;
        argument.beginMapEntry();
        argument >> key >> map;
        argument.endMapEntry();
        stream.map.insert(key, map);
    }
    argument.endMap();
    argument.endStructure();
    return argument;
}

void
ScreenCastPortal::startHandler(uint response, const QVariantMap &results)
{
    if (response != 0) {
        nhlog::ui()->error("org.freedesktop.portal.ScreenCast (Start Response): {}", response);
        close();
        return;
    }

    QVector<PipeWireStream> streams =
      qdbus_cast<QVector<PipeWireStream>>(results.value(QStringLiteral("streams")));
    if (streams.size() == 0) {
        nhlog::ui()->error("org.freedesktop.portal.ScreenCast: No stream was returned");
        close();
        return;
    }

    stream.nodeId = streams[0].nodeId;
    nhlog::ui()->debug("org.freedesktop.portal.ScreenCast: nodeId = {}", stream.nodeId);
    openPipeWireRemote();
}

void
ScreenCastPortal::openPipeWireRemote()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.portal.ScreenCast"),
                                              QStringLiteral("OpenPipeWireRemote"));
    msg << QVariant::fromValue(sessionHandle) << QVariantMap{};

    QDBusPendingCall pendingCall     = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(
      watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
          QDBusPendingReply<QDBusUnixFileDescriptor> reply = *self;
          if (!reply.isValid()) {
              nhlog::ui()->error("org.freedesktop.portal.ScreenCast (OpenPipeWireRemote): {}",
                                 reply.error().message().toStdString());
              close();
          } else {
              stream.fd = reply.value().fileDescriptor();
              nhlog::ui()->debug("org.freedesktop.portal.ScreenCast: fd = {}", stream.fd);

              state = State::Started;
              emit readyChanged();
          }
      });
}

#endif
