// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <MatrixClient.h>

class UIA : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title NOTIFY titleChanged)

public:
    static UIA *instance();

    UIA(QObject *parent = nullptr)
      : QObject(parent)
    {}

    mtx::http::UIAHandler genericHandler(QString context);

    QString title() const { return title_; }

public slots:
    void continuePassword(QString password);

signals:
    void password();

    void titleChanged();

private:
    std::optional<mtx::http::UIAHandler> currentHandler;
    mtx::user_interactive::Unauthorized currentStatus;
    QString title_;
};
