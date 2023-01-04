// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>
#include <QString>

#include <mtx/events/power_levels.hpp>

#include "TimelineModel.h"

class TimelineFilter : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString filterByThread READ filterByThread WRITE setThreadId NOTIFY threadIdChanged)
    Q_PROPERTY(QString filterByContent READ filterByContent WRITE setContentFilter NOTIFY
                 contentFilterChanged)
    Q_PROPERTY(TimelineModel *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(bool filteringInProgress READ isFiltering NOTIFY isFilteringChanged)

public:
    explicit TimelineFilter(QObject *parent = nullptr);

    QString filterByThread() const { return threadId; }
    QString filterByContent() const { return contentFilter; }
    TimelineModel *source() const;
    int currentIndex() const;
    bool isFiltering() const;

    void setThreadId(const QString &t);
    void setContentFilter(const QString &t);
    void setSource(TimelineModel *t);
    void setCurrentIndex(int idx);

    Q_INVOKABLE QVariant dataByIndex(int i, int role = Qt::DisplayRole) const
    {
        return data(index(i, 0), role);
    }

    bool event(QEvent *ev) override;

signals:
    void threadIdChanged();
    void contentFilterChanged();
    void sourceChanged();
    void currentIndexChanged();
    void isFilteringChanged();

private slots:
    void fetchAgain();
    void sourceDataChanged(const QModelIndex &topLeft,
                           const QModelIndex &bottomRight,
                           const QVector<int> &roles);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    void startFiltering();
    void continueFiltering();

    QString threadId, contentFilter;
    int cachedCount = 0, incrementalSearchIndex = 0;
};
