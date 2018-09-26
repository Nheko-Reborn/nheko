#pragma once

#include <QSize>
#include <QString>
#include <QTextBrowser>
#include <QUrl>

class QMouseEvent;
class QFocusEvent;
class QWheelEvent;

class TextLabel : public QTextBrowser
{
        Q_OBJECT

public:
        TextLabel(const QString &text, QWidget *parent = nullptr);
        TextLabel(QWidget *parent = nullptr);

        void wheelEvent(QWheelEvent *event) override;
        void clearLinks() { link_.clear(); }

protected:
        void mousePressEvent(QMouseEvent *e) override;
        void mouseReleaseEvent(QMouseEvent *e) override;
        void focusOutEvent(QFocusEvent *e) override;

private slots:
        void adjustHeight(const QSizeF &size) { setFixedHeight(size.height()); }
        void handleLinkActivation(const QUrl &link);

signals:
        void userProfileTriggered(const QString &user_id);
        void linkActivated(const QUrl &link);

private:
        QString link_;
};
