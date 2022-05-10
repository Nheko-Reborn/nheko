// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>

#include <QObject>

typedef struct _GstDevice GstDevice;

class CallDevices : public QObject
{
    Q_OBJECT

public:
    static CallDevices &instance()
    {
        static CallDevices instance;
        return instance;
    }

    bool haveMic() const;
    bool haveCamera() const;
    std::vector<std::string> names(bool isVideo, const std::string &defaultDevice) const;
    std::vector<std::string> resolutions(const std::string &cameraName) const;
    std::vector<std::string>
    frameRates(const std::string &cameraName, const std::string &resolution) const;

signals:
    void devicesChanged();

private:
    CallDevices();

    friend class WebRTCSession;
    void init();
    GstDevice *audioDevice() const;
    GstDevice *videoDevice(std::pair<int, int> &resolution, std::pair<int, int> &frameRate) const;

public:
    CallDevices(CallDevices const &) = delete;
    void operator=(CallDevices const &) = delete;
};
