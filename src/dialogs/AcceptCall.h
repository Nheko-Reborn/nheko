#pragma once

#include <QString>
#include <QWidget>

class QPushButton;

namespace dialogs {

class AcceptCall : public QWidget
{
        Q_OBJECT

public:
        AcceptCall(const QString &caller, const QString &displayName, QWidget *parent = nullptr);

signals:
        void accept();
        void reject();

private:
        QPushButton *acceptBtn_;
        QPushButton *rejectBtn_;
};

}
