// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemView>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QWidget>

#include "popups/SuggestionsPopup.h"
#include "ui/TextField.h"

class RoomSearchInput : public TextField
{
        Q_OBJECT
public:
        explicit RoomSearchInput(QWidget *parent = nullptr);

signals:
        void selectNextCompletion();
        void selectPreviousCompletion();
        void hiding();

protected:
        void keyPressEvent(QKeyEvent *event) override;
        void hideEvent(QHideEvent *event) override;
        bool focusNextPrevChild(bool) override { return false; };
};

class QuickSwitcher : public QWidget
{
        Q_OBJECT

public:
        QuickSwitcher(QWidget *parent = nullptr);

signals:
        void closing();
        void roomSelected(const QString &roomid);
        void queryResults(const std::vector<RoomSearchResult> &rooms);

protected:
        void keyPressEvent(QKeyEvent *event) override;
        void showEvent(QShowEvent *) override { roomSearch_->setFocus(); }
        void paintEvent(QPaintEvent *event) override;

private:
        void reset()
        {
                emit closing();
                roomSearch_->clear();
        }

        // Current highlighted selection from the completer.
        int selection_ = -1;

        QVBoxLayout *topLayout_;
        RoomSearchInput *roomSearch_;

        //! Autocomplete popup box with the room suggestions.
        SuggestionsPopup popup_;
};
