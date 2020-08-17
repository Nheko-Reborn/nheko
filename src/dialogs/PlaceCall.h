#pragma once

#include <string>
#include <vector>

#include <QSharedPointer>
#include <QWidget>

class QPushButton;
class QString;
class UserSettings;

namespace dialogs {

class PlaceCall : public QWidget
{
        Q_OBJECT

public:
        PlaceCall(const QString &callee,
                  const QString &displayName,
                  const QString &roomName,
                  const QString &avatarUrl,
                  QSharedPointer<UserSettings> settings,
                  QWidget *parent = nullptr);

signals:
        void voice();
        void cancel();

private:
        QPushButton *voiceBtn_;
        QPushButton *cancelBtn_;
        std::vector<std::string> audioDevices_;
};

}
