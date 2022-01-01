// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractButton>
#include <QColor>

class ToggleTrack;
class ToggleThumb;

enum class Position
{
    Left,
    Right
};

class Toggle : public QAbstractButton
{
    Q_OBJECT

    Q_PROPERTY(QColor activeColor WRITE setActiveColor READ activeColor NOTIFY activeColorChanged)
    Q_PROPERTY(
      QColor disabledColor WRITE setDisabledColor READ disabledColor NOTIFY disabledColorChanged)
    Q_PROPERTY(
      QColor inactiveColor WRITE setInactiveColor READ inactiveColor NOTIFY inactiveColorChanged)
    Q_PROPERTY(QColor trackColor WRITE setTrackColor READ trackColor NOTIFY trackColorChanged)

public:
    Toggle(QWidget *parent = nullptr);

    void setState(bool isEnabled);

    void setActiveColor(const QColor &color);
    void setDisabledColor(const QColor &color);
    void setInactiveColor(const QColor &color);
    void setTrackColor(const QColor &color);

    QColor activeColor() const { return activeColor_; };
    QColor disabledColor() const { return disabledColor_; };
    QColor inactiveColor() const { return inactiveColor_; };
    QColor trackColor() const
    {
        return trackColor_.isValid() ? trackColor_ : QColor(0xee, 0xee, 0xee);
    };

    QSize sizeHint() const override { return QSize(64, 48); };

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void activeColorChanged();
    void disabledColorChanged();
    void inactiveColorChanged();
    void trackColorChanged();

private:
    void init();
    void setupProperties();

    ToggleTrack *track_;
    ToggleThumb *thumb_;

    QColor disabledColor_;
    QColor activeColor_;
    QColor inactiveColor_;
    QColor trackColor_;
};

class ToggleThumb : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QColor thumbColor WRITE setThumbColor READ thumbColor NOTIFY thumbColorChanged)

public:
    ToggleThumb(Toggle *parent);

    Position shift() const { return position_; };
    qreal offset() const { return offset_; };
    QColor thumbColor() const { return thumbColor_; };

    void setShift(Position position);
    void setThumbColor(const QColor &color)
    {
        thumbColor_ = color;
        emit thumbColorChanged();
        update();
    };

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

signals:
    void thumbColorChanged();

private:
    void updateOffset();

    Toggle *const toggle_;
    QColor thumbColor_;

    Position position_;
    qreal offset_;
};

class ToggleTrack : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QColor trackColor WRITE setTrackColor READ trackColor NOTIFY trackColor)

public:
    ToggleTrack(Toggle *parent);

    void setTrackColor(const QColor &color);
    QColor trackColor() const { return trackColor_; };

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

signals:
    void trackColorChanged();

private:
    Toggle *const toggle_;
    QColor trackColor_;
};
