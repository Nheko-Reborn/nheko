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

#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

#include "DropShadow.h"
#include "EmojiCategory.h"
#include "EmojiPanel.h"
#include "FlatButton.h"

EmojiPanel::EmojiPanel(QWidget *parent)
  : QWidget(parent)
  , shadowMargin_{ 2 }
  , width_{ 370 }
  , height_{ 350 }
  , animationDuration_{ 100 }
  , categoryIconSize_{ 20 }
{
        setStyleSheet("QWidget {background: #fff; color: #e8e8e8; border: none;}"
                      "QScrollBar:vertical { background-color: #fff; width: 8px; margin: 0px "
                      "2px 0 2px; }"
                      "QScrollBar::handle:vertical { background-color: #d6dde3; min-height: "
                      "20px; }"
                      "QScrollBar::add-line:vertical { border: none; background: none; }"
                      "QScrollBar::sub-line:vertical { border: none; background: none; }");

        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);

        auto mainWidget = new QWidget(this);
        mainWidget->setMaximumSize(width_, height_);

        auto topLayout = new QVBoxLayout(this);
        topLayout->addWidget(mainWidget);
        topLayout->setMargin(shadowMargin_);

        auto contentLayout = new QVBoxLayout(mainWidget);
        contentLayout->setMargin(0);

        auto emojiCategories = new QFrame(mainWidget);
        emojiCategories->setStyleSheet("background-color: #f2f2f2");

        auto categoriesLayout = new QHBoxLayout(emojiCategories);
        categoriesLayout->setSpacing(6);
        categoriesLayout->setMargin(5);

        QIcon icon;

        auto peopleCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/people.png");
        peopleCategory->setIcon(icon);
        peopleCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));
        peopleCategory->setForegroundColor("gray");

        auto natureCategory_ = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/nature.png");
        natureCategory_->setIcon(icon);
        natureCategory_->setIconSize(QSize(categoryIconSize_, categoryIconSize_));
        natureCategory_->setForegroundColor("gray");

        auto foodCategory_ = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/foods.png");
        foodCategory_->setIcon(icon);
        foodCategory_->setIconSize(QSize(categoryIconSize_, categoryIconSize_));
        foodCategory_->setForegroundColor("gray");

        auto activityCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/activity.png");
        activityCategory->setIcon(icon);
        activityCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));
        activityCategory->setForegroundColor("gray");

        auto travelCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/travel.png");
        travelCategory->setIcon(icon);
        travelCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));
        travelCategory->setForegroundColor("gray");

        auto objectsCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/objects.png");
        objectsCategory->setIcon(icon);
        objectsCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));
        objectsCategory->setForegroundColor("gray");

        auto symbolsCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/symbols.png");
        symbolsCategory->setIcon(icon);
        symbolsCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));
        symbolsCategory->setForegroundColor("gray");

        auto flagsCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/flags.png");
        flagsCategory->setIcon(icon);
        flagsCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));
        flagsCategory->setForegroundColor("gray");

        categoriesLayout->addWidget(peopleCategory);
        categoriesLayout->addWidget(natureCategory_);
        categoriesLayout->addWidget(foodCategory_);
        categoriesLayout->addWidget(activityCategory);
        categoriesLayout->addWidget(travelCategory);
        categoriesLayout->addWidget(objectsCategory);
        categoriesLayout->addWidget(symbolsCategory);
        categoriesLayout->addWidget(flagsCategory);

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        auto scrollWidget = new QWidget(this);
        auto scrollLayout = new QVBoxLayout(scrollWidget);

        scrollLayout->setMargin(0);
        scrollArea_->setWidget(scrollWidget);

        auto peopleEmoji =
          new EmojiCategory(tr("Smileys & People"), emoji_provider_.people, scrollWidget);
        scrollLayout->addWidget(peopleEmoji);

        auto natureEmoji =
          new EmojiCategory(tr("Animals & Nature"), emoji_provider_.nature, scrollWidget);
        scrollLayout->addWidget(natureEmoji);

        auto foodEmoji = new EmojiCategory(tr("Food & Drink"), emoji_provider_.food, scrollWidget);
        scrollLayout->addWidget(foodEmoji);

        auto activityEmoji =
          new EmojiCategory(tr("Activity"), emoji_provider_.activity, scrollWidget);
        scrollLayout->addWidget(activityEmoji);

        auto travelEmoji =
          new EmojiCategory(tr("Travel & Places"), emoji_provider_.travel, scrollWidget);
        scrollLayout->addWidget(travelEmoji);

        auto objectsEmoji = new EmojiCategory(tr("Objects"), emoji_provider_.objects, scrollWidget);
        scrollLayout->addWidget(objectsEmoji);

        auto symbolsEmoji = new EmojiCategory(tr("Symbols"), emoji_provider_.symbols, scrollWidget);
        scrollLayout->addWidget(symbolsEmoji);

        auto flagsEmoji = new EmojiCategory(tr("Flags"), emoji_provider_.flags, scrollWidget);
        scrollLayout->addWidget(flagsEmoji);

        contentLayout->addStretch(1);
        contentLayout->addWidget(scrollArea_);
        contentLayout->addWidget(emojiCategories);

        opacity_ = new QGraphicsOpacityEffect(this);
        opacity_->setOpacity(1.0);

        setGraphicsEffect(opacity_);

        animation_ = new QPropertyAnimation(opacity_, "opacity", this);
        animation_->setDuration(animationDuration_);
        animation_->setStartValue(1);
        animation_->setEndValue(0);

        connect(peopleEmoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
        connect(peopleCategory, &QPushButton::clicked, [this, peopleEmoji]() {
                this->showEmojiCategory(peopleEmoji);
        });

        connect(natureEmoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
        connect(natureCategory_, &QPushButton::clicked, [this, natureEmoji]() {
                this->showEmojiCategory(natureEmoji);
        });

        connect(foodEmoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
        connect(foodCategory_, &QPushButton::clicked, [this, foodEmoji]() {
                this->showEmojiCategory(foodEmoji);
        });

        connect(activityEmoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
        connect(activityCategory, &QPushButton::clicked, [this, activityEmoji]() {
                this->showEmojiCategory(activityEmoji);
        });

        connect(travelEmoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
        connect(travelCategory, &QPushButton::clicked, [this, travelEmoji]() {
                this->showEmojiCategory(travelEmoji);
        });

        connect(objectsEmoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
        connect(objectsCategory, &QPushButton::clicked, [this, objectsEmoji]() {
                this->showEmojiCategory(objectsEmoji);
        });

        connect(symbolsEmoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
        connect(symbolsCategory, &QPushButton::clicked, [this, symbolsEmoji]() {
                this->showEmojiCategory(symbolsEmoji);
        });

        connect(flagsEmoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
        connect(flagsCategory, &QPushButton::clicked, [this, flagsEmoji]() {
                this->showEmojiCategory(flagsEmoji);
        });

        connect(animation_, &QAbstractAnimation::finished, [this]() {
                if (animation_->direction() == QAbstractAnimation::Forward)
                        this->hide();
        });
}

void
EmojiPanel::showEmojiCategory(const EmojiCategory *category)
{
        auto posToGo = category->mapToParent(QPoint()).y();
        auto current = scrollArea_->verticalScrollBar()->value();

        if (current == posToGo)
                return;

        // HACK
        // If we want to go to a previous category and position the label at the top
        // the 6*50 offset won't work because not all the categories have the same
        // height. To ensure the category is at the top, we move to the top and go as
        // normal to the next category.
        if (current > posToGo)
                this->scrollArea_->ensureVisible(0, 0, 0, 0);

        posToGo += 6 * 50;
        this->scrollArea_->ensureVisible(0, posToGo, 0, 0);
}

void
EmojiPanel::leaveEvent(QEvent *event)
{
        Q_UNUSED(event);

        fadeOut();
}

void
EmojiPanel::paintEvent(QPaintEvent *event)
{
        QPainter p(this);
        DropShadow::draw(p,
                         shadowMargin_,
                         4.0,
                         QColor(120, 120, 120, 92),
                         QColor(255, 255, 255, 0),
                         0.0,
                         1.0,
                         0.6,
                         width(),
                         height());

        QWidget::paintEvent(event);
}

void
EmojiPanel::fadeOut()
{
        animation_->setDirection(QAbstractAnimation::Forward);
        animation_->start();
}

void
EmojiPanel::fadeIn()
{
        animation_->setDirection(QAbstractAnimation::Backward);
        animation_->start();
}
