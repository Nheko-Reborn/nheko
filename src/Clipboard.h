// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

class Clipboard final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    Clipboard(QObject *parent = nullptr);

    QString text() const;
    void setText(QString text_);
signals:
    void textChanged();
};
