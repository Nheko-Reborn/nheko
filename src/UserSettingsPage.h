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
class QFontComboBox;
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
        Q_PROPERTY(bool messageHoverHighlight READ messageHoverHighlight WRITE
                     setMessageHoverHighlight NOTIFY messageHoverHighlightChanged)
        Q_PROPERTY(bool enlargeEmojiOnlyMessages READ enlargeEmojiOnlyMessages WRITE
                     setEnlargeEmojiOnlyMessages NOTIFY enlargeEmojiOnlyMessagesChanged)
        Q_PROPERTY(bool tray READ tray WRITE setTray NOTIFY trayChanged)
        Q_PROPERTY(bool startInTray READ startInTray WRITE setStartInTray NOTIFY startInTrayChanged)
        Q_PROPERTY(bool groupView READ groupView WRITE setGroupView NOTIFY groupViewStateChanged)
        Q_PROPERTY(bool markdown READ markdown WRITE setMarkdown NOTIFY markdownChanged)
        Q_PROPERTY(bool typingNotifications READ typingNotifications WRITE setTypingNotifications
                     NOTIFY typingNotificationsChanged)
        Q_PROPERTY(bool sortByImportance READ sortByImportance WRITE setSortByImportance NOTIFY
                     roomSortingChanged)
        Q_PROPERTY(bool buttonsInTimeline READ buttonsInTimeline WRITE setButtonsInTimeline NOTIFY
                     buttonInTimelineChanged)
        Q_PROPERTY(
          bool readReceipts READ readReceipts WRITE setReadReceipts NOTIFY readReceiptsChanged)
        Q_PROPERTY(bool desktopNotifications READ hasDesktopNotifications WRITE
                     setDesktopNotifications NOTIFY desktopNotificationsChanged)
        Q_PROPERTY(bool alertOnNotification READ hasAlertOnNotification WRITE setAlertOnNotification
                     NOTIFY alertOnNotificationChanged)
        Q_PROPERTY(
          bool avatarCircles READ avatarCircles WRITE setAvatarCircles NOTIFY avatarCirclesChanged)
        Q_PROPERTY(bool decryptSidebar READ decryptSidebar WRITE setDecryptSidebar NOTIFY
                     decryptSidebarChanged)
        Q_PROPERTY(int timelineMaxWidth READ timelineMaxWidth WRITE setTimelineMaxWidth NOTIFY
                     timelineMaxWidthChanged)
        Q_PROPERTY(bool mobileMode READ mobileMode WRITE setMobileMode NOTIFY mobileModeChanged)
        Q_PROPERTY(double fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
        Q_PROPERTY(QString font READ font WRITE setFontFamily NOTIFY fontChanged)
        Q_PROPERTY(
          QString emojiFont READ emojiFont WRITE setEmojiFontFamily NOTIFY emojiFontChanged)
        Q_PROPERTY(Presence presence READ presence WRITE setPresence NOTIFY presenceChanged)
        Q_PROPERTY(QString microphone READ microphone WRITE setMicrophone NOTIFY microphoneChanged)
        Q_PROPERTY(QString camera READ camera WRITE setCamera NOTIFY cameraChanged)
        Q_PROPERTY(QString cameraResolution READ cameraResolution WRITE setCameraResolution NOTIFY
                     cameraResolutionChanged)
        Q_PROPERTY(QString cameraFrameRate READ cameraFrameRate WRITE setCameraFrameRate NOTIFY
                     cameraFrameRateChanged)
        Q_PROPERTY(
          bool useStunServer READ useStunServer WRITE setUseStunServer NOTIFY useStunServerChanged)
        Q_PROPERTY(bool shareKeysWithTrustedUsers READ shareKeysWithTrustedUsers WRITE
                     setShareKeysWithTrustedUsers NOTIFY shareKeysWithTrustedUsersChanged)

public:
        UserSettings();

        enum class Presence
        {
                AutomaticPresence,
                Online,
                Unavailable,
                Offline,
        };
        Q_ENUM(Presence);

        void save();
        void load();
        void applyTheme();
        void setTheme(QString theme);
        void setMessageHoverHighlight(bool state);
        void setEnlargeEmojiOnlyMessages(bool state);
        void setTray(bool state);
        void setStartInTray(bool state);
        void setMobileMode(bool mode);
        void setFontSize(double size);
        void setFontFamily(QString family);
        void setEmojiFontFamily(QString family);
        void setGroupView(bool state);
        void setMarkdown(bool state);
        void setReadReceipts(bool state);
        void setTypingNotifications(bool state);
        void setSortByImportance(bool state);
        void setButtonsInTimeline(bool state);
        void setTimelineMaxWidth(int state);
        void setDesktopNotifications(bool state);
        void setAlertOnNotification(bool state);
        void setAvatarCircles(bool state);
        void setDecryptSidebar(bool state);
        void setPresence(Presence state);
        void setMicrophone(QString microphone);
        void setCamera(QString camera);
        void setCameraResolution(QString resolution);
        void setCameraFrameRate(QString frameRate);
        void setUseStunServer(bool state);
        void setShareKeysWithTrustedUsers(bool state);

        QString theme() const { return !theme_.isEmpty() ? theme_ : defaultTheme_; }
        bool messageHoverHighlight() const { return messageHoverHighlight_; }
        bool enlargeEmojiOnlyMessages() const { return enlargeEmojiOnlyMessages_; }
        bool tray() const { return tray_; }
        bool startInTray() const { return startInTray_; }
        bool groupView() const { return groupView_; }
        bool avatarCircles() const { return avatarCircles_; }
        bool decryptSidebar() const { return decryptSidebar_; }
        bool markdown() const { return markdown_; }
        bool typingNotifications() const { return typingNotifications_; }
        bool sortByImportance() const { return sortByImportance_; }
        bool buttonsInTimeline() const { return buttonsInTimeline_; }
        bool mobileMode() const { return mobileMode_; }
        bool readReceipts() const { return readReceipts_; }
        bool hasDesktopNotifications() const { return hasDesktopNotifications_; }
        bool hasAlertOnNotification() const { return hasAlertOnNotification_; }
        bool hasNotifications() const
        {
                return hasDesktopNotifications() || hasAlertOnNotification();
        }
        int timelineMaxWidth() const { return timelineMaxWidth_; }
        double fontSize() const { return baseFontSize_; }
        QString font() const { return font_; }
        QString emojiFont() const { return emojiFont_; }
        Presence presence() const { return presence_; }
        QString microphone() const { return microphone_; }
        QString camera() const { return camera_; }
        QString cameraResolution() const { return cameraResolution_; }
        QString cameraFrameRate() const { return cameraFrameRate_; }
        bool useStunServer() const { return useStunServer_; }
        bool shareKeysWithTrustedUsers() const { return shareKeysWithTrustedUsers_; }

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
        void alertOnNotificationChanged(bool state);
        void avatarCirclesChanged(bool state);
        void decryptSidebarChanged(bool state);
        void timelineMaxWidthChanged(int state);
        void mobileModeChanged(bool mode);
        void fontSizeChanged(double state);
        void fontChanged(QString state);
        void emojiFontChanged(QString state);
        void presenceChanged(Presence state);
        void microphoneChanged(QString microphone);
        void cameraChanged(QString camera);
        void cameraResolutionChanged(QString resolution);
        void cameraFrameRateChanged(QString frameRate);
        void useStunServerChanged(bool state);
        void shareKeysWithTrustedUsersChanged(bool state);

private:
        // Default to system theme if QT_QPA_PLATFORMTHEME var is set.
        QString defaultTheme_ =
          QProcessEnvironment::systemEnvironment().value("QT_QPA_PLATFORMTHEME", "").isEmpty()
            ? "light"
            : "system";
        QString theme_;
        bool messageHoverHighlight_;
        bool enlargeEmojiOnlyMessages_;
        bool tray_;
        bool startInTray_;
        bool groupView_;
        bool markdown_;
        bool typingNotifications_;
        bool sortByImportance_;
        bool buttonsInTimeline_;
        bool readReceipts_;
        bool hasDesktopNotifications_;
        bool hasAlertOnNotification_;
        bool avatarCircles_;
        bool decryptSidebar_;
        bool shareKeysWithTrustedUsers_;
        bool mobileMode_;
        int timelineMaxWidth_;
        double baseFontSize_;
        QString font_;
        QString emojiFont_;
        Presence presence_;
        QString microphone_;
        QString camera_;
        QString cameraResolution_;
        QString cameraFrameRate_;
        bool useStunServer_;
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
        Toggle *markdown_;
        Toggle *desktopNotifications_;
        Toggle *alertOnNotification_;
        Toggle *avatarCircles_;
        Toggle *useStunServer_;
        Toggle *decryptSidebar_;
        Toggle *shareKeysWithTrustedUsers_;
        Toggle *mobileMode_;
        QLabel *deviceFingerprintValue_;
        QLabel *deviceIdValue_;

        QComboBox *themeCombo_;
        QComboBox *scaleFactorCombo_;
        QComboBox *fontSizeCombo_;
        QFontComboBox *fontSelectionCombo_;
        QComboBox *emojiFontSelectionCombo_;
        QComboBox *microphoneCombo_;
        QComboBox *cameraCombo_;
        QComboBox *cameraResolutionCombo_;
        QComboBox *cameraFrameRateCombo_;

        QSpinBox *timelineMaxWidthSpin_;

        int sideMargin_ = 0;
};
