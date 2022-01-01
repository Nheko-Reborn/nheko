// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "httplib.h"

#include <QObject>
#include <string>

class SSOHandler : public QObject
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
    httplib::Server svr;
    int port = 0;
};
