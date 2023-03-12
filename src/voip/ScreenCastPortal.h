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

class ScreenCastPortal final : public QObject
{
    Q_OBJECT

public:
    struct Stream
    {
        int fd;
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
};

#endif
