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
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>

#include "Config.h"
#include "MatrixClient.h"
#include "Olm.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "ui/FlatButton.h"
#include "ui/ToggleButton.h"

#include "version.h"

UserSettings::UserSettings() { load(); }

void
UserSettings::load()
{
        QSettings settings;
        isTrayEnabled_                = settings.value("user/window/tray", true).toBool();
        hasDesktopNotifications_      = settings.value("user/desktop_notifications", true).toBool();
        isStartInTrayEnabled_         = settings.value("user/window/start_in_tray", false).toBool();
        isOrderingEnabled_            = settings.value("user/room_ordering", true).toBool();
        isGroupViewEnabled_           = settings.value("user/group_view", true).toBool();
        isTypingNotificationsEnabled_ = settings.value("user/typing_notifications", true).toBool();
        isReadReceiptsEnabled_        = settings.value("user/read_receipts", true).toBool();
        theme_                        = settings.value("user/theme", "light").toString();

        applyTheme();
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
        QPalette pal;

        if (theme() == "light") {
                stylefile.setFileName(":/styles/styles/nheko.qss");
                pal.setColor(QPalette::Link, QColor("#333"));
        } else if (theme() == "dark") {
                stylefile.setFileName(":/styles/styles/nheko-dark.qss");
                pal.setColor(QPalette::Link, QColor("#d7d9dc"));
        } else {
                stylefile.setFileName(":/styles/styles/system.qss");
        }

        stylefile.open(QFile::ReadOnly);
        QString stylesheet = QString(stylefile.readAll());

        QApplication::setPalette(pal);
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

        settings.setValue("room_ordering", isOrderingEnabled_);
        settings.setValue("typing_notifications", isTypingNotificationsEnabled_);
        settings.setValue("read_receipts", isReadReceiptsEnabled_);
        settings.setValue("group_view", isGroupViewEnabled_);
        settings.setValue("desktop_notifications", hasDesktopNotifications_);
        settings.setValue("theme", theme());
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
        trayOptionLayout_->addWidget(trayToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto startInTrayOptionLayout_ = new QHBoxLayout;
        startInTrayOptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto startInTrayLabel = new QLabel(tr("Start in tray"), this);
        startInTrayLabel->setFont(font);
        startInTrayToggle_ = new Toggle(this);
        if (!settings_->isTrayEnabled())
                startInTrayToggle_->setDisabled(true);

        startInTrayOptionLayout_->addWidget(startInTrayLabel);
        startInTrayOptionLayout_->addWidget(
          startInTrayToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto orderRoomLayout = new QHBoxLayout;
        orderRoomLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto orderLabel = new QLabel(tr("Re-order rooms based on activity"), this);
        orderLabel->setFont(font);
        roomOrderToggle_ = new Toggle(this);

        orderRoomLayout->addWidget(orderLabel);
        orderRoomLayout->addWidget(roomOrderToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto groupViewLayout = new QHBoxLayout;
        groupViewLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto groupViewLabel = new QLabel(tr("Group's sidebar"), this);
        groupViewLabel->setFont(font);
        groupViewToggle_ = new Toggle(this);

        groupViewLayout->addWidget(groupViewLabel);
        groupViewLayout->addWidget(groupViewToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto typingLayout = new QHBoxLayout;
        typingLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto typingLabel = new QLabel(tr("Typing notifications"), this);
        typingLabel->setFont(font);
        typingNotifications_ = new Toggle(this);

        typingLayout->addWidget(typingLabel);
        typingLayout->addWidget(typingNotifications_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto receiptsLayout = new QHBoxLayout;
        receiptsLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto receiptsLabel = new QLabel(tr("Read receipts"), this);
        receiptsLabel->setFont(font);
        readReceipts_ = new Toggle(this);

        receiptsLayout->addWidget(receiptsLabel);
        receiptsLayout->addWidget(readReceipts_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto desktopLayout = new QHBoxLayout;
        desktopLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto desktopLabel = new QLabel(tr("Desktop notifications"), this);
        desktopLabel->setFont(font);
        desktopNotifications_ = new Toggle(this);

        desktopLayout->addWidget(desktopLabel);
        desktopLayout->addWidget(desktopNotifications_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto scaleFactorOptionLayout = new QHBoxLayout;
        scaleFactorOptionLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto scaleFactorLabel = new QLabel(tr("Scale factor (requires restart)"), this);
        scaleFactorLabel->setFont(font);
        scaleFactorCombo_ = new QComboBox(this);
        scaleFactorCombo_->addItem("1");
        scaleFactorCombo_->addItem("1.25");
        scaleFactorCombo_->addItem("1.5");
        scaleFactorCombo_->addItem("1.75");
        scaleFactorCombo_->addItem("2");
        scaleFactorCombo_->addItem("2.25");
        scaleFactorCombo_->addItem("2.5");
        scaleFactorCombo_->addItem("2.75");
        scaleFactorCombo_->addItem("3");

        scaleFactorOptionLayout->addWidget(scaleFactorLabel);
        scaleFactorOptionLayout->addWidget(scaleFactorCombo_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto themeOptionLayout_ = new QHBoxLayout;
        themeOptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto themeLabel_ = new QLabel(tr("Theme"), this);
        themeLabel_->setFont(font);
        themeCombo_ = new QComboBox(this);
        themeCombo_->addItem("Light");
        themeCombo_->addItem("Dark");
        themeCombo_->addItem("System");

        themeOptionLayout_->addWidget(themeLabel_);
        themeOptionLayout_->addWidget(themeCombo_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto encryptionLayout_ = new QVBoxLayout;
        encryptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);

        QFont monospaceFont = QFont(font);
        monospaceFont.setFamily("Courier New");
        monospaceFont.setStyleHint(QFont::Courier);
        monospaceFont.setPointSizeF(monospaceFont.pointSizeF() * 0.9);

        auto deviceIdWidget = new QHBoxLayout;
        deviceIdWidget->setContentsMargins(0, OptionMargin, 0, OptionMargin);

        auto deviceIdLabel = new QLabel(tr("Device ID"), this);
        deviceIdLabel->setFont(font);
        deviceIdValue_ = new QLabel();
        deviceIdValue_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        deviceIdValue_->setFont(monospaceFont);
        deviceIdWidget->addWidget(deviceIdLabel, 1);
        deviceIdWidget->addWidget(deviceIdValue_);

        auto deviceFingerprintWidget = new QHBoxLayout;
        deviceFingerprintWidget->setContentsMargins(0, OptionMargin, 0, OptionMargin);

        auto deviceFingerprintLabel = new QLabel(tr("Device Fingerprint"), this);
        deviceFingerprintLabel->setFont(font);
        deviceFingerprintValue_ = new QLabel();
        deviceFingerprintValue_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        deviceFingerprintValue_->setFont(monospaceFont);
        deviceFingerprintWidget->addWidget(deviceFingerprintLabel, 1);
        deviceFingerprintWidget->addWidget(deviceFingerprintValue_);

        encryptionLayout_->addLayout(deviceIdWidget);
        encryptionLayout_->addLayout(deviceFingerprintWidget);

        font.setWeight(65);

        auto encryptionLabel_ = new QLabel(tr("ENCRYPTION"), this);
        encryptionLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        encryptionLabel_->setFont(font);
        // encryptionLabel_->setContentsMargins(0, 50, 0, 0);

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
        mainLayout_->addLayout(orderRoomLayout);
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
#else
        mainLayout_->addLayout(scaleFactorOptionLayout);
        mainLayout_->addWidget(new HorizontalLine(this));
#endif

        mainLayout_->addLayout(themeOptionLayout_);
        mainLayout_->addWidget(new HorizontalLine(this));

        mainLayout_->addSpacing(50);

        mainLayout_->addWidget(encryptionLabel_, 1, Qt::AlignLeft | Qt::AlignBottom);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(encryptionLayout_);
        mainLayout_->addStretch(1);

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
                [this](const QString &text) { settings_->setTheme(text.toLower()); });
        connect(scaleFactorCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [](const QString &factor) { utils::setScaleFactor(factor.toFloat()); });

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

        connect(roomOrderToggle_, &Toggle::toggled, this, [this](bool isDisabled) {
                settings_->setRoomOrdering(!isDisabled);
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
        restoreThemeCombo();
        restoreScaleFactor();

        // FIXME: Toggle treats true as "off"
        trayToggle_->setState(!settings_->isTrayEnabled());
        startInTrayToggle_->setState(!settings_->isStartInTrayEnabled());
        roomOrderToggle_->setState(!settings_->isOrderingEnabled());
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
UserSettingsPage::restoreScaleFactor() const
{
        auto factor = utils::scaleFactor();

        if (factor == 1)
                scaleFactorCombo_->setCurrentIndex(0);
        else if (factor == 1.25)
                scaleFactorCombo_->setCurrentIndex(1);
        else if (factor == 1.5)
                scaleFactorCombo_->setCurrentIndex(2);
        else if (factor == 1.75)
                scaleFactorCombo_->setCurrentIndex(3);
        else if (factor == 2)
                scaleFactorCombo_->setCurrentIndex(4);
        else if (factor == 2.25)
                scaleFactorCombo_->setCurrentIndex(5);
        else if (factor == 2.5)
                scaleFactorCombo_->setCurrentIndex(6);
        else if (factor == 2.75)
                scaleFactorCombo_->setCurrentIndex(7);
        else if (factor == 3)
                scaleFactorCombo_->setCurrentIndex(7);
        else
                scaleFactorCombo_->setCurrentIndex(0);
}

void
UserSettingsPage::restoreThemeCombo() const
{
        if (settings_->theme() == "light")
                themeCombo_->setCurrentIndex(0);
        else if (settings_->theme() == "dark")
                themeCombo_->setCurrentIndex(1);
        else
                themeCombo_->setCurrentIndex(2);
}
