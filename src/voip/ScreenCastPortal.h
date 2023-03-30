// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef GSTREAMER_AVAILABLE

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <optional>

class ScreenCastPortal final : public QObject
{
    Q_OBJECT

public:
    struct Stream
    {
        QDBusUnixFileDescriptor fd;
        quint32 nodeId;
    };

    static ScreenCastPortal &instance()
    {
        static ScreenCastPortal instance;
        return instance;
    }

    void init();
    const Stream *getStream() const;
    bool ready() const;
    void close(bool reinit = false);

public slots:
    void createSessionHandler(uint response, const QVariantMap &results);
    void closedHandler(uint response, const QVariantMap &results);
    void selectSourcesHandler(uint response, const QVariantMap &results);
    void startHandler(uint response, const QVariantMap &results);

signals:
    void readyChanged();

private:
    void createSession();
    void getAvailableSourceTypes();
    void getAvailableCursorModes();
    void selectSources();
    void start();
    void openPipeWireRemote();
    bool makeConnection(QString service,
                        QString path,
                        QString interface,
                        QString name,
                        const char *slot);
    void removeConnection();
    void disconnectClose();
    QDBusObjectPath sessionHandle;
    uint availableSourceTypes;
    uint availableCursorModes;

    Stream stream;

    enum class State
    {
        Closed,
        Starting,
        Started,
        Closing,
    };
    State state = State::Closed;
    std::optional<std::array<QString, 5>> last_connection;
};

#endif
