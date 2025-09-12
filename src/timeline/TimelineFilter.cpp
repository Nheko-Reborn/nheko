// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimelineFilter.h"

#include <QCoreApplication>
#include <QEvent>

#include "Logging.h"
#include "TimelineModel.h"

/// Searching currently can be done incrementally. For that we define a specific role to filter on
/// and then process that role in chunk. This is the `FilterRole`. Of course we need to then also
/// send proper update signals. Filtering then works as follows:
///
/// - At first no range is filtered (incrementalSearchIndex == 0).
/// - Then, when filtering is requested, we start posting events to the
/// event loop with lower than low priority (low prio - 1). The only thing those events do is
/// increment the incrementalSearchIndex and emit a dataChanged for that range of events.
/// - This then causes those events to be reevaluated if they should be visible.

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
    emit isFilteringChanged();
    invalidateFilter();
    beginResetModel();
    endResetModel();

    continueFiltering();
}

void
TimelineFilter::continueFiltering()
{
    if (auto s = source(); s) {
        if (s->rowCount() > incrementalSearchIndex) {
            auto ev = new QEvent(getFilterEventType());
            // request filtering a new chunk with lower than low priority.
            QCoreApplication::postEvent(this, ev, Qt::LowEventPriority - 1);
        } else {
            // We reached the end, so fetch more!
            fetchAgain();
        }
    }
}

bool
TimelineFilter::event(QEvent *ev)
{
    if (ev->type() == getFilterEventType()) {
        if (incrementalSearchIndex < std::numeric_limits<int>::max()) {
            int orgIndex = incrementalSearchIndex;
            // process the next 100 events by claiming their "filterrole" data has changed.
            incrementalSearchIndex += 100;

            if (auto s = source(); s) {
                auto count = s->rowCount();
                if (incrementalSearchIndex >= count) {
                    incrementalSearchIndex = std::numeric_limits<int>::max();
                }
                nhlog::ui()->debug("Filter progress {}/{}", incrementalSearchIndex, count);
                s->dataChanged(s->index(orgIndex),
                               s->index(std::min(incrementalSearchIndex, count - 1)),
                               {FilterRole});

                continueFiltering();
            }
            emit isFilteringChanged();
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        beginFilterChange();
#endif

        this->threadId = t;

        emit threadIdChanged();
        startFiltering();
        fetchMore({});
    }
}

void
TimelineFilter::setFilterNotifications(bool filter)
{
    nhlog::ui()->debug("Filtering by notifications '{}'", filter);
    if (this->filterByNotifications_ != filter) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        beginFilterChange();
#endif
        this->filterByNotifications_ = filter;

        emit filterNotificationsChanged();
        startFiltering();
        fetchMore({});
    }
}

void
TimelineFilter::setContentFilter(const QString &c)
{
    nhlog::ui()->debug("Filtering by content '{}'", c.toStdString());
    if (this->contentFilter != c) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        beginFilterChange();
#endif
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

    if (auto s = source(); s && incrementalSearchIndex == std::numeric_limits<int>::max()) {
        if (this->rowCount() == cachedCount && s->canFetchMore(QModelIndex()) &&
            // If we already have the event id of the thread in the timeline and we are filtering by
            // thread, we can stop fetching more messages In theory an event could have been edited
            // earlier in the timeline into the thread. So in theory this check is insufficient and
            // we should instead verify that all events referring to this thread are in the timeline
            // instead of just the thread root, but only Nheko supports that atm and the check would
            // be expensive.
            // TODO(Nico): check that all thread referrencing events are in the timeline by also
            // checking all edits inside the thread.
            (threadId.isEmpty() || s->idToIndex(threadId) == -1))
            s->fetchMore(QModelIndex());
        else
            cachedCount = this->rowCount();
    }
}

void
TimelineFilter::sourceDataChanged(const QModelIndex &topLeft,
                                  const QModelIndex &bottomRight,
                                  const QVector<int> &roles)
{
    if (!roles.contains(TimelineModel::Roles::Body) && !roles.contains(TimelineModel::ThreadId) &&
        !roles.contains(TimelineModel::Notificationlevel))
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

        if (s) {
            connect(
              s, &TimelineModel::currentIndexChanged, this, &TimelineFilter::currentIndexChanged);
            connect(s,
                    &TimelineModel::fetchedMore,
                    this,
                    &TimelineFilter::fetchAgain,
                    Qt::QueuedConnection);
            connect(s,
                    &TimelineModel::dataChanged,
                    this,
                    &TimelineFilter::sourceDataChanged,
                    Qt::QueuedConnection);
        }

        // reset the search index a second time just to be safe.
        incrementalSearchIndex = 0;

        emit sourceChanged();
        emit isFilteringChanged();
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
TimelineFilter::isFiltering() const
{
    return incrementalSearchIndex != std::numeric_limits<int>::max() &&
           !(threadId.isEmpty() && contentFilter.isEmpty());
}

bool
TimelineFilter::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    // this chunk is still unfiltered.
    if (source_row > incrementalSearchIndex)
        return false;

    if (threadId.isEmpty() && contentFilter.isEmpty() && !filterByNotifications_)
        return true;

    if (auto s = sourceModel()) {
        auto idx = s->index(source_row, 0);

        if (!contentFilter.isEmpty() && !s->data(idx, TimelineModel::Body)
                                           .toString()
                                           .contains(contentFilter, Qt::CaseInsensitive)) {
            return false;
        }

        if (filterByNotifications_ && s->data(idx, TimelineModel::Notificationlevel)
                                          .value<qml_mtx_events::NotificationLevel>() !=
                                        qml_mtx_events::NotificationLevel::Highlight) {
            return false;
        }

        if (threadId.isEmpty()) {
            return true;
        }

        return s->data(idx, TimelineModel::EventId) == threadId ||
               s->data(idx, TimelineModel::ThreadId) == threadId;
    } else {
        return true;
    }
}

#include "moc_TimelineFilter.cpp"
