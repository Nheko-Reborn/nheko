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

#include "emoji/Panel.h"
#include "emoji/PickButton.h"

using namespace emoji;

// Number of milliseconds after which the panel will be hidden
// if the mouse cursor is not on top of the widget.
constexpr int TimeoutDuration = 300;

PickButton::PickButton(QWidget *parent)
  : FlatButton(parent)
  , panel_{nullptr}
{
        connect(&hideTimer_, &QTimer::timeout, this, [this]() {
                if (panel_ && !panel_->underMouse()) {
                        hideTimer_.stop();
                        panel_->hide();
                }
        });
}

void
PickButton::enterEvent(QEvent *e)
{
        Q_UNUSED(e);

        if (panel_.isNull()) {
                panel_ = QSharedPointer<Panel>(new Panel(this));
                connect(panel_.data(), &Panel::emojiSelected, this, &PickButton::emojiSelected);
                connect(panel_.data(), &Panel::leaving, this, [this]() { panel_->hide(); });
        }

        if (panel_->isVisible())
                return;

        QPoint pos(rect().x(), rect().y());
        pos = this->mapToGlobal(pos);

        auto panel_size = panel_->sizeHint();

        auto x = pos.x() - panel_size.width() + horizontal_distance_;
        auto y = pos.y() - panel_size.height() - vertical_distance_;

        panel_->move(x, y);
        panel_->show();
}

void
PickButton::leaveEvent(QEvent *e)
{
        hideTimer_.start(TimeoutDuration);
        FlatButton::leaveEvent(e);
}
