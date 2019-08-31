#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QHash>

#include <mtx/responses.hpp>

class TimelineModel : public QAbstractListModel
{
        Q_OBJECT

public:
        explicit TimelineModel(QObject *parent = 0)
          : QAbstractListModel(parent)
        {}

        enum Roles
        {
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

        void addEvents(const mtx::responses::Timeline &events);

private:
        QHash<QString, mtx::events::collections::TimelineEvents> events;
        std::vector<QString> eventOrder;

        QHash<QString, QColor> userColors;
};
  
