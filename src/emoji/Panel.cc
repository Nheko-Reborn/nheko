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
#include "FlatButton.h"

#include "emoji/Category.h"
#include "emoji/Panel.h"

using namespace emoji;

Panel::Panel(QWidget *parent)
  : QWidget(parent)
  , shadowMargin_{2}
  , width_{370}
  , height_{350}
  , categoryIconSize_{20}
{
        setStyleSheet("QWidget {border: none;}"
                      "QScrollBar:vertical { width: 0px; margin: 0px; }"
                      "QScrollBar::handle:vertical { min-height: 30px; }");

        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);

        auto mainWidget = new QWidget(this);
        mainWidget->setMaximumSize(width_, height_);

        auto topLayout = new QVBoxLayout(this);
        topLayout->addWidget(mainWidget);
        topLayout->setMargin(shadowMargin_);
        topLayout->setSpacing(0);

        auto contentLayout = new QVBoxLayout(mainWidget);
        contentLayout->setMargin(0);
        contentLayout->setSpacing(0);

        auto emojiCategories = new QFrame(mainWidget);

        auto categoriesLayout = new QHBoxLayout(emojiCategories);
        categoriesLayout->setSpacing(0);
        categoriesLayout->setMargin(0);

        QIcon icon;

        auto peopleCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/people.png");
        peopleCategory->setIcon(icon);
        peopleCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));

        auto natureCategory_ = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/nature.png");
        natureCategory_->setIcon(icon);
        natureCategory_->setIconSize(QSize(categoryIconSize_, categoryIconSize_));

        auto foodCategory_ = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/foods.png");
        foodCategory_->setIcon(icon);
        foodCategory_->setIconSize(QSize(categoryIconSize_, categoryIconSize_));

        auto activityCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/activity.png");
        activityCategory->setIcon(icon);
        activityCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));

        auto travelCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/travel.png");
        travelCategory->setIcon(icon);
        travelCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));

        auto objectsCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/objects.png");
        objectsCategory->setIcon(icon);
        objectsCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));

        auto symbolsCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/symbols.png");
        symbolsCategory->setIcon(icon);
        symbolsCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));

        auto flagsCategory = new FlatButton(emojiCategories);
        icon.addFile(":/icons/icons/emoji-categories/flags.png");
        flagsCategory->setIcon(icon);
        flagsCategory->setIconSize(QSize(categoryIconSize_, categoryIconSize_));

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
        scrollLayout->setSpacing(0);
        scrollArea_->setWidget(scrollWidget);

        auto peopleEmoji =
          new Category(tr("Smileys & People"), emoji_provider_.people, scrollWidget);
        scrollLayout->addWidget(peopleEmoji);

        auto natureEmoji =
          new Category(tr("Animals & Nature"), emoji_provider_.nature, scrollWidget);
        scrollLayout->addWidget(natureEmoji);

        auto foodEmoji = new Category(tr("Food & Drink"), emoji_provider_.food, scrollWidget);
        scrollLayout->addWidget(foodEmoji);

        auto activityEmoji = new Category(tr("Activity"), emoji_provider_.activity, scrollWidget);
        scrollLayout->addWidget(activityEmoji);

        auto travelEmoji =
          new Category(tr("Travel & Places"), emoji_provider_.travel, scrollWidget);
        scrollLayout->addWidget(travelEmoji);

        auto objectsEmoji = new Category(tr("Objects"), emoji_provider_.objects, scrollWidget);
        scrollLayout->addWidget(objectsEmoji);

        auto symbolsEmoji = new Category(tr("Symbols"), emoji_provider_.symbols, scrollWidget);
        scrollLayout->addWidget(symbolsEmoji);

        auto flagsEmoji = new Category(tr("Flags"), emoji_provider_.flags, scrollWidget);
        scrollLayout->addWidget(flagsEmoji);

        contentLayout->addWidget(scrollArea_);
        contentLayout->addWidget(emojiCategories);

        connect(peopleEmoji, &Category::emojiSelected, this, &Panel::emojiSelected);
        connect(peopleCategory, &QPushButton::clicked, [this, peopleEmoji]() {
                this->showCategory(peopleEmoji);
        });

        connect(natureEmoji, &Category::emojiSelected, this, &Panel::emojiSelected);
        connect(natureCategory_, &QPushButton::clicked, [this, natureEmoji]() {
                this->showCategory(natureEmoji);
        });

        connect(foodEmoji, &Category::emojiSelected, this, &Panel::emojiSelected);
        connect(foodCategory_, &QPushButton::clicked, [this, foodEmoji]() {
                this->showCategory(foodEmoji);
        });

        connect(activityEmoji, &Category::emojiSelected, this, &Panel::emojiSelected);
        connect(activityCategory, &QPushButton::clicked, [this, activityEmoji]() {
                this->showCategory(activityEmoji);
        });

        connect(travelEmoji, &Category::emojiSelected, this, &Panel::emojiSelected);
        connect(travelCategory, &QPushButton::clicked, [this, travelEmoji]() {
                this->showCategory(travelEmoji);
        });

        connect(objectsEmoji, &Category::emojiSelected, this, &Panel::emojiSelected);
        connect(objectsCategory, &QPushButton::clicked, [this, objectsEmoji]() {
                this->showCategory(objectsEmoji);
        });

        connect(symbolsEmoji, &Category::emojiSelected, this, &Panel::emojiSelected);
        connect(symbolsCategory, &QPushButton::clicked, [this, symbolsEmoji]() {
                this->showCategory(symbolsEmoji);
        });

        connect(flagsEmoji, &Category::emojiSelected, this, &Panel::emojiSelected);
        connect(flagsCategory, &QPushButton::clicked, [this, flagsEmoji]() {
                this->showCategory(flagsEmoji);
        });
}

void
Panel::showCategory(const Category *category)
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
Panel::leaveEvent(QEvent *)
{
        hide();
}

void
Panel::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

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
}
