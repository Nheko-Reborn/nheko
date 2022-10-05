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
    }
    emit threadIdChanged();
}

void
TimelineFilter::setSource(TimelineModel *s)
{
    if (auto orig = this->source(); orig != s) {
        if (orig)
            disconnect(orig,
                       &TimelineModel::currentIndexChanged,
                       this,
                       &TimelineFilter::currentIndexChanged);
        this->setSourceModel(s);
        connect(s, &TimelineModel::currentIndexChanged, this, &TimelineFilter::currentIndexChanged);
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
    if (threadId.isEmpty())
        return true;

    if (auto s = sourceModel()) {
        auto idx = s->index(source_row, 0);
        return s->data(idx, TimelineModel::EventId) == threadId ||
               s->data(idx, TimelineModel::ThreadId) == threadId;
    } else {
        return true;
    }
}
