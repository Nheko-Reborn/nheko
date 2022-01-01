// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QStateMachine>
#include <QtGlobal>

class TextField;
class TextFieldLabel;
class TextFieldStateMachine;

class TextField : public QLineEdit
{
    Q_OBJECT

    Q_PROPERTY(QColor inkColor WRITE setInkColor READ inkColor NOTIFY inkColorChanged)
    Q_PROPERTY(QColor labelColor WRITE setLabelColor READ labelColor NOTIFY labelColorChanged)
    Q_PROPERTY(QColor underlineColor WRITE setUnderlineColor READ underlineColor NOTIFY
                 underlineColorChanged)
    Q_PROPERTY(QColor backgroundColor WRITE setBackgroundColor READ backgroundColor NOTIFY
                 backgroundColorChanged)

public:
    explicit TextField(QWidget *parent = nullptr);

    void setInkColor(const QColor &color);
    void setBackgroundColor(const QColor &color);
    void setLabel(const QString &label);
    void setLabelColor(const QColor &color);
    void setLabelFontSize(qreal size);
    void setShowLabel(bool value);
    void setUnderlineColor(const QColor &color);
    void setRegexp(const QRegularExpression &regexp);
    void setValid(bool valid);

    QColor inkColor() const;
    QColor labelColor() const;
    QColor underlineColor() const;
    QColor backgroundColor() const;
    QString label() const;
    bool hasLabel() const;
    bool isValid() const;
    qreal labelFontSize() const;

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

signals:
    void inkColorChanged();
    void labelColorChanged();
    void underlineColorChanged();
    void backgroundColorChanged();

private:
    void init();

    QColor ink_color_;
    QColor background_color_;
    QColor label_color_;
    QColor underline_color_;
    QString label_text_;
    TextFieldLabel *label_;
    TextFieldStateMachine *state_machine_;
    bool show_label_;
    QRegularExpression regexp_;
    bool is_valid_;
    qreal label_font_size_;
};

class TextFieldLabel : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal scale WRITE setScale READ scale NOTIFY scaleChanged)
    Q_PROPERTY(QPointF offset WRITE setOffset READ offset NOTIFY offsetChanged)
    Q_PROPERTY(QColor color WRITE setColor READ color NOTIFY colorChanged)

public:
    TextFieldLabel(TextField *parent);

    inline void setColor(const QColor &color);
    inline void setOffset(QPointF pos);
    inline void setScale(qreal scale);

    inline QColor color() const;
    inline QPointF offset() const;
    inline qreal scale() const;

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void scaleChanged();
    void offsetChanged();
    void colorChanged();

private:
    TextField *const text_field_;

    QColor color_;
    qreal scale_;
    qreal x_;
    qreal y_;
};

inline void
TextFieldLabel::setColor(const QColor &color)
{
    color_ = color;
    emit colorChanged();
    update();
}

inline void
TextFieldLabel::setOffset(QPointF pos)
{
    x_ = pos.x();
    y_ = pos.y();
    emit offsetChanged();
    update();
}

inline void
TextFieldLabel::setScale(qreal scale)
{
    scale_ = scale;
    emit scaleChanged();
    update();
}

inline QPointF
TextFieldLabel::offset() const
{
    return QPointF(x_, y_);
}
inline qreal
TextFieldLabel::scale() const
{
    return scale_;
}
inline QColor
TextFieldLabel::color() const
{
    return color_;
}

class TextFieldStateMachine : public QStateMachine
{
    Q_OBJECT

    Q_PROPERTY(qreal progress WRITE setProgress READ progress NOTIFY progressChanged)

public:
    TextFieldStateMachine(TextField *parent);

    inline void setProgress(qreal progress);
    void setLabel(TextFieldLabel *label);

    inline qreal progress() const;

public slots:
    void setupProperties();

signals:
    void progressChanged();

private:
    QPropertyAnimation *color_anim_;
    QPropertyAnimation *offset_anim_;

    QState *focused_state_;
    QState *normal_state_;

    TextField *text_field_;
    TextFieldLabel *label_;

    qreal progress_;
};

inline void
TextFieldStateMachine::setProgress(qreal progress)
{
    progress_ = progress;
    emit progressChanged();
    text_field_->update();
}

inline qreal
TextFieldStateMachine::progress() const
{
    return progress_;
}
