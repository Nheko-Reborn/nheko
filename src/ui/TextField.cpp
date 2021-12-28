// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TextField.h"

#include <QCoreApplication>
#include <QFontDatabase>
#include <QPaintEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QRegularExpressionValidator>

TextField::TextField(QWidget *parent)
  : QLineEdit(parent)
{
    // Get rid of the focus border on macOS.
    setAttribute(Qt::WA_MacShowFocusRect, 0);

    QPalette pal;

    label_            = nullptr;
    label_font_size_  = 15;
    show_label_       = false;
    background_color_ = pal.color(QPalette::Window);
    is_valid_         = true;

    setFrame(false);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    setTextMargins(0, 4, 0, 6);

    QCoreApplication::processEvents();
}

void
TextField::setBackgroundColor(const QColor &color)
{
    background_color_ = color;
}

QColor
TextField::backgroundColor() const
{
    return background_color_;
}

void
TextField::setShowLabel(bool value)
{
    if (show_label_ == value) {
        return;
    }

    show_label_ = value;

    if (!label_ && value) {
        label_ = new TextFieldLabel(this);
    }

    if (value) {
        setContentsMargins(0, 23, 0, 0);
    } else {
        setContentsMargins(0, 0, 0, 0);
    }
}

bool
TextField::hasLabel() const
{
    return show_label_;
}

void
TextField::setValid(bool valid)
{
    is_valid_ = valid;
}

bool
TextField::isValid() const
{
    QString s = text();
    int pos   = 0;
    if (regexp_.pattern().isEmpty()) {
        return is_valid_;
    }
    QRegularExpressionValidator v(QRegularExpression(regexp_), 0);
    return v.validate(s, pos) == QValidator::Acceptable;
}

void
TextField::setLabelFontSize(qreal size)
{
    label_font_size_ = size;

    if (label_) {
        QFont font(label_->font());
        font.setPointSizeF(size);
        label_->setFont(font);
        label_->update();
    }
}

qreal
TextField::labelFontSize() const
{
    return label_font_size_;
}

void
TextField::setLabel(const QString &label)
{
    label_text_ = label;
    setShowLabel(true);
    label_->update();
}

QString
TextField::label() const
{
    return label_text_;
}

void
TextField::setLabelColor(const QColor &color)
{
    label_color_ = color;
    update();
}

QColor
TextField::labelColor() const
{
    if (!label_color_.isValid()) {
        return QPalette().color(QPalette::Text);
    }

    return label_color_;
}

void
TextField::setInkColor(const QColor &color)
{
    ink_color_ = color;
    update();
}

QColor
TextField::inkColor() const
{
    if (!ink_color_.isValid()) {
        return QPalette().color(QPalette::Text);
    }

    return ink_color_;
}

void
TextField::setUnderlineColor(const QColor &color)
{
    underline_color_ = color;
    update();
}

void
TextField::setRegexp(const QRegularExpression &regexp)
{
    regexp_ = regexp;
}

QColor
TextField::underlineColor() const
{
    if (!underline_color_.isValid()) {
        if ((hasAcceptableInput() && isValid()) || !isModified())
            return QPalette().color(QPalette::Highlight);
        else
            return Qt::red;
    }

    return underline_color_;
}

bool
TextField::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Resize:
    case QEvent::Move: {
        if (label_)
            label_->setGeometry(rect());
        break;
    }
    default:
        break;
    }

    return QLineEdit::event(event);
}

void
TextField::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);

    QPainter painter(this);

    if (text().isEmpty()) {
        painter.fillRect(rect(), backgroundColor());
    }

    const int y  = height() - 1;
    const int wd = width() - 5;

    QPen pen;
    pen.setWidth(1);
    pen.setColor(underlineColor());
    painter.setPen(pen);
    painter.setOpacity(1);
    painter.drawLine(2, y, wd, y);

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(inkColor());
}
