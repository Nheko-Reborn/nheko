// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoEventObserver.h"

#include <QMouseEvent>

#include "Logging.h"

NhekoEventObserver::NhekoEventObserver(QQuickItem *parent)
  : QQuickItem(parent)
{
    setFiltersChildMouseEvents(true);
}

bool
NhekoEventObserver::childMouseEventFilter(QQuickItem * /*item*/, QEvent *event)
{
    // nhlog::ui()->debug("Touched {}", item->metaObject()->className());

    auto setTouched = [this](bool touched) {
        if (touched != this->wasTouched_) {
            this->wasTouched_ = touched;
            emit wasTouchedChanged();
        }
    };

    // see
    // https://code.qt.io/cgit/qt/qtdeclarative.git/tree/src/quicktemplates2/qquickscrollview.cpp?id=7f29e89c26ae2babc358b1c4e6f965af6ec759f4#n471
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
        setTouched(true);
        break;

    case QEvent::MouseButtonPress:
        if (static_cast<QMouseEvent *>(event)->source() == Qt::MouseEventNotSynthesized) {
            setTouched(false);
        }
        break;

    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        if (static_cast<QMouseEvent *>(event)->source() == Qt::MouseEventNotSynthesized)
            setTouched(false);
        break;

    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    case QEvent::Wheel:
        setTouched(false);
        break;

    default:
        break;
    }

    return false;
}
