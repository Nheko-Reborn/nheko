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

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScroller>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>

#include "Cache.h"
#include "Config.h"
#include "MatrixClient.h"
#include "Olm.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "ui/FlatButton.h"
#include "ui/ToggleButton.h"

#include "config/nheko.h"

UserSettings::UserSettings() { load(); }

void
UserSettings::load()
{
        QSettings settings;
        isTrayEnabled_                = settings.value("user/window/tray", false).toBool();
        hasDesktopNotifications_      = settings.value("user/desktop_notifications", true).toBool();
        isStartInTrayEnabled_         = settings.value("user/window/start_in_tray", false).toBool();
        isGroupViewEnabled_           = settings.value("user/group_view", true).toBool();
        isButtonsInTimelineEnabled_   = settings.value("user/timeline/buttons", true).toBool();
        isMarkdownEnabled_            = settings.value("user/markdown_enabled", true).toBool();
        isTypingNotificationsEnabled_ = settings.value("user/typing_notifications", true).toBool();
        sortByImportance_             = settings.value("user/sort_by_unread", true).toBool();
        isReadReceiptsEnabled_        = settings.value("user/read_receipts", true).toBool();
        theme_                        = settings.value("user/theme", defaultTheme_).toString();
        font_                         = settings.value("user/font_family", "default").toString();
        avatarCircles_                = settings.value("user/avatar_circles", true).toBool();
        decryptSidebar_               = settings.value("user/decrypt_sidebar", true).toBool();
        emojiFont_    = settings.value("user/emoji_font_family", "default").toString();
        baseFontSize_ = settings.value("user/font_size", QFont().pointSizeF()).toDouble();

        applyTheme();
}

void
UserSettings::setFontSize(double size)
{
        baseFontSize_ = size;
        save();
}

void
UserSettings::setFontFamily(QString family)
{
        font_ = family;
        save();
}

void
UserSettings::setEmojiFontFamily(QString family)
{
        emojiFont_ = family;
        save();
}

void
UserSettings::setTheme(QString theme)
{
        theme_ = theme;
        save();
        applyTheme();
}

void
UserSettings::applyTheme()
{
        QFile stylefile;

        static QPalette original;
        if (this->theme() == "light") {
                stylefile.setFileName(":/styles/styles/nheko.qss");
                QPalette lightActive(
                  /*windowText*/ QColor("#333"),
                  /*button*/ QColor("#333"),
                  /*light*/ QColor(0xef, 0xef, 0xef),
                  /*dark*/ QColor(220, 220, 220),
                  /*mid*/ QColor(110, 110, 110),
                  /*text*/ QColor("#333"),
                  /*bright_text*/ QColor("#333"),
                  /*base*/ QColor("white"),
                  /*window*/ QColor("white"));
                lightActive.setColor(QPalette::Highlight, QColor("#38a3d8"));
                lightActive.setColor(QPalette::ToolTipBase, lightActive.base().color());
                lightActive.setColor(QPalette::ToolTipText, lightActive.text().color());
                lightActive.setColor(QPalette::Link, QColor("#0077b5"));
                lightActive.setColor(QPalette::ButtonText, QColor("gray"));
                QApplication::setPalette(lightActive);
        } else if (this->theme() == "dark") {
                stylefile.setFileName(":/styles/styles/nheko-dark.qss");
                QPalette darkActive(
                  /*windowText*/ QColor("#caccd1"),
                  /*button*/ QColor(0xff, 0xff, 0xff),
                  /*light*/ QColor("#caccd1"),
                  /*dark*/ QColor("#2d3139"),
                  /*mid*/ QColor(110, 110, 110), // not used anywhere, this is for debugging
                  /*text*/ QColor("#caccd1"),
                  /*bright_text*/ QColor(0xff, 0xff, 0xff),
                  /*base*/ QColor("#2d3139"),
                  /*window*/ QColor("#202228"));
                darkActive.setColor(QPalette::Highlight, QColor("#38a3d8"));
                darkActive.setColor(QPalette::ToolTipBase, darkActive.base().color());
                darkActive.setColor(QPalette::ToolTipText, darkActive.text().color());
                darkActive.setColor(QPalette::Link, QColor("#38a3d8"));
                darkActive.setColor(QPalette::ButtonText, QColor(77, 77, 77));
                QApplication::setPalette(darkActive);
        } else {
                stylefile.setFileName(":/styles/styles/system.qss");
                QApplication::setPalette(original);
        }

        stylefile.open(QFile::ReadOnly);
        QString stylesheet = QString(stylefile.readAll());

        qobject_cast<QApplication *>(QApplication::instance())->setStyleSheet(stylesheet);
}

void
UserSettings::save()
{
        QSettings settings;
        settings.beginGroup("user");

        settings.beginGroup("window");
        settings.setValue("tray", isTrayEnabled_);
        settings.setValue("start_in_tray", isStartInTrayEnabled_);
        settings.endGroup();

        settings.beginGroup("timeline");
        settings.setValue("buttons", isButtonsInTimelineEnabled_);
        settings.endGroup();

        settings.setValue("avatar_circles", avatarCircles_);
        settings.setValue("decrypt_sidebar", decryptSidebar_);
        settings.setValue("font_size", baseFontSize_);
        settings.setValue("typing_notifications", isTypingNotificationsEnabled_);
        settings.setValue("minor_events", sortByImportance_);
        settings.setValue("read_receipts", isReadReceiptsEnabled_);
        settings.setValue("group_view", isGroupViewEnabled_);
        settings.setValue("markdown_enabled", isMarkdownEnabled_);
        settings.setValue("desktop_notifications", hasDesktopNotifications_);
        settings.setValue("theme", theme());
        settings.setValue("font_family", font_);
        settings.setValue("emoji_font_family", emojiFont_);

        settings.endGroup();
}

HorizontalLine::HorizontalLine(QWidget *parent)
  : QFrame{parent}
{
        setFrameShape(QFrame::HLine);
        setFrameShadow(QFrame::Sunken);
}

UserSettingsPage::UserSettingsPage(QSharedPointer<UserSettings> settings, QWidget *parent)
  : QWidget{parent}
  , settings_{settings}
{
        topLayout_ = new QVBoxLayout{this};

        QIcon icon;
        icon.addFile(":/icons/icons/ui/angle-pointing-to-left.png");

        auto backBtn_ = new FlatButton{this};
        backBtn_->setMinimumSize(QSize(24, 24));
        backBtn_->setIcon(icon);
        backBtn_->setIconSize(QSize(24, 24));

        QFont font;
        font.setPointSizeF(font.pointSizeF() * 1.1);

        auto versionInfo = new QLabel(QString("%1 | %2").arg(nheko::version).arg(nheko::build_os));
        versionInfo->setTextInteractionFlags(Qt::TextBrowserInteraction);

        topBarLayout_ = new QHBoxLayout;
        topBarLayout_->setSpacing(0);
        topBarLayout_->setMargin(0);
        topBarLayout_->addWidget(backBtn_, 1, Qt::AlignLeft | Qt::AlignVCenter);
        topBarLayout_->addStretch(1);

        formLayout_ = new QFormLayout;

        formLayout_->setLabelAlignment(Qt::AlignLeft);
        formLayout_->setFormAlignment(Qt::AlignRight);
        formLayout_->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        formLayout_->setRowWrapPolicy(QFormLayout::WrapLongRows);
        formLayout_->setHorizontalSpacing(0);

        auto general_ = new QLabel{tr("GENERAL"), this};
        general_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        general_->setFont(font);

        trayToggle_              = new Toggle{this};
        startInTrayToggle_       = new Toggle{this};
        avatarCircles_           = new Toggle{this};
        decryptSidebar_          = new Toggle(this);
        groupViewToggle_         = new Toggle{this};
        timelineButtonsToggle_   = new Toggle{this};
        typingNotifications_     = new Toggle{this};
        sortByImportance_        = new Toggle{this};
        readReceipts_            = new Toggle{this};
        markdownEnabled_         = new Toggle{this};
        desktopNotifications_    = new Toggle{this};
        scaleFactorCombo_        = new QComboBox{this};
        fontSizeCombo_           = new QComboBox{this};
        fontSelectionCombo_      = new QComboBox{this};
        emojiFontSelectionCombo_ = new QComboBox{this};

        if (!settings_->isTrayEnabled())
                startInTrayToggle_->setDisabled(true);

        avatarCircles_->setFixedSize(64, 48);

        auto uiLabel_ = new QLabel{tr("INTERFACE"), this};
        uiLabel_->setFixedHeight(uiLabel_->minimumHeight() + LayoutTopMargin);
        uiLabel_->setAlignment(Qt::AlignBottom);
        uiLabel_->setFont(font);

        for (double option = 1; option <= 3; option += 0.25)
                scaleFactorCombo_->addItem(QString::number(option));
        for (double option = 10; option < 17; option += 0.5)
                fontSizeCombo_->addItem(QString("%1 ").arg(QString::number(option)));

        QFontDatabase fontDb;
        auto fontFamilies = fontDb.families();
        for (const auto &family : fontFamilies) {
                fontSelectionCombo_->addItem(family);
        }

        // TODO: Is there a way to limit to just emojis, rather than
        // all emoji fonts?
        auto emojiFamilies = fontDb.families(QFontDatabase::Symbol);
        for (const auto &family : emojiFamilies) {
                emojiFontSelectionCombo_->addItem(family);
        }

        fontSelectionCombo_->setCurrentIndex(fontSelectionCombo_->findText(settings_->font()));

        emojiFontSelectionCombo_->setCurrentIndex(
          emojiFontSelectionCombo_->findText(settings_->emojiFont()));

        themeCombo_ = new QComboBox{this};
        themeCombo_->addItem("Light");
        themeCombo_->addItem("Dark");
        themeCombo_->addItem("System");

        QString themeStr = settings_->theme();
        themeStr.replace(0, 1, themeStr[0].toUpper());
        int themeIndex = themeCombo_->findText(themeStr);
        themeCombo_->setCurrentIndex(themeIndex);

        auto encryptionLabel_ = new QLabel{tr("ENCRYPTION"), this};
        encryptionLabel_->setFixedHeight(encryptionLabel_->minimumHeight() + LayoutTopMargin);
        encryptionLabel_->setAlignment(Qt::AlignBottom);
        encryptionLabel_->setFont(font);

        QFont monospaceFont;
        monospaceFont.setFamily("Monospace");
        monospaceFont.setStyleHint(QFont::Monospace);
        monospaceFont.setPointSizeF(monospaceFont.pointSizeF() * 0.9);

        deviceIdValue_ = new QLabel{this};
        deviceIdValue_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        deviceIdValue_->setFont(monospaceFont);

        deviceFingerprintValue_ = new QLabel{this};
        deviceFingerprintValue_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        deviceFingerprintValue_->setFont(monospaceFont);

        deviceFingerprintValue_->setText(utils::humanReadableFingerprint(QString(44, 'X')));

        auto sessionKeysLabel = new QLabel{tr("Session Keys"), this};
        sessionKeysLabel->setFont(font);
        sessionKeysLabel->setMargin(OptionMargin);

        auto sessionKeysImportBtn = new QPushButton{tr("IMPORT"), this};
        auto sessionKeysExportBtn = new QPushButton{tr("EXPORT"), this};

        auto sessionKeysLayout = new QHBoxLayout;
        sessionKeysLayout->addWidget(new QLabel{"", this}, 1, Qt::AlignRight);
        sessionKeysLayout->addWidget(sessionKeysExportBtn, 0, Qt::AlignRight);
        sessionKeysLayout->addWidget(sessionKeysImportBtn, 0, Qt::AlignRight);

        auto boxWrap = [this, &font](QString labelText, QWidget *field) {
                auto label = new QLabel{labelText, this};
                label->setFont(font);
                label->setMargin(OptionMargin);

                auto layout = new QHBoxLayout;
                layout->addWidget(field, 0, Qt::AlignRight);

                formLayout_->addRow(label, layout);
        };

        formLayout_->addRow(general_);
        formLayout_->addRow(new HorizontalLine{this});
        boxWrap(tr("Minimize to tray"), trayToggle_);
        boxWrap(tr("Start in tray"), startInTrayToggle_);
        formLayout_->addRow(new HorizontalLine{this});
        boxWrap(tr("Circular Avatars"), avatarCircles_);
        boxWrap(tr("Group's sidebar"), groupViewToggle_);
        boxWrap(tr("Decrypt messages in sidebar"), decryptSidebar_);
        boxWrap(tr("Show buttons in timeline"), timelineButtonsToggle_);
        boxWrap(tr("Typing notifications"), typingNotifications_);
        boxWrap(tr("Sort rooms by unreads"), sortByImportance_);
        formLayout_->addRow(new HorizontalLine{this});
        boxWrap(tr("Read receipts"), readReceipts_);
        boxWrap(tr("Send messages as Markdown"), markdownEnabled_);
        boxWrap(tr("Desktop notifications"), desktopNotifications_);
        formLayout_->addRow(uiLabel_);
        formLayout_->addRow(new HorizontalLine{this});

#if !defined(Q_OS_MAC)
        boxWrap(tr("Scale factor"), scaleFactorCombo_);
#else
        scaleFactorCombo_->hide();
#endif
        boxWrap(tr("Font size"), fontSizeCombo_);
        boxWrap(tr("Font Family"), fontSelectionCombo_);

#if !defined(Q_OS_MAC)
        boxWrap(tr("Emoji Font Family"), emojiFontSelectionCombo_);
#else
        emojiFontSelectionCombo_->hide();
#endif

        boxWrap(tr("Theme"), themeCombo_);
        formLayout_->addRow(encryptionLabel_);
        formLayout_->addRow(new HorizontalLine{this});
        boxWrap(tr("Device ID"), deviceIdValue_);
        boxWrap(tr("Device Fingerprint"), deviceFingerprintValue_);
        formLayout_->addRow(new HorizontalLine{this});
        formLayout_->addRow(sessionKeysLabel, sessionKeysLayout);

        auto scrollArea_ = new QScrollArea{this};
        scrollArea_->setFrameShape(QFrame::NoFrame);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setAlignment(Qt::AlignTop | Qt::AlignVCenter);

        QScroller::grabGesture(scrollArea_, QScroller::TouchGesture);

        auto spacingAroundForm = new QHBoxLayout;
        spacingAroundForm->addStretch(1);
        spacingAroundForm->addLayout(formLayout_, 0);
        spacingAroundForm->addStretch(1);

        auto scrollAreaContents_ = new QWidget{this};
        scrollAreaContents_->setObjectName("UserSettingScrollWidget");
        scrollAreaContents_->setLayout(spacingAroundForm);

        scrollArea_->setWidget(scrollAreaContents_);
        topLayout_->addLayout(topBarLayout_);
        topLayout_->addWidget(scrollArea_, Qt::AlignTop);
        topLayout_->addStretch(1);
        topLayout_->addWidget(versionInfo);

        connect(themeCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [this](const QString &text) {
                        settings_->setTheme(text.toLower());
                        emit themeChanged();
                });
        connect(scaleFactorCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [](const QString &factor) { utils::setScaleFactor(factor.toFloat()); });
        connect(fontSizeCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [this](const QString &size) { settings_->setFontSize(size.trimmed().toDouble()); });
        connect(fontSelectionCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [this](const QString &family) { settings_->setFontFamily(family.trimmed()); });
        connect(emojiFontSelectionCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [this](const QString &family) { settings_->setEmojiFontFamily(family.trimmed()); });
        connect(trayToggle_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setTray(!isDisabled);
                if (isDisabled) {
                        startInTrayToggle_->setDisabled(true);
                } else {
                        startInTrayToggle_->setEnabled(true);
                }
                emit trayOptionChanged(!isDisabled);
        });

        connect(startInTrayToggle_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setStartInTray(!isDisabled);
        });

        connect(groupViewToggle_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setGroupView(!isDisabled);
        });

        connect(decryptSidebar_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setDecryptSidebar(!isDisabled);
                emit decryptSidebarChanged();
        });

        connect(avatarCircles_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setAvatarCircles(!isDisabled);
        });

        connect(markdownEnabled_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setMarkdownEnabled(!isDisabled);
        });

        connect(typingNotifications_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setTypingNotifications(!isDisabled);
        });

        connect(sortByImportance_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setSortByImportance(!isDisabled);
        });

        connect(timelineButtonsToggle_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setButtonsInTimeline(!isDisabled);
        });

        connect(readReceipts_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setReadReceipts(!isDisabled);
        });

        connect(desktopNotifications_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setDesktopNotifications(!isDisabled);
        });

        connect(
          sessionKeysImportBtn, &QPushButton::clicked, this, &UserSettingsPage::importSessionKeys);

        connect(
          sessionKeysExportBtn, &QPushButton::clicked, this, &UserSettingsPage::exportSessionKeys);

        connect(backBtn_, &QPushButton::clicked, this, [this]() {
                settings_->save();
                emit moveBack();
        });
}

void
UserSettingsPage::showEvent(QShowEvent *)
{
        // FIXME macOS doesn't show the full option unless a space is added.
        utils::restoreCombobox(fontSizeCombo_, QString::number(settings_->fontSize()) + " ");
        utils::restoreCombobox(scaleFactorCombo_, QString::number(utils::scaleFactor()));
        utils::restoreCombobox(themeCombo_, settings_->theme());

        // FIXME: Toggle treats true as "off"
        trayToggle_->setState(!settings_->isTrayEnabled());
        startInTrayToggle_->setState(!settings_->isStartInTrayEnabled());
        groupViewToggle_->setState(!settings_->isGroupViewEnabled());
        decryptSidebar_->setState(!settings_->isDecryptSidebarEnabled());
        avatarCircles_->setState(!settings_->isAvatarCirclesEnabled());
        typingNotifications_->setState(!settings_->isTypingNotificationsEnabled());
        sortByImportance_->setState(!settings_->isSortByImportanceEnabled());
        timelineButtonsToggle_->setState(!settings_->isButtonsInTimelineEnabled());
        readReceipts_->setState(!settings_->isReadReceiptsEnabled());
        markdownEnabled_->setState(!settings_->isMarkdownEnabled());
        desktopNotifications_->setState(!settings_->hasDesktopNotifications());
        deviceIdValue_->setText(QString::fromStdString(http::client()->device_id()));

        deviceFingerprintValue_->setText(
          utils::humanReadableFingerprint(olm::client()->identity_keys().ed25519));
}

void
UserSettingsPage::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
UserSettingsPage::importSessionKeys()
{
        const QString homeFolder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        const QString fileName =
          QFileDialog::getOpenFileName(this, tr("Open Sessions File"), homeFolder, "");

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, tr("Error"), file.errorString());
                return;
        }

        auto bin     = file.peek(file.size());
        auto payload = std::string(bin.data(), bin.size());

        bool ok;
        auto password = QInputDialog::getText(this,
                                              tr("File Password"),
                                              tr("Enter the passphrase to decrypt the file:"),
                                              QLineEdit::Password,
                                              "",
                                              &ok);
        if (!ok)
                return;

        if (password.isEmpty()) {
                QMessageBox::warning(this, tr("Error"), tr("The password cannot be empty"));
                return;
        }

        try {
                auto sessions =
                  mtx::crypto::decrypt_exported_sessions(payload, password.toStdString());
                cache::importSessionKeys(std::move(sessions));
        } catch (const mtx::crypto::sodium_exception &e) {
                QMessageBox::warning(this, tr("Error"), e.what());
        } catch (const lmdb::error &e) {
                QMessageBox::warning(this, tr("Error"), e.what());
        } catch (const nlohmann::json::exception &e) {
                QMessageBox::warning(this, tr("Error"), e.what());
        }
}

void
UserSettingsPage::exportSessionKeys()
{
        // Open password dialog.
        bool ok;
        auto password = QInputDialog::getText(this,
                                              tr("File Password"),
                                              tr("Enter passphrase to encrypt your session keys:"),
                                              QLineEdit::Password,
                                              "",
                                              &ok);
        if (!ok)
                return;

        if (password.isEmpty()) {
                QMessageBox::warning(this, tr("Error"), tr("The password cannot be empty"));
                return;
        }

        // Open file dialog to save the file.
        const QString homeFolder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        const QString fileName =
          QFileDialog::getSaveFileName(this, tr("File to save the exported session keys"), "", "");

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QMessageBox::warning(this, tr("Error"), file.errorString());
                return;
        }

        // Export sessions & save to file.
        try {
                auto encrypted_blob = mtx::crypto::encrypt_exported_sessions(
                  cache::exportSessionKeys(), password.toStdString());

                QString b64 = QString::fromStdString(mtx::crypto::bin2base64(encrypted_blob));

                QString prefix("-----BEGIN MEGOLM SESSION DATA-----");
                QString suffix("-----END MEGOLM SESSION DATA-----");
                QString newline("\n");
                QTextStream out(&file);
                out << prefix << newline << b64 << newline << suffix;
                file.close();
        } catch (const mtx::crypto::sodium_exception &e) {
                QMessageBox::warning(this, tr("Error"), e.what());
        } catch (const lmdb::error &e) {
                QMessageBox::warning(this, tr("Error"), e.what());
        } catch (const nlohmann::json::exception &e) {
                QMessageBox::warning(this, tr("Error"), e.what());
        }
}
