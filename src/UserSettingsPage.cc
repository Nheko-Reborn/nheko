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
        isTrayEnabled_     = settings.value("user/window/tray", true).toBool();
        isOrderingEnabled_ = settings.value("user/room_ordering", true).toBool();
        theme_             = settings.value("user/theme", "light").toString();

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
        settings.endGroup();

        settings.setValue("room_ordering", isOrderingEnabled_);
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
        heading_->setStyleSheet("font-weight: bold; font-size: 22px;");

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
        trayLabel->setStyleSheet("font-size: 15px;");

        trayOptionLayout_->addWidget(trayLabel);
        trayOptionLayout_->addWidget(trayToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto orderRoomLayout = new QHBoxLayout;
        orderRoomLayout->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto orderLabel  = new QLabel(tr("Re-order rooms based on activity"), this);
        roomOrderToggle_ = new Toggle(this);
        roomOrderToggle_->setActiveColor(QColor("#38A3D8"));
        roomOrderToggle_->setInactiveColor(QColor("gray"));
        orderLabel->setStyleSheet("font-size: 15px;");

        orderRoomLayout->addWidget(orderLabel);
        orderRoomLayout->addWidget(roomOrderToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto themeOptionLayout_ = new QHBoxLayout;
        themeOptionLayout_->setContentsMargins(0, OptionMargin, 0, OptionMargin);
        auto themeLabel_ = new QLabel(tr("App theme"), this);
        themeCombo_      = new QComboBox(this);
        themeCombo_->addItem("Light");
        themeCombo_->addItem("Dark");
        themeCombo_->addItem("System");
        themeLabel_->setStyleSheet("font-size: 15px;");

        themeOptionLayout_->addWidget(themeLabel_);
        themeOptionLayout_->addWidget(themeCombo_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto general_ = new QLabel(tr("GENERAL"), this);
        general_->setStyleSheet("font-size: 17px");

        mainLayout_ = new QVBoxLayout;
        mainLayout_->setSpacing(7);
        mainLayout_->setContentsMargins(
          sideMargin_, LayoutTopMargin, sideMargin_, LayoutBottomMargin);
        mainLayout_->addWidget(general_, 1, Qt::AlignLeft | Qt::AlignVCenter);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(trayOptionLayout_);
        mainLayout_->addWidget(new HorizontalLine(this));
        mainLayout_->addLayout(orderRoomLayout);
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

        connect(roomOrderToggle_, &Toggle::toggled, this, [=](bool isDisabled) {
                settings_->setRoomOrdering(!isDisabled);
        });

        connect(backBtn_, &QPushButton::clicked, this, [=]() {
                settings_->save();
                emit moveBack();
        });
}

void
UserSettingsPage::showEvent(QShowEvent *)
{
        restoreThemeCombo();
        trayToggle_->setState(!settings_->isTrayEnabled());          // Treats true as "off"
        roomOrderToggle_->setState(!settings_->isOrderingEnabled()); // Treats true as "off"
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
UserSettingsPage::restoreThemeCombo() const
{
        if (settings_->theme() == "light")
                themeCombo_->setCurrentIndex(0);
        else if (settings_->theme() == "dark")
                themeCombo_->setCurrentIndex(1);
        else
                themeCombo_->setCurrentIndex(2);
}
