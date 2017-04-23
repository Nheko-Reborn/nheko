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

#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "Avatar.h"
#include "EmojiCategory.h"
#include "EmojiPanel.h"
#include "FlatButton.h"

EmojiPanel::EmojiPanel(QWidget *parent)
    : QFrame(parent)
{
	setStyleSheet(
		"QWidget {background: #f8fbfe; color: #e8e8e8; border: none;}"
		"QScrollBar:vertical { background-color: #f8fbfe; width: 8px; border-radius: 20px; margin: 0px 2px 0 2px; }"
		"QScrollBar::handle:vertical { border-radius : 50px; background-color : #d6dde3; }"
		"QScrollBar::add-line:vertical { border: none; background: none; }"
		"QScrollBar::sub-line:vertical { border: none; background: none; }");

	setParent(0);  // Create TopLevel-Widget
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);

	// TODO: Make it MainWindow aware
	auto main_frame_ = new QFrame(this);
	main_frame_->setMinimumSize(370, 350);
	main_frame_->setMaximumSize(370, 350);

	auto top_layout = new QVBoxLayout(this);
	top_layout->addWidget(main_frame_);
	top_layout->setMargin(0);

	auto content_layout = new QVBoxLayout(main_frame_);
	content_layout->setMargin(0);

	auto emoji_categories = new QFrame(main_frame_);
	emoji_categories->setStyleSheet("background-color: #f2f2f2");

	auto categories_layout = new QHBoxLayout(emoji_categories);
	categories_layout->setSpacing(6);
	categories_layout->setMargin(5);

	auto people_category = new FlatButton(emoji_categories);
	people_category->setIcon(QIcon(":/icons/icons/emoji-categories/people.png"));
	people_category->setIconSize(QSize(category_icon_size_, category_icon_size_));
	people_category->setForegroundColor("gray");

	auto nature_category = new FlatButton(emoji_categories);
	nature_category->setIcon(QIcon(":/icons/icons/emoji-categories/nature.png"));
	nature_category->setIconSize(QSize(category_icon_size_, category_icon_size_));
	nature_category->setForegroundColor("gray");

	auto food_category = new FlatButton(emoji_categories);
	food_category->setIcon(QIcon(":/icons/icons/emoji-categories/foods.png"));
	food_category->setIconSize(QSize(category_icon_size_, category_icon_size_));
	food_category->setForegroundColor("gray");

	auto activity_category = new FlatButton(emoji_categories);
	activity_category->setIcon(QIcon(":/icons/icons/emoji-categories/activity.png"));
	activity_category->setIconSize(QSize(category_icon_size_, category_icon_size_));
	activity_category->setForegroundColor("gray");

	auto travel_category = new FlatButton(emoji_categories);
	travel_category->setIcon(QIcon(":/icons/icons/emoji-categories/travel.png"));
	travel_category->setIconSize(QSize(category_icon_size_, category_icon_size_));
	travel_category->setForegroundColor("gray");

	auto objects_category = new FlatButton(emoji_categories);
	objects_category->setIcon(QIcon(":/icons/icons/emoji-categories/objects.png"));
	objects_category->setIconSize(QSize(category_icon_size_, category_icon_size_));
	objects_category->setForegroundColor("gray");

	auto symbols_category = new FlatButton(emoji_categories);
	symbols_category->setIcon(QIcon(":/icons/icons/emoji-categories/symbols.png"));
	symbols_category->setIconSize(QSize(category_icon_size_, category_icon_size_));
	symbols_category->setForegroundColor("gray");

	auto flags_category = new FlatButton(emoji_categories);
	flags_category->setIcon(QIcon(":/icons/icons/emoji-categories/flags.png"));
	flags_category->setIconSize(QSize(category_icon_size_, category_icon_size_));
	flags_category->setForegroundColor("gray");

	categories_layout->addWidget(people_category);
	categories_layout->addWidget(nature_category);
	categories_layout->addWidget(food_category);
	categories_layout->addWidget(activity_category);
	categories_layout->addWidget(travel_category);
	categories_layout->addWidget(objects_category);
	categories_layout->addWidget(symbols_category);
	categories_layout->addWidget(flags_category);

	scroll_area_ = new QScrollArea(this);
	scroll_area_->setWidgetResizable(true);
	scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	auto scroll_widget_ = new QWidget(this);
	auto scroll_layout_ = new QVBoxLayout(scroll_widget_);

	scroll_layout_->setMargin(0);
	scroll_area_->setWidget(scroll_widget_);

	auto people_emoji = new EmojiCategory("Smileys & People", emoji_provider_.people, scroll_widget_);
	scroll_layout_->addWidget(people_emoji);

	auto nature_emoji = new EmojiCategory("Animals & Nature", emoji_provider_.nature, scroll_widget_);
	scroll_layout_->addWidget(nature_emoji);

	auto food_emoji = new EmojiCategory("Food & Drink", emoji_provider_.food, scroll_widget_);
	scroll_layout_->addWidget(food_emoji);

	auto activity_emoji = new EmojiCategory("Activity", emoji_provider_.activity, scroll_widget_);
	scroll_layout_->addWidget(activity_emoji);

	auto travel_emoji = new EmojiCategory("Travel & Places", emoji_provider_.travel, scroll_widget_);
	scroll_layout_->addWidget(travel_emoji);

	auto objects_emoji = new EmojiCategory("Objects", emoji_provider_.objects, scroll_widget_);
	scroll_layout_->addWidget(objects_emoji);

	auto symbols_emoji = new EmojiCategory("Symbols", emoji_provider_.symbols, scroll_widget_);
	scroll_layout_->addWidget(symbols_emoji);

	auto flags_emoji = new EmojiCategory("Flags", emoji_provider_.flags, scroll_widget_);
	scroll_layout_->addWidget(flags_emoji);

	content_layout->addStretch(1);
	content_layout->addWidget(scroll_area_);
	content_layout->addWidget(emoji_categories);

	setLayout(top_layout);

	// TODO: Add parallel animation with geometry
	opacity_ = new QGraphicsOpacityEffect(this);
	opacity_->setOpacity(1.0);

	setGraphicsEffect(opacity_);

	animation_ = new QPropertyAnimation(opacity_, "opacity", this);
	animation_->setDuration(180);
	animation_->setStartValue(1.0);
	animation_->setEndValue(0.0);

	connect(people_emoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
	connect(people_category, &QPushButton::clicked, [this, people_emoji]() {
		this->showEmojiCategory(people_emoji);
	});

	connect(nature_emoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
	connect(nature_category, &QPushButton::clicked, [this, nature_emoji]() {
		this->showEmojiCategory(nature_emoji);
	});

	connect(food_emoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
	connect(food_category, &QPushButton::clicked, [this, food_emoji]() {
		this->showEmojiCategory(food_emoji);
	});

	connect(activity_emoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
	connect(activity_category, &QPushButton::clicked, [this, activity_emoji]() {
		this->showEmojiCategory(activity_emoji);
	});

	connect(travel_emoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
	connect(travel_category, &QPushButton::clicked, [this, travel_emoji]() {
		this->showEmojiCategory(travel_emoji);
	});

	connect(objects_emoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
	connect(objects_category, &QPushButton::clicked, [this, objects_emoji]() {
		this->showEmojiCategory(objects_emoji);
	});

	connect(symbols_emoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
	connect(symbols_category, &QPushButton::clicked, [this, symbols_emoji]() {
		this->showEmojiCategory(symbols_emoji);
	});

	connect(flags_emoji, &EmojiCategory::emojiSelected, this, &EmojiPanel::emojiSelected);
	connect(flags_category, &QPushButton::clicked, [this, flags_emoji]() {
		this->showEmojiCategory(flags_emoji);
	});

	connect(animation_, &QPropertyAnimation::finished, [this]() {
		if (animation_->direction() == QAbstractAnimation::Forward)
			this->hide();
	});
}

void EmojiPanel::showEmojiCategory(const EmojiCategory *category)
{
	auto posToGo = category->mapToParent(QPoint()).y();
	auto current = scroll_area_->verticalScrollBar()->value();

	if (current == posToGo)
		return;

	// HACK
	// If we want to go to a previous category and position the label at the top
	// the 6*50 offset won't work because not all the categories have the same height.
	// To ensure the category is at the top, we move to the top
	// and go as normal to the next category.
	if (current > posToGo)
		this->scroll_area_->ensureVisible(0, 0, 0, 0);

	posToGo += 6 * 50;
	this->scroll_area_->ensureVisible(0, posToGo, 0, 0);
}

void EmojiPanel::leaveEvent(QEvent *event)
{
	Q_UNUSED(event);

	fadeOut();
}

void EmojiPanel::fadeOut()
{
	animation_->setDirection(QAbstractAnimation::Forward);
	animation_->start();
}

void EmojiPanel::fadeIn()
{
	animation_->setDirection(QAbstractAnimation::Backward);
	animation_->start();
}
