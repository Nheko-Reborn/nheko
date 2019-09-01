#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDate>
#include <QHash>

#include <mtx/responses.hpp>

class TimelineModel : public QAbstractListModel
{
        Q_OBJECT

public:
        explicit TimelineModel(QString room_id, QObject *parent = 0);

        enum Roles
        {
                Section,
                Type,
                Body,
                FormattedBody,
                UserId,
                UserName,
                Timestamp,
        };

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

        Q_INVOKABLE QColor userColor(QString id, QColor background);
        Q_INVOKABLE QString displayName(QString id) const;
        Q_INVOKABLE QString formatDateSeparator(QDate date) const;

        void addEvents(const mtx::responses::Timeline &events);

public slots:
        void fetchHistory();

private slots:
        // Add old events at the top of the timeline.
        void addBackwardsEvents(const mtx::responses::Messages &msgs);

signals:
        void oldMessagesRetrieved(const mtx::responses::Messages &res);

private:
        QHash<QString, mtx::events::collections::TimelineEvents> events;
        std::vector<QString> eventOrder;

        QString room_id_;
        QString prev_batch_token_;

        bool isInitialSync = true;
        bool paginationInProgress = false;

        QHash<QString, QColor> userColors;
};
  
