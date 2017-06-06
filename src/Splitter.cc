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

#include <QDebug>

#include "Splitter.h"
#include "Theme.h"

Splitter::Splitter(QWidget *parent)
    : QSplitter(parent)
{
	connect(this, &QSplitter::splitterMoved, this, &Splitter::onSplitterMoved);
	setChildrenCollapsible(false);
}

void Splitter::onSplitterMoved(int pos, int index)
{
	Q_UNUSED(pos);
	Q_UNUSED(index);

	auto s = sizes();

	if (s.count() < 2) {
		qWarning() << "Splitter needs at least two children";
		return;
	}

	if (s[0] == ui::sidebar::NormalSize) {
		rightMoveCount_ += 1;

		if (rightMoveCount_ > moveEventLimit_) {
			auto left = widget(0);
			auto pos = left->mapFromGlobal(QCursor::pos());

			// if we are coming from the right, the cursor should
			// end up on the first widget.
			if (left->rect().contains(pos)) {
				left->setMinimumWidth(ui::sidebar::SmallSize);
				left->setMaximumWidth(ui::sidebar::SmallSize);

				rightMoveCount_ = 0;
			}
		}
	} else if (s[0] == ui::sidebar::SmallSize) {
		leftMoveCount_ += 1;

		if (leftMoveCount_ > moveEventLimit_) {
			auto left = widget(0);
			auto right = widget(1);
			auto pos = right->mapFromGlobal(QCursor::pos());

			// We move the start a little further so the transition isn't so abrupt.
			auto extended = right->rect();
			extended.translate(100, 0);

			// if we are coming from the left, the cursor should
			// end up on the second widget.
			if (extended.contains(pos)) {
				left->setMinimumWidth(ui::sidebar::NormalSize);
				left->setMaximumWidth(2 * ui::sidebar::NormalSize);

				leftMoveCount_ = 0;
			}
		}
	}
}
