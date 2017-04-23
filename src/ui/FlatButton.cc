#include <QEventTransition>
#include <QFontDatabase>
#include <QIcon>
#include <QMouseEvent>
#include <QPainterPath>
#include <QResizeEvent>
#include <QSignalTransition>

#include "FlatButton.h"
#include "Ripple.h"
#include "RippleOverlay.h"
#include "ThemeManager.h"

void FlatButton::init()
{
	ripple_overlay_ = new RippleOverlay(this);
	state_machine_ = new FlatButtonStateMachine(this);
	role_ = ui::Default;
	ripple_style_ = ui::PositionedRipple;
	icon_placement_ = ui::LeftIcon;
	overlay_style_ = ui::GrayOverlay;
	bg_mode_ = Qt::TransparentMode;
	fixed_ripple_radius_ = 64;
	corner_radius_ = 3;
	base_opacity_ = 0.13;
	font_size_ = 10;  // 10.5;
	use_fixed_ripple_radius_ = false;

	setStyle(&ThemeManager::instance());
	setAttribute(Qt::WA_Hover);
	setMouseTracking(true);

	QPainterPath path;
	path.addRoundedRect(rect(), corner_radius_, corner_radius_);

	ripple_overlay_->setClipPath(path);
	ripple_overlay_->setClipping(true);

	state_machine_->setupProperties();
	state_machine_->startAnimations();
}

FlatButton::FlatButton(QWidget *parent, ui::ButtonPreset preset)
    : QPushButton(parent)
{
	init();
	applyPreset(preset);
}

FlatButton::FlatButton(const QString &text, QWidget *parent, ui::ButtonPreset preset)
    : QPushButton(text, parent)
{
	init();
	applyPreset(preset);
}

FlatButton::FlatButton(const QString &text, ui::Role role, QWidget *parent, ui::ButtonPreset preset)
    : QPushButton(text, parent)
{
	init();
	applyPreset(preset);
	setRole(role);
}

FlatButton::~FlatButton()
{
}

void FlatButton::applyPreset(ui::ButtonPreset preset)
{
	switch (preset) {
	case ui::FlatPreset:
		setOverlayStyle(ui::NoOverlay);
		break;
	case ui::CheckablePreset:
		setOverlayStyle(ui::NoOverlay);
		setCheckable(true);
		break;
	default:
		break;
	}
}

void FlatButton::setRole(ui::Role role)
{
	role_ = role;
	state_machine_->setupProperties();
}

ui::Role FlatButton::role() const
{
	return role_;
}

void FlatButton::setForegroundColor(const QColor &color)
{
	foreground_color_ = color;
}

QColor FlatButton::foregroundColor() const
{
	if (!foreground_color_.isValid()) {
		if (bg_mode_ == Qt::OpaqueMode) {
			return ThemeManager::instance().themeColor("BrightWhite");
		}

		switch (role_) {
		case ui::Primary:
			return ThemeManager::instance().themeColor("Blue");
		case ui::Secondary:
			return ThemeManager::instance().themeColor("Gray");
		case ui::Default:
		default:
			return ThemeManager::instance().themeColor("Black");
		}
	}

	return foreground_color_;
}

void FlatButton::setBackgroundColor(const QColor &color)
{
	background_color_ = color;
}

QColor FlatButton::backgroundColor() const
{
	if (!background_color_.isValid()) {
		switch (role_) {
		case ui::Primary:
			return ThemeManager::instance().themeColor("Blue");
		case ui::Secondary:
			return ThemeManager::instance().themeColor("Gray");
		case ui::Default:
		default:
			return ThemeManager::instance().themeColor("Black");
		}
	}

	return background_color_;
}

void FlatButton::setOverlayColor(const QColor &color)
{
	overlay_color_ = color;
	setOverlayStyle(ui::TintedOverlay);
}

QColor FlatButton::overlayColor() const
{
	if (!overlay_color_.isValid()) {
		return foregroundColor();
	}

	return overlay_color_;
}

void FlatButton::setDisabledForegroundColor(const QColor &color)
{
	disabled_color_ = color;
}

QColor FlatButton::disabledForegroundColor() const
{
	if (!disabled_color_.isValid()) {
		return ThemeManager::instance().themeColor("FadedWhite");
	}

	return disabled_color_;
}

void FlatButton::setDisabledBackgroundColor(const QColor &color)
{
	disabled_background_color_ = color;
}

QColor FlatButton::disabledBackgroundColor() const
{
	if (!disabled_background_color_.isValid()) {
		return ThemeManager::instance().themeColor("FadedWhite");
	}

	return disabled_background_color_;
}

void FlatButton::setFontSize(qreal size)
{
	font_size_ = size;

	QFont f(font());
	f.setPointSizeF(size);
	setFont(f);

	update();
}

qreal FlatButton::fontSize() const
{
	return font_size_;
}

void FlatButton::setOverlayStyle(ui::OverlayStyle style)
{
	overlay_style_ = style;
	update();
}

ui::OverlayStyle FlatButton::overlayStyle() const
{
	return overlay_style_;
}

void FlatButton::setRippleStyle(ui::RippleStyle style)
{
	ripple_style_ = style;
}

ui::RippleStyle FlatButton::rippleStyle() const
{
	return ripple_style_;
}

void FlatButton::setIconPlacement(ui::ButtonIconPlacement placement)
{
	icon_placement_ = placement;
	update();
}

ui::ButtonIconPlacement FlatButton::iconPlacement() const
{
	return icon_placement_;
}

void FlatButton::setCornerRadius(qreal radius)
{
	corner_radius_ = radius;
	updateClipPath();
	update();
}

qreal FlatButton::cornerRadius() const
{
	return corner_radius_;
}

void FlatButton::setBackgroundMode(Qt::BGMode mode)
{
	bg_mode_ = mode;
	state_machine_->setupProperties();
}

Qt::BGMode FlatButton::backgroundMode() const
{
	return bg_mode_;
}

void FlatButton::setBaseOpacity(qreal opacity)
{
	base_opacity_ = opacity;
	state_machine_->setupProperties();
}

qreal FlatButton::baseOpacity() const
{
	return base_opacity_;
}

void FlatButton::setCheckable(bool value)
{
	state_machine_->updateCheckedStatus();
	state_machine_->setCheckedOverlayProgress(0);

	QPushButton::setCheckable(value);
}

void FlatButton::setHasFixedRippleRadius(bool value)
{
	use_fixed_ripple_radius_ = value;
}

bool FlatButton::hasFixedRippleRadius() const
{
	return use_fixed_ripple_radius_;
}

void FlatButton::setFixedRippleRadius(qreal radius)
{
	fixed_ripple_radius_ = radius;
	setHasFixedRippleRadius(true);
}

QSize FlatButton::sizeHint() const
{
	ensurePolished();

	QSize label(fontMetrics().size(Qt::TextSingleLine, text()));

	int w = 20 + label.width();
	int h = label.height();

	if (!icon().isNull()) {
		w += iconSize().width() + FlatButton::IconPadding;
		h = qMax(h, iconSize().height());
	}

	return QSize(w, 20 + h);
}

void FlatButton::checkStateSet()
{
	state_machine_->updateCheckedStatus();
	QPushButton::checkStateSet();
}

void FlatButton::mousePressEvent(QMouseEvent *event)
{
	if (ui::NoRipple != ripple_style_) {
		QPoint pos;
		qreal radiusEndValue;

		if (ui::CenteredRipple == ripple_style_) {
			pos = rect().center();
		} else {
			pos = event->pos();
		}

		if (use_fixed_ripple_radius_) {
			radiusEndValue = fixed_ripple_radius_;
		} else {
			radiusEndValue = static_cast<qreal>(width()) / 2;
		}

		Ripple *ripple = new Ripple(pos);

		ripple->setRadiusEndValue(radiusEndValue);
		ripple->setOpacityStartValue(0.35);
		ripple->setColor(foregroundColor());
		ripple->radiusAnimation()->setDuration(250);
		ripple->opacityAnimation()->setDuration(400);

		ripple_overlay_->addRipple(ripple);
	}

	QPushButton::mousePressEvent(event);
}

void FlatButton::mouseReleaseEvent(QMouseEvent *event)
{
	QPushButton::mouseReleaseEvent(event);
	state_machine_->updateCheckedStatus();
}

void FlatButton::resizeEvent(QResizeEvent *event)
{
	QPushButton::resizeEvent(event);
	updateClipPath();
}

void FlatButton::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	const qreal cr = corner_radius_;

	if (cr > 0) {
		QPainterPath path;
		path.addRoundedRect(rect(), cr, cr);

		painter.setClipPath(path);
		painter.setClipping(true);
	}

	paintBackground(&painter);

	painter.setOpacity(1);
	painter.setClipping(false);

	paintForeground(&painter);
}

void FlatButton::paintBackground(QPainter *painter)
{
	const qreal overlayOpacity = state_machine_->overlayOpacity();
	const qreal checkedProgress = state_machine_->checkedOverlayProgress();

	if (Qt::OpaqueMode == bg_mode_) {
		QBrush brush;
		brush.setStyle(Qt::SolidPattern);

		if (isEnabled()) {
			brush.setColor(backgroundColor());
		} else {
			brush.setColor(disabledBackgroundColor());
		}

		painter->setOpacity(1);
		painter->setBrush(brush);
		painter->setPen(Qt::NoPen);
		painter->drawRect(rect());
	}

	QBrush brush;
	brush.setStyle(Qt::SolidPattern);
	painter->setPen(Qt::NoPen);

	if (!isEnabled()) {
		return;
	}

	if ((ui::NoOverlay != overlay_style_) && (overlayOpacity > 0)) {
		if (ui::TintedOverlay == overlay_style_) {
			brush.setColor(overlayColor());
		} else {
			brush.setColor(Qt::gray);
		}

		painter->setOpacity(overlayOpacity);
		painter->setBrush(brush);
		painter->drawRect(rect());
	}

	if (isCheckable() && checkedProgress > 0) {
		const qreal q = Qt::TransparentMode == bg_mode_ ? 0.45 : 0.7;
		brush.setColor(foregroundColor());
		painter->setOpacity(q * checkedProgress);
		painter->setBrush(brush);
		QRect r(rect());
		r.setHeight(static_cast<qreal>(r.height()) * checkedProgress);
		painter->drawRect(r);
	}
}

#define COLOR_INTERPOLATE(CH) (1 - progress) * source.CH() + progress *dest.CH()

void FlatButton::paintForeground(QPainter *painter)
{
	if (isEnabled()) {
		painter->setPen(foregroundColor());
		const qreal progress = state_machine_->checkedOverlayProgress();

		if (isCheckable() && progress > 0) {
			QColor source = foregroundColor();
			QColor dest = Qt::TransparentMode == bg_mode_ ? Qt::white
								      : backgroundColor();
			if (qFuzzyCompare(1, progress)) {
				painter->setPen(dest);
			} else {
				painter->setPen(QColor(COLOR_INTERPOLATE(red),
						       COLOR_INTERPOLATE(green),
						       COLOR_INTERPOLATE(blue),
						       COLOR_INTERPOLATE(alpha)));
			}
		}
	} else {
		painter->setPen(disabledForegroundColor());
	}

	if (icon().isNull()) {
		painter->drawText(rect(), Qt::AlignCenter, text());
		return;
	}

	QSize textSize(fontMetrics().size(Qt::TextSingleLine, text()));
	QSize base(size() - textSize);

	const int iw = iconSize().width() + IconPadding;
	QPoint pos((base.width() - iw) / 2, 0);

	QRect textGeometry(pos + QPoint(0, base.height() / 2), textSize);
	QRect iconGeometry(pos + QPoint(0, (height() - iconSize().height()) / 2), iconSize());

	/* if (ui::LeftIcon == icon_placement_) { */
	/* 	textGeometry.translate(iw, 0); */
	/* } else { */
	/* 	iconGeometry.translate(textSize.width() + IconPadding, 0); */
	/* } */

	painter->drawText(textGeometry, Qt::AlignCenter, text());

	QPixmap pixmap = icon().pixmap(iconSize());
	QPainter icon(&pixmap);
	icon.setCompositionMode(QPainter::CompositionMode_SourceIn);
	icon.fillRect(pixmap.rect(), painter->pen().color());
	painter->drawPixmap(iconGeometry, pixmap);
}

void FlatButton::updateClipPath()
{
	const qreal radius = corner_radius_;

	QPainterPath path;
	path.addRoundedRect(rect(), radius, radius);
	ripple_overlay_->setClipPath(path);
}

FlatButtonStateMachine::FlatButtonStateMachine(FlatButton *parent)
    : QStateMachine(parent)
    , button_(parent)
    , top_level_state_(new QState(QState::ParallelStates))
    , config_state_(new QState(top_level_state_))
    , checkable_state_(new QState(top_level_state_))
    , checked_state_(new QState(checkable_state_))
    , unchecked_state_(new QState(checkable_state_))
    , neutral_state_(new QState(config_state_))
    , neutral_focused_state_(new QState(config_state_))
    , hovered_state_(new QState(config_state_))
    , hovered_focused_state_(new QState(config_state_))
    , pressed_state_(new QState(config_state_))
    , overlay_opacity_(0)
    , checked_overlay_progress_(parent->isChecked() ? 1 : 0)
    , was_checked_(false)
{
	Q_ASSERT(parent);

	parent->installEventFilter(this);

	config_state_->setInitialState(neutral_state_);
	addState(top_level_state_);
	setInitialState(top_level_state_);

	checkable_state_->setInitialState(parent->isChecked() ? checked_state_
							      : unchecked_state_);
	QSignalTransition *transition;
	QPropertyAnimation *animation;

	transition = new QSignalTransition(this, SIGNAL(buttonChecked()));
	transition->setTargetState(checked_state_);
	unchecked_state_->addTransition(transition);

	animation = new QPropertyAnimation(this, "checkedOverlayProgress", this);
	animation->setDuration(200);
	transition->addAnimation(animation);

	transition = new QSignalTransition(this, SIGNAL(buttonUnchecked()));
	transition->setTargetState(unchecked_state_);
	checked_state_->addTransition(transition);

	animation = new QPropertyAnimation(this, "checkedOverlayProgress", this);
	animation->setDuration(200);
	transition->addAnimation(animation);

	addTransition(button_, QEvent::FocusIn, neutral_state_, neutral_focused_state_);
	addTransition(button_, QEvent::FocusOut, neutral_focused_state_, neutral_state_);
	addTransition(button_, QEvent::Enter, neutral_state_, hovered_state_);
	addTransition(button_, QEvent::Leave, hovered_state_, neutral_state_);
	addTransition(button_, QEvent::Enter, neutral_focused_state_, hovered_focused_state_);
	addTransition(button_, QEvent::Leave, hovered_focused_state_, neutral_focused_state_);
	addTransition(button_, QEvent::FocusIn, hovered_state_, hovered_focused_state_);
	addTransition(button_, QEvent::FocusOut, hovered_focused_state_, hovered_state_);
	addTransition(this, SIGNAL(buttonPressed()), hovered_state_, pressed_state_);
	addTransition(button_, QEvent::Leave, pressed_state_, neutral_focused_state_);
	addTransition(button_, QEvent::FocusOut, pressed_state_, hovered_state_);
}

FlatButtonStateMachine::~FlatButtonStateMachine()
{
}

void FlatButtonStateMachine::setOverlayOpacity(qreal opacity)
{
	overlay_opacity_ = opacity;
	button_->update();
}

void FlatButtonStateMachine::setCheckedOverlayProgress(qreal opacity)
{
	checked_overlay_progress_ = opacity;
	button_->update();
}

void FlatButtonStateMachine::startAnimations()
{
	start();
}

void FlatButtonStateMachine::setupProperties()
{
	QColor overlayColor;

	if (Qt::TransparentMode == button_->backgroundMode()) {
		overlayColor = button_->backgroundColor();
	} else {
		overlayColor = button_->foregroundColor();
	}

	const qreal baseOpacity = button_->baseOpacity();

	neutral_state_->assignProperty(this, "overlayOpacity", 0);
	neutral_focused_state_->assignProperty(this, "overlayOpacity", 0);
	hovered_state_->assignProperty(this, "overlayOpacity", baseOpacity);
	hovered_focused_state_->assignProperty(this, "overlayOpacity", baseOpacity);
	pressed_state_->assignProperty(this, "overlayOpacity", baseOpacity);
	checked_state_->assignProperty(this, "checkedOverlayProgress", 1);
	unchecked_state_->assignProperty(this, "checkedOverlayProgress", 0);

	button_->update();
}

void FlatButtonStateMachine::updateCheckedStatus()
{
	const bool checked = button_->isChecked();
	if (was_checked_ != checked) {
		was_checked_ = checked;
		if (checked) {
			emit buttonChecked();
		} else {
			emit buttonUnchecked();
		}
	}
}

bool FlatButtonStateMachine::eventFilter(QObject *watched,
					 QEvent *event)
{
	if (QEvent::FocusIn == event->type()) {
		QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);
		if (focusEvent && Qt::MouseFocusReason == focusEvent->reason()) {
			emit buttonPressed();
			return true;
		}
	}

	return QStateMachine::eventFilter(watched, event);
}

void FlatButtonStateMachine::addTransition(QObject *object,
					   const char *signal,
					   QState *fromState,
					   QState *toState)
{
	addTransition(new QSignalTransition(object, signal), fromState, toState);
}

void FlatButtonStateMachine::addTransition(QObject *object,
					   QEvent::Type eventType,
					   QState *fromState,
					   QState *toState)
{
	addTransition(new QEventTransition(object, eventType), fromState, toState);
}

void FlatButtonStateMachine::addTransition(QAbstractTransition *transition,
					   QState *fromState,
					   QState *toState)
{
	transition->setTargetState(toState);

	QPropertyAnimation *animation;

	animation = new QPropertyAnimation(this, "overlayOpacity", this);
	animation->setDuration(150);
	transition->addAnimation(animation);

	fromState->addTransition(transition);
}
