# Changelog

## [0.10.2] -- 2022-09-22

### Security release

- Fixes potential secret poisoning by the homeserver
- A crash when validation malicious html

Thanks to the matrix.org security team for disclosing this issue.

An update is highly recommended. Otherwise you can temporarily protect against
this issue by not verifying your own devices and not pressing the request button
in the setting.

## [0.10.1] -- 2022-09-07

### Highlights

- Community editing support 🤼
  - You can now create communities.
  - You can add and remove rooms to and from communities in various ways.
- Prettier joins ✨
  - You can now see the avatar, title, topic and member count of the room you
    are trying to join.
  - You can see if a room requires knocking or can be joined directly.
  - Requires MSC3266 support from your server.

### Features

- Add a discrete edit button to room profiles. (Hiers)
- Don't escape the summary tags on sending.
- Focus message area when pressing Escape. (Forest)
- Barebones spoiler support on desktop platforms.
- Enable encryption for DMs started from a profile by default.
- Enable option to disable notification sounds and badges on macOS.
- Speed up the completion trie. (nenomius)

### Translations

- Polish (Romanik, luff)
- Dutch (Thulinma)
- Finnish (Lurkki)
- Estonian (Priit)
- Indonesian (Linerly)

### Bugfixes

- Fix crash on empty private receipts sent by some servers.
- Don't set a transient parent for child chat windows.
- Validate roomid, state_key, event_id and userids sent by the server.
- Fix empty widgets showing up in the widget list.
- Clean up linter config. (Forest)
- Use the right palette colors for reactions. (Forest)
- Fix groups sidebar's grammar. (Forest)
- Fix version position. (Zirnc)
- Properly validate urls in image tags.
- Case insensitive member search.
- Fix crash on global profiles.
- Fix crash on incomplete identity keys.
- Fix message notification format on Windows.
- Fix room members menu opening profiles for the wrong room.

### Notes

Requires mtxclient 0.8.1 and fixes a few crashes that can be abused by remote
users.

## [0.10.0] -- 2022-07-22

### Highlights

- Notification counts 💯 (LorenDB, d42)
  - You can now see notification counts in more spaces, like your task bar or in
      the community sidebar.
  - For better work-life balance you can hide the notification counts on a per
      space basis.
  - For notification counts in the task bar your desktop environment needs to
      support the Unity protocol.
  - Notifications are also preserved across restarts now.
- Moderation 👮‍♀️
  - You can now change the permissions and aliases of a room.
  - Permissions are shown in the Memberlist
  - A new `/redact` command to redact an event or all messages by a user.
  - You can now provide a reason when inviting, knocking, kicking and banning
      users.
- Faster startup ⚡
  - On at least some systems startup should now be instant even with thousands
      of rooms.
- Encryption improvements 🛡️
  - Support for the most recent changes to Matrix E2EE including fallback keys,
      no longer relying on the sender_key.
  - Compatibility and stability improvements when dealing with different base64
      encodings and when verifying users and devices.
  - Fetch the whole online key backup at the klick of a button.
- Integration with external apps 🗺️ (LorenDB)
  - Nheko now has a D-Bus API, which you can enable in the settings menu.
  - This allows applications like KRunner or Rofi to list and switch between
      rooms.

### Features

- Create a room link from a room. (brausepulver)
- Support rendering policy rules.
- Show notification counts for spaces (with options to disable them per space).
   (LorenDB)
- Keep notification counts across restarts.
- Support the new call events (but not the signaling yet). (r0hit)
- Add a dbus API, which allows external applications to list and switch rooms in
    Nheko. (LorenDB)
- Support editing room aliases.
- Support editing room permissions.
- Allow redacting all locally cached messages of a user using `/redact
    @userid:server.tld reason`.
- Request full online key backup when toggling the online backup button.
- Support the `knock_restricted` join rule.
- Allow cancelling uploads using escape. (r0hit)
- Send images on enter.
- Close image viewer when clicking on the background.
- Speedup startup by not loading messages for the room preview.
- Make settings slightly narrower.
- Show unread counts in the taskbar (if the Unity protocol is supported). (d42)
- Indicate if a room has no topic in the settings. (LorenDB)
- Simplify Fedora build instructions. (DaKnig)
- Support e2ee fallback keys.
- Allow opening rooms in separate windows.
- Support more image formats in flatpak.
- Show powerlevels in the memberlist.
- Use less exotic emoji shortcodes. (Bulby)
- Support sorting and filtering the memberlist. (LorenDB)
- Make initial spinner half transparent. (LorenDB)
- Fancier rendering for image pack changes. (tastytea)
- Allow accessing member list and room settings for spaces. (LorenDB)
- Add zsh completions. (tastytea)
- Fancy rendering for Powerlevel changes. (MTRNord)
- Make sender_key in encrypted messages optional.
- Close current room using Ctrl-W. (LorenDB)
- Allow knocking on failed room joins.
- Allow knocking via matrix.to urls.
- Allow specifying reasons for every room membership change.
- Make room name and topic editing inline.
- Add a jump to bottom button. (Malte)
- Port room creation to qml. (Malte)
- Streamline direct chat creation. (Malte)

### Translations

- Russian (Alexey Murz Korepov, Artem, Herecore, balsoft, librehacker,
    glebasson, Mihail Iosilevich)
- Chinese (Nekogawa Mio, Poesty Li, Reiuji Utsuho, hulb, ling, RainSlide, hosxy)
- German
- Dutch (Jaron Viëtor)
- Finnish (Lurkki, Aminda)
- Indonesian (Linerly)
- Estonian (Priit)
- French (Symphorien, Glandos, Eldred)
- Serbian (Miroslav)

### Bugfixes

- Fix verification requests not stopping properly when initiated from this
    instance.
- Don't send matrix.to markdown links in replies.
- Make the database work on 32bit systems again. (MayeulC)
- Add missing window decoration to room directory dialog on macOS.
- Don't crash on empty image packs.
- Fix spacing of encryption indicator in the room tite if it contains widgets.
- Emojis during verification should no longer be clipped.
- Don't ping the whole room when replying to users with a localpart of `room`.
- Make icons sharp on all platforms. (q234rty)
- Work around synapse not sending the original resolution when requesting large
    thumbnails to make large thumbnails less blurry. (brausepulver)
- Fix weak symbols from private object destructor. (Jason)
- Fix failed uploads not cancelling properly.
- Edits now properly update in replies again.
- Improve text paste experience. (Syldra)
- Pins should now properly update when the events are fetched.
- Support latest iteration of the hidden read receipts MSC.
- Fix cursor movement with some themes. (Syldra)
- Properly handle glare during verification.
- Set an Element Android compatible height for custom emotes.
- Don't crash because of reusing items in completer on some platforms.
- Fix the privacy screen on popped out windows.
- Properly scale animated images.
- Don't clip pinned messages.
- Use correct powerlevels for direct chats.
- Properly close cursors before committing txn.
- Don't fail if a different client used the wrong base64 encoding when setting
    up SSSS.
- Spaces usually aren't DMs. (LorenDB)
- Don't send invalid aliases to the server on room creation. (Apurv)
- Fix invite dialog.

### Notes

This release requires Matrix API v1.1-v1.3. Please make sure your server is up
to date.

This release limits the maximum connections per host to 8. For best performance
we recommend your server supports http/2 so that slow requests don't slow down
other parts of the app (like sending messages).

Nheko now has KRunner and Rofi plugins (developed by LorenDB and LordMZTE
respectively).

## [0.9.3] -- 2022-03-25

### Highlights

- New upload UX
  - Queue multiple uploads by pasting or dragging multiple files.
  - Videos will now properly have a thumbnail as well as images.
  - Duration, width and height is now also properly included so that clients can resize appropriately.
  - Thumbnails are excluded if they are bigger than the original image. (tastytea)
- Improvements for mobile devices (Malte E)
  - You should now be able to scroll by touching anywhere with no random dead zones.
  - Preedit text can now be used in a completer and is properly sent
  - If an input method is active, pressing Enter will not send the current message.

### Features

- Optionally always open videos and images in an external program. (math)

### Improvements

- Build macOS releases against Qt 5.15.3 to resolve missing spaces after some punctuation.
- Send the shortcode as the body for stickers without a body.
- Elide long usernames in the timeline. (Malte E)
- Cleanup the reply popup. (Malte E)
- Use standard buttons where possible. (tastytea)
- Various improvements to the bubble layout. (Malte E)
- Enable online key backup by default.
- Update the bundled gstreamer in our Flatpaks.

### Translations

- Indonesian (Linerly)
- Estonian (Priit)
- Finnish (Priit)
- Esperanto (Tirifto)

### Bugfixes

- Fix hovering the action menu.
- Try to avoid using unknown UIA flows.
- Don't Components actively in use.
- Fix screensharing.
- Fix device id when doing SSO logins.

## [0.9.2] -- 2022-03-09

### Highlights

- Message bubbles (Malte E) 💬
  - Give a colorful and space saving background to messages.
  - Optionally shrink the usernames to save even more space.
  - Your messages are on the opposite side of messages sent by other users.
- Basic widgets 🗔
  - Widgets in a room are shown below the topic.
  - Open them in your browser to view them.

### Features

- Autocompleter for custom emotes using `~`. Note that this currently inserts raw html into the message input.
- Support running Nheko without a secrets service using a hidden setting.
- Add zooming and panning to the image overlay.
- Add a manpage. (tastytea)
- Offline indicator. (LorenDB)
- Proper previews for unjoined rooms in spaces (on supported servers).
- `/reset-state` /command to reset the state of a single room.
- Allow hiding some events from the timeline. (tastytea)
- Hidden read receipts. (Symphorien)
- Open room members dialog when clicking the encryption indicator.
- Click to copy room id. (Malte E)
- Allow specifiying a reason for message removal, bans and kicks. (tastytea)

### Improvements

- Speed up blurhash and jdenticon rendering.
- Use fewer threads for image decoding reducing memory use.
- Document secret service installation on Arch. (Marshall Lochbaum)
- Make edits replace previous notifications for the same message on Linux.
- Add alternatives for Alt-A as a shortcut on systems where that is already used.
- Apply clang-tidy suggestions. (MTRNord)
- Make custom emotes twice as high as the text to improve legibility. (tastytea)
- Ensure high quality scaling is used for custom emotes. (tastytea)
- Reduce allocations for the timeline by around a factor of 2.
- Render messages half as often, when displaying them for the first time.
- Increase maximum number of items in completers to 30.
- Run the gstreamer event loop also on macOS and Windows.
- Make presence update dynamically.
- Cleanup the raw message dialog.
- Make settings responsive.
- Improve Login and Registration pages.
- Add custom stickers & emotes to Q&A.
- Improve scrolling on touch screens. (Malte E)
- Reduce size of state events.
- Update OpenSUSE install instructions. (LorenDB)
- Use newer flatpak runtime.
- Fallback to using the shortcode in custom emotes, when there is no title set. (Ivan Pavluk)
- Improve a lot of hovering behaviours.
- Make spinboxes in scrollable pages unscrollable. (Malte E)
- Fix deprecation warnings in gstreamer code. (Scow)
- Make room directory fit mobile screens. (Malte E)
- Make room search accessible on mobile. (Malte E)
- Fix calls on mobile.
- Add arch binary repo. (digital-mystik)
- Improve long topics in the room settings. (Malte E)
- Fix typos. (ISSOtm)
- Improve the message input on mobile devices. (Malte E)

### Translations

- Indonesian (Linerly)
- Spanish (Lluise, Diego Collado, Richard, Edd Ludd, Drake)
- Catalan (Edd Ludd)
- French (ISSOtm)
- Estonian (Priit)
- Dutch (Thulinma)
- Chinese (hulb)

### Bugfixes

- Wrap member events.
- Fix rendering of some emoji.
- Fix crash when accepting invites.
- Don't fail startup on servers without presence.
- Fix grayscale images in notifications when using dunst.
- Clear sticker search. (tastytea)
- Limit width of username and roomname in the respective settings.
- Application name on Wayland.
- Memory leak when closing dialogs.
- Fix editing pending messages.
- Fix missing Windows runtime. (MTRNord)
- Fix a long standing issue where the font was set to a random one instead of the system default.
- Allow clicking on images in replies to scroll to that image again.
- Don't force https, when logging into a http only server.

## [0.9.1-1] -- 2021-02-24

- Rebuild against newer mtxclient to fix an incompatibility with Matrix v1.1 and newer.

If you use the binary packages for macOS or Windows, you will need this update.
Otherwise you can just wait for your distribution to update the mtxclient
package instead.

## [0.9.1] -- 2021-12-21

### Highlights

- Support pinned messages.

### Features

- Add recently used reactions. (LorenDB)
- Show spaces as a tree, that allows you to collapse sections.
- Add a filter for direct chats

### Improvements

- Set the app_id on Wayland. Useful for custom WM rules.
- Set notification category on Linux.
- Make Nheko show up in system notification settings on Linux.
- Make notification count bubbles expand some more. (LorenDB)
- Strip space chars from recovery passphrase. Should make them easier to enter.
- Make it obvious that undecryptable messages are a notification and not the actual message. (LorenDB)
- Added window role to image overlay. (Thulinma)
- Only show room pack button, when you can actually create one.
- Show some avatar for image packs.
- Allow clicking links in replies.
- Limit max memory usage of images.
- Allow swiping between views in single page mode Allows access to spaces on mobile for example.
- Get rid of a few clang-tidy warnings. (Marcus Hoffmann)
- Navigate to subspaces by clicking on them.
- Delete rooms even if we fail to leave.
- Change QML UI for redactions.
- If the locale is set to C, force english locale This fixes date formatting as well as count based translations.
- Use a more random hash to generate user colors.
- Mark rooms as direct chats in the proper places.
- Update macOS icon package to macOS-y style. (Quinn)
- Preliminary gstreamer 1.20 compatibility.

### Translations

- Indonesian (Linerly)
- Estonian (Priit Jõerüüt)
- French (Eldred HABERT)
- Dutch (Thulinma)
- Esperanto (Tirifto)
- Finnish (Priit Jõerüüt)
- Italian (Elia Tomasi)
- French (Mayeul Cantan)

### Bugfixes

- Fix crash when receiving matrix uri.
- Make opening room members from rooms settings dialog work. (LorenDB)
- Fix turnserver check not being started when restoring from cache.
- Vertically align message input.
- Properly set position of resize handler after letting it go.
- Fix escaped html showing up in playable media message labels.

## [0.9.0] -- 2021-11-19

### Highlights

- Somewhat stable end to end encryption
  - Show the room verification status
  - Configure Nheko to only send to verified users
  - Store the encryption keys securely in the OS-provided secrets service.
  - Support online keybackup as well as sharing historical session keys.
- Crosssigning bootstrapping
  - Crosssigning is used to simplify the verification process. In this release
     Nheko can setup crosssigning on a new account without having to use a
     different client.
  - Nheko now also prompts you, if there are any unverified devices and asks you to verify them.
- Room directory (Manu)
  - Search for rooms on your server and other servers. (Prezu)
  - If their topic interests you and it has the right amount of members, join
      the room and the discussion!
- Custom sticker packs
  - Add a custom sticker picker, that allows you to send stickers from MSC2545.
  - Support creating new sticker (and emote) packs.
  - You can share packs in a room and enable them globally or just for that
      room.
- Token authenticated registration (Callum)
  - Sign up with a token to servers, that have otherwise disabled registration.
  - This was done as part of GSoC and makes it easier to run private servers for
      your family and friends!

### Features

- Support email in registration (required on matrix.org for example)
- Warn, if an @room would mention the whole room, because some people don't like that.
- Support device removal as well as renaming. (Thulinma)
- Show your devices without encryption support, when showing your profile.
    (Thulinma)
- Move to the next room with unread messages by pressing `Alt-A`. (Symphorien)
- Support jdenticons as a placeholder for rooms or users without avatars.
    (LorenDB)
  - You will need to install https://github.com/Nheko-Reborn/qt-jdenticon
- Properly sign macOS builds.
- Support animated images like GIF and WebP.
  - Optionally just play them on hover.
- Support accepting knocks in the timeline.
- Close a room when clicking it again. (LorenDB)
- Close image overlay with escape.
- Support .well-known discovery during registration.
- Limited spaces support.
  - No nice display of nested spaces.
  - No previews of unjoined rooms.
  - No way to edit a space.
- Render room avatar changes in the timeline. (BShipman)
- Support pulling out the sidebar to make it wider.
- Allow editing pending messages instead of blocking until they are sent.
    (balsoft)
- Support mnemonics in the context menus. (AppAraat)
- Support TOFU for encryption. (Trust on first use)
- Right click -> copy address location.
- Forward messages. (Jedi18)
- Alt-F to forward messages.
- A new video and audio player, that should look a bit nicer.

### Improvements

- Translation updates:
  - French by MayeulC, ISSOtm, Glandos, Carl Schwan
  - Dutch by Thulinma, Bas van Rossem, Glael, Thijs
  - Esperanto by Tirifto, Colin
  - Estonian by Priit
  - Indonesian by Linerly
  - German by 123, Konstantin, fnetX, Mr. X, CryptKid
  - Portuguese (Portugal) by Tnpod, Xenovox, Gabriel R
  - Portuguese (Brazil) by Terry, zerowhy
  - Finnish by sdrrespudro, Priit
  - Polish by Prezu, AXD, stabor
  - Malayalam by vachan-maker
  - Italian by Daniele, Lorenzo
  - Spanish by lluise
  - Russian by kirillpt
  - Various wording improvements throughout.
- Verification status and identity keys should now update properly after login.
- Clicking the user in a read receipt opens their profile. (LorenDB)
- Invites should now work properly on mobile.
- Use the modern notifications on macOS.
- Decode blurhashes faster.
- Port various dialogs to Qml. (LorenDB)
- Improve paste support on Windows, when mimetype detection fails and pasting
    SVGs (Thulinma)
- --help and --version now work, even if Nheko is already running somewhere.
- Update emoji support to version 14.
- Properly navigate to linked to events. (Thulinma)
- Lots of smaller bugfixes and refactorings. (LorenDB)
- Scroll entire profile page and properly trim contents. (Thulinma)
- Make it easy to switch between global and room specific profiles. (Thulinma)
- Deduplicate messages sent by the server. (Thulinma)
- Decrease the margin of blockquotes. (tastytea)
- Alerts now work, if the homeserver does not implement the notifications
    endpoint. (Thulinma)
- Right click menu now works on replies.
- Decrypt encrypted media only in memory. (On macOS it still uses a tempfile because of <https://bugreports.qt.io/browse/QTBUG-69101>)
- Don't use CC-BY in the appstream license to not confuse Gnome Software.
- Document how to sync the repo on Gentoo. (alfasi)
- Support online key backup.
- Improve FAQ. (harmathy)
- Support Backtab/Shift-Tab for moving backwards in completer selections.
- Clear cache to support the new features.
- Improve the emoji completer (less jitter and fix places where it didn't open). (Thomaps Karpiniec)
- Cleanup @room escape logic.
- Improve performance of timeline rendering.
- Add fallback for sent stickers, so that they show on iOS.
- Load rooms somewhat lazily.
- Properly scale avatars to DPI.
- Round avatars once in the backend instead of on every render.
- Request keys of all members, when opening a room the first time.
- Timeout TCP connections, if heartbeat can't be heard.
- Change secrets name. You might need to rerequest your secrets after upgrading!
- Protect against replay attacks where megolm sessions are reused.
- Add "request keys" button to undecryptable messages.
- Remove superfluous permissions in Flatpak.
- Properly set window parents on Wayland.
- Properly show users and allow opening their profiles in the members and read
    receipt dialogs. (LorenDB)
- Use Qt5.15 Connections syntax in Qml.
- Remove "respond to keyrequests option". We now reply to the right requests
    automatically and securely.
- Show confirmation prompt when leaving a room.
- Add trailing newline to session export for gomuks compatibility.
- Use a fancy Nheko logo as the loading indicator.
- Improve how the invite dialog handles users. (LorenDB)
- Store more data about megolm sessions.
- Speed up database queries by caching transactions.
- Use curl for network requests.
  - This removes the boost dependency.
  - Proxies now work using the usual curl variables.
  - Fixes a myriad of crashes.
  - Faster.
  - Less CPU load.
  - Less bandwidth usage.
- Cleanup user color generation.
- Show borders around tables.
- Improve wording of a few menu entries. (absorber)
- Highlight navigated to message.
- Switched to the fluent icon set. (LorenDB)

### Bugfixes

- Redaction of edited messages should now actually show those messages as removed.
- Bootstrap after registration should run properly now.
- Getting logged out after registration should not happen anymore.
- Removed edgecases where identity keys could get uploaded twice.
- Fix the event loop when fetching secrets breaking random things like scrolling.
- Don't crash when clearing an empty timeline.
- Opening an invite in your browser or a matrix: URI should not crash Nheko anymore or do nothing.
- When clicking on an item in the roomlist, you don't have to move your mouse anymore, before being able to click again.
- Don't hide space childs when viewing that specific space and its children are hidden.
- Only allow specific URI schemes to be followed automatically.
- Properly hide day change indicator, when loading older messages.
- Rotate session properly when 'verified only' is set.
- Handle missing keys in key queries properly.
- Properly show the window title for Qml dialogs on windows.
- Don't show decryption errors in replies.
- Don't crash when storing secrets.
- Don't send megolm messages to ourselves, if possible.
- Fix SSSS without a password.
- Fix a few edge cases with OTK upload.
- Cache more media properly (i.e. in the Goose Chooser).
- Inline images in messages now load properly.
- Don't show verification requests after startup.
- Emoji picker now follows the theme.
- Send less newlines in the reply fallback.
- Fix tags going missing when joining spaces.
- Handle inline images with single quotes. (Cadair)
- Delay key requests until a room is opened.
- Fix rooms not showing, when groups endpoint is missing.
- Don't use deprecated parameters in /login.
- Fix encoding issues when translating matrix.to to matrix: URIs.
- Prevent edits from stripping the whole message, if it had a quote.

### Packaging changes

- Removed the AppImage
- Removed dependency on boost
- Now depends on [coeurl](https://nheko.im/nheko-reborn/coeurl), which depends on libevent and libcurl.
- VOIP support now needs to be explicitly controlled using the VOIP and SCREENSHARE_X11 cmake options.

## [0.8.2] -- 2021-04-23

### Highlights

- Edits
  - If you made a typo, just press the `Up` key and edit what you wrote.
  - Messages other users edited will get updated automatically and have a small
      pen symbol next to them.
- Privacy Screen
  - Blur your messages, when Nheko looses focus, which prevents others from
    peeking at your messages.
  - You can configure the timeout of when this happens.
- Improved notifications (contributed by lorendb)
  - No more breakage, because the message included a > on KDE based DEs.
  - Render html and images where possible in the notification.
  - Render if a message is a reply or someone sent an emote message more nicely
      where possible.
  - Encrypted notifications now show, that the content is encrypted instead of
      being empty.
- Screenshare support in calls on X11 (contributed by trilene)
  - Share your screen in a call!
  - Select if your mouse cursor should be shown or not and if your webcam should
      be included.
- SEND MESSAGES AS RAINBOWS! (contributed by LordMZTE)
  - YES MESSAGES, EMOTES AND NOTICES!

### Features

- Set your displayname and avatar from Nheko either globally or per room.
    (contributed by jedi18)
- Show room topic in the room settings.
- Double tap a message to reply to it.
- Leave a room using `/part` or `/leave`. (contributed by lorendb)
- Show mxid when hovering a username or avatar.
- Allow opening matrix: uris on Windows.
- Disable room pings caused by replies sent via Nheko (unless you are using
    Element Web/Desktop).

### Improvements
- Userprofile can be closed via the Escape key. No more hotel california!
    (contributed by lorendb)
- Most dialogs are now centered on the Nheko window.  (contributed by lorendb)
- Update Hungarian translations. (contributed by maxigaz)
- Update Estonian translations. (contributed by Priit)
- Update Russian translations. (contributed by Alexey Murz and Artem)
- Update Swedish translations. (contributed by Emilie)
- Update French translations. (contributed by MayeulC, Nicolas Guichard and Carl Schwan)
- Allow drag and drop of files on the whole timeline. (contributed by lorendb)
- Enable notifications on Haiku. (contributed by kallisti5)
- Update scheme handler to the latest matrix: scheme proposal.
- Close completers when typing a space after the colon. (contributed by jedi18)
- Port room settings to Qml. (contributed by jedi18)
- Improved read marker handling. Read marker should now get stuck less often.
- Various changes around hover and tap handling in the timeline, which hopefully
    now works more predicatably.
- Buttons in the timeline are now rendered in a box on hover on desktop
    platforms.
- Complete room links in the timeline after typing a # character. (contributed
    by jedi18)
- An improved quick switcher with better rendering and search. (contributed by jedi18)
- Some fixes around inline emoji and images.
- Jump into new rooms, after you created them. (contrubuted by jedi18)
- Improved search in the emoji picker.
- Allow disabling certificate checks via the config file.
- Use native menus where possible.
- Fix video playback on Windows. (contrubuted by jedi18)
- Send image messages by pressing Enter. (contributed by salahmak)
- Escape closes the upload widget. (contributed by salahmak)
- Improve session rotation and sharing in E2EE rooms.

### Bugfixes
- Emojis joined from separate emojis with a 0xfe0f in the middle should now
    render correctly.
- Fix a bug when logging out of a non default profile clearing the wrong
    profile. (contrubuted by lorendb)
- Various fixed around profile handling. (contributed by lorendb)
- Focus message input after a reaction. (contributed by jedi18)
- Disable native rendering to prevent kerning bugs on non integer scale factors.
- Fix duplex call devices not showing up. (contributed by trilene)
- Fix a few crashes when leaving a room. (contributed by jedi18)
- Fix hidden tags not updating properly. (contributed by jedi18)
- Fix some issues with login, when a server had SSO as well as password login
    enabled (for example matrix.org).
- Properly set the dialog flag for dialogs on most platforms. (Wayland does not
    support that.)
- Properly add license to source files.
- Fix fingerprint increasing the minimum window size.
- Don't send markdown links in the plain text body of events when autocompleting
    user or room names.
- Fix webcam not working in flatpaks.
- Fix markdown override in replies.
- Fix unsupported events causing errors when saving them. (contributed by
    anjanik)
- Fix exif rotation not being respected anymore in E2EE rooms.
- Remove unused qml plugins in the windows package.
- Fix broken olm channels automatically when noticed.
- Fix pasting not overwriting the selection.
- Fix Nheko sometimes overwriting received keys with keys it requested, even if
    they have a higher minimum index.

### Packaging changes
- Added xcb dependency on X11 based platforms for screensharing (optional)
- Bumped lmdbxx version from 0.9.14.0 to 1.0.0, which is a BREAKING change. You
    can get the new version here: https://github.com/hoytech/lmdbxx/releases
    (repo changed)
- Removed tweeny as a dependency.


## [0.8.1] -- 2021-01-27

### Features

- `/plain` and `/md` commands to override the current markdown setting. (contributed by lorendb)
- Allow persistent hiding of rooms with a specific tag (or from a community) via a context menu.
- Allow open media messages in an external program immediately. (contributed by rnhmjoj)

### Improvements

- Use async dbus connection for notifications. (contributed by lorendb)
- Update Hungarian translations. (contributed by maxigaz)
- Update Finnish translations. (contributed by Priit)
- Update Malayalam translations. (contributed by vachan-maker)
- Update Dutch translations. (contributed by Glael)
- Store splitter size across restarts.
- Add a border around the completer. (contributed by lorendb)
- Request keys for messages with unknown message indices (once per restart, when they are shown).
- Move the database location to XDG_DATA_DIR. (contributed by rnhmjoj)
- Reload the timeline after key backup import.
- Autoclose completer on `space`, when there are no matches.
- Make completer only react, when the mouse cursor is moved.

### Bugfixes

- Fix unhandled exception, when a device has no keys.
- Fix some cmake warnings regarding GNUInstallDirs.
- Fix tags being broken. If you have no tags showing up, you may want to logout and login again.
- Fix versionOk being called on the wrong thread. (contributed by Jedi18)
- Fix font tags showing up in media message filenames.
- Fix user profile in dark themes showing the wrong colors. (contributed by lorendb)
- Fix emoji category switching on old Qt versions. (contributed by lorendb)
- Fix old messages being replayed after a limited timeline.
- Fix empty secrets being returned from the wallet breaking verification.
- Make matrix link chat invites create a direct chat.
- Fix focus handling on room change or reply button clicks.
- Fix username completion deleting the character before it.

## [0.8.0] -- 2021-01-21

### Highlights

- Voice and Video Calls (contributed by trilene)
  - Call your friends right from within Nheko.
  - Use your camera if you want them to see your face!
  - This requires a somewhat new gstreamer, so our builds don't support it on all platforms yet.
- Cross-Signing and Device/User Verification (contributed by Chethan)
  - Verify who you are talking to!
  - Ensure no malicious people eavesdrop on you!
  - Enable your connected devices to access key backup and your friends to see, which of your devices you trust!
  - Show devices in a users profile.
- Separate profiles (contributed by lorendb)
  - Run multiple Nheko instances with separate profiles side by side.
  - Use multiple accounts at the same time in separate windows.

### Features

- Before a call select which audio device to use. (contributed by trilene)
- Auto request unknown keys from your own devices.
- Add a command to clear the timeline and reload it. (/clear-timeline).
- Add a command to rotate the outbound megolm session. (/rotate-megolm-session).
- React to messages instead of replying with arbitrary strings using `/react`.
- Inline emoji and user completers. (contributed by Lurkki)
- Show filename on hover over an image. (contributed by kamathmanu)
- Mobile mode, that disables text selection and changes some dialogs.
- Allow sending text after a `/shrug` command. (contributed by MayeulC)
- Allow selecting a ringtone. (contributed by trilene)
- View avatars fullscreen. (contributed by kamathmanu)
- Request or download cross signing secrets in the settings.
- Support 'matrix:' URIs. This works in app on all platforms and on Linux Nheko may be opened by clicking a 'matrix:' link.
- Support inline replies on notifications on Linux.

### Improvements

- Remove dependency on libsodium.
- Keep a cache of received messages on disk.
- Warn when kicking, banning or inviting people.
- Align day separators in the timeline. (contributed by not-chicken)
- Confirm quit during an active call. (contributed by trilene)
- Make timestamps somwhat fixed width.
- Add NixOS to readme. (contributed by Tony)
- Speed up database accesses.
- A lot of translation updates by various users.
- Port a few more parts of the UI to Qml.
- Various end-to-end encryption fixes.
- Use a QFontComboBox to select fonts. (contributed by lorendb)
- Delete text in input area with Ctrl+U. (contributed by lorendb)
- Reduce memory usage by not loading members into RAM.
- Speed up rendering the timeline by a lot by removing excessive clipping.
- Reload encrypted message when room_key is received.
- Improve wording in various places. (contributed by MayeulC)
- Improve rendering of avatars in various places. (contributed by MayeulC)
- Riot -> Element in README. (Contributed by Kim)
- Improve login and registration page error reporting. (contributed by kirillpt)
- Move CI to Gitlab.
- Use system Nheko icon on login page. (contributed by lorendb)
- Add Fedora build requirements. (contributed by trilene)
- Add ripple effect to various buttons.
- Allow more font sizes to be selected.
- Swedish translation. (contributed by Emilie)
- German translation. (contributed by Mr X and various others)
- Romanian translation. (contributed by Mihai)
- Polish translation. (contributed by luff)
- Russian translation. (contributed by kirillpt and librehacker)
- Italian translation. (contributed by Lorenzo)
- French translation. (contributed by MayeulC)
- Hungarian translation. (contributed by maxigaz)
- Show read markers when clicking read indicator. (contributed by lorendb)

### Bugfixes

- Fix text sometimes being rendered blurry.
- Fix not being able to change theme (contributed by not-chicken)
- Fix relations sometimes being sent as null in encrypted messages.
- Don't send formatted body without format.
- Links sometimes not opening properly from Qml.
- Fix autolinking breaking on single quotes.
- Fix translation loading on some locales.
- Don't send url in encrypted file events.
- Prevent duplicate messages from showing up in the timeline.
- Fix crash when pasting image from clipboard on macOS.
- Settings toggles don't get stuck anymore. (contributed by kirillpt)
- Fix some emojis being rendered as two emoji.
- Fix SSO login on some servers that allow multiple login methods. (contributed by d42)

### For packagers

- Nheko now depends on QtKeychain.
- Nheko optionally depends on GStreamer for VOIP.
- Nheko does not depend on Sodium anymore.
- Minimum OpenSSL version is now 1.1.

## [0.7.2] -- 2020-06-12

### Highlights

- Reactions
  - React to a message with an emoji! 🎉
  - Reactions are shown below a message in a small bubble with a counter.
  - By clicking on that, others can add to the reaction count.
  - It may help you celebrating a new Nheko Release or react with a 👎 to a failed build to express your frustration.
  - This uses a new emoji picker. The picker will be improved in the near future (better scrolling, sections, favorites, recently used or similar) and then probably replace the current picker.
- Support for tagging rooms `[tag]`
  - Assign custom tags to rooms from the context menu in the room list.
  - This allows filtering rooms via the group list. This puts you in a focus mode showing only the selected tags.
  - You can assign multiple tags to group rooms however you like.
- SSO Login
  - With this you can now login on servers, that only provide SSO.
  - Just enter any mxid on the server. Nheko will figure out that you need to use SSO and redirect your browser to the login page.
  - Complete the login in your browser and Nheko should automatically log you in.
- Presence
  - Shows online status of the people you are talking to.
  - You can define a custom status message to tell others what you are currently up to.
  - The status message appears next to the usernames in the timeline.
  - Your server needs to have presence enabled for this to work.

### Features

- Respect exif rotation of images
- An italian translation (contributed by Lorenzo Ancora)
- Optional alerts in your taskbar (contributed by z33ky)
- Optional bigger emoji only messages in the timeline (contributed by lkito)
- Optional hover feedback on messages (contributed by lkito)
- `/roomnick` to change your displayname in a single room.
- Preliminary support for showing inline images.
- Warn about unencrypted messages in encrypted rooms.

### Improvements

- perf: Use less CPU to sort the room list.
- Limit size of replies. This currently looks a bit rough, but should improve in the future with a gradient or at some other transition.
- perf: Only clean out old messages from the database every 500 syncs. (There is usually more than one sync every second)
- Improve the login and register masks a bit with hints and validation.
- Descriptions for settings (contributed by lkito)
- A visual indicator, that nheko is fetching messages and improved scrolling (contributed by Lasath Fernando)

### Bugfixes

- Fix not being able to join rooms
- Fix scale factor setting
- Buildfixes against gcc10 and Qt5.15 (missing includes)
- Settings now apply immediately again after changing them (only exception should be the scale factor)
- Join messages should never have empty texts now
- Timeline should now fail to render less often on platforms with native sibling windows.
- Don't rescale images on every frame on highdpi screens.

### Upgrade Notes

<span style="color: red;">This updates includes some changes to the database. Older versions don't handle that gracefully and will delete your database. It is therefore recommended to not downgrade below this version!</span>

## [0.7.1] -- 2020-04-24

### Features

- Show decrypted message source (helps debugging)
- Allow user to show / hide messages in encrypted rooms in sidebar

### Bugfixes

- Fix display of images sent by the user (thank you, wnereiz and not-chicken for reporting)
- Fix crash when trying to maximize image, that wasn't downloaded yet.
- Fix Binding restoreMode flooding logs on Qt 5.14.2+
- Fix with some qml styles hidden menu items leave empty space
- Fix encrypted messages not showing a user in the sidebar
- Fix hangs when generating colors with some system theme color schemes (#172)

## [0.7.0] -- 2020-04-19

0.7.0 *requires* mtxclient 0.3.0.  Make sure you compile against 0.3.0
if you do not use the mtxclient bundled with nheko.

### Features
- Make nheko session import / export format match riot.  Fixes #48
- Implement proper replies
- Add .well-known support for auto-completing homeserver information
- Add mentions viewer so you can see all the messages you have been mentioned in
  - Currently broken due to QML changes.  Will be fixed in the future.
- Add emoji font selection preference
- Encryption and decryption of media in E2EE rooms
- Square avatars
- Support for muting and unmuting rooms
- Basic support for playing audio and video messages in the timeline
- Support for a lot more event types (hiding them will come in the future)
- Support for sending all messages as plain text
- Support for inviting, kicking, banning and unbanning users
- Sort the room list by importance of messages (thanks @Alch-Emi)
- Experimental support for [blurhashes](https://github.com/matrix-org/matrix-doc/pull/2448)

### Improvements
- Add dedicated reply button to Timeline items.  Add button for other options so
    that right click isn't always required.
- Fix various things with regards to emoji rendering and the emoji picker
- Lots and lots and lots of localization updates.
- Additional tweaks to the system theme
- Render timeline in Qml to drop memory usage
- Reduce memory usage of avatars
- Close notifications after they have been read on Linux
- Escape html properly in most places
- A lot of improvements around the image overlay
- The settings page now resizes properly for small screens
- Miscellaneous styling improvements
- Simplify and speedup build
- Display more emojis in the selected emoji font
- Use 'system' theme as default if QT_QPA_PLATFORMTHEME is set

### Bugfixes

- Fix messages stuck on unread
- Reduce the amount of messages shown as "xxx sent an encrypted message"
- Fix various race conditions and crashes
- Fix some compatibility issues with the construct homeserver

Be aware, that Nheko now requires Qt 5.10 and boost 1.70 or higher.

## [0.6.4] - 2019-05-22

*Most* of the below fixes are due to updates in mtxclient.  Make sure you compile against 0.2.1
if you do not use the mtxclient bundled with nheko to get these fixes.

### Features
- Support V3 Rooms

### Improvements
- Fix #19
    - Fix initial sync issue caused by matrix-org/synapse#4898 (thanks @monokelpinguin)
    - Add additional lmbd max_dbs setting (thanks @AndrewJDR)
- Update DE translations (thanks @miocho)
- Update Dutch translations (thanks @vistaus)
- Fix text input UI bug (thanks @0xd800)
- Update linkifyMessage to parse HTML better (thanks @monokelpinguin)
- Update to Boost 1.69.0
- Fix some memory-leak scenarios due to mismatched new / delete (thanks @monokelpinguin)

### Other Changes
- mtxclient now builds as a Shared Library by default (instead of statically)

## [0.6.3] - 2019-02-08

### Features
- Room notifications now distinguish between general and user mentions by using different colors
- User names are now colored based on both the theme and a hash from their user id.
- Add font selection preference


### Improvements
- Fix room joining issue by escaping (thanks rnhmjoj)
- Mild tweaks to the dark and light themes
- Add paragraph tags back to markdown, fixing #2 / mujx#438
- Tweak author text to help differentiate it from the message text
- Some Russian translations have been added/fixed (thanks tim77)
- Partially address some build issues (related to #10)

## [0.6.2] - 2018-10-07

### Features
- Display tags as sorting items in the community panel (#401 @vberger)
- Add ability to configure the font size.

### Improvements
- Don't enable tray by default.
- Hard-coded pixel values were removed. The sizes are derived from the font.

### Other changes
- Removed room re-ordering option.

## [0.6.1] - 2018-09-26

### Improvements
- Add infinite scroll in member list. (#446)
- Use QPushButton on the preview modal.

### Bug fixes
- Clear text selection when focus is lost. (#409)
- Don't clear the member list when the modal is hidden. (#447)

## [0.6.0] - 2018-09-21

### Features
- Support for sending & receiving markdown formatted messages. (#283)
- Import/Export of megolm session keys. (Incompatible with Riot) (#358)
- macOS: The native emoji picker can be used.
- Context menu option to show the raw text message of an event. (#437)
- Rooms with unread messages are marked in the room list. (#313)
- Clicking on a user pill link will open the user profile.

### Improvements
- Update Polish translation (#430)
- Enable Qt auto scaling. (#397)
- Enable colors in the console logger.
- Refactor styling to better work with the system theme.

### Bug fixes
- Fixed crash when switching rooms. (#433)
- Fixed crash during low connectivity. (#406)
- Fixed issue with downloading media that don't have a generated thumbnail.
- macOS: Add missing border on the top bar.
- Fallback to the login screen when the one-time keys cannot be uploaded.
- Show the sidebar after initial sync. (#412)
- Fix regression, where cache format changes didn't trigger a logout.

## [0.5.5] - 2018-09-01

### Features
- Add the ability to change the room avatar. (#418)
- Show the room id in the room settings modal. (#416)

### Improvements
- More flicker improvements.
- Auto remove old messages from cache.

### Bug fixes
- Fixed issue where nheko will stop retrying initial sync. (#422)
- Fixed the incomplete version string on Info.plist (macOS) (#423)
- Fixed a use-after-free error during logout.
- Temporary fix to work with servers that don't support e2ee. i.e Construct, Dendrite (#371)

## [0.5.4] - 2018-08-21

Small release to address a crash during logout.

### Features

- The settings page now includes the device id & device fingerprint (thanks @valkum )
- The Polish translation has been updated (thanks @m4sk1n )

## [0.5.3] - 2018-08-12

### Features
- Add option to disable desktop notifications (#388)
- Allow user to configure join rules for a room.
- Add tab-completion for usernames (#394).

### Improvements
- Remove the space gap taken by the typing notifications.
- Remove hover event from emoji picker.
- Add tooltips for the message indicators (#377).
- Fix compilation on FreeBSD (#403)
- Update Polish translation.
- Small modal improvements.

### Bug fixes
- Remove dash from the version string when building outside of git.
- Remove unwanted whitespace from the user settings menu.
- Consider the scale ratio when scaling down images (#393).

## [0.5.2] - 2018-07-28

### Features
- Mark own read messages with a double checkmark (#377).
- Add option to specify the scale factor (#357, #335, #230).
- Add input field to specify the device name on login.
- Add option to ignore key requests altogether.
- Show device list in user profile & add option to create 1-1 chat.

### Improvements
- Add foreground color for disabled buttons on the dark theme.
- Increase the opacity of the hover color on the room list.
- Add missing tooltips on buttons (#249).
- Performance Improvements when filtering large number for rooms.
- Clear timeline widgets when they exceed a certain limit, to reduce the memory footprint (#158).
- Use native scrollbar in the timeline.
- Enable scrollbar on the room list for macOS.
- Remove some timeline flickering on macOS.
- Remove pixel values from modals.
- Update Polish translation.

### Bug fixes
- Fix crash when the server doesn't have the joined_groups endpoint (#389).
- Reject key requests for users that are not members of the room.
- Add user avatar after the `encryption is enabled` message (#378)

## [0.5.1] - 2018-07-17

### Improvements
- Add the -v / --version option, which displays the version of the application.
- Explicitly set no timeout for the Linux notifications.

### Bug fixes
- Fix crash when drag 'n drop files on text input. (#363)
- Convert MXC to HTTP URI for video files.
- Don't display the `m.room.encryption` event twice.
- Properly reset the auto-complete anchor when the popup closes. (#305)
- Use a brighter color for button's text on the system theme. (#355)

## [0.5.0] - 2018-07-15

### Features
- End-to-End encryption for text messages.
- Context menu option to request missing encryption keys.
- Desktop notifications on all platforms (Linux, macOS, Windows).
- Responsive UI (hidden sidebar/timeline).
- Basic support for replies (#292)

### Improvements
- Save timeline messages in cache for faster startup times.
- Debug logs will now be saved in a file.
- New translations
    - French (#329)
    - Polish (#349)
- No dependencies will be downloaded during build.

### Bug fixes
- Allow close events from the session manager (#353).
- Send image dimensions in m.image event (#215).
- Allow arbitrary resizing of the main window & restore sidebar's size (#160, #163, #187, #127).

## [0.4.3] - 2018-06-02

### Bug fixes
- Overdue fixes for some regressions with regard to widget height introduced in the previous two releases
- The matrix id will be shown on hover on the display name.

## [0.4.2] - 2018-05-25

### Bug fixes
- Make the number of unread messages fit in the bubble (#330)
- Use white for messages on the dark theme (#331)
- Fix "jumpy messages" regression

## [0.4.1] - 2018-05-23

### Features
- Menu to modify the name & topic of the room.
- Desktop notifications for macOS.
- Option to start in tray (#319)
- Russian translation (#318)
- Read support for the room access level (#324)

### Improvements
- HiDPI avatars.

### Bug fixes
- Fix for the line break on messages with very long words/links.
- Translations are working again.

## [0.4.0] - 2018-05-03

### Features
- Basic member list
- Basic room settings menu
- Support for displaying stickers
- Fuzzy search for rooms

### Improvements
- Cache refactoring (reduced memory consumption)
- Implement media cache (faster avatar loading)
- Show room tooltips when the sidebar is collapsed
- Flicker-free auto-completion menus (rooms, users)
- Improved message spacing in the timeline
- Improved macOS installer
- Fancier date separator widget
- Minor popup improvements

### Bug fixes
- Fix UI inconsistencies between room list & communities
- Adjust popup completion menu to fit its contents
- Fix stuck typing notifications
- Handle invalid access tokens

## [0.3.1] - 2018-04-13

### Improvements
- The auto-completion menu can be navigated with TAB.  (#294 )

### Bug fixes
- Show auto-completion menu even if there are fewer than the max number of matches (#294)
- Hide emoji panel when it's not under the mouse cursor. (#254, #246)

## [0.3.0] - 2018-04-03

### Features
- Add auto-completion pop-up for usernames (triggered with @). (#40)
- Add ability to redact messages.
- Add context menu option to save images. (#265)
- Pin invites to the top of the room list. (#252)
- Add environment variable to allow insecure connections (e.g self-signed certs) (#260)

### Improvements
- Updated dark theme.
- Add border in community list.
- Add version & build info in the settings menu.
- Easier packaging by allowing to use already built dependencies.

### Bug fixes
- Fix invite unreadable button colors on the system theme. (#248)
- Track invites so they can be removed from other clients. (#213)
- Fix text color on the room switcher. (#245)

## [0.2.1] - 2018-03-13

### Features
- Implement user registration with reCAPTCHA
- Add context menu option to mark events individually as read

### Bug fixes
- Update room name & avatar on newly created rooms
- Fix image uploading that was causing other image items to render its content

## [0.2.0] - 2018-03-05

### Features
- Support for pasting media & files into a room (#180)
- Server-side message & mention notifications
- Two new commands (`/fliptable`, `/shrug`)
- Option to disable typing notifications
- Check-mark to messages that have been received by the server (#93)
- Basic communities support (#195)
- Read receipt support
- Re-ordering of rooms based on activity

### Improvements
- Automatically focus on input when opening a dialog
- Keep syncing regardless of connectivity (#93)
- Create widgets on demand for messages added to the end of the timeline
- Messages received by `/messages` will be rendered upon entering the room
- Load last content from all rooms
- Load the initial cache data without blocking the UI

### Bug fixes
- Messages will be visible in the side bar after initial sync
- Enable room switcher only in the chat view (#251)
- Retry initial sync forever (#234)
- Show loading indicator while waiting for `/login` & `/logout`
- Disable minimize to tray except for the chat view.
- Fixed transparency issue on custom dialogs (#87)
- Fixed crash when there weren't any rooms

## [0.1.0] - 2017-12-26

The first release providing the basic features to make use of the Matrix network.
