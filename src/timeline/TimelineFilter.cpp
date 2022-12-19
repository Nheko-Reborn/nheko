// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimelineFilter.h"

#include <QCoreApplication>
#include <QEvent>

#include "Logging.h"

static int FilterRole = Qt::UserRole * 3;

static QEvent::Type
getFilterEventType()
{
    static QEvent::Type t = static_cast<QEvent::Type>(QEvent::registerEventType());
    return t;
}

TimelineFilter::TimelineFilter(QObject *parent)
  : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setFilterRole(FilterRole);
}

void
TimelineFilter::startFiltering()
{
    incrementalSearchIndex = 0;
    continueFiltering();
}

void
TimelineFilter::continueFiltering()
{
    if (auto s = source(); s && s->rowCount() > incrementalSearchIndex) {
        auto ev = new QEvent(getFilterEventType());
        QCoreApplication::postEvent(this, ev, Qt::LowEventPriority - 1);
    }
}

bool
TimelineFilter::event(QEvent *ev)
{
    if (ev->type() == getFilterEventType()) {
        int orgIndex = incrementalSearchIndex;
        incrementalSearchIndex += 30;

        if (auto s = source(); s) {
            s->dataChanged(s->index(orgIndex),
                           s->index(std::min(incrementalSearchIndex, s->rowCount() - 1)),
                           {FilterRole});
            continueFiltering();
        }
        return true;
    }
    return QSortFilterProxyModel::event(ev);
}

void
TimelineFilter::setThreadId(const QString &t)
{
    nhlog::ui()->debug("Filtering by thread '{}'", t.toStdString());
    if (this->threadId != t) {
        this->threadId = t;

        emit threadIdChanged();
        startFiltering();
        fetchMore({});
    }
}

void
TimelineFilter::setContentFilter(const QString &c)
{
    nhlog::ui()->debug("Filtering by content '{}'", c.toStdString());
    if (this->contentFilter != c) {
        this->contentFilter = c;

        emit contentFilterChanged();
        startFiltering();
        fetchMore({});
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
TimelineFilter::sourceDataChanged(const QModelIndex &topLeft,
                                  const QModelIndex &bottomRight,
                                  const QVector<int> &roles)
{
    if (!roles.contains(TimelineModel::Roles::Body) && !roles.contains(TimelineModel::ThreadId))
        return;

    if (auto s = source()) {
        s->dataChanged(topLeft, bottomRight, {FilterRole});
    }
}

void
TimelineFilter::setSource(TimelineModel *s)
{
    if (auto orig = this->source(); orig != s) {
        cachedCount            = 0;
        incrementalSearchIndex = 0;

        if (orig) {
            disconnect(orig,
                       &TimelineModel::currentIndexChanged,
                       this,
                       &TimelineFilter::currentIndexChanged);
            disconnect(orig, &TimelineModel::fetchedMore, this, &TimelineFilter::fetchAgain);
            disconnect(orig, &TimelineModel::dataChanged, this, &TimelineFilter::sourceDataChanged);
        }

        this->setSourceModel(s);

        connect(s, &TimelineModel::currentIndexChanged, this, &TimelineFilter::currentIndexChanged);
        connect(
          s, &TimelineModel::fetchedMore, this, &TimelineFilter::fetchAgain, Qt::QueuedConnection);
        connect(s,
                &TimelineModel::dataChanged,
                this,
                &TimelineFilter::sourceDataChanged,
                Qt::QueuedConnection);

        incrementalSearchIndex = 0;
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
    if (source_row > incrementalSearchIndex)
        return true;

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
