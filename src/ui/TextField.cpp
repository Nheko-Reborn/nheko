// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TextField.h"

#include <QCoreApplication>
#include <QEventTransition>
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

    state_machine_    = new TextFieldStateMachine(this);
    label_            = nullptr;
    label_font_size_  = 15;
    show_label_       = false;
    background_color_ = pal.color(QPalette::Window);
    is_valid_         = true;

    setFrame(false);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    setTextMargins(0, 4, 0, 6);

    state_machine_->start();
    QCoreApplication::processEvents();
}

void
TextField::setBackgroundColor(const QColor &color)
{
    background_color_ = color;
    emit backgroundColorChanged();
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
        state_machine_->setLabel(label_);
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
    emit labelColorChanged();
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
    emit inkColorChanged();
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
    emit underlineColorChanged();
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
        painter.setOpacity(1 - state_machine_->progress());
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

    const qreal progress = state_machine_->progress();

    if (progress > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(brush);
        const int w = (1 - progress) * static_cast<qreal>(wd / 2);
        painter.drawRect(w + 2.5, height() - 2, wd - 2 * w, 2);
    }
}

TextFieldStateMachine::TextFieldStateMachine(TextField *parent)
  : QStateMachine(parent)
  , text_field_(parent)
{
    normal_state_  = new QState;
    focused_state_ = new QState;

    label_       = nullptr;
    offset_anim_ = nullptr;
    color_anim_  = nullptr;
    progress_    = 0.0;

    addState(normal_state_);
    addState(focused_state_);

    setInitialState(normal_state_);

    QEventTransition *transition;
    QPropertyAnimation *animation;

    transition = new QEventTransition(parent, QEvent::FocusIn);
    transition->setTargetState(focused_state_);
    normal_state_->addTransition(transition);

    animation = new QPropertyAnimation(this, "progress", this);
    animation->setEasingCurve(QEasingCurve::InCubic);
    animation->setDuration(310);
    transition->addAnimation(animation);

    transition = new QEventTransition(parent, QEvent::FocusOut);
    transition->setTargetState(normal_state_);
    focused_state_->addTransition(transition);

    animation = new QPropertyAnimation(this, "progress", this);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setDuration(310);
    transition->addAnimation(animation);

    normal_state_->assignProperty(this, "progress", 0);
    focused_state_->assignProperty(this, "progress", 1);

    setupProperties();

    connect(text_field_, SIGNAL(textChanged(QString)), this, SLOT(setupProperties()));
}

void
TextFieldStateMachine::setLabel(TextFieldLabel *label)
{
    if (label_) {
        delete label_;
    }

    if (offset_anim_) {
        removeDefaultAnimation(offset_anim_);
        delete offset_anim_;
    }

    if (color_anim_) {
        removeDefaultAnimation(color_anim_);
        delete color_anim_;
    }

    label_ = label;

    if (label_) {
        offset_anim_ = new QPropertyAnimation(label_, "offset", this);
        offset_anim_->setDuration(210);
        offset_anim_->setEasingCurve(QEasingCurve::OutCubic);
        addDefaultAnimation(offset_anim_);

        color_anim_ = new QPropertyAnimation(label_, "color", this);
        color_anim_->setDuration(210);
        addDefaultAnimation(color_anim_);
    }

    setupProperties();
}

void
TextFieldStateMachine::setupProperties()
{
    if (label_) {
        const int m = text_field_->textMargins().top();

        if (text_field_->text().isEmpty()) {
            normal_state_->assignProperty(label_, "offset", QPointF(0, 26));
        } else {
            normal_state_->assignProperty(label_, "offset", QPointF(0, 0 - m));
        }

        focused_state_->assignProperty(label_, "offset", QPointF(0, 0 - m));
        focused_state_->assignProperty(label_, "color", text_field_->inkColor());
        normal_state_->assignProperty(label_, "color", text_field_->labelColor());

        if (0 != label_->offset().y() && !text_field_->text().isEmpty()) {
            label_->setOffset(QPointF(0, 0 - m));
        } else if (!text_field_->hasFocus() && label_->offset().y() <= 0 &&
                   text_field_->text().isEmpty()) {
            label_->setOffset(QPointF(0, 26));
        }
    }

    text_field_->update();
}

TextFieldLabel::TextFieldLabel(TextField *parent)
  : QWidget(parent)
  , text_field_(parent)
{
    x_     = 0;
    y_     = 26;
    scale_ = 1;
    color_ = parent->labelColor();

    QFont font;
    font.setWeight(60);
    font.setLetterSpacing(QFont::PercentageSpacing, 102);
    setFont(font);
}

void
TextFieldLabel::paintEvent(QPaintEvent *)
{
    if (!text_field_->hasLabel())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(scale_, scale_);
    painter.setPen(color_);
    painter.setOpacity(1);

    QPointF pos(2 + x_, height() - 36 + y_);
    painter.drawText(pos.x(), pos.y(), text_field_->label());
}
