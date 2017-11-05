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

#include <QComboBox>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QSettings>

#include "Config.h"
#include "FlatButton.h"
#include "UserSettingsPage.h"
#include <ToggleButton.h>

UserSettings::UserSettings() { load(); }

void
UserSettings::load()
{
        QSettings settings;
        isTrayEnabled_ = settings.value("user/window/tray", true).toBool();
        theme_         = settings.value("user/theme", "default").toString();
}

void
UserSettings::save()
{
        QSettings settings;
        settings.beginGroup("user");

        settings.beginGroup("window");
        settings.setValue("tray", isTrayEnabled_);
        settings.endGroup();

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

        auto heading_ = new QLabel(tr("User Settings"));
        heading_->setFont(QFont("Open Sans Bold", 22));

        topBarLayout_ = new QHBoxLayout;
        topBarLayout_->setSpacing(0);
        topBarLayout_->setMargin(0);
        topBarLayout_->addWidget(backBtn_, 1, Qt::AlignLeft | Qt::AlignVCenter);
        topBarLayout_->addWidget(heading_, 0, Qt::AlignBottom);
        topBarLayout_->addStretch(1);

        auto trayOptionLayout_ = new QHBoxLayout;
        trayOptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto trayLabel = new QLabel(tr("Minimize to tray"), this);
        trayToggle_    = new Toggle(this);
        trayToggle_->setActiveColor(QColor("#38A3D8"));
        trayToggle_->setInactiveColor(QColor("gray"));
        trayLabel->setFont(QFont("Open Sans", 15));

        trayOptionLayout_->addWidget(trayLabel);
        trayOptionLayout_->addWidget(trayToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto themeOptionLayout_ = new QHBoxLayout;
        themeOptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto themeLabel_ = new QLabel(tr("App theme"), this);
        themeCombo_      = new QComboBox(this);
        themeCombo_->addItem("Default");
        themeCombo_->addItem("System");
        themeLabel_->setFont(QFont("Open Sans", 15));

        themeOptionLayout_->addWidget(themeLabel_);
        themeOptionLayout_->addWidget(themeCombo_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto general_ = new QLabel(tr("GENERAL"), this);
        general_->setFont(QFont("Open Sans Bold", 17));
        general_->setStyleSheet("color: #5d6565");

        auto mainLayout_ = new QVBoxLayout;
        mainLayout_->setSpacing(7);
        mainLayout_->setContentsMargins(
          LayoutSideMargin, LayoutSideMargin / 6, LayoutSideMargin, LayoutSideMargin / 6);
        mainLayout_->addWidget(general_, 1, Qt::AlignLeft | Qt::AlignVCenter);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(trayOptionLayout_);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(themeOptionLayout_);
        mainLayout_->addWidget(new HorizontalLine(this));

        topLayout_->addLayout(topBarLayout_);
        topLayout_->addLayout(mainLayout_);
        topLayout_->addStretch(1);

        connect(themeCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [=](const QString &text) { settings_->setTheme(text.toLower()); });

        connect(trayToggle_, &Toggle::toggled, this, [=](bool isDisabled) {
                settings_->setTray(!isDisabled);
                emit trayOptionChanged(!isDisabled);
        });

        connect(backBtn_, &QPushButton::clicked, this, [=]() {
                settings_->save();
                emit moveBack();
        });
}

void
UserSettingsPage::showEvent(QShowEvent *)
{
        themeCombo_->setCurrentIndex((settings_->theme() == "default" ? 0 : 1));
        trayToggle_->setState(!settings_->isTrayEnabled()); // Treats true as "off"
}
