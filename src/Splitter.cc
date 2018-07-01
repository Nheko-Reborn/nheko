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

#include "Splitter.h"
#include "Theme.h"

Splitter::Splitter(QWidget *parent)
  : QSplitter(parent)
{
        connect(this, &QSplitter::splitterMoved, this, &Splitter::onSplitterMoved);
        setChildrenCollapsible(false);
        setStyleSheet("QSplitter::handle { image: none; }");

        auto showChatShortcut = new QShortcut(QKeySequence(tr("Ctrl+O", "Show chat")), parent);
        auto showSidebarShortcut =
          new QShortcut(QKeySequence(tr("Ctrl+L", "Show sidebar")), parent);

        connect(showChatShortcut, &QShortcut::activated, this, [this]() {
                if (count() != 2)
                        return;

                hideSidebar();
                widget(1)->show();
        });
        connect(showSidebarShortcut, &QShortcut::activated, this, [this]() {
                if (count() != 2)
                        return;

                widget(0)->setMinimumWidth(ui::sidebar::NormalSize);
                widget(0)->setMaximumWidth(QApplication::desktop()->screenGeometry().height());

                widget(0)->show();
                widget(1)->hide();
        });
}

void
Splitter::restoreSizes(int fallback)
{
        QSettings settings;
        int savedWidth = settings.value("sidebar/width").toInt();

        auto left = widget(0);
        if (savedWidth == ui::sidebar::SmallSize) {
                if (left) {
                        left->setMinimumWidth(ui::sidebar::SmallSize);
                        left->setMaximumWidth(ui::sidebar::SmallSize);
                        return;
                }
        }

        if (savedWidth >= ui::sidebar::NormalSize && savedWidth <= 2 * ui::sidebar::NormalSize) {
                if (left) {
                        left->setMinimumWidth(ui::sidebar::NormalSize);
                        left->setMaximumWidth(2 * ui::sidebar::NormalSize);
                        setSizes({savedWidth, fallback - savedWidth});
                        return;
                }
        }

        if (savedWidth == 0) {
                hideSidebar();
                return;
        }

        setSizes({ui::sidebar::NormalSize, fallback - ui::sidebar::NormalSize});
}

Splitter::~Splitter()
{
        auto left = widget(0);

        if (left) {
                QSettings settings;

                if (!left->isVisible())
                        settings.setValue("sidebar/width", 0);
                else
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
                        auto left = widget(0);
                        auto pos  = left->mapFromGlobal(QCursor::pos());

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
                        auto left  = widget(0);
                        auto right = widget(1);
                        auto pos   = right->mapFromGlobal(QCursor::pos());

                        // We move the start a little further so the transition isn't so abrupt.
                        auto extended = right->rect();
                        extended.translate(100, 0);

                        // if we are coming from the left, the cursor should
                        // end up on the second widget.
                        if (extended.contains(pos)) {
                                left->setMinimumWidth(ui::sidebar::NormalSize);
                                left->setMaximumWidth(2 * ui::sidebar::NormalSize);

                                leftMoveCount_ = 0;
                        } else if (left->rect().contains(left->mapFromGlobal(QCursor::pos()))) {
                                hideSidebar();
                        }
                }
        }
}
void
Splitter::showChatView()
{
        if (count() != 2)
                return;

        auto right = widget(1);

        // We are in Roomlist-only view so we'll switch into Chat-only view.
        if (!right->isVisible()) {
                right->show();
                hideSidebar();
        }
}

void
Splitter::showSidebar()
{
        auto left = widget(0);
        if (left) {
                left->setMinimumWidth(ui::sidebar::SmallSize);
                left->setMaximumWidth(ui::sidebar::SmallSize);
                left->show();
        }
}

void
Splitter::hideSidebar()
{
        auto left = widget(0);
        if (left) {
                left->hide();
                emit hiddenSidebar();
        }
}
