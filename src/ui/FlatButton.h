// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPushButton>

#include "Theme.h"

class RippleOverlay;


class FlatButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QColor foregroundColor WRITE setForegroundColor READ foregroundColor)
    Q_PROPERTY(QColor backgroundColor WRITE setBackgroundColor READ backgroundColor)
    Q_PROPERTY(QColor overlayColor WRITE setOverlayColor READ overlayColor)
    Q_PROPERTY(
      QColor disabledForegroundColor WRITE setDisabledForegroundColor READ disabledForegroundColor)
    Q_PROPERTY(
      QColor disabledBackgroundColor WRITE setDisabledBackgroundColor READ disabledBackgroundColor)
    Q_PROPERTY(qreal fontSize WRITE setFontSize READ fontSize)

public:
    explicit FlatButton(QWidget *parent         = nullptr,
                        ui::ButtonPreset preset = ui::ButtonPreset::FlatPreset);
    explicit FlatButton(const QString &text,
                        QWidget *parent         = nullptr,
                        ui::ButtonPreset preset = ui::ButtonPreset::FlatPreset);
    FlatButton(const QString &text,
               ui::Role role,
               QWidget *parent         = nullptr,
               ui::ButtonPreset preset = ui::ButtonPreset::FlatPreset);

    void applyPreset(ui::ButtonPreset preset);

    void setBackgroundColor(const QColor &color);
    void setBackgroundMode(Qt::BGMode mode);
    void setBaseOpacity(qreal opacity);
    void setCheckable(bool value);
    void setCornerRadius(qreal radius);
    void setDisabledBackgroundColor(const QColor &color);
    void setDisabledForegroundColor(const QColor &color);
    void setFixedRippleRadius(qreal radius);
    void setFontSize(qreal size);
    void setForegroundColor(const QColor &color);
    void setHasFixedRippleRadius(bool value);
    void setIconPlacement(ui::ButtonIconPlacement placement);
    void setOverlayColor(const QColor &color);
    void setOverlayStyle(ui::OverlayStyle style);
    void setRippleStyle(ui::RippleStyle style);
    void setRole(ui::Role role);

    QColor foregroundColor() const;
    QColor backgroundColor() const;
    QColor overlayColor() const;
    QColor disabledForegroundColor() const;
    QColor disabledBackgroundColor() const;

    qreal fontSize() const;
    qreal cornerRadius() const;
    qreal baseOpacity() const;

    bool hasFixedRippleRadius() const;

    ui::Role role() const;
    ui::OverlayStyle overlayStyle() const;
    ui::RippleStyle rippleStyle() const;
    ui::ButtonIconPlacement iconPlacement() const;

    Qt::BGMode backgroundMode() const;

    QSize sizeHint() const override;

protected:
    int IconPadding = 0;

    void checkStateSet() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    virtual void paintBackground(QPainter *painter);
    virtual void paintForeground(QPainter *painter);
    virtual void updateClipPath();

    void init();

private:
    RippleOverlay *ripple_overlay_;

    ui::Role role_;
    ui::RippleStyle ripple_style_;
    ui::ButtonIconPlacement icon_placement_;
    ui::OverlayStyle overlay_style_;

    Qt::BGMode bg_mode_;

    QColor background_color_;
    QColor foreground_color_;
    QColor overlay_color_;
    QColor disabled_color_;
    QColor disabled_background_color_;

    qreal fixed_ripple_radius_;
    qreal corner_radius_;
    qreal base_opacity_;
    qreal font_size_;

    bool use_fixed_ripple_radius_;
};
