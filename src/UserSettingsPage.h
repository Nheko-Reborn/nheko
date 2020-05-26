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

#include <QFontDatabase>
#include <QFrame>
#include <QProcessEnvironment>
#include <QSharedPointer>
#include <QWidget>

class Toggle;
class QLabel;
class QFormLayout;
class QComboBox;
class QSpinBox;
class QHBoxLayout;
class QVBoxLayout;

constexpr int OptionMargin       = 6;
constexpr int LayoutTopMargin    = 50;
constexpr int LayoutBottomMargin = LayoutTopMargin;

class UserSettings : public QObject
{
        Q_OBJECT

        Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
        Q_PROPERTY(bool isMessageHoverHighlightEnabled READ isMessageHoverHighlightEnabled WRITE
                     setMessageHoverHighlight NOTIFY messageHoverHighlightChanged)
        Q_PROPERTY(bool enlargeEmojiOnlyMessages READ isEnlargeEmojiOnlyMessagesEnabled WRITE
                     setEnlargeEmojiOnlyMessages NOTIFY enlargeEmojiOnlyMessagesChanged)
        Q_PROPERTY(bool trayEnabled READ isTrayEnabled WRITE setTray NOTIFY trayChanged)
        Q_PROPERTY(bool startInTrayEnabled READ isStartInTrayEnabled WRITE setStartInTray NOTIFY
                     startInTrayChanged)
        Q_PROPERTY(bool groupViewEnabled READ isGroupViewEnabled WRITE setGroupView NOTIFY
                     groupViewStateChanged)
        Q_PROPERTY(
          bool markdown READ isMarkdownEnabled WRITE setMarkdownEnabled NOTIFY markdownChanged)
        Q_PROPERTY(bool typingNotifications READ isTypingNotificationsEnabled WRITE
                     setTypingNotifications NOTIFY typingNotificationsChanged)
        Q_PROPERTY(bool sortByImportance READ isSortByImportanceEnabled WRITE setSortByImportance
                     NOTIFY roomSortingChanged)
        Q_PROPERTY(bool buttonsInTimeline READ isButtonsInTimelineEnabled WRITE setButtonsInTimeline
                     NOTIFY buttonInTimelineChanged)
        Q_PROPERTY(bool readReceipts READ isReadReceiptsEnabled WRITE setReadReceipts NOTIFY
                     readReceiptsChanged)
        Q_PROPERTY(bool desktopNotifications READ hasDesktopNotifications WRITE
                     setDesktopNotifications NOTIFY desktopNotificationsChanged)
        Q_PROPERTY(bool avatarCircles READ isAvatarCirclesEnabled WRITE setAvatarCircles NOTIFY
                     avatarCirclesChanged)
        Q_PROPERTY(bool decryptSidebar READ isDecryptSidebarEnabled WRITE setDecryptSidebar NOTIFY
                     decryptSidebarChanged)
        Q_PROPERTY(int timelineMaxWidth READ timelineMaxWidth WRITE setTimelineMaxWidth NOTIFY
                     timelineMaxWidthChanged)
        Q_PROPERTY(double fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
        Q_PROPERTY(QString font READ font WRITE setFontFamily NOTIFY fontChanged)
        Q_PROPERTY(
          QString emojiFont READ emojiFont WRITE setEmojiFontFamily NOTIFY emojiFontChanged)

public:
        UserSettings();

        void save();
        void load();
        void applyTheme();
        void setTheme(QString theme);
        void setMessageHoverHighlight(bool state);
        void setEnlargeEmojiOnlyMessages(bool state);
        void setTray(bool state);
        void setStartInTray(bool state);
        void setFontSize(double size);
        void setFontFamily(QString family);
        void setEmojiFontFamily(QString family);
        void setGroupView(bool state);
        void setMarkdownEnabled(bool state);
        void setReadReceipts(bool state);
        void setTypingNotifications(bool state);
        void setSortByImportance(bool state);
        void setButtonsInTimeline(bool state);
        void setTimelineMaxWidth(int state);
        void setDesktopNotifications(bool state);
        void setAvatarCircles(bool state);
        void setDecryptSidebar(bool state);

        QString theme() const { return !theme_.isEmpty() ? theme_ : defaultTheme_; }
        bool isMessageHoverHighlightEnabled() const { return isMessageHoverHighlightEnabled_; }
        bool isEnlargeEmojiOnlyMessagesEnabled() const
        {
                return isEnlargeEmojiOnlyMessagesEnabled_;
        }
        bool isTrayEnabled() const { return isTrayEnabled_; }
        bool isStartInTrayEnabled() const { return isStartInTrayEnabled_; }
        bool isGroupViewEnabled() const { return isGroupViewEnabled_; }
        bool isAvatarCirclesEnabled() const { return avatarCircles_; }
        bool isDecryptSidebarEnabled() const { return decryptSidebar_; }
        bool isMarkdownEnabled() const { return isMarkdownEnabled_; }
        bool isTypingNotificationsEnabled() const { return isTypingNotificationsEnabled_; }
        bool isSortByImportanceEnabled() const { return sortByImportance_; }
        bool isButtonsInTimelineEnabled() const { return isButtonsInTimelineEnabled_; }
        bool isReadReceiptsEnabled() const { return isReadReceiptsEnabled_; }
        bool hasDesktopNotifications() const { return hasDesktopNotifications_; }
        int timelineMaxWidth() const { return timelineMaxWidth_; }
        double fontSize() const { return baseFontSize_; }
        QString font() const { return font_; }
        QString emojiFont() const { return emojiFont_; }

signals:
        void groupViewStateChanged(bool state);
        void roomSortingChanged(bool state);
        void themeChanged(QString state);
        void messageHoverHighlightChanged(bool state);
        void enlargeEmojiOnlyMessagesChanged(bool state);
        void trayChanged(bool state);
        void startInTrayChanged(bool state);
        void markdownChanged(bool state);
        void typingNotificationsChanged(bool state);
        void buttonInTimelineChanged(bool state);
        void readReceiptsChanged(bool state);
        void desktopNotificationsChanged(bool state);
        void avatarCirclesChanged(bool state);
        void decryptSidebarChanged(bool state);
        void timelineMaxWidthChanged(int state);
        void fontSizeChanged(double state);
        void fontChanged(QString state);
        void emojiFontChanged(QString state);

private:
        // Default to system theme if QT_QPA_PLATFORMTHEME var is set.
        QString defaultTheme_ =
          QProcessEnvironment::systemEnvironment().value("QT_QPA_PLATFORMTHEME", "").isEmpty()
            ? "light"
            : "system";
        QString theme_;
        bool isMessageHoverHighlightEnabled_;
        bool isEnlargeEmojiOnlyMessagesEnabled_;
        bool isTrayEnabled_;
        bool isStartInTrayEnabled_;
        bool isGroupViewEnabled_;
        bool isMarkdownEnabled_;
        bool isTypingNotificationsEnabled_;
        bool sortByImportance_;
        bool isButtonsInTimelineEnabled_;
        bool isReadReceiptsEnabled_;
        bool hasDesktopNotifications_;
        bool avatarCircles_;
        bool decryptSidebar_;
        int timelineMaxWidth_;
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
        UserSettingsPage(QSharedPointer<UserSettings> settings, QWidget *parent = nullptr);

protected:
        void showEvent(QShowEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

signals:
        void moveBack();
        void trayOptionChanged(bool value);
        void themeChanged();
        void decryptSidebarChanged();

private slots:
        void importSessionKeys();
        void exportSessionKeys();

private:
        // Layouts
        QVBoxLayout *topLayout_;
        QHBoxLayout *topBarLayout_;
        QFormLayout *formLayout_;

        // Shared settings object.
        QSharedPointer<UserSettings> settings_;

        Toggle *trayToggle_;
        Toggle *startInTrayToggle_;
        Toggle *groupViewToggle_;
        Toggle *timelineButtonsToggle_;
        Toggle *typingNotifications_;
        Toggle *messageHoverHighlight_;
        Toggle *enlargeEmojiOnlyMessages_;
        Toggle *sortByImportance_;
        Toggle *readReceipts_;
        Toggle *markdownEnabled_;
        Toggle *desktopNotifications_;
        Toggle *avatarCircles_;
        Toggle *decryptSidebar_;
        QLabel *deviceFingerprintValue_;
        QLabel *deviceIdValue_;

        QComboBox *themeCombo_;
        QComboBox *scaleFactorCombo_;
        QComboBox *fontSizeCombo_;
        QComboBox *fontSelectionCombo_;
        QComboBox *emojiFontSelectionCombo_;

        QSpinBox *timelineMaxWidthSpin_;

        int sideMargin_ = 0;
};
