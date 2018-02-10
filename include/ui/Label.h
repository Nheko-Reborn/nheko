#pragma once

#include <QLabel>

class Label : public QLabel
{
        Q_OBJECT

public:
        explicit Label(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
        explicit Label(const QString &text,
                       QWidget *parent   = Q_NULLPTR,
                       Qt::WindowFlags f = Qt::WindowFlags());

signals:
        void clicked(QMouseEvent *e);
        void pressed(QMouseEvent *e);
        void released(QMouseEvent *e);

protected:
        void mousePressEvent(QMouseEvent *e) override;
        void mouseReleaseEvent(QMouseEvent *e) override;

        QPoint pressPosition_;
};
