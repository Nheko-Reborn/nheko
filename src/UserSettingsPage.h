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

#pragma once

#include <QComboBox>
#include <QFontDatabase>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QSharedPointer>
#include <QWidget>

class Toggle;

constexpr int OptionMargin       = 6;
constexpr int LayoutTopMargin    = 50;
constexpr int LayoutBottomMargin = LayoutTopMargin;

class UserSettings : public QObject
{
        Q_OBJECT

public:
        UserSettings();

        void save();
        void load();
        void applyTheme();
        void setTheme(QString theme);
        void setTray(bool state)
        {
                isTrayEnabled_ = state;
                save();
        }

        void setStartInTray(bool state)
        {
                isStartInTrayEnabled_ = state;
                save();
        }

        void setFontSize(double size);
        void setFontFamily(QString family);
        void setEmojiFontFamily(QString family);

        void setGroupView(bool state)
        {
                if (isGroupViewEnabled_ != state)
                        emit groupViewStateChanged(state);

                isGroupViewEnabled_ = state;
                save();
        }

        void setReadReceipts(bool state)
        {
                isReadReceiptsEnabled_ = state;
                save();
        }

        void setTypingNotifications(bool state)
        {
                isTypingNotificationsEnabled_ = state;
                save();
        }

        void setDesktopNotifications(bool state)
        {
                hasDesktopNotifications_ = state;
                save();
        }

        QString theme() const { return !theme_.isEmpty() ? theme_ : defaultTheme_; }
        bool isTrayEnabled() const { return isTrayEnabled_; }
        bool isStartInTrayEnabled() const { return isStartInTrayEnabled_; }
        bool isGroupViewEnabled() const { return isGroupViewEnabled_; }
        bool isTypingNotificationsEnabled() const { return isTypingNotificationsEnabled_; }
        bool isReadReceiptsEnabled() const { return isReadReceiptsEnabled_; }
        bool hasDesktopNotifications() const { return hasDesktopNotifications_; }
        double fontSize() const { return baseFontSize_; }
        QString font() const { return font_; }
        QString emojiFont() const { return emojiFont_; }

signals:
        void groupViewStateChanged(bool state);

private:
        // Default to system theme if QT_QPA_PLATFORMTHEME var is set.
        QString defaultTheme_ =
          QProcessEnvironment::systemEnvironment().value("QT_QPA_PLATFORMTHEME", "").isEmpty()
            ? "light"
            : "system";
        QString theme_;
        bool isTrayEnabled_;
        bool isStartInTrayEnabled_;
        bool isGroupViewEnabled_;
        bool isTypingNotificationsEnabled_;
        bool isReadReceiptsEnabled_;
        bool hasDesktopNotifications_;
        double baseFontSize_;
        QString font_;
        QString emojiFont_;
};

class HorizontalLine : public QFrame
{
        Q_OBJECT

public:
        HorizontalLine(QWidget *parent = nullptr);
};

class UserSettingsPage : public QWidget
{
        Q_OBJECT

public:
        UserSettingsPage(QSharedPointer<UserSettings> settings, QWidget *parent = 0);

protected:
        void showEvent(QShowEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

signals:
        void moveBack();
        void trayOptionChanged(bool value);
        void themeChanged();

private slots:
        void importSessionKeys();
        void exportSessionKeys();

private:
        // Layouts
        QVBoxLayout *topLayout_;
        QVBoxLayout *mainLayout_;
        QHBoxLayout *topBarLayout_;

        // Shared settings object.
        QSharedPointer<UserSettings> settings_;

        Toggle *trayToggle_;
        Toggle *startInTrayToggle_;
        Toggle *groupViewToggle_;
        Toggle *typingNotifications_;
        Toggle *readReceipts_;
        Toggle *desktopNotifications_;
        QLabel *deviceFingerprintValue_;
        QLabel *deviceIdValue_;

        QComboBox *themeCombo_;
        QComboBox *scaleFactorCombo_;
        QComboBox *fontSizeCombo_;
        QComboBox *fontSelectionCombo_;
        QComboBox *emojiFontSelectionCombo_;

        int sideMargin_ = 0;
};
