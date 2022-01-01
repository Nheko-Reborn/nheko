// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSize>
#include <QString>
#include <QTextBrowser>
#include <QUrl>

class QMouseEvent;
class QFocusEvent;
class QWheelEvent;

class ContextMenuFilter : public QObject
{
    Q_OBJECT

public:
    explicit ContextMenuFilter(QWidget *parent)
      : QObject(parent)
    {}

signals:
    void contextMenuIsOpening();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

class TextLabel : public QTextBrowser
{
    Q_OBJECT

public:
    TextLabel(const QString &text, QWidget *parent = nullptr);
    TextLabel(QWidget *parent = nullptr);

    void wheelEvent(QWheelEvent *event) override;
    void clearLinks() { link_.clear(); }

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private slots:
    void adjustHeight(const QSizeF &size) { setFixedHeight(size.height()); }
    void handleLinkActivation(const QUrl &link);

signals:
    void userProfileTriggered(const QString &user_id);
    void linkActivated(const QUrl &link);

private:
    QString link_;
    bool contextMenuRequested_ = false;
};
