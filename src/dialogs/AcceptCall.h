#pragma once

#include <string>
#include <vector>

#include <QSharedPointer>
#include <QWidget>

class QPushButton;
class QString;
class UserSettings;

namespace dialogs {

class AcceptCall : public QWidget
{
        Q_OBJECT

public:
        AcceptCall(const QString &caller,
                   const QString &displayName,
                   const QString &roomName,
                   const QString &avatarUrl,
                   QSharedPointer<UserSettings> settings,
                   QWidget *parent = nullptr);

signals:
        void accept();
        void reject();

private:
        QPushButton *acceptBtn_;
        QPushButton *rejectBtn_;
        std::vector<std::string> audioDevices_;
};
}
