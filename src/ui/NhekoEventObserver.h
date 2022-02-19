// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickItem>

class NhekoEventObserver : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(bool wasTouched READ wasTouched NOTIFY wasTouchedChanged)

public:
    explicit NhekoEventObserver(QQuickItem *parent = 0);

    bool childMouseEventFilter(QQuickItem *item, QEvent *event) override;

private:
    bool wasTouched() { return wasTouched_; }

    bool wasTouched_ = false;

signals:
    void wasTouchedChanged();
};
