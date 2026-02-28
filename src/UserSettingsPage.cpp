// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>
#include <mtx/secret_storage.hpp>

#include "Cache.h"
#include "JdenticonProvider.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "encryption/Olm.h"
#include "ui/Theme.h"
#include "voip/CallDevices.h"

#include "config/nheko.h"

QStringList themes{
  QStringLiteral("light"),
  QStringLiteral("dark"),
  QStringLiteral("system"),
};

QSharedPointer<UserSettings> UserSettings::instance_;

UserSettings::UserSettings()
{
    if (settings.contains("user/invert_enter_key")) {
        auto oldValue =
          (settings.value("user/invert_enter_key", false).toBool() ? SendMessageKey::ShiftEnter
                                                                   : SendMessageKey::Enter);
        settings.setValue("user/send_message_key", static_cast<int>(oldValue));
        settings.remove("user/invert_enter_key");
    }

    connect(
      QCoreApplication::instance(), &QCoreApplication::aboutToQuit, []() { instance_.clear(); });
}

QSharedPointer<UserSettings>
UserSettings::instance()
{
    return instance_;
}

void
UserSettings::initialize(std::optional<QString> profile)
{
    instance_.reset(new UserSettings());
    instance_->load(profile);
}

void
UserSettings::load(std::optional<QString> profile)
{
    tray_        = settings.value("user/window/tray", false).toBool();
    startInTray_ = settings.value("user/window/start_in_tray", false).toBool();

    roomListWidth_      = settings.value("user/sidebar/room_list_width", -1).toInt();
    communityListWidth_ = settings.value("user/sidebar/community_list_width", -1).toInt();

    hasDesktopNotifications_ = settings.value("user/desktop_notifications", true).toBool();
    hasAlertOnNotification_  = settings.value("user/alert_on_notification", false).toBool();
    groupView_               = settings.value("user/group_view", true).toBool();
    scrollbarsInRoomlist_    = settings.value("user/scrollbars_in_roomlist", false).toBool();
    buttonsInTimeline_       = settings.value("user/timeline/buttons", true).toBool();
    timelineMaxWidth_        = settings.value("user/timeline/max_width", 0).toInt();
    messageHoverHighlight_ =
      settings.value("user/timeline/message_hover_highlight", false).toBool();
    enlargeEmojiOnlyMessages_ =
      settings.value("user/timeline/enlarge_emoji_only_msg", false).toBool();
    markdown_ = settings.value("user/markdown_enabled", true).toBool();

    auto sendMessageKey = settings.value("user/send_message_key", 0).toInt();
    if (sendMessageKey < 0 || sendMessageKey > 2)
        sendMessageKey = static_cast<int>(SendMessageKey::Enter);
    sendMessageKey_ = static_cast<SendMessageKey>(sendMessageKey);

    bubbles_              = settings.value("user/bubbles_enabled", false).toBool();
    smallAvatars_         = settings.value("user/small_avatars_enabled", false).toBool();
    animateImagesOnHover_ = settings.value("user/animate_images_on_hover", false).toBool();
    typingNotifications_  = settings.value("user/typing_notifications", true).toBool();
    sortByImportance_     = settings.value("user/sort_by_unread", true).toBool();
    sortByAlphabet_       = settings.value("user/sort_by_alphabet", false).toBool();
    readReceipts_         = settings.value("user/read_receipts", true).toBool();
    theme_                = settings.value("user/theme", defaultTheme_).toString();

    font_ = settings.value("user/font_family", "").toString();

    avatarCircles_        = settings.value("user/avatar_circles", true).toBool();
    useIdenticon_         = settings.value("user/use_identicon", true).toBool();
    openImageExternal_    = settings.value("user/open_image_external", false).toBool();
    openVideoExternal_    = settings.value("user/open_video_external", false).toBool();
    decryptSidebar_       = settings.value("user/decrypt_sidebar", true).toBool();
    decryptNotifications_ = settings.value("user/decrypt_notifications", true).toBool();
    spaceNotifications_   = settings.value("user/space_notifications", true).toBool();
    fancyEffects_         = settings.value("user/fancy_effects", true).toBool();
    reducedMotion_        = settings.value("user/reduced_motion", false).toBool();
    privacyScreen_        = settings.value("user/privacy_screen", false).toBool();
    privacyScreenTimeout_ = settings.value("user/privacy_screen_timeout", 0).toInt();
    exposeDBusApi_        = settings.value("user/expose_dbus_api", false).toBool();
    updateSpaceVias_      = settings.value("user/space_background_maintenance", true).toBool();
    expireEvents_ = settings.value("user/expired_events_background_maintenance", false).toBool();

    mobileMode_             = settings.value("user/mobile_mode", false).toBool();
    disableSwipe_           = settings.value("user/disable_swipe", false).toBool();
    emojiFont_              = settings.value("user/emoji_font_family", "emoji").toString();
    baseFontSize_           = settings.value("user/font_size", QFont().pointSizeF()).toDouble();
    ringtone_               = settings.value("user/ringtone", "Default").toString();
    microphone_             = settings.value("user/microphone", QString()).toString();
    camera_                 = settings.value("user/camera", QString()).toString();
    cameraResolution_       = settings.value("user/camera_resolution", QString()).toString();
    cameraFrameRate_        = settings.value("user/camera_frame_rate", QString()).toString();
    screenShareFrameRate_   = settings.value("user/screen_share_frame_rate", 5).toInt();
    screenSharePiP_         = settings.value("user/screen_share_pip", true).toBool();
    screenShareRemoteVideo_ = settings.value("user/screen_share_remote_video", false).toBool();
    screenShareHideCursor_  = settings.value("user/screen_share_hide_cursor", false).toBool();
    useStunServer_          = settings.value("user/use_stun_server", false).toBool();

    if (profile) // set to "" if it's the default to maintain compatibility
        profile_ = (*profile == QLatin1String("default")) ? QLatin1String("") : *profile;
    else
        profile_ = settings.value("user/currentProfile", "").toString();

    QString prefix = (profile_ != QLatin1String("") && profile_ != QLatin1String("default"))
                       ? "profile/" + profile_ + "/"
                       : QLatin1String("");
    accessToken_   = settings.value(prefix + "auth/access_token", "").toString();
    homeserver_    = settings.value(prefix + "auth/home_server", "").toString();
    userId_        = settings.value(prefix + "auth/user_id", "").toString();
    deviceId_      = settings.value(prefix + "auth/device_id", "").toString();
    currentTagId_  = settings.value(prefix + "user/current_tag_id", "").toString();
    hiddenTags_    = settings.value(prefix + "user/hidden_tags", QStringList{}).toStringList();
    mutedTags_  = settings.value(prefix + "user/muted_tags", QStringList{"global"}).toStringList();
    hiddenPins_ = settings.value(prefix + "user/hidden_pins", QStringList{}).toStringList();
    hiddenWidgets_ = settings.value(prefix + "user/hidden_widgets", QStringList{}).toStringList();
    recentReactions_ =
      settings.value(prefix + "user/recent_reactions", QStringList{}).toStringList();
    auto tempPresence  = settings.value(prefix + "user/presence", "").toString().toStdString();
    auto presenceValue = QMetaEnum::fromType<Presence>().keyToValue(tempPresence.c_str());
    if (presenceValue < 0)
        presenceValue = 0;
    presence_ = static_cast<Presence>(presenceValue);

    auto tempShowImage  = settings.value(prefix + "user/show_images", "").toString().toStdString();
    auto showImageValue = QMetaEnum::fromType<ShowImage>().keyToValue(tempShowImage.c_str());
    if (showImageValue < 0)
        showImageValue = 0;
    showImage_ = static_cast<ShowImage>(showImageValue);

    collapsedSpaces_.clear();
    auto tempSpaces = settings.value(prefix + "user/collapsed_spaces", QList<QVariant>{}).toList();
    for (const auto &e : std::as_const(tempSpaces))
        collapsedSpaces_.push_back(e.toStringList());

    shareKeysWithTrustedUsers_ =
      settings.value(prefix + "user/automatically_share_keys_with_trusted_users", false).toBool();
    onlyShareKeysWithVerifiedUsers_ =
      settings.value(prefix + "user/only_share_keys_with_verified_users", false).toBool();
    useOnlineKeyBackup_ = settings.value(prefix + "user/online_key_backup", true).toBool();

    disableCertificateValidation_ =
      settings.value("disable_certificate_validation", false).toBool();

    applyTheme();

    if (profile)
        setProfile(profile_);
}

bool
UserSettings::useIdenticon() const
{
    return useIdenticon_ && JdenticonProvider::isAvailable();
}

void
UserSettings::setMessageHoverHighlight(bool state)
{
    if (state == messageHoverHighlight_)
        return;
    messageHoverHighlight_ = state;
    emit messageHoverHighlightChanged(state);
    save();
}
void
UserSettings::setEnlargeEmojiOnlyMessages(bool state)
{
    if (state == enlargeEmojiOnlyMessages_)
        return;
    enlargeEmojiOnlyMessages_ = state;
    emit enlargeEmojiOnlyMessagesChanged(state);
    save();
}
void
UserSettings::setTray(bool state)
{
    if (state == tray_)
        return;
    tray_ = state;
    emit trayChanged(state);
    save();
}

void
UserSettings::setStartInTray(bool state)
{
    if (state == startInTray_)
        return;
    startInTray_ = state;
    emit startInTrayChanged(state);
    save();
}

void
UserSettings::setMobileMode(bool state)
{
    if (state == mobileMode_)
        return;
    mobileMode_ = state;
    emit mobileModeChanged(state);
    save();
}

void
UserSettings::setDisableSwipe(bool state)
{
    if (state == disableSwipe_)
        return;
    disableSwipe_ = state;
    emit disableSwipeChanged(state);
    save();
}

void
UserSettings::setGroupView(bool state)
{
    if (groupView_ == state)
        return;

    groupView_ = state;
    emit groupViewStateChanged(state);
    save();
}

void
UserSettings::setScrollbarsInRoomlist(bool state)
{
    if (scrollbarsInRoomlist_ == state)
        return;

    scrollbarsInRoomlist_ = state;
    emit scrollbarsInRoomlistChanged(state);
    save();
}

void
UserSettings::setHiddenTags(const QStringList &hiddenTags)
{
    hiddenTags_ = hiddenTags;
    save();
}

void
UserSettings::setMutedTags(const QStringList &mutedTags)
{
    mutedTags_ = mutedTags;
    save();
}

void
UserSettings::setHiddenPins(const QStringList &hiddenTags)
{
    hiddenPins_ = hiddenTags;
    save();
    emit hiddenPinsChanged();
}

void
UserSettings::setHiddenWidgets(const QStringList &hiddenTags)
{
    hiddenWidgets_ = hiddenTags;
    save();
    emit hiddenWidgetsChanged();
}

void
UserSettings::setRecentReactions(QStringList recent)
{
    recentReactions_ = recent;
    save();
    emit recentReactionsChanged();
}

void
UserSettings::setCollapsedSpaces(QList<QStringList> spaces)
{
    collapsedSpaces_ = spaces;
    save();
}

void
UserSettings::setExposeDBusApi(bool state)
{
    if (exposeDBusApi_ == state)
        return;

    exposeDBusApi_ = state;
    emit exposeDBusApiChanged(state);
    save();
}

void
UserSettings::setUpdateSpaceVias(bool state)
{
    if (updateSpaceVias_ == state)
        return;

    updateSpaceVias_ = state;
    emit updateSpaceViasChanged(state);
    save();
}

void
UserSettings::setExpireEvents(bool state)
{
    if (expireEvents_ == state)
        return;

    expireEvents_ = state;
    emit expireEventsChanged(state);
    save();
}

void
UserSettings::setMarkdown(bool state)
{
    if (state == markdown_)
        return;
    markdown_ = state;
    emit markdownChanged(state);
    save();
}

void
UserSettings::setSendMessageKey(SendMessageKey key)
{
    if (key == sendMessageKey_)
        return;
    sendMessageKey_ = key;
    emit sendMessageKeyChanged(key);
    save();
}

void
UserSettings::setBubbles(bool state)
{
    if (state == bubbles_)
        return;
    bubbles_ = state;
    emit bubblesChanged(state);
    save();
}

void
UserSettings::setSmallAvatars(bool state)
{
    if (state == smallAvatars_)
        return;
    smallAvatars_ = state;
    emit smallAvatarsChanged(state);
    save();
}

void
UserSettings::setAnimateImagesOnHover(bool state)
{
    if (state == animateImagesOnHover_)
        return;
    animateImagesOnHover_ = state;
    emit animateImagesOnHoverChanged(state);
    save();
}

void
UserSettings::setReadReceipts(bool state)
{
    if (state == readReceipts_)
        return;
    readReceipts_ = state;
    emit readReceiptsChanged(state);
    save();
}

void
UserSettings::setTypingNotifications(bool state)
{
    if (state == typingNotifications_)
        return;
    typingNotifications_ = state;
    emit typingNotificationsChanged(state);
    save();
}

void
UserSettings::setSortByImportance(bool state)
{
    if (state == sortByImportance_)
        return;
    sortByImportance_ = state;
    emit roomSortingChangedImportance(state);
    save();
}

void
UserSettings::setSortByAlphabet(bool state)
{
    if (state == sortByAlphabet_)
        return;
    sortByAlphabet_ = state;
    emit roomSortingChangedAlphabetical(state);
    save();
}

void
UserSettings::setButtonsInTimeline(bool state)
{
    if (state == buttonsInTimeline_)
        return;
    buttonsInTimeline_ = state;
    emit buttonInTimelineChanged(state);
    save();
}

void
UserSettings::setTimelineMaxWidth(int state)
{
    if (state == timelineMaxWidth_)
        return;
    timelineMaxWidth_ = state;
    emit timelineMaxWidthChanged(state);
    save();
}
void
UserSettings::setCommunityListWidth(int state)
{
    if (state == communityListWidth_)
        return;
    communityListWidth_ = state;
    emit communityListWidthChanged(state);
    save();
}
void
UserSettings::setRoomListWidth(int state)
{
    if (state == roomListWidth_)
        return;
    roomListWidth_ = state;
    emit roomListWidthChanged(state);
    save();
}

void
UserSettings::setDesktopNotifications(bool state)
{
    if (state == hasDesktopNotifications_)
        return;
    hasDesktopNotifications_ = state;
    emit desktopNotificationsChanged(state);
    save();
}

void
UserSettings::setAlertOnNotification(bool state)
{
    if (state == hasAlertOnNotification_)
        return;
    hasAlertOnNotification_ = state;
    emit alertOnNotificationChanged(state);
    save();
}

void
UserSettings::setAvatarCircles(bool state)
{
    if (state == avatarCircles_)
        return;
    avatarCircles_ = state;
    emit avatarCirclesChanged(state);
    save();
}

void
UserSettings::setDecryptSidebar(bool state)
{
    if (state == decryptSidebar_)
        return;
    decryptSidebar_ = state;
    emit decryptSidebarChanged(state);
    save();
}

void
UserSettings::setDecryptNotifications(bool state)
{
    if (state == decryptNotifications_)
        return;
    decryptNotifications_ = state;
    emit decryptNotificationsChanged(state);
    save();
}

void
UserSettings::setSpaceNotifications(bool state)
{
    if (state == spaceNotifications_)
        return;
    spaceNotifications_ = state;
    emit spaceNotificationsChanged(state);
    save();
}

void
UserSettings::setFancyEffects(bool state)
{
    if (state == fancyEffects_)
        return;
    fancyEffects_ = state;
    emit fancyEffectsChanged(state);
    save();
}

void
UserSettings::setReducedMotion(bool state)
{
    if (state == reducedMotion_)
        return;
    reducedMotion_ = state;
    emit reducedMotionChanged(state);
    save();

    // Also toggle other motion related settings
    if (reducedMotion_) {
        setFancyEffects(false);
        setAnimateImagesOnHover(true);
    }
}

void
UserSettings::setPrivacyScreen(bool state)
{
    if (state == privacyScreen_) {
        return;
    }
    privacyScreen_ = state;
    emit privacyScreenChanged(state);
    save();
}

void
UserSettings::setPrivacyScreenTimeout(int state)
{
    if (state == privacyScreenTimeout_) {
        return;
    }
    privacyScreenTimeout_ = state;
    emit privacyScreenTimeoutChanged(state);
    save();
}

void
UserSettings::setFontSize(double size)
{
    if (size == baseFontSize_)
        return;
    baseFontSize_ = size;

    const static auto defaultFamily = QFont().defaultFamily();
    QFont f((font_.isEmpty() || font_ == QStringLiteral("default")) ? defaultFamily : font_);
    f.setPointSizeF(fontSize());
    QApplication::setFont(f);

    emit fontSizeChanged(size);
    save();
}

void
UserSettings::setFontFamily(QString family)
{
    if (family == font_)
        return;
    font_ = family;

    const static auto defaultFamily = QFont().defaultFamily();
    QFont f((family.isEmpty() || family == QStringLiteral("default")) ? defaultFamily : family);
    f.setPointSizeF(fontSize());
    QApplication::setFont(f);

    emit fontChanged(family);
    save();
}

void
UserSettings::setEmojiFontFamily(QString family)
{
    if (family == emojiFont_)
        return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    if (emojiFont_.isEmpty()) {
        QFontDatabase::removeApplicationEmojiFontFamily(emojiFont_);
    }
#endif

    if (family.isEmpty() || family == QStringLiteral("emoji")) {
        emojiFont_.clear();
    } else {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        QFontDatabase::addApplicationEmojiFontFamily(family);
#endif
        emojiFont_ = family;
    }

    emit emojiFontChanged(family);
    save();
}

void
UserSettings::setPresence(Presence state)
{
    if (state == presence_)
        return;
    presence_ = state;
    emit presenceChanged(state);
    save();
}

void
UserSettings::setShowImage(ShowImage state)
{
    if (state == showImage_)
        return;
    showImage_ = state;
    emit showImageChanged(state);
    save();
}

void
UserSettings::setTheme(QString theme)
{
    if (theme == theme_ || !themes.contains(theme))
        return;
    theme_ = theme;
    save();
    applyTheme();
    emit themeChanged(theme);
}

void
UserSettings::setUseStunServer(bool useStunServer)
{
    if (useStunServer == useStunServer_)
        return;
    useStunServer_ = useStunServer;
    emit useStunServerChanged(useStunServer);
    save();
}

void
UserSettings::setOnlyShareKeysWithVerifiedUsers(bool shareKeys)
{
    if (shareKeys == onlyShareKeysWithVerifiedUsers_)
        return;

    onlyShareKeysWithVerifiedUsers_ = shareKeys;
    emit onlyShareKeysWithVerifiedUsersChanged(shareKeys);
    save();
}

void
UserSettings::setShareKeysWithTrustedUsers(bool shareKeys)
{
    if (shareKeys == shareKeysWithTrustedUsers_)
        return;

    shareKeysWithTrustedUsers_ = shareKeys;
    emit shareKeysWithTrustedUsersChanged(shareKeys);
    save();
}

void
UserSettings::setUseOnlineKeyBackup(bool useBackup)
{
    if (useBackup == useOnlineKeyBackup_)
        return;

    useOnlineKeyBackup_ = useBackup;
    emit useOnlineKeyBackupChanged(useBackup);
    save();

    if (useBackup)
        olm::download_full_keybackup();
}

void
UserSettings::setRingtone(QString ringtone)
{
    if (ringtone == ringtone_)
        return;
    ringtone_ = ringtone;
    emit ringtoneChanged(ringtone);
    save();
}

void
UserSettings::setMicrophone(QString microphone)
{
    if (microphone == microphone_)
        return;
    microphone_ = microphone;
    emit microphoneChanged(microphone);
    save();
}

void
UserSettings::setCamera(QString camera)
{
    if (camera == camera_)
        return;
    camera_ = camera;
    emit cameraChanged(camera);
    save();
}

void
UserSettings::setCameraResolution(QString resolution)
{
    if (resolution == cameraResolution_)
        return;
    cameraResolution_ = resolution;
    emit cameraResolutionChanged(resolution);
    save();
}

void
UserSettings::setCameraFrameRate(QString frameRate)
{
    if (frameRate == cameraFrameRate_)
        return;
    cameraFrameRate_ = frameRate;
    emit cameraFrameRateChanged(frameRate);
    save();
}

void
UserSettings::setScreenShareFrameRate(int frameRate)
{
    if (frameRate == screenShareFrameRate_)
        return;
    screenShareFrameRate_ = frameRate;
    emit screenShareFrameRateChanged(frameRate);
    save();
}

void
UserSettings::setScreenSharePiP(bool state)
{
    if (state == screenSharePiP_)
        return;
    screenSharePiP_ = state;
    emit screenSharePiPChanged(state);
    save();
}

void
UserSettings::setScreenShareRemoteVideo(bool state)
{
    if (state == screenShareRemoteVideo_)
        return;
    screenShareRemoteVideo_ = state;
    emit screenShareRemoteVideoChanged(state);
    save();
}

void
UserSettings::setScreenShareHideCursor(bool state)
{
    if (state == screenShareHideCursor_)
        return;
    screenShareHideCursor_ = state;
    emit screenShareHideCursorChanged(state);
    save();
}

void
UserSettings::setProfile(QString profile)
{
    // always set this to allow setting this when loading and it is overwritten on the cli
    profile_ = profile;
    emit profileChanged(profile_);
    save();
}

void
UserSettings::setUserId(QString userId)
{
    if (userId == userId_)
        return;
    userId_ = userId;
    emit userIdChanged(userId_);
    save();
}

void
UserSettings::setAccessToken(QString accessToken)
{
    if (accessToken == accessToken_)
        return;
    accessToken_ = accessToken;
    emit accessTokenChanged(accessToken_);
    save();
}

void
UserSettings::setDeviceId(QString deviceId)
{
    if (deviceId == deviceId_)
        return;
    deviceId_ = deviceId;
    emit deviceIdChanged(deviceId_);
    save();
}

void
UserSettings::setCurrentTagId(const QString currentTagId)
{
    if (currentTagId == currentTagId_)
        return;
    currentTagId_ = currentTagId;
    save();
}

void
UserSettings::setHomeserver(QString homeserver)
{
    if (homeserver == homeserver_)
        return;
    homeserver_ = homeserver;
    emit homeserverChanged(homeserver_);
    save();
}

void
UserSettings::setDisableCertificateValidation(bool disabled)
{
    if (disabled == disableCertificateValidation_)
        return;
    disableCertificateValidation_ = disabled;
    http::client()->verify_certificates(!disabled);
    emit disableCertificateValidationChanged(disabled);
}

void
UserSettings::setUseIdenticon(bool state)
{
    if (state == useIdenticon_)
        return;
    useIdenticon_ = state;
    emit useIdenticonChanged(useIdenticon_);
    save();
}

void
UserSettings::setOpenImageExternal(bool state)
{
    if (state == openImageExternal_)
        return;
    openImageExternal_ = state;
    emit openImageExternalChanged(openImageExternal_);
    save();
}

void
UserSettings::setOpenVideoExternal(bool state)
{
    if (state == openVideoExternal_)
        return;
    openVideoExternal_ = state;
    emit openVideoExternalChanged(openVideoExternal_);
    save();
}

void
UserSettings::applyTheme()
{
    QGuiApplication::setPalette(Theme::paletteFromTheme(this->theme()));
    QApplication::setPalette(Theme::paletteFromTheme(this->theme()));
}

void
UserSettings::save()
{
    settings.beginGroup("user");

    settings.beginGroup("window");
    settings.setValue("tray", tray_);
    settings.setValue("start_in_tray", startInTray_);
    settings.endGroup(); // window

    settings.beginGroup("sidebar");
    settings.setValue("community_list_width", communityListWidth_);
    settings.setValue("room_list_width", roomListWidth_);
    settings.endGroup(); // window

    settings.beginGroup("timeline");
    settings.setValue("buttons", buttonsInTimeline_);
    settings.setValue("message_hover_highlight", messageHoverHighlight_);
    settings.setValue("enlarge_emoji_only_msg", enlargeEmojiOnlyMessages_);
    settings.setValue("max_width", timelineMaxWidth_);
    settings.endGroup(); // timeline

    settings.setValue("avatar_circles", avatarCircles_);
    settings.setValue("decrypt_sidebar", decryptSidebar_);
    settings.setValue("decrypt_notifications", decryptNotifications_);
    settings.setValue("space_notifications", spaceNotifications_);
    settings.setValue("fancy_effects", fancyEffects_);
    settings.setValue("reduced_motion", reducedMotion_);
    settings.setValue("privacy_screen", privacyScreen_);
    settings.setValue("privacy_screen_timeout", privacyScreenTimeout_);
    settings.setValue("mobile_mode", mobileMode_);
    settings.setValue("disable_swipe", disableSwipe_);
    settings.setValue("font_size", baseFontSize_);
    settings.setValue("typing_notifications", typingNotifications_);
    settings.setValue("sort_by_unread", sortByImportance_);
    settings.setValue("sort_by_alphabet", sortByAlphabet_);
    settings.setValue("minor_events", sortByImportance_);
    settings.setValue("read_receipts", readReceipts_);
    settings.setValue("group_view", groupView_);
    settings.setValue("scrollbars_in_roomlist", scrollbarsInRoomlist_);
    settings.setValue("markdown_enabled", markdown_);
    settings.setValue("send_message_key", static_cast<int>(sendMessageKey_));
    settings.setValue("bubbles_enabled", bubbles_);
    settings.setValue("small_avatars_enabled", smallAvatars_);
    settings.setValue("animate_images_on_hover", animateImagesOnHover_);
    settings.setValue("desktop_notifications", hasDesktopNotifications_);
    settings.setValue("alert_on_notification", hasAlertOnNotification_);
    settings.setValue("theme", theme());
    settings.setValue("font_family", font_);
    settings.setValue("emoji_font_family", emojiFont_);
    settings.setValue("ringtone", ringtone_);
    settings.setValue("microphone", microphone_);
    settings.setValue("camera", camera_);
    settings.setValue("camera_resolution", cameraResolution_);
    settings.setValue("camera_frame_rate", cameraFrameRate_);
    settings.setValue("screen_share_frame_rate", screenShareFrameRate_);
    settings.setValue("screen_share_pip", screenSharePiP_);
    settings.setValue("screen_share_remote_video", screenShareRemoteVideo_);
    settings.setValue("screen_share_hide_cursor", screenShareHideCursor_);
    settings.setValue("use_stun_server", useStunServer_);
    settings.setValue("currentProfile", profile_);
    settings.setValue("use_identicon", useIdenticon_);
    settings.setValue("open_image_external", openImageExternal_);
    settings.setValue("open_video_external", openVideoExternal_);
    settings.setValue("expose_dbus_api", exposeDBusApi_);
    settings.setValue("space_background_maintenance", updateSpaceVias_);
    settings.setValue("expired_events_background_maintenance", expireEvents_);

    settings.endGroup(); // user

    QString prefix = (profile_ != QLatin1String("") && profile_ != QLatin1String("default"))
                       ? "profile/" + profile_ + "/"
                       : QLatin1String("");
    settings.setValue(prefix + "auth/access_token", accessToken_);
    settings.setValue(prefix + "auth/home_server", homeserver_);
    settings.setValue(prefix + "auth/user_id", userId_);
    settings.setValue(prefix + "auth/device_id", deviceId_);
    settings.setValue(prefix + "user/current_tag_id", currentTagId_);
    settings.setValue(prefix + "user/automatically_share_keys_with_trusted_users",
                      shareKeysWithTrustedUsers_);
    settings.setValue(prefix + "user/only_share_keys_with_verified_users",
                      onlyShareKeysWithVerifiedUsers_);
    settings.setValue(prefix + "user/online_key_backup", useOnlineKeyBackup_);
    settings.setValue(prefix + "user/hidden_tags", hiddenTags_);
    settings.setValue(prefix + "user/muted_tags", mutedTags_);
    settings.setValue(prefix + "user/hidden_pins", hiddenPins_);
    settings.setValue(prefix + "user/hidden_widgets", hiddenWidgets_);
    settings.setValue(prefix + "user/recent_reactions", recentReactions_);
    settings.setValue(
      prefix + "user/presence",
      QString::fromUtf8(QMetaEnum::fromType<Presence>().valueToKey(static_cast<int>(presence_))));
    settings.setValue(
      prefix + "user/show_images",
      QString::fromUtf8(QMetaEnum::fromType<ShowImage>().valueToKey(static_cast<int>(showImage_))));

    QVariantList v;
    v.reserve(collapsedSpaces_.size());
    for (const auto &e : std::as_const(collapsedSpaces_))
        v.push_back(e);
    settings.setValue(prefix + "user/collapsed_spaces", v);

    settings.setValue("disable_certificate_validation", disableCertificateValidation_);

    settings.sync();
}

QHash<int, QByteArray>
UserSettingsModel::roleNames() const
{
    static QHash<int, QByteArray> roles{
      {Name, "name"},
      {Description, "description"},
      {Value, "value"},
      {Type, "type"},
      {ValueLowerBound, "valueLowerBound"},
      {ValueUpperBound, "valueUpperBound"},
      {ValueStep, "valueStep"},
      {Values, "values"},
      {Good, "good"},
      {Enabled, "enabled"},
    };

    return roles;
}

QVariant
UserSettingsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= COUNT)
        return {};

    auto i = UserSettings::instance();
    if (!i)
        return {};

    if (role == Name) {
        switch (index.row()) {
        case Theme:
            return tr("Theme");
        case ScaleFactor:
            return tr("Scale factor");
        case MessageHoverHighlight:
            return tr("Highlight message on hover");
        case EnlargeEmojiOnlyMessages:
            return tr("Large Emoji in timeline");
        case Tray:
            return tr("Minimize to tray");
        case StartInTray:
            return tr("Start in tray");
        case GroupView:
            return tr("Communities sidebar");
        case ScrollbarsInRoomlist:
            return tr("Scrollbars in room list");
        case Markdown:
            return tr("Send messages as Markdown");
        case SendMessageKey:
            return tr("Send messages with a shortcut");
        case Bubbles:
            return tr("Enable message bubbles");
        case SmallAvatars:
            return tr("Enable small Avatars");
        case AnimateImagesOnHover:
            return tr("Play animated images only on hover");
        case ShowImage:
            return tr("Show images automatically");
        case TypingNotifications:
            return tr("Typing notifications");
        case SortByImportance:
            return tr("Sort rooms by unreads");
        case SortByAlphabet:
            return tr("Sort rooms alphabetically");
        case ButtonsInTimeline:
            return tr("Show buttons in timeline");
        case TimelineMaxWidth:
            return tr("Limit width of timeline");
        case ReadReceipts:
            return tr("Read receipts");
        case HiddenTimelineEvents:
            return tr("Hidden events");
        case IgnoredUsers:
            return tr("Ignored users");
        case DesktopNotifications:
            return tr("Desktop notifications");
        case AlertOnNotification:
            return tr("Alert on notification");
        case AvatarCircles:
            return tr("Circular Avatars");
        case UseIdenticon:
            return tr("Use identicons");
        case OpenImageExternal:
            return tr("Open images with external program");
        case OpenVideoExternal:
            return tr("Open videos with external program");
        case DecryptSidebar:
            return tr("Decrypt messages in sidebar");
        case DecryptNotifications:
            return tr("Decrypt notifications");
        case SpaceNotifications:
            return tr("Show message counts for communities and tags");
        case FancyEffects:
            return tr("Display fancy effects such as confetti");
        case ReducedMotion:
            return tr("Reduce or disable animations");
        case PrivacyScreen:
            return tr("Privacy Screen");
        case PrivacyScreenTimeout:
            return tr("Privacy screen timeout (in seconds [0 - 3600])");
        case MobileMode:
            return tr("Touchscreen mode");
        case DisableSwipe:
            return tr("Disable swipe motions");
        case FontSize:
            return tr("Font size");
        case Font:
            return tr("Font Family");
        case EmojiFont:
            return tr("Emoji Font Family");
        case Ringtone:
            return tr("Ringtone");
        case Microphone:
            return tr("Microphone");
        case Camera:
            return tr("Camera");
        case CameraResolution:
            return tr("Camera resolution");
        case CameraFrameRate:
            return tr("Camera frame rate");
        case UseStunServer:
            return tr("Allow fallback call assist server");
        case OnlyShareKeysWithVerifiedUsers:
            return tr("Send encrypted messages to verified users only");
        case ShareKeysWithTrustedUsers:
            return tr("Share keys with verified users and devices");
        case UseOnlineKeyBackup:
            return tr("Online Key Backup");
        case Profile:
            return tr("Profile");
        case UserId:
            return tr("User ID");
        case AccessToken:
            return tr("Accesstoken");
        case DeviceId:
            return tr("Device ID");
        case DeviceFingerprint:
            return tr("Device Fingerprint");
        case Homeserver:
            return tr("Homeserver");
        case Version:
            return tr("Version");
        case Platform:
            return tr("Platform");
        case GeneralSection:
            return tr("GENERAL");
        case AccessibilitySection:
            return tr("ACCESSIBILITY");
        case TimelineSection:
            return tr("TIMELINE");
        case SidebarSection:
            return tr("SIDEBAR");
        case TraySection:
            return tr("TRAY");
        case MessageVisibilitySection:
            return tr("GLOBAL MESSAGE VISIBILITY");
        case NotificationsSection:
            return tr("NOTIFICATIONS");
        case VoipSection:
            return tr("CALLS");
        case RescanAudioVideoDevices:
            return tr("Rescan Audio/Video Devices");
        case EncryptionSection:
            return tr("ENCRYPTION");
        case LoginInfoSection:
            return tr("INFO");
        case SessionKeys:
            return tr("Session Keys");
        case CrossSigningSecrets:
            return tr("Cross Signing Secrets");
        case OnlineBackupKey:
            return tr("Online backup key");
        case SelfSigningKey:
            return tr("Self signing key");
        case UserSigningKey:
            return tr("User signing key");
        case MasterKey:
            return tr("Master signing key");
        case ExposeDBusApi:
            return tr("Expose room information via D-Bus");
        case UpdateSpaceVias:
            return tr("Periodically update community routing information");
        case ExpireEvents:
            return tr("Periodically delete expired events");
        }
    } else if (role == Value) {
        switch (index.row()) {
        case Theme:
            return themes.indexOf(i->theme());
        case ScaleFactor:
            return utils::scaleFactor();
        case MessageHoverHighlight:
            return i->messageHoverHighlight();
        case EnlargeEmojiOnlyMessages:
            return i->enlargeEmojiOnlyMessages();
        case Tray:
            return i->tray();
        case StartInTray:
            return i->startInTray();
        case GroupView:
            return i->groupView();
        case ScrollbarsInRoomlist:
            return i->scrollbarsInRoomlist();
        case Markdown:
            return i->markdown();
        case SendMessageKey:
            return static_cast<int>(i->sendMessageKey());
        case Bubbles:
            return i->bubbles();
        case SmallAvatars:
            return i->smallAvatars();
        case AnimateImagesOnHover:
            return i->animateImagesOnHover();
        case ShowImage:
            return static_cast<int>(i->showImage());
        case TypingNotifications:
            return i->typingNotifications();
        case SortByImportance:
            return i->sortByImportance();
        case SortByAlphabet:
            return i->sortByAlphabet();
        case ButtonsInTimeline:
            return i->buttonsInTimeline();
        case TimelineMaxWidth:
            return i->timelineMaxWidth();
        case ReadReceipts:
            return i->readReceipts();
        case DesktopNotifications:
            return i->hasDesktopNotifications();
        case AlertOnNotification:
            return i->hasAlertOnNotification();
        case AvatarCircles:
            return i->avatarCircles();
        case UseIdenticon:
            return i->useIdenticon();
        case OpenImageExternal:
            return i->openImageExternal();
        case OpenVideoExternal:
            return i->openVideoExternal();
        case DecryptSidebar:
            return i->decryptSidebar();
        case DecryptNotifications:
            return i->decryptNotifications();
        case SpaceNotifications:
            return i->spaceNotifications();
        case FancyEffects:
            return i->fancyEffects();
        case ReducedMotion:
            return i->reducedMotion();
        case PrivacyScreen:
            return i->privacyScreen();
        case PrivacyScreenTimeout:
            return i->privacyScreenTimeout();
        case MobileMode:
            return i->mobileMode();
        case DisableSwipe:
            return i->disableSwipe();
        case FontSize:
            return i->fontSize();
        case Font: {
            if (i->font().isEmpty())
                return 0;
            else
                return data(index, Values).toStringList().indexOf(i->font());
        }
        case EmojiFont: {
            if (i->emojiFont().isEmpty() || i->emojiFont() == QLatin1String("emoji"))
                return 0;
            else
                return data(index, Values).toStringList().indexOf(i->emojiFont());
        }
        case Ringtone: {
            auto v = i->ringtone();
            if (v == QStringView(u"Mute"))
                return 0;
            else if (v == QStringView(u"Default"))
                return 1;
            else if (v == QStringView(u"Other"))
                return 2;
            else
                return 3;
        }
        case Microphone:
            return data(index, Values).toStringList().indexOf(i->microphone());
        case Camera:
            return data(index, Values).toStringList().indexOf(i->camera());
        case CameraResolution:
            return data(index, Values).toStringList().indexOf(i->cameraResolution());
        case CameraFrameRate:
            return data(index, Values).toStringList().indexOf(i->cameraFrameRate());
        case UseStunServer:
            return i->useStunServer();
        case OnlyShareKeysWithVerifiedUsers:
            return i->onlyShareKeysWithVerifiedUsers();
        case ShareKeysWithTrustedUsers:
            return i->shareKeysWithTrustedUsers();
        case UseOnlineKeyBackup:
            return i->useOnlineKeyBackup();
        case Profile:
            return i->profile().isEmpty() ? tr("Default") : i->profile();
        case UserId:
            return i->userId();
        case AccessToken:
            return i->accessToken();
        case DeviceId:
            return i->deviceId();
        case DeviceFingerprint:
            return utils::humanReadableFingerprint(olm::client()->identity_keys().ed25519);
        case Homeserver:
            return i->homeserver();
        case Version:
            return QString::fromStdString(nheko::version);
        case Platform:
            return QString::fromStdString(nheko::build_os);
        case OnlineBackupKey:
            return cache::secret(mtx::secret_storage::secrets::megolm_backup_v1).has_value();
        case SelfSigningKey:
            return cache::secret(mtx::secret_storage::secrets::cross_signing_self_signing)
              .has_value();
        case UserSigningKey:
            return cache::secret(mtx::secret_storage::secrets::cross_signing_user_signing)
              .has_value();
        case MasterKey:
            return cache::secret(mtx::secret_storage::secrets::cross_signing_master).has_value();
        case ExposeDBusApi:
            return i->exposeDBusApi();
        case UpdateSpaceVias:
            return i->updateSpaceVias();
        case ExpireEvents:
            return i->expireEvents();
        }
    } else if (role == Description) {
        switch (index.row()) {
        case Theme:
        case Font:
        case EmojiFont:
            return {};
        case Microphone:
            return tr("Set the notification sound to play when a call invite arrives");
        case Camera:
        case CameraResolution:
        case CameraFrameRate:
        case Ringtone:
            return {};
        case RescanAudioVideoDevices:
            return tr("Rescan for new devices (webcams/microphones). "
                      "Nheko does not scan continuously to save resources.");
        case TimelineMaxWidth:
            return tr("Set the max width of messages in the timeline (in pixels). This can help "
                      "readability on wide screen when Nheko is maximized");
        case PrivacyScreenTimeout:
            return tr(
              "Set timeout (in seconds) for how long after window loses\nfocus before the screen"
              " will be blurred.\nSet to 0 to blur immediately after focus loss. Max value of 1 "
              "hour (3600 seconds)");
        case FontSize:
            return {};
        case MessageHoverHighlight:
            return tr("Change the background color of messages when you hover over them.");
        case EnlargeEmojiOnlyMessages:
            return tr("Make font size larger if messages with only a few emojis are displayed.");
        case Tray:
            return tr(
              "Keep the application running in the background after closing the client window.");
        case StartInTray:
            return tr("Start the application in the background without showing the client window.");
        case GroupView:
            return tr("Show a column containing communities and tags next to the room list.");
        case ScrollbarsInRoomlist:
            return tr("Shows scrollbars in the room list and communities list.");
        case Markdown:
            return tr(
              "Allow using markdown in messages.\nWhen disabled, all messages are sent as a plain "
              "text.");
        case SendMessageKey:
            return tr(
              "Select what Enter key combination sends the message. Shift+Enter adds a new line, "
              "unless it has been selected, in which case Enter adds a new line instead.\n\n"
              "If an emoji picker or a mention picker is open, it is always handled first.");
        case Bubbles:
            return tr(
              "Messages get a bubble background. This also triggers some layout changes (WIP).");
        case SmallAvatars:
            return tr("Avatars are resized to fit above the message.");
        case AnimateImagesOnHover:
            return tr("Plays media like GIFs or WEBPs only when explicitly hovering over them.");
        case ShowImage:
            return tr("If images should be automatically displayed. You can select between always "
                      "showing images by default, only show them by default in private rooms or "
                      "always require interaction to show images.");
        case TypingNotifications:
            return tr(
              "Show who is typing in a room.\nThis will also enable or disable sending typing "
              "notifications to others.");
        case SortByImportance:
            return tr(
              "Display rooms with new messages first.\nIf this is off, the list of rooms will only "
              "be sorted by the preferred sorting order.\nIf this is on, rooms "
              "which "
              "have active notifications (the small circle with a number in it) will be sorted on "
              "top. Rooms that you have muted will still be sorted by the preferred sorting order, "
              "since you don't "
              "seem to consider them as important as the other rooms.");
        case SortByAlphabet:
            return tr(
              "Sort rooms alphabetically.\nIf this is off, the list of rooms will be sorted by the "
              "timestamp of the last message in a room.\nIf this is on, rooms that come first "
              "alphabetically "
              "will be sorted earlier than ones that come later.");
        case ButtonsInTimeline:
            return tr(
              "Show buttons to quickly reply, react or access additional options next to each "
              "message.");
        case ReadReceipts:
            return tr(
              "Show if your message was read.\nStatus is displayed next to timestamps.\nWarning: "
              "If your homeserver does not support this, your rooms will never be marked as read!");
        case HiddenTimelineEvents:
            return tr("Configure whether to show or hide certain events like room joins.");
        case DesktopNotifications:
            return tr("Notify about received messages when the client is not currently focused.");
        case AlertOnNotification:
            return tr(
              "Show an alert when a message is received.\nThis usually causes the application "
              "icon in the task bar to animate in some fashion.");
        case AvatarCircles:
            return tr(
              "Change the appearance of user avatars in chats.\nOFF - square, ON - circle.");
        case UseIdenticon:
            return tr("Display an identicon instead of a letter when no avatar is set.");
        case OpenImageExternal:
            return tr("Opens images with an external program when tapping the image.\nNote that "
                      "when this option is ON, opened files are left unencrypted on disk and must "
                      "be manually deleted.");
        case OpenVideoExternal:
            return tr("Opens videos with an external program when tapping the video.\nNote that "
                      "when this option is ON, opened files are left unencrypted on disk and must "
                      "be manually deleted.");
        case DecryptSidebar:
            return tr("Decrypt the messages shown in the sidebar.\nOnly affects messages in "
                      "encrypted chats.");
        case DecryptNotifications:
            return tr("Decrypt messages shown in notifications for encrypted chats.");
        case SpaceNotifications:
            return tr("Choose where to show the total number of notifications contained within a "
                      "community or tag.");
        case FancyEffects:
            return tr("Some messages can be sent with fancy effects. For example, messages sent "
                      "with '/confetti' will show confetti on screen.");
        case ReducedMotion:
            return tr("Nheko uses animations in several places to make stuff pretty. This allows "
                      "you to turn those off if they make you feel unwell.");
        case PrivacyScreen:
            return tr("When the window loses focus, the timeline will\nbe blurred.");
        case MobileMode:
            return tr(
              "Will prevent text selection in the timeline to make touch scrolling easier.");
        case DisableSwipe:
            return tr("Will prevent swipe motions like swiping left/right between Rooms and "
                      "Timeline, or swiping a message to reply.");
        case ScaleFactor:
            return tr("Change the scale factor of the whole user interface. Requires a restart to "
                      "take effect.");
        case UseStunServer:
            return tr(
              "Will use turn.matrix.org as assist when your home server does not offer one.");
        case OnlyShareKeysWithVerifiedUsers:
            return tr("Requires a user to be verified to send encrypted messages to them. This "
                      "improves safety but makes E2EE more tedious.");
        case ShareKeysWithTrustedUsers:
            return tr(
              "Automatically replies to key requests from other users if they are verified, "
              "even if that device shouldn't have access to those keys otherwise.");
        case UseOnlineKeyBackup:
            return tr(
              "Download message encryption keys from and upload to the encrypted online key "
              "backup.");
        case Profile:
        case UserId:
        case AccessToken:
        case DeviceId:
        case DeviceFingerprint:
        case Homeserver:
        case Version:
        case Platform:
        case GeneralSection:
        case AccessibilitySection:
        case TimelineSection:
        case SidebarSection:
        case TraySection:
        case MessageVisibilitySection:
        case NotificationsSection:
        case VoipSection:
        case EncryptionSection:
        case LoginInfoSection:
        case SessionKeys:
        case CrossSigningSecrets:
            return {};
        case OnlineBackupKey:
            return tr(
              "The key to decrypt online key backups. If it is cached, you can enable online "
              "key backup to store encryption keys securely encrypted on the server.");
        case SelfSigningKey:
            return tr(
              "The key to verify your own devices. If it is cached, verifying one of your devices "
              "will mark it verified for all your other devices and for users that have verified "
              "you.");
        case UserSigningKey:
            return tr(
              "The key to verify other users. If it is cached, verifying a user will verify "
              "all their devices.");
        case MasterKey:
            return tr(
              "Your most important key. You don't need to have it cached, since not caching "
              "it makes it less likely it can be stolen and it is only needed to rotate your "
              "other signing keys.");
        case ExposeDBusApi:
            return tr("Allow third-party plugins and applications to load information about rooms "
                      "you are in via D-Bus. "
                      "This can have useful applications, but it also could be used for nefarious "
                      "purposes. Enable at your own risk.\n\n"
                      "This setting will take effect upon restart.");
        case UpdateSpaceVias:
            return tr(
              "To allow new users to join a community, the community needs to expose some "
              "information about what servers participate in a room to community members. Since "
              "the room participants can change over time, this needs to be updated from time to "
              "time. This setting enables a background job to do that automatically.");
        case ExpireEvents:
            return tr("Regularly redact expired events as specified in the event expiration "
                      "configuration. Since this is currently not executed server side, you need "
                      "to have one client running this regularly.");
        case IgnoredUsers:
            return tr("Manage your ignored users.");
        }
    } else if (role == Type) {
        switch (index.row()) {
        case Theme:
        case Font:
        case EmojiFont:
        case Microphone:
        case Camera:
        case CameraResolution:
        case CameraFrameRate:
            return DeviceOptions;
        case Ringtone:
        case ShowImage:
        case SendMessageKey:
            return Options;
        case TimelineMaxWidth:
        case PrivacyScreenTimeout:
            return Integer;
        case FontSize:
        case ScaleFactor:
            return Double;
        case MessageHoverHighlight:
        case EnlargeEmojiOnlyMessages:
        case Tray:
        case StartInTray:
        case GroupView:
        case ScrollbarsInRoomlist:
        case Markdown:
        case Bubbles:
        case SmallAvatars:
        case AnimateImagesOnHover:
        case TypingNotifications:
        case SortByImportance:
        case SortByAlphabet:
        case ButtonsInTimeline:
        case ReadReceipts:
        case DesktopNotifications:
        case AlertOnNotification:
        case AvatarCircles:
        case UseIdenticon:
        case OpenImageExternal:
        case OpenVideoExternal:
        case DecryptSidebar:
        case DecryptNotifications:
        case PrivacyScreen:
        case MobileMode:
        case DisableSwipe:
        case UseStunServer:
        case OnlyShareKeysWithVerifiedUsers:
        case ShareKeysWithTrustedUsers:
        case UseOnlineKeyBackup:
        case ExposeDBusApi:
        case UpdateSpaceVias:
        case ExpireEvents:
        case SpaceNotifications:
        case FancyEffects:
        case ReducedMotion:
            return Toggle;
        case Profile:
        case UserId:
        case AccessToken:
        case DeviceId:
        case DeviceFingerprint:
        case Homeserver:
        case Version:
        case Platform:
            return ReadOnlyText;
        case GeneralSection:
        case AccessibilitySection:
        case TimelineSection:
        case SidebarSection:
        case TraySection:
        case MessageVisibilitySection:
        case NotificationsSection:
        case VoipSection:
        case EncryptionSection:
        case LoginInfoSection:
            return SectionTitle;
        case SessionKeys:
            return SessionKeyImportExport;
        case CrossSigningSecrets:
            return XSignKeysRequestDownload;
        case OnlineBackupKey:
        case SelfSigningKey:
        case UserSigningKey:
        case MasterKey:
            return KeyStatus;
        case HiddenTimelineEvents:
            return ConfigureHiddenEvents;
        case RescanAudioVideoDevices:
            return RescanDevs;
        case IgnoredUsers:
            return ManageIgnoredUsers;
        }
    } else if (role == ValueLowerBound) {
        switch (index.row()) {
        case TimelineMaxWidth:
            return 0;
        case PrivacyScreenTimeout:
            return 0;
        case FontSize:
            return 8.0;
        case ScaleFactor:
            return 1.0;
        }
    } else if (role == ValueUpperBound) {
        switch (index.row()) {
        case TimelineMaxWidth:
            return 20000;
        case PrivacyScreenTimeout:
            return 3600;
        case FontSize:
            return 24.0;
        case ScaleFactor:
            return 3.0;
        }
    } else if (role == ValueStep) {
        switch (index.row()) {
        case TimelineMaxWidth:
            return 20;
        case PrivacyScreenTimeout:
            return 10;
        case FontSize:
            return 0.5;
        case ScaleFactor:
            return .25;
        }
    } else if (role == Values) {
        auto vecToList = [](const std::vector<std::string> &vec) {
            QStringList l;
            for (const auto &d : vec)
                l.push_back(QString::fromStdString(d));

            return l;
        };
        switch (index.row()) {
        case Theme:
            return QStringList{
              QStringLiteral("Light"),
              QStringLiteral("Dark"),
              QStringLiteral("System"),
            };
        case ShowImage:
            return QStringList{
              tr("Always"),
              tr("Only in private rooms"),
              tr("Never"),
            };
        case SendMessageKey:
            return QStringList{
              tr("Enter"),
              tr("Shift+Enter"),
              tr("Ctrl+Enter"),
            };
        case Microphone:
            return vecToList(CallDevices::instance().names(false, i->microphone().toStdString()));
        case Camera:
            return vecToList(CallDevices::instance().names(true, i->camera().toStdString()));
        case CameraResolution:
            return vecToList(CallDevices::instance().resolutions(i->camera().toStdString()));
        case CameraFrameRate:
            return vecToList(CallDevices::instance().frameRates(
              i->camera().toStdString(), i->cameraResolution().toStdString()));

        case Font: {
            auto fonts = QFontDatabase::families();
            fonts.prepend(tr("System font"));
            return fonts;
        }
        case EmojiFont: {
            auto fonts = QFontDatabase::families(QFontDatabase::WritingSystem::Symbol);
            fonts.prepend(tr("System emoji font"));
            return fonts;
        }
        case Ringtone: {
            QStringList l{
              QStringLiteral("Mute"),
              QStringLiteral("Default"),
              QStringLiteral("Other"),
            };
            if (!l.contains(i->ringtone()))
                l.push_back(i->ringtone());
            return l;
        }
        }
    } else if (role == Good) {
        switch (index.row()) {
        case OnlineBackupKey:
            return cache::secret(mtx::secret_storage::secrets::megolm_backup_v1).has_value();
        case SelfSigningKey:
            return cache::secret(mtx::secret_storage::secrets::cross_signing_self_signing)
              .has_value();
        case UserSigningKey:
            return cache::secret(mtx::secret_storage::secrets::cross_signing_user_signing)
              .has_value();
        case MasterKey:
            return true;
        }
    } else if (role == Enabled) {
        switch (index.row()) {
        case StartInTray:
            return i->tray();
        case PrivacyScreenTimeout:
            return i->privacyScreen();
        case UseIdenticon:
            return JdenticonProvider::isAvailable();
        case Microphone:
        case Camera:
        case CameraResolution:
        case CameraFrameRate:
            return !CallDevices::instance().scanning();
        default:
            return true;
        }
    }

    return {};
}

bool
UserSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    auto i = UserSettings::instance();
    if (role == Value) {
        switch (index.row()) {
        case Theme: {
            auto idx = value.toInt();

            if (idx >= 0 && idx < themes.size()) {
                i->setTheme(themes[idx]);
                return true;
            } else
                return false;
        }
        case ShowImage: {
            auto showImageValue = value.toInt();
            if (showImageValue < 0 ||

                QMetaEnum::fromType<UserSettings::ShowImage>().keyCount() <= showImageValue)
                return false;

            i->setShowImage(static_cast<UserSettings::ShowImage>(showImageValue));
            return true;
        }
        case MessageHoverHighlight: {
            if (value.userType() == QMetaType::Bool) {
                i->setMessageHoverHighlight(value.toBool());
                return true;
            } else
                return false;
        }
        case ScaleFactor: {
            if (value.canConvert(QMetaType::fromType<double>())) {
                utils::setScaleFactor(static_cast<float>(value.toDouble()));
                return true;
            } else
                return false;
        }
        case EnlargeEmojiOnlyMessages: {
            if (value.userType() == QMetaType::Bool) {
                i->setEnlargeEmojiOnlyMessages(value.toBool());
                return true;
            } else
                return false;
        }
        case Tray: {
            if (value.userType() == QMetaType::Bool) {
                i->setTray(value.toBool());
                return true;
            } else
                return false;
        }
        case StartInTray: {
            if (value.userType() == QMetaType::Bool) {
                i->setStartInTray(value.toBool());
                return true;
            } else
                return false;
        }
        case GroupView: {
            if (value.userType() == QMetaType::Bool) {
                i->setGroupView(value.toBool());
                return true;
            } else
                return false;
        }
        case ScrollbarsInRoomlist: {
            if (value.userType() == QMetaType::Bool) {
                i->setScrollbarsInRoomlist(value.toBool());
                return true;
            } else
                return false;
        }
        case Markdown: {
            if (value.userType() == QMetaType::Bool) {
                i->setMarkdown(value.toBool());
                return true;
            } else
                return false;
        }
        case SendMessageKey: {
            auto newKey = value.toInt();
            if (newKey < 0 ||
                QMetaEnum::fromType<UserSettings::SendMessageKey>().keyCount() <= newKey)
                return false;

            i->setSendMessageKey(static_cast<UserSettings::SendMessageKey>(newKey));
            return true;
        }
        case Bubbles: {
            if (value.userType() == QMetaType::Bool) {
                i->setBubbles(value.toBool());
                return true;
            } else
                return false;
        }
        case SmallAvatars: {
            if (value.userType() == QMetaType::Bool) {
                i->setSmallAvatars(value.toBool());
                return true;
            } else
                return false;
        }
        case AnimateImagesOnHover: {
            if (value.userType() == QMetaType::Bool) {
                i->setAnimateImagesOnHover(value.toBool());
                return true;
            } else
                return false;
        }
        case TypingNotifications: {
            if (value.userType() == QMetaType::Bool) {
                i->setTypingNotifications(value.toBool());
                return true;
            } else
                return false;
        }
        case SortByImportance: {
            if (value.userType() == QMetaType::Bool) {
                i->setSortByImportance(value.toBool());
                return true;
            } else
                return false;
        }
        case SortByAlphabet: {
            if (value.userType() == QMetaType::Bool) {
                i->setSortByAlphabet(value.toBool());
                return true;
            } else
                return false;
        }
        case ButtonsInTimeline: {
            if (value.userType() == QMetaType::Bool) {
                i->setButtonsInTimeline(value.toBool());
                return true;
            } else
                return false;
        }
        case TimelineMaxWidth: {
            if (value.canConvert(QMetaType::fromType<int>())) {
                i->setTimelineMaxWidth(value.toInt());
                return true;
            } else
                return false;
        }
        case ReadReceipts: {
            if (value.userType() == QMetaType::Bool) {
                i->setReadReceipts(value.toBool());
                return true;
            } else
                return false;
        }
        case DesktopNotifications: {
            if (value.userType() == QMetaType::Bool) {
                i->setDesktopNotifications(value.toBool());
                return true;
            } else
                return false;
        }
        case AlertOnNotification: {
            if (value.userType() == QMetaType::Bool) {
                i->setAlertOnNotification(value.toBool());
                return true;
            } else
                return false;
        }
        case AvatarCircles: {
            if (value.userType() == QMetaType::Bool) {
                i->setAvatarCircles(value.toBool());
                return true;
            } else
                return false;
        }
        case UseIdenticon: {
            if (value.userType() == QMetaType::Bool) {
                i->setUseIdenticon(value.toBool());
                return true;
            } else
                return false;
        }
        case OpenImageExternal: {
            if (value.userType() == QMetaType::Bool) {
                i->setOpenImageExternal(value.toBool());
                return true;
            } else
                return false;
        }
        case OpenVideoExternal: {
            if (value.userType() == QMetaType::Bool) {
                i->setOpenVideoExternal(value.toBool());
                return true;
            } else
                return false;
        }
        case DecryptSidebar: {
            if (value.userType() == QMetaType::Bool) {
                i->setDecryptSidebar(value.toBool());
                return true;
            } else
                return false;
        }
        case DecryptNotifications: {
            if (value.userType() == QMetaType::Bool) {
                i->setDecryptNotifications(value.toBool());
                return true;
            } else
                return false;
        }
        case SpaceNotifications: {
            if (value.userType() == QMetaType::Bool) {
                i->setSpaceNotifications(value.toBool());
                return true;
            } else
                return false;
        }
        case FancyEffects: {
            if (value.userType() == QMetaType::Bool) {
                i->setFancyEffects(value.toBool());
                return true;
            } else
                return false;
        }
        case ReducedMotion: {
            if (value.userType() == QMetaType::Bool) {
                i->setReducedMotion(value.toBool());
                return true;
            } else
                return false;
        }
        case PrivacyScreen: {
            if (value.userType() == QMetaType::Bool) {
                i->setPrivacyScreen(value.toBool());
                return true;
            } else
                return false;
        }
        case PrivacyScreenTimeout: {
            if (value.canConvert(QMetaType::fromType<int>())) {
                i->setPrivacyScreenTimeout(value.toInt());
                return true;
            } else
                return false;
        }
        case MobileMode: {
            if (value.userType() == QMetaType::Bool) {
                i->setMobileMode(value.toBool());
                return true;
            } else
                return false;
        }
        case DisableSwipe: {
            if (value.userType() == QMetaType::Bool) {
                i->setDisableSwipe(value.toBool());
                return true;
            } else
                return false;
        }
        case FontSize: {
            if (value.canConvert(QMetaType::fromType<double>())) {
                i->setFontSize(value.toDouble());
                return true;
            } else
                return false;
        }
        case Font: {
            if (value.userType() == QMetaType::Int) {
                // Special handling to grab our injected system font option
                auto v = value.toInt();
                i->setFontFamily(v == 0 ? QString{} : QFontDatabase::families().at(v - 1));
                return true;
            } else
                return false;
        }
        case EmojiFont: {
            if (value.userType() == QMetaType::Int) {
                // More special handling for the default font option
                auto v = value.toInt();
                i->setEmojiFontFamily(
                  v <= 0 ? QString()
                         : QFontDatabase::families(QFontDatabase::WritingSystem::Symbol).at(v - 1));
                return true;
            } else
                return false;
        }
        case Ringtone: {
            if (value.userType() == QMetaType::Int) {
                int ringtone = value.toInt();

                // setRingtone is called twice, because updating the list breaks the set value,
                // because it does not exist yet!
                if (ringtone == 2) {
                    QString homeFolder =
                      QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
                    auto filepath = QFileDialog::getOpenFileName(
                      nullptr, tr("Select a file"), homeFolder, tr("All Files (*)"));
                    if (!filepath.isEmpty()) {
                        i->setRingtone(filepath);
                        i->setRingtone(filepath);
                    }
                } else if (ringtone == 0) {
                    i->setRingtone(QStringLiteral("Mute"));
                    i->setRingtone(QStringLiteral("Mute"));
                } else if (ringtone == 1) {
                    i->setRingtone(QStringLiteral("Default"));
                    i->setRingtone(QStringLiteral("Default"));
                }
                return true;
            }
            return false;
        }
        case Microphone: {
            if (value.userType() == QMetaType::Int) {
                i->setMicrophone(data(index, Values).toStringList().at(value.toInt()));
                return true;
            } else
                return false;
        }
        case Camera: {
            if (value.userType() == QMetaType::Int) {
                i->setCamera(data(index, Values).toStringList().at(value.toInt()));
                return true;
            } else
                return false;
        }
        case CameraResolution: {
            if (value.userType() == QMetaType::Int) {
                i->setCameraResolution(data(index, Values).toStringList().at(value.toInt()));
                return true;
            } else
                return false;
        }
        case CameraFrameRate: {
            if (value.userType() == QMetaType::Int) {
                i->setCameraFrameRate(data(index, Values).toStringList().at(value.toInt()));
                return true;
            } else
                return false;
        }
        case UseStunServer: {
            if (value.userType() == QMetaType::Bool) {
                i->setUseStunServer(value.toBool());
                return true;
            } else
                return false;
        }
        case OnlyShareKeysWithVerifiedUsers: {
            if (value.userType() == QMetaType::Bool) {
                i->setOnlyShareKeysWithVerifiedUsers(value.toBool());
                return true;
            } else
                return false;
        }
        case ShareKeysWithTrustedUsers: {
            if (value.userType() == QMetaType::Bool) {
                i->setShareKeysWithTrustedUsers(value.toBool());
                return true;
            } else
                return false;
        }
        case UseOnlineKeyBackup: {
            if (value.userType() == QMetaType::Bool) {
                i->setUseOnlineKeyBackup(value.toBool());
                return true;
            } else
                return false;
        }
        case ExposeDBusApi: {
            if (value.userType() == QMetaType::Bool) {
                i->setExposeDBusApi(value.toBool());
                return true;
            } else
                return false;
        }
        case UpdateSpaceVias: {
            if (value.userType() == QMetaType::Bool) {
                i->setUpdateSpaceVias(value.toBool());
                return true;
            } else
                return false;
        }
        case ExpireEvents: {
            if (value.userType() == QMetaType::Bool) {
                i->setExpireEvents(value.toBool());
                return true;
            } else
                return false;
        }
        }
    }
    return false;
}

void
UserSettingsModel::importSessionKeys()
{
    const QString homeFolder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString fileName   = QFileDialog::getOpenFileName(
      nullptr, tr("Open Sessions File"), homeFolder, QLatin1String(""));

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(nullptr, tr("Error"), file.errorString());
        return;
    }

    auto bin     = file.peek(file.size());
    auto payload = std::string(bin.data(), bin.size());

    bool ok;
    auto password = QInputDialog::getText(nullptr,
                                          tr("File Password"),
                                          tr("Enter the passphrase to decrypt the file:"),
                                          QLineEdit::Password,
                                          QLatin1String(""),
                                          &ok);
    if (!ok)
        return;

    if (password.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Error"), tr("The password cannot be empty"));
        return;
    }

    try {
        auto sessions = mtx::crypto::decrypt_exported_sessions(payload, password.toStdString());
        cache::importSessionKeys(std::move(sessions));
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error"), e.what());
    }
}
void
UserSettingsModel::exportSessionKeys()
{
    // Open password dialog.
    bool ok;
    auto password = QInputDialog::getText(nullptr,
                                          tr("File Password"),
                                          tr("Enter passphrase to encrypt your session keys:"),
                                          QLineEdit::Password,
                                          QLatin1String(""),
                                          &ok);
    if (!ok)
        return;

    if (password.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Error"), tr("The password cannot be empty"));
        return;
    }

    auto repeatedPassword = QInputDialog::getText(nullptr,
                                                  tr("Repeat File Password"),
                                                  tr("Repeat the passphrase:"),
                                                  QLineEdit::Password,
                                                  QLatin1String(""),
                                                  &ok);
    if (!ok)
        return;

    if (password != repeatedPassword) {
        QMessageBox::warning(nullptr, tr("Error"), tr("Passwords don't match"));
        return;
    }

    // Open file dialog to save the file.
    const QString homeFolder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString fileName   = QFileDialog::getSaveFileName(
      nullptr, tr("File to save the exported session keys"), homeFolder);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, tr("Error"), file.errorString());
        return;
    }

    // Export sessions & save to file.
    try {
        auto encrypted_blob = mtx::crypto::encrypt_exported_sessions(cache::exportSessionKeys(),
                                                                     password.toStdString());

        QString b64 = QString::fromStdString(mtx::crypto::bin2base64(encrypted_blob));

        QString prefix(QStringLiteral("-----BEGIN MEGOLM SESSION DATA-----"));
        QString suffix(QStringLiteral("-----END MEGOLM SESSION DATA-----"));
        QString newline(QStringLiteral("\n"));
        QTextStream out(&file);
        out << prefix << newline << b64 << newline << suffix << newline;
        file.close();
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error"), e.what());
    }
}
void
UserSettingsModel::requestCrossSigningSecrets()
{
    olm::request_cross_signing_keys();
}
void
UserSettingsModel::downloadCrossSigningSecrets()
{
    olm::download_cross_signing_keys();
}

void
UserSettingsModel::refreshDevices()
{
    CallDevices::instance().refresh();
}

UserSettingsModel::UserSettingsModel(QObject *p)
  : QAbstractListModel(p)
{
    auto s = UserSettings::instance();
    connect(s.get(), &UserSettings::themeChanged, this, [this]() {
        emit dataChanged(index(Theme), index(Theme), {Value});
    });
    connect(s.get(), &UserSettings::mobileModeChanged, this, [this]() {
        emit dataChanged(index(MobileMode), index(MobileMode), {Value});
    });
    connect(s.get(), &UserSettings::disableSwipeChanged, this, [this]() {
        emit dataChanged(index(DisableSwipe), index(DisableSwipe), {Value});
    });

    connect(s.get(), &UserSettings::fontChanged, this, [this]() {
        emit dataChanged(index(Font), index(Font), {Value});
    });
    connect(s.get(), &UserSettings::fontSizeChanged, this, [this]() {
        emit dataChanged(index(FontSize), index(FontSize), {Value});
    });
    connect(s.get(), &UserSettings::emojiFontChanged, this, [this]() {
        emit dataChanged(index(EmojiFont), index(EmojiFont), {Value});
    });
    connect(s.get(), &UserSettings::avatarCirclesChanged, this, [this]() {
        emit dataChanged(index(AvatarCircles), index(AvatarCircles), {Value});
    });
    connect(s.get(), &UserSettings::useIdenticonChanged, this, [this]() {
        emit dataChanged(index(UseIdenticon), index(UseIdenticon), {Value});
    });
    connect(s.get(), &UserSettings::openImageExternalChanged, this, [this]() {
        emit dataChanged(index(OpenImageExternal), index(OpenImageExternal), {Value});
    });
    connect(s.get(), &UserSettings::openVideoExternalChanged, this, [this]() {
        emit dataChanged(index(OpenVideoExternal), index(OpenVideoExternal), {Value});
    });
    connect(s.get(), &UserSettings::privacyScreenChanged, this, [this]() {
        emit dataChanged(index(PrivacyScreen), index(PrivacyScreen), {Value});
        emit dataChanged(index(PrivacyScreenTimeout), index(PrivacyScreenTimeout), {Enabled});
    });
    connect(s.get(), &UserSettings::privacyScreenTimeoutChanged, this, [this]() {
        emit dataChanged(index(PrivacyScreenTimeout), index(PrivacyScreenTimeout), {Value});
    });

    connect(s.get(), &UserSettings::timelineMaxWidthChanged, this, [this]() {
        emit dataChanged(index(TimelineMaxWidth), index(TimelineMaxWidth), {Value});
    });
    connect(s.get(), &UserSettings::messageHoverHighlightChanged, this, [this]() {
        emit dataChanged(index(MessageHoverHighlight), index(MessageHoverHighlight), {Value});
    });
    connect(s.get(), &UserSettings::enlargeEmojiOnlyMessagesChanged, this, [this]() {
        emit dataChanged(index(EnlargeEmojiOnlyMessages), index(EnlargeEmojiOnlyMessages), {Value});
    });
    connect(s.get(), &UserSettings::animateImagesOnHoverChanged, this, [this]() {
        emit dataChanged(index(AnimateImagesOnHover), index(AnimateImagesOnHover), {Value});
    });
    connect(s.get(), &UserSettings::showImageChanged, this, [this]() {
        emit dataChanged(index(ShowImage), index(ShowImage), {Value});
    });
    connect(s.get(), &UserSettings::typingNotificationsChanged, this, [this]() {
        emit dataChanged(index(TypingNotifications), index(TypingNotifications), {Value});
    });
    connect(s.get(), &UserSettings::readReceiptsChanged, this, [this]() {
        emit dataChanged(index(ReadReceipts), index(ReadReceipts), {Value});
    });
    connect(s.get(), &UserSettings::buttonInTimelineChanged, this, [this]() {
        emit dataChanged(index(ButtonsInTimeline), index(ButtonsInTimeline), {Value});
    });
    connect(s.get(), &UserSettings::markdownChanged, this, [this]() {
        emit dataChanged(index(Markdown), index(Markdown), {Value});
    });
    connect(s.get(), &UserSettings::sendMessageKeyChanged, this, [this]() {
        emit dataChanged(index(SendMessageKey), index(SendMessageKey), {Value});
    });
    connect(s.get(), &UserSettings::bubblesChanged, this, [this]() {
        emit dataChanged(index(Bubbles), index(Bubbles), {Value});
    });
    connect(s.get(), &UserSettings::smallAvatarsChanged, this, [this]() {
        emit dataChanged(index(SmallAvatars), index(SmallAvatars), {Value});
    });
    connect(s.get(), &UserSettings::groupViewStateChanged, this, [this]() {
        emit dataChanged(index(GroupView), index(GroupView), {Value});
    });
    connect(s.get(), &UserSettings::scrollbarsInRoomlistChanged, this, [this]() {
        emit dataChanged(index(ScrollbarsInRoomlist), index(ScrollbarsInRoomlist), {Value});
    });
    connect(s.get(), &UserSettings::roomSortingChangedImportance, this, [this]() {
        emit dataChanged(index(SortByImportance), index(SortByImportance), {Value});
    });
    connect(s.get(), &UserSettings::roomSortingChangedAlphabetical, this, [this]() {
        emit dataChanged(index(SortByAlphabet), index(SortByAlphabet), {Value});
    });
    connect(s.get(), &UserSettings::decryptSidebarChanged, this, [this]() {
        emit dataChanged(index(DecryptSidebar), index(DecryptSidebar), {Value});
    });
    connect(s.get(), &UserSettings::decryptNotificationsChanged, this, [this]() {
        emit dataChanged(index(DecryptNotifications), index(DecryptNotifications), {Value});
    });
    connect(s.get(), &UserSettings::spaceNotificationsChanged, this, [this]() {
        emit dataChanged(index(SpaceNotifications), index(SpaceNotifications), {Value});
    });
    connect(s.get(), &UserSettings::fancyEffectsChanged, this, [this]() {
        emit dataChanged(index(FancyEffects), index(FancyEffects), {Value});
    });
    connect(s.get(), &UserSettings::reducedMotionChanged, this, [this]() {
        emit dataChanged(index(ReducedMotion), index(ReducedMotion), {Value});
    });
    connect(s.get(), &UserSettings::trayChanged, this, [this]() {
        emit dataChanged(index(Tray), index(Tray), {Value});
        emit dataChanged(index(StartInTray), index(StartInTray), {Enabled});
    });
    connect(s.get(), &UserSettings::startInTrayChanged, this, [this]() {
        emit dataChanged(index(StartInTray), index(StartInTray), {Value});
    });

    connect(s.get(), &UserSettings::desktopNotificationsChanged, this, [this]() {
        emit dataChanged(index(DesktopNotifications), index(DesktopNotifications), {Value});
    });
    connect(s.get(), &UserSettings::alertOnNotificationChanged, this, [this]() {
        emit dataChanged(index(AlertOnNotification), index(AlertOnNotification), {Value});
    });

    connect(s.get(), &UserSettings::useStunServerChanged, this, [this]() {
        emit dataChanged(index(UseStunServer), index(UseStunServer), {Value});
    });
    connect(s.get(), &UserSettings::microphoneChanged, this, [this]() {
        emit dataChanged(index(Microphone), index(Microphone), {Value, Values});
    });
    connect(s.get(), &UserSettings::cameraChanged, this, [this]() {
        emit dataChanged(index(Camera), index(Camera), {Value, Values});
    });
    connect(s.get(), &UserSettings::cameraResolutionChanged, this, [this]() {
        emit dataChanged(index(CameraResolution), index(CameraResolution), {Value, Values});
    });
    connect(s.get(), &UserSettings::cameraFrameRateChanged, this, [this]() {
        emit dataChanged(index(CameraFrameRate), index(CameraFrameRate), {Value, Values});
    });
    connect(s.get(), &UserSettings::ringtoneChanged, this, [this]() {
        emit dataChanged(index(Ringtone), index(Ringtone), {Values, Value});
    });

    connect(s.get(), &UserSettings::onlyShareKeysWithVerifiedUsersChanged, this, [this]() {
        emit dataChanged(
          index(OnlyShareKeysWithVerifiedUsers), index(OnlyShareKeysWithVerifiedUsers), {Value});
    });
    connect(s.get(), &UserSettings::shareKeysWithTrustedUsersChanged, this, [this]() {
        emit dataChanged(
          index(ShareKeysWithTrustedUsers), index(ShareKeysWithTrustedUsers), {Value});
    });
    connect(s.get(), &UserSettings::useOnlineKeyBackupChanged, this, [this]() {
        emit dataChanged(index(UseOnlineKeyBackup), index(UseOnlineKeyBackup), {Value});
    });
    connect(MainWindow::instance(), &MainWindow::secretsChanged, this, [this]() {
        emit dataChanged(index(OnlineBackupKey), index(MasterKey), {Value, Good});
    });
    connect(s.get(), &UserSettings::exposeDBusApiChanged, this, [this] {
        emit dataChanged(index(ExposeDBusApi), index(ExposeDBusApi), {Value});
    });
    connect(s.get(), &UserSettings::updateSpaceViasChanged, this, [this] {
        emit dataChanged(index(UpdateSpaceVias), index(UpdateSpaceVias), {Value});
    });
    connect(s.get(), &UserSettings::expireEventsChanged, this, [this] {
        emit dataChanged(index(ExpireEvents), index(ExpireEvents), {Value});
    });

    connect(&CallDevices::instance(), &CallDevices::devicesChanged, this, [this] {
        emit dataChanged(index(Microphone), index(Microphone), {Value, Values});
        emit dataChanged(index(Camera), index(Camera), {Value, Values});
        emit dataChanged(index(CameraResolution), index(CameraResolution), {Value, Values});
        emit dataChanged(index(CameraFrameRate), index(CameraFrameRate), {Value, Values});
    });

    connect(&CallDevices::instance(), &CallDevices::scanningChanged, this, [this] {
        emit dataChanged(index(RescanAudioVideoDevices), index(RescanAudioVideoDevices), {Value});
        emit dataChanged(index(Microphone), index(Microphone), {Enabled});
        emit dataChanged(index(Camera), index(Camera), {Enabled});
        emit dataChanged(index(CameraResolution), index(CameraResolution), {Enabled});
        emit dataChanged(index(CameraFrameRate), index(CameraFrameRate), {Enabled});
    });
}

#include "moc_UserSettingsPage.cpp"
