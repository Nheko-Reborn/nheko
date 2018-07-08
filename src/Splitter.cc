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

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QSettings>
#include <QShortcut>

#include "Config.h"
#include "Splitter.h"
#include "Theme.h"

constexpr auto MaxWidth = (1 << 24) - 1;

Splitter::Splitter(QWidget *parent)
  : QSplitter(parent)
{
        connect(this, &QSplitter::splitterMoved, this, &Splitter::onSplitterMoved);
        setChildrenCollapsible(false);
        setStyleSheet("QSplitter::handle { image: none; }");
}

void
Splitter::restoreSizes(int fallback)
{
        QSettings settings;
        int savedWidth = settings.value("sidebar/width").toInt();

        auto left = widget(0);
        if (savedWidth == 0) {
                hideSidebar();
                return;
        } else if (savedWidth == ui::sidebar::SmallSize) {
                if (left) {
                        left->setMinimumWidth(ui::sidebar::SmallSize);
                        left->setMaximumWidth(ui::sidebar::SmallSize);
                        return;
                }
        }

        left->setMinimumWidth(ui::sidebar::NormalSize);
        left->setMaximumWidth(2 * ui::sidebar::NormalSize);
        setSizes({ui::sidebar::NormalSize, fallback - ui::sidebar::NormalSize});

        setStretchFactor(0, 0);
        setStretchFactor(1, 1);
}

Splitter::~Splitter()
{
        auto left = widget(0);

        if (left) {
                QSettings settings;
                settings.setValue("sidebar/width", left->width());
        }
}

void
Splitter::onSplitterMoved(int pos, int index)
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
                        auto left           = widget(0);
                        auto cursorPosition = left->mapFromGlobal(QCursor::pos());

                        // if we are coming from the right, the cursor should
                        // end up on the first widget.
                        if (left->rect().contains(cursorPosition)) {
                                left->setMinimumWidth(ui::sidebar::SmallSize);
                                left->setMaximumWidth(ui::sidebar::SmallSize);

                                rightMoveCount_ = 0;
                        }
                }
        } else if (s[0] == ui::sidebar::SmallSize) {
                leftMoveCount_ += 1;

                if (leftMoveCount_ > moveEventLimit_) {
                        auto left           = widget(0);
                        auto right          = widget(1);
                        auto cursorPosition = right->mapFromGlobal(QCursor::pos());

                        // We move the start a little further so the transition isn't so abrupt.
                        auto extended = right->rect();
                        extended.translate(100, 0);

                        // if we are coming from the left, the cursor should
                        // end up on the second widget.
                        if (extended.contains(cursorPosition) &&
                            right->size().width() >=
                              conf::sideBarCollapsePoint + ui::sidebar::NormalSize) {
                                left->setMinimumWidth(ui::sidebar::NormalSize);
                                left->setMaximumWidth(2 * ui::sidebar::NormalSize);

                                leftMoveCount_ = 0;
                        }
                }
        }
}

void
Splitter::hideSidebar()
{
        auto left = widget(0);
        if (left)
                left->hide();
}

void
Splitter::showChatView()
{
        auto left  = widget(0);
        auto right = widget(1);

        if (right->isHidden()) {
                left->hide();
                right->show();

                // Restore previous size.
                if (left->minimumWidth() == ui::sidebar::SmallSize) {
                        left->setMinimumWidth(ui::sidebar::SmallSize);
                        left->setMaximumWidth(ui::sidebar::SmallSize);
                } else {
                        left->setMinimumWidth(ui::sidebar::NormalSize);
                        left->setMaximumWidth(2 * ui::sidebar::NormalSize);
                }
        }
}

void
Splitter::showFullRoomList()
{
        auto left  = widget(0);
        auto right = widget(1);

        right->hide();

        left->show();
        left->setMaximumWidth(MaxWidth);
}
