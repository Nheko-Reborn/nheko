// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>

#include <QObject>
#include <QQmlEngine>

typedef struct _GstDevice GstDevice;

class CallDevices final : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HardwareCallDevices)
    QML_SINGLETON

    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)

public:
    static CallDevices &instance()
    {
        static CallDevices instance;
        return instance;
    }

    static CallDevices *create(QQmlEngine *qmlEngine, QJSEngine *)
    {
        // The instance has to exist before it is used. We cannot replace it.
        auto instance = &CallDevices::instance();

        // The engine has to have the same thread affinity as the singleton.
        Q_ASSERT(qmlEngine->thread() == instance->thread());

        // There can only be one engine accessing the singleton.
        static QJSEngine *s_engine = nullptr;
        if (s_engine)
            Q_ASSERT(qmlEngine == s_engine);
        else
            s_engine = qmlEngine;

        QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
        return instance;
    }

    bool scanning() const { return scanning_; }
    bool haveMic() const;
    bool haveCamera() const;
    std::vector<std::string> names(bool isVideo, const std::string &defaultDevice) const;
    std::vector<std::string> resolutions(const std::string &cameraName) const;
    std::vector<std::string>
    frameRates(const std::string &cameraName, const std::string &resolution) const;

signals:
    void devicesChanged();
    void scanningChanged();

private:
    CallDevices();

    friend class WebRTCSession;
    void init();
    GstDevice *audioDevice() const;
    GstDevice *videoDevice(std::pair<int, int> &resolution, std::pair<int, int> &frameRate) const;
    unsigned int busWatchId_ = 0;
    bool scanning_           = false;

public:
    CallDevices(CallDevices const &)    = delete;
    void operator=(CallDevices const &) = delete;
    void deinit();
    void refresh();
};
