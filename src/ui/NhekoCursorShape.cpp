// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoCursorShape.h"

#include <QCursor>

NhekoCursorShape::NhekoCursorShape(QQuickItem *parent)
  : QQuickItem(parent)
  , currentShape_(Qt::CursorShape::ArrowCursor)
{}

Qt::CursorShape
NhekoCursorShape::cursorShape() const
{
    return cursor().shape();
}

void
NhekoCursorShape::setCursorShape(Qt::CursorShape cursorShape)
{
    if (currentShape_ == cursorShape)
        return;

    currentShape_ = cursorShape;
    setCursor(cursorShape);
    emit cursorShapeChanged();
}
