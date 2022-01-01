// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// see
// https://stackoverflow.com/questions/27821054/how-to-change-cursor-shape-in-qml-when-mousearea-is-covered-with-another-mousear/29382092#29382092

#include <QQuickItem>

class NhekoCursorShape : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(
      Qt::CursorShape cursorShape READ cursorShape WRITE setCursorShape NOTIFY cursorShapeChanged)

public:
    explicit NhekoCursorShape(QQuickItem *parent = 0);

private:
    Qt::CursorShape cursorShape() const;
    void setCursorShape(Qt::CursorShape cursorShape);

    Qt::CursorShape currentShape_;

signals:
    void cursorShapeChanged();
};
