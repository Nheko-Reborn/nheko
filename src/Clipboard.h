#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QApplication>
#include <QClipboard>

class Clipboard : public QObject
{
        Q_OBJECT

public:
        explicit Clipboard(QObject *parent = 0)
          : QObject{parent}
          , clipboard_{QApplication::clipboard()}
        {}

        Q_INVOKABLE void setText(const QString &text) { clipboard_->setText(text); }

private:
        QClipboard *clipboard_;
};

#endif // CLIPBOARD_H
