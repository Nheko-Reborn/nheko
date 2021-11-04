// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QDateTime>
#include <QWidget>

class InfoMessage : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)
    Q_PROPERTY(QColor boxColor WRITE setBoxColor READ boxColor)

public:
    explicit InfoMessage(QWidget *parent = nullptr);
    InfoMessage(QString msg, QWidget *parent = nullptr);

    void setTextColor(QColor color) { textColor_ = color; }
    void setBoxColor(QColor color) { boxColor_ = color; }
    void saveDatetime(QDateTime datetime) { datetime_ = datetime; }

    QColor textColor() const { return textColor_; }
    QColor boxColor() const { return boxColor_; }
    QDateTime datetime() const { return datetime_; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void initFont()
    {
        QFont f;
        f.setWeight(QFont::Medium);
        setFont(f);
    }

    int width_;
    int height_;

    QString msg_;

    QDateTime datetime_;

    QColor textColor_ = QColor("black");
    QColor boxColor_  = QColor("white");
};

class DateSeparator : public InfoMessage
{
    Q_OBJECT

public:
    DateSeparator(QDateTime datetime, QWidget *parent = nullptr);
};
