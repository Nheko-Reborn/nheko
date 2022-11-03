// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimelineFilter.h"

#include "Logging.h"

TimelineFilter::TimelineFilter(QObject *parent)
  : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void
TimelineFilter::setThreadId(const QString &t)
{
    nhlog::ui()->debug("Filtering by thread '{}'", t.toStdString());
    if (this->threadId != t) {
        this->threadId = t;
        invalidateFilter();

        fetchMore({});
        emit threadIdChanged();
    }
}

void
TimelineFilter::setContentFilter(const QString &c)
{
    nhlog::ui()->debug("Filtering by content '{}'", c.toStdString());
    if (this->contentFilter != c) {
        this->contentFilter = c;
        invalidateFilter();

        fetchMore({});
        emit contentFilterChanged();
    }
}

void
TimelineFilter::fetchAgain()
{
    if (threadId.isEmpty() && contentFilter.isEmpty())
        return;

    if (auto s = source()) {
        if (rowCount() == cachedCount && s->canFetchMore(QModelIndex()))
            s->fetchMore(QModelIndex());
        else
            cachedCount = rowCount();
    }
}

void
TimelineFilter::setSource(TimelineModel *s)
{
    if (auto orig = this->source(); orig != s) {
        cachedCount = 0;

        if (orig) {
            disconnect(orig,
                       &TimelineModel::currentIndexChanged,
                       this,
                       &TimelineFilter::currentIndexChanged);
            disconnect(orig, &TimelineModel::fetchedMore, this, &TimelineFilter::fetchAgain);
        }

        this->setSourceModel(s);

        connect(s, &TimelineModel::currentIndexChanged, this, &TimelineFilter::currentIndexChanged);
        connect(s, &TimelineModel::fetchedMore, this, &TimelineFilter::fetchAgain);

        emit sourceChanged();
        invalidateFilter();
    }
}

TimelineModel *
TimelineFilter::source() const
{
    return qobject_cast<TimelineModel *>(sourceModel());
}

void
TimelineFilter::setCurrentIndex(int idx)
{
    // TODO: maybe send read receipt in thread timeline? Or not at all?
    if (auto s = source()) {
        s->setCurrentIndex(this->mapToSource(index(idx, 0)).row());
    }
}

int
TimelineFilter::currentIndex() const
{
    if (auto s = source())
        return this->mapFromSource(s->index(s->currentIndex())).row();
    else
        return -1;
}

bool
TimelineFilter::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    if (threadId.isEmpty() && contentFilter.isEmpty())
        return true;

    if (auto s = sourceModel()) {
        auto idx = s->index(source_row, 0);
        if (!contentFilter.isEmpty() && !s->data(idx, TimelineModel::Body)
                                           .toString()
                                           .contains(contentFilter, Qt::CaseInsensitive)) {
            return false;
        }

        if (threadId.isEmpty())
            return true;

        return s->data(idx, TimelineModel::EventId) == threadId ||
               s->data(idx, TimelineModel::ThreadId) == threadId;
    } else {
        return true;
    }
}
