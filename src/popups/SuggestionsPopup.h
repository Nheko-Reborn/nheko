// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

#include "CacheStructs.h"

class QVBoxLayout;
class QLayoutItem;

class SuggestionsPopup : public QWidget
{
        Q_OBJECT

public:
        explicit SuggestionsPopup(QWidget *parent = nullptr);

        void selectHoveredSuggestion();

public slots:
        void addRooms(const std::vector<RoomSearchResult> &rooms);

        //! Move to the next available suggestion item.
        void selectNextSuggestion();
        //! Move to the previous available suggestion item.
        void selectPreviousSuggestion();
        //! Remove hovering from all items.
        void resetHovering();
        //! Set hovering to the item in the given layout position.
        void setHovering(int pos);

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void itemSelected(const QString &user);

private:
        QString displayName(QString roomid, QString userid);
        void hoverSelection();
        void resetSelection() { selectedItem_ = -1; }
        void selectFirstItem() { selectedItem_ = 0; }
        void selectLastItem();
        void removeLayoutItemsAfter(size_t startingPos);

        QVBoxLayout *layout_;

        //! Counter for tab completion (cycling).
        int selectedItem_ = -1;
};
