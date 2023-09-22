// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

// #include "httplib.h"

#include <QHttpServer>
#include <QObject>

#include <string>

class SSOHandler final : public QObject
{
    Q_OBJECT

public:
    SSOHandler(QObject *parent = nullptr);

    ~SSOHandler();

    std::string url() const;

signals:
    void ssoSuccess(std::string token);
    void ssoFailed();

private:
    QHttpServer *server;
    int port = 0;
};
