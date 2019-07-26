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
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QTextStream>

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
        isTypingNotificationsEnabled_ = settings.value("user/typing_notifications", true).toBool();
        isReadReceiptsEnabled_        = settings.value("user/read_receipts", true).toBool();
        theme_                        = settings.value("user/theme", "light").toString();
        font_                         = settings.value("user/font_family", "default").toString();
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

        if (theme() == "light") {
                stylefile.setFileName(":/styles/styles/nheko.qss");
        } else if (theme() == "dark") {
                stylefile.setFileName(":/styles/styles/nheko-dark.qss");
        } else {
                stylefile.setFileName(":/styles/styles/system.qss");
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

        settings.setValue("font_size", baseFontSize_);
        settings.setValue("typing_notifications", isTypingNotificationsEnabled_);
        settings.setValue("read_receipts", isReadReceiptsEnabled_);
        settings.setValue("group_view", isGroupViewEnabled_);
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
        topLayout_ = new QVBoxLayout(this);

        QIcon icon;
        icon.addFile(":/icons/icons/ui/angle-pointing-to-left.png");

        auto backBtn_ = new FlatButton(this);
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

        auto trayOptionLayout_ = new QHBoxLayout;
        trayOptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto trayLabel = new QLabel(tr("Minimize to tray"), this);
        trayLabel->setFont(font);
        trayToggle_ = new Toggle(this);

        trayOptionLayout_->addWidget(trayLabel);
        trayOptionLayout_->addWidget(trayToggle_, 0, Qt::AlignRight);

        auto startInTrayOptionLayout_ = new QHBoxLayout;
        startInTrayOptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto startInTrayLabel = new QLabel(tr("Start in tray"), this);
        startInTrayLabel->setFont(font);
        startInTrayToggle_ = new Toggle(this);
        if (!settings_->isTrayEnabled())
                startInTrayToggle_->setDisabled(true);

        startInTrayOptionLayout_->addWidget(startInTrayLabel);
        startInTrayOptionLayout_->addWidget(startInTrayToggle_, 0, Qt::AlignRight);

        auto groupViewLayout = new QHBoxLayout;
        groupViewLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto groupViewLabel = new QLabel(tr("Group's sidebar"), this);
        groupViewLabel->setFont(font);
        groupViewToggle_ = new Toggle(this);

        groupViewLayout->addWidget(groupViewLabel);
        groupViewLayout->addWidget(groupViewToggle_, 0, Qt::AlignRight);

        auto typingLayout = new QHBoxLayout;
        typingLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto typingLabel = new QLabel(tr("Typing notifications"), this);
        typingLabel->setFont(font);
        typingNotifications_ = new Toggle(this);

        typingLayout->addWidget(typingLabel);
        typingLayout->addWidget(typingNotifications_, 0, Qt::AlignRight);

        auto receiptsLayout = new QHBoxLayout;
        receiptsLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto receiptsLabel = new QLabel(tr("Read receipts"), this);
        receiptsLabel->setFont(font);
        readReceipts_ = new Toggle(this);

        receiptsLayout->addWidget(receiptsLabel);
        receiptsLayout->addWidget(readReceipts_, 0, Qt::AlignRight);

        auto desktopLayout = new QHBoxLayout;
        desktopLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto desktopLabel = new QLabel(tr("Desktop notifications"), this);
        desktopLabel->setFont(font);
        desktopNotifications_ = new Toggle(this);

        desktopLayout->addWidget(desktopLabel);
        desktopLayout->addWidget(desktopNotifications_, 0, Qt::AlignRight);

        auto scaleFactorOptionLayout = new QHBoxLayout;
        scaleFactorOptionLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto scaleFactorLabel = new QLabel(tr("Scale factor"), this);
        scaleFactorLabel->setFont(font);
        scaleFactorCombo_ = new QComboBox(this);
        for (double option = 1; option <= 3; option += 0.25)
                scaleFactorCombo_->addItem(QString::number(option));

        scaleFactorOptionLayout->addWidget(scaleFactorLabel);
        scaleFactorOptionLayout->addWidget(scaleFactorCombo_, 0, Qt::AlignRight);

        auto fontSizeOptionLayout = new QHBoxLayout;
        fontSizeOptionLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto fontSizeLabel = new QLabel(tr("Font size"), this);
        fontSizeLabel->setFont(font);
        fontSizeCombo_ = new QComboBox(this);
        for (double option = 10; option < 17; option += 0.5)
                fontSizeCombo_->addItem(QString("%1 ").arg(QString::number(option)));

        fontSizeOptionLayout->addWidget(fontSizeLabel);
        fontSizeOptionLayout->addWidget(fontSizeCombo_, 0, Qt::AlignRight);

        auto fontFamilyOptionLayout      = new QHBoxLayout;
        auto emojiFontFamilyOptionLayout = new QHBoxLayout;
        fontFamilyOptionLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        emojiFontFamilyOptionLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto fontFamilyLabel  = new QLabel(tr("Font Family"), this);
        auto emojiFamilyLabel = new QLabel(tr("Emoji Font Famly"), this);
        fontFamilyLabel->setFont(font);
        emojiFamilyLabel->setFont(font);
        fontSelectionCombo_      = new QComboBox(this);
        emojiFontSelectionCombo_ = new QComboBox(this);
        QFontDatabase fontDb;
        auto fontFamilies = fontDb.families();
        // TODO: Is there a way to limit to just emojis, rather than
        // all emoji fonts?
        auto emojiFamilies = fontDb.families(QFontDatabase::Symbol);

        for (const auto &family : fontFamilies) {
                fontSelectionCombo_->addItem(family);
        }

        for (const auto &family : emojiFamilies) {
                emojiFontSelectionCombo_->addItem(family);
        }

        int fontIndex = fontSelectionCombo_->findText(settings_->font());
        fontSelectionCombo_->setCurrentIndex(fontIndex);

        fontIndex = emojiFontSelectionCombo_->findText(settings_->emojiFont());
        emojiFontSelectionCombo_->setCurrentIndex(fontIndex);

        fontFamilyOptionLayout->addWidget(fontFamilyLabel);
        fontFamilyOptionLayout->addWidget(fontSelectionCombo_, 0, Qt::AlignRight);

        emojiFontFamilyOptionLayout->addWidget(emojiFamilyLabel);
        emojiFontFamilyOptionLayout->addWidget(emojiFontSelectionCombo_, 0, Qt::AlignRight);

        auto themeOptionLayout_ = new QHBoxLayout;
        themeOptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto themeLabel_ = new QLabel(tr("Theme"), this);
        themeLabel_->setFont(font);
        themeCombo_ = new QComboBox(this);
        themeCombo_->addItem("Light");
        themeCombo_->addItem("Dark");
        themeCombo_->addItem("System");

        QString themeStr = settings_->theme();
        themeStr.replace(0, 1, themeStr[0].toUpper());
        int themeIndex = themeCombo_->findText(themeStr);
        themeCombo_->setCurrentIndex(themeIndex);

        themeOptionLayout_->addWidget(themeLabel_);
        themeOptionLayout_->addWidget(themeCombo_, 0, Qt::AlignRight);

        auto encryptionLayout_ = new QVBoxLayout;
        encryptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        encryptionLayout_->setAlignment(Qt::AlignVCenter);

        QFont monospaceFont;
        monospaceFont.setFamily("Monospace");
        monospaceFont.setStyleHint(QFont::Monospace);
        monospaceFont.setPointSizeF(monospaceFont.pointSizeF() * 0.9);

        auto deviceIdLayout = new QHBoxLayout;
        deviceIdLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);

        auto deviceIdLabel = new QLabel(tr("Device ID"), this);
        deviceIdLabel->setFont(font);
        deviceIdLabel->setMargin(0);
        deviceIdValue_ = new QLabel{this};
        deviceIdValue_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        deviceIdValue_->setFont(monospaceFont);
        deviceIdLayout->addWidget(deviceIdLabel, 1);
        deviceIdLayout->addWidget(deviceIdValue_);

        auto deviceFingerprintLayout = new QHBoxLayout;
        deviceFingerprintLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);

        auto deviceFingerprintLabel = new QLabel(tr("Device Fingerprint"), this);
        deviceFingerprintLabel->setFont(font);
        deviceFingerprintLabel->setMargin(0);
        deviceFingerprintValue_ = new QLabel{this};
        deviceFingerprintValue_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        deviceFingerprintValue_->setFont(monospaceFont);
        deviceFingerprintLayout->addWidget(deviceFingerprintLabel, 1);
        deviceFingerprintLayout->addWidget(deviceFingerprintValue_);

        auto sessionKeysLayout = new QHBoxLayout;
        sessionKeysLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto sessionKeysLabel = new QLabel(tr("Session Keys"), this);
        sessionKeysLabel->setFont(font);
        sessionKeysLayout->addWidget(sessionKeysLabel, 1);

        auto sessionKeysImportBtn = new QPushButton{tr("IMPORT"), this};
        connect(
          sessionKeysImportBtn, &QPushButton::clicked, this, &UserSettingsPage::importSessionKeys);
        auto sessionKeysExportBtn = new QPushButton{tr("EXPORT"), this};
        connect(
          sessionKeysExportBtn, &QPushButton::clicked, this, &UserSettingsPage::exportSessionKeys);
        sessionKeysLayout->addWidget(sessionKeysExportBtn, 0, Qt::AlignRight);
        sessionKeysLayout->addWidget(sessionKeysImportBtn, 0, Qt::AlignRight);

        encryptionLayout_->addLayout(deviceIdLayout);
        encryptionLayout_->addLayout(deviceFingerprintLayout);
        encryptionLayout_->addWidget(new HorizontalLine{this});
        encryptionLayout_->addLayout(sessionKeysLayout);

        font.setWeight(QFont::Medium);

        auto encryptionLabel_ = new QLabel(tr("ENCRYPTION"), this);
        encryptionLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        encryptionLabel_->setFont(font);

        auto general_ = new QLabel(tr("GENERAL"), this);
        general_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        general_->setFont(font);

        mainLayout_ = new QVBoxLayout;
        mainLayout_->setAlignment(Qt::AlignTop);
        mainLayout_->setSpacing(7);
        mainLayout_->setContentsMargins(
          sideMargin_, LayoutTopMargin, sideMargin_, LayoutBottomMargin);
        mainLayout_->addWidget(general_, 1, Qt::AlignLeft | Qt::AlignBottom);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(trayOptionLayout_);
        mainLayout_->addLayout(startInTrayOptionLayout_);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(groupViewLayout);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(typingLayout);
        mainLayout_->addLayout(receiptsLayout);
        mainLayout_->addLayout(desktopLayout);
        mainLayout_->addWidget(new HorizontalLine(this));

#if defined(Q_OS_MAC)
        scaleFactorLabel->hide();
        scaleFactorCombo_->hide();
        emojiFamilyLabel->hide();
        emojiFontSelectionCombo_->hide();
#endif

        mainLayout_->addLayout(scaleFactorOptionLayout);
        mainLayout_->addLayout(fontSizeOptionLayout);
        mainLayout_->addLayout(fontFamilyOptionLayout);
        mainLayout_->addLayout(emojiFontFamilyOptionLayout);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(themeOptionLayout_);
        mainLayout_->addWidget(new HorizontalLine(this));

        mainLayout_->addSpacing(50);

        mainLayout_->addWidget(encryptionLabel_, 1, Qt::AlignLeft | Qt::AlignBottom);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(encryptionLayout_);

        auto scrollArea_ = new QScrollArea(this);
        scrollArea_->setFrameShape(QFrame::NoFrame);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setAlignment(Qt::AlignTop | Qt::AlignVCenter);

        auto scrollAreaContents_ = new QWidget(this);
        scrollAreaContents_->setObjectName("UserSettingScrollWidget");
        scrollAreaContents_->setLayout(mainLayout_);

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

        connect(typingNotifications_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setTypingNotifications(!isDisabled);
        });

        connect(readReceipts_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setReadReceipts(!isDisabled);
        });

        connect(desktopNotifications_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setDesktopNotifications(!isDisabled);
        });

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
        typingNotifications_->setState(!settings_->isTypingNotificationsEnabled());
        readReceipts_->setState(!settings_->isReadReceiptsEnabled());
        desktopNotifications_->setState(!settings_->hasDesktopNotifications());
        deviceIdValue_->setText(QString::fromStdString(http::client()->device_id()));

        deviceFingerprintValue_->setText(
          utils::humanReadableFingerprint(olm::client()->identity_keys().ed25519));
}

void
UserSettingsPage::resizeEvent(QResizeEvent *event)
{
        sideMargin_ = width() * 0.2;
        mainLayout_->setContentsMargins(
          sideMargin_, LayoutTopMargin, sideMargin_, LayoutBottomMargin);

        QWidget::resizeEvent(event);
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
        auto fileName = QFileDialog::getOpenFileName(this, tr("Open Sessions File"), "", "");

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
                cache::client()->importSessionKeys(std::move(sessions));
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
        auto fileName =
          QFileDialog::getSaveFileName(this, tr("File to save the exported session keys"), "", "");

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QMessageBox::warning(this, tr("Error"), file.errorString());
                return;
        }

        // Export sessions & save to file.
        try {
                auto encrypted_blob = mtx::crypto::encrypt_exported_sessions(
                  cache::client()->exportSessionKeys(), password.toStdString());

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
