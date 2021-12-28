// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFontDatabase>
#include <QIcon>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>

#include "FlatButton.h"
#include "Ripple.h"
#include "RippleOverlay.h"
#include "ThemeManager.h"

// The ampersand is automatically set in QPushButton or QCheckbx
// by KDEPlatformTheme plugin in Qt5.
// [https://bugs.kde.org/show_bug.cgi?id=337491]
//
// A workaroud is to add
//
// [Development]
// AutoCheckAccelerators=false
//
// to ~/.config/kdeglobals
static QString
removeKDEAccelerators(QString text)
{
    return text.remove(QChar('&'));
}

void
FlatButton::init()
{
    ripple_overlay_          = new RippleOverlay(this);
    role_                    = ui::Role::Default;
    ripple_style_            = ui::RippleStyle::PositionedRipple;
    icon_placement_          = ui::ButtonIconPlacement::LeftIcon;
    overlay_style_           = ui::OverlayStyle::GrayOverlay;
    bg_mode_                 = Qt::TransparentMode;
    fixed_ripple_radius_     = 64;
    corner_radius_           = 3;
    base_opacity_            = 0.13;
    font_size_               = 10; // 10.5;
    use_fixed_ripple_radius_ = false;

    setStyle(&ThemeManager::instance());
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    setCursor(QCursor(Qt::PointingHandCursor));

    QPainterPath path;
    path.addRoundedRect(rect(), corner_radius_, corner_radius_);

    ripple_overlay_->setClipPath(path);
    ripple_overlay_->setClipping(true);
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

void
FlatButton::applyPreset(ui::ButtonPreset preset)
{
    switch (preset) {
    case ui::ButtonPreset::FlatPreset:
        setOverlayStyle(ui::OverlayStyle::NoOverlay);
        break;
    case ui::ButtonPreset::CheckablePreset:
        setOverlayStyle(ui::OverlayStyle::NoOverlay);
        setCheckable(true);
        break;
    default:
        break;
    }
}

void
FlatButton::setRole(ui::Role role)
{
    role_ = role;
}

ui::Role
FlatButton::role() const
{
    return role_;
}

void
FlatButton::setForegroundColor(const QColor &color)
{
    foreground_color_ = color;
}

QColor
FlatButton::foregroundColor() const
{
    if (!foreground_color_.isValid()) {
        if (bg_mode_ == Qt::OpaqueMode) {
            return ThemeManager::instance().themeColor("BrightWhite");
        }

        switch (role_) {
        case ui::Role::Primary:
            return ThemeManager::instance().themeColor("Blue");
        case ui::Role::Secondary:
            return ThemeManager::instance().themeColor("Gray");
        case ui::Role::Default:
        default:
            return ThemeManager::instance().themeColor("Black");
        }
    }

    return foreground_color_;
}

void
FlatButton::setBackgroundColor(const QColor &color)
{
    background_color_ = color;
}

QColor
FlatButton::backgroundColor() const
{
    if (!background_color_.isValid()) {
        switch (role_) {
        case ui::Role::Primary:
            return ThemeManager::instance().themeColor("Blue");
        case ui::Role::Secondary:
            return ThemeManager::instance().themeColor("Gray");
        case ui::Role::Default:
        default:
            return ThemeManager::instance().themeColor("Black");
        }
    }

    return background_color_;
}

void
FlatButton::setOverlayColor(const QColor &color)
{
    overlay_color_ = color;
    setOverlayStyle(ui::OverlayStyle::TintedOverlay);
}

QColor
FlatButton::overlayColor() const
{
    if (!overlay_color_.isValid()) {
        return foregroundColor();
    }

    return overlay_color_;
}

void
FlatButton::setDisabledForegroundColor(const QColor &color)
{
    disabled_color_ = color;
}

QColor
FlatButton::disabledForegroundColor() const
{
    if (!disabled_color_.isValid()) {
        return ThemeManager::instance().themeColor("FadedWhite");
    }

    return disabled_color_;
}

void
FlatButton::setDisabledBackgroundColor(const QColor &color)
{
    disabled_background_color_ = color;
}

QColor
FlatButton::disabledBackgroundColor() const
{
    if (!disabled_background_color_.isValid()) {
        return ThemeManager::instance().themeColor("FadedWhite");
    }

    return disabled_background_color_;
}

void
FlatButton::setFontSize(qreal size)
{
    font_size_ = size;

    QFont f(font());
    f.setPointSizeF(size);
    setFont(f);

    update();
}

qreal
FlatButton::fontSize() const
{
    return font_size_;
}

void
FlatButton::setOverlayStyle(ui::OverlayStyle style)
{
    overlay_style_ = style;
    update();
}

ui::OverlayStyle
FlatButton::overlayStyle() const
{
    return overlay_style_;
}

void
FlatButton::setRippleStyle(ui::RippleStyle style)
{
    ripple_style_ = style;
}

ui::RippleStyle
FlatButton::rippleStyle() const
{
    return ripple_style_;
}

void
FlatButton::setIconPlacement(ui::ButtonIconPlacement placement)
{
    icon_placement_ = placement;
    update();
}

ui::ButtonIconPlacement
FlatButton::iconPlacement() const
{
    return icon_placement_;
}

void
FlatButton::setCornerRadius(qreal radius)
{
    corner_radius_ = radius;
    updateClipPath();
    update();
}

qreal
FlatButton::cornerRadius() const
{
    return corner_radius_;
}

void
FlatButton::setBackgroundMode(Qt::BGMode mode)
{
    bg_mode_ = mode;
}

Qt::BGMode
FlatButton::backgroundMode() const
{
    return bg_mode_;
}

void
FlatButton::setBaseOpacity(qreal opacity)
{
    base_opacity_ = opacity;
}

qreal
FlatButton::baseOpacity() const
{
    return base_opacity_;
}

void
FlatButton::setCheckable(bool value)
{
    QPushButton::setCheckable(value);
}

void
FlatButton::setHasFixedRippleRadius(bool value)
{
    use_fixed_ripple_radius_ = value;
}

bool
FlatButton::hasFixedRippleRadius() const
{
    return use_fixed_ripple_radius_;
}

void
FlatButton::setFixedRippleRadius(qreal radius)
{
    fixed_ripple_radius_ = radius;
    setHasFixedRippleRadius(true);
}

QSize
FlatButton::sizeHint() const
{
    ensurePolished();

    QSize label(fontMetrics().size(Qt::TextSingleLine, removeKDEAccelerators(text())));

    int w = 20 + label.width();
    int h = label.height();

    if (!icon().isNull()) {
        w += iconSize().width() + FlatButton::IconPadding;
        h = qMax(h, iconSize().height());
    }

    return QSize(w, 20 + h);
}

void
FlatButton::checkStateSet()
{
    QPushButton::checkStateSet();
}

void
FlatButton::mousePressEvent(QMouseEvent *event)
{
    if (ui::RippleStyle::NoRipple != ripple_style_) {
        QPoint pos;
        qreal radiusEndValue;

        if (ui::RippleStyle::CenteredRipple == ripple_style_) {
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
        ripple->opacityAnimation()->setDuration(250);

        ripple_overlay_->addRipple(ripple);
    }

    QPushButton::mousePressEvent(event);
}

void
FlatButton::mouseReleaseEvent(QMouseEvent *event)
{
    QPushButton::mouseReleaseEvent(event);
}

void
FlatButton::resizeEvent(QResizeEvent *event)
{
    QPushButton::resizeEvent(event);
    updateClipPath();
}

void
FlatButton::paintEvent(QPaintEvent *event)
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

void
FlatButton::paintBackground(QPainter *painter)
{
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

}

#define COLOR_INTERPOLATE(CH) (1 - progress) * source.CH() + progress *dest.CH()

void
FlatButton::paintForeground(QPainter *painter)
{
    if (isEnabled()) {
        painter->setPen(foregroundColor());
    } else {
        painter->setPen(disabledForegroundColor());
    }

    if (icon().isNull()) {
        painter->drawText(rect(), Qt::AlignCenter, removeKDEAccelerators(text()));
        return;
    }

    QSize textSize(fontMetrics().size(Qt::TextSingleLine, removeKDEAccelerators(text())));
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

    painter->drawText(textGeometry, Qt::AlignCenter, removeKDEAccelerators(text()));

    QPixmap pixmap = icon().pixmap(iconSize());
    QPainter icon(&pixmap);
    icon.setCompositionMode(QPainter::CompositionMode_SourceIn);
    icon.fillRect(pixmap.rect(), painter->pen().color());
    painter->drawPixmap(iconGeometry, pixmap);
}

void
FlatButton::updateClipPath()
{
    const qreal radius = corner_radius_;

    QPainterPath path;
    path.addRoundedRect(rect(), radius, radius);
    ripple_overlay_->setClipPath(path);
}

