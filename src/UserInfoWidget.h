// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class Avatar;
class FlatButton;
class OverlayModal;

class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QMenu;

class UserInfoWidget : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)

public:
        UserInfoWidget(QWidget *parent = nullptr);

        void setDisplayName(const QString &name);
        void setUserId(const QString &userid);
        void setAvatar(const QString &url);

        void reset();

        QColor borderColor() const { return borderColor_; }
        void setBorderColor(QColor &color) { borderColor_ = color; }

protected:
        void resizeEvent(QResizeEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
        void contextMenuEvent(QContextMenuEvent *) override;

signals:
        void openGlobalUserProfile();

private:
        Avatar *userAvatar_;

        QHBoxLayout *topLayout_;
        QHBoxLayout *avatarLayout_;
        QVBoxLayout *textLayout_;
        QHBoxLayout *buttonLayout_;

        FlatButton *logoutButton_;

        QLabel *displayNameLabel_;
        QLabel *userIdLabel_;

        QString display_name_;
        QString user_id_;

        QImage avatar_image_;

        int logoutButtonSize_;

        QColor borderColor_;

        QMenu *menu = nullptr;
};
