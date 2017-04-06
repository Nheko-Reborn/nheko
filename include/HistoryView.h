/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HISTORY_VIEW_H
#define HISTORY_VIEW_H

#include <QHBoxLayout>
#include <QList>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "HistoryViewItem.h"
#include "Sync.h"

class HistoryView : public QWidget
{
	Q_OBJECT

public:
	explicit HistoryView(QWidget *parent = 0);
	explicit HistoryView(const QList<Event> &events, QWidget *parent = 0);
	~HistoryView();

	void addHistoryItem(const Event &event, const QString &color, bool with_sender);
	void addEvents(const QList<Event> &events);

public slots:
	void sliderRangeChanged(int min, int max);

private:
	static const QList<QString> COLORS;

	void init();

	QString chooseRandomColor();

	QVBoxLayout *top_layout_;
	QVBoxLayout *scroll_layout_;

	QScrollArea *scroll_area_;
	QWidget *scroll_widget_;

	QString last_sender_;
	QMap<QString, QString> nick_colors_;
};

#endif  // HISTORY_VIEW_H
