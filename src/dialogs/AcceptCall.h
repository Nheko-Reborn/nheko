#pragma once

#include <string>
#include <vector>

#include <QSharedPointer>
#include <QWidget>

class QComboBox;
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
                   bool isVideo,
                   QWidget *parent = nullptr);

signals:
        void accept();
        void reject();

private:
        QPushButton *acceptBtn_     = nullptr;
        QPushButton *rejectBtn_     = nullptr;
        QComboBox *microphoneCombo_ = nullptr;
        QComboBox *cameraCombo_     = nullptr;
        std::vector<std::string> microphones_;
        std::vector<std::string> cameras_;
};

}
