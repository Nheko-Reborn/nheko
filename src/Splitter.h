// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSplitter>

namespace splitter {
struct SideBarSizes
{
        int small;
        int normal;
        int groups;
        int collapsePoint;
};

SideBarSizes
calculateSidebarSizes(const QFont &f);
}

class Splitter : public QSplitter
{
        Q_OBJECT
public:
        explicit Splitter(QWidget *parent = nullptr);
        ~Splitter() override;

        void restoreSizes(int fallback);

public slots:
        void hideSidebar();
        void showFullRoomList();
        void showChatView();

signals:
        void hiddenSidebar();

private:
        void onSplitterMoved(int pos, int index);

        int moveEventLimit_ = 50;

        int leftMoveCount_  = 0;
        int rightMoveCount_ = 0;

        splitter::SideBarSizes sz_;
};
