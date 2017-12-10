#pragma once

#include <QColor>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QStateMachine>
#include <QtGlobal>

class TextField;
class TextFieldLabel;
class TextFieldStateMachine;

class TextField : public QLineEdit
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)
        Q_PROPERTY(QColor inkColor WRITE setInkColor READ inkColor)
        Q_PROPERTY(QColor labelColor WRITE setLabelColor READ labelColor)
        Q_PROPERTY(QColor underlineColor WRITE setUnderlineColor READ underlineColor)
        Q_PROPERTY(QColor backgroundColor WRITE setBackgroundColor READ backgroundColor)

public:
        explicit TextField(QWidget *parent = 0);
        ~TextField();

        void setInkColor(const QColor &color);
        void setBackgroundColor(const QColor &color);
        void setLabel(const QString &label);
        void setLabelColor(const QColor &color);
        void setLabelFontSize(qreal size);
        void setShowLabel(bool value);
        void setTextColor(const QColor &color);
        void setUnderlineColor(const QColor &color);

        QColor inkColor() const;
        QColor labelColor() const;
        QColor textColor() const;
        QColor underlineColor() const;
        QColor backgroundColor() const;
        QString label() const;
        bool hasLabel() const;
        qreal labelFontSize() const;

protected:
        bool event(QEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

private:
        void init();

        QColor ink_color_;
        QColor background_color_;
        QColor label_color_;
        QColor text_color_;
        QColor underline_color_;
        QString label_text_;
        TextFieldLabel *label_;
        TextFieldStateMachine *state_machine_;
        bool show_label_;
        qreal label_font_size_;
};

class TextFieldLabel : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(qreal scale WRITE setScale READ scale)
        Q_PROPERTY(QPointF offset WRITE setOffset READ offset)
        Q_PROPERTY(QColor color WRITE setColor READ color)

public:
        TextFieldLabel(TextField *parent);
        ~TextFieldLabel();

        inline void setColor(const QColor &color);
        inline void setOffset(const QPointF &pos);
        inline void setScale(qreal scale);

        inline QColor color() const;
        inline QPointF offset() const;
        inline qreal scale() const;

protected:
        void paintEvent(QPaintEvent *event) override;

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
        update();
}

inline void
TextFieldLabel::setOffset(const QPointF &pos)
{
        x_ = pos.x();
        y_ = pos.y();
        update();
}

inline void
TextFieldLabel::setScale(qreal scale)
{
        scale_ = scale;
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

        Q_PROPERTY(qreal progress WRITE setProgress READ progress)

public:
        TextFieldStateMachine(TextField *parent);
        ~TextFieldStateMachine();

        inline void setProgress(qreal progress);
        void setLabel(TextFieldLabel *label);

        inline qreal progress() const;

public slots:
        void setupProperties();

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
        text_field_->update();
}

inline qreal
TextFieldStateMachine::progress() const
{
        return progress_;
}
