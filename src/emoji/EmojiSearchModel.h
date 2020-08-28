#pragma once

#include "EmojiModel.h"

#include <QDebug>
#include <QEvent>
#include <QSortFilterProxyModel>
#include <qabstractitemmodel.h>
#include <qsortfilterproxymodel.h>

namespace emoji {

// Map emoji data to searchable data
class EmojiSearchModel : public QSortFilterProxyModel {
public:
	EmojiSearchModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {
		setSourceModel(new EmojiModel(this));
	}
	QVariant data(const QModelIndex &index, int role = Qt::UserRole + 1) const override {
		if (role == Qt::DisplayRole) {
			auto emoji = QSortFilterProxyModel::data(index, role).toString();
			return emoji + " :" + toShortcode(data(index, EmojiModel::ShortName).toString())
				+ ":";
		}
		return QSortFilterProxyModel::data(index, role);
	}	
	/*int rowCount(const QModelIndex &parent) const override {
    	auto row_count = QSortFilterProxyModel::rowCount(parent);
        return (row_count < 7) ? row_count : 7;
    }*/
private:
	QString toShortcode(QString shortname) const {
		return shortname.replace(" ", "-").replace(":", "-").replace("--", "-").toLower();
	}
};

}
