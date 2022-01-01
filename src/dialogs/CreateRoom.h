// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFrame>

#include <mtx/requests.hpp>

class QPushButton;
class TextField;
class QComboBox;
class Toggle;

namespace dialogs {

class CreateRoom : public QFrame
{
    Q_OBJECT
public:
    CreateRoom(QWidget *parent = nullptr);

signals:
    void createRoom(const mtx::requests::CreateRoom &request);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void clearFields();

    QComboBox *visibilityCombo_;
    QComboBox *presetCombo_;

    Toggle *directToggle_;

    QPushButton *confirmBtn_;
    QPushButton *cancelBtn_;

    TextField *nameInput_;
    TextField *topicInput_;
    TextField *aliasInput_;

    mtx::requests::CreateRoom request_;
};

} // dialogs
