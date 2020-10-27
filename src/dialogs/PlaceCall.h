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
        void video();
        void cancel();

private:
        const int iconSize_         = 18;
        QPushButton *voiceBtn_      = nullptr;
        QPushButton *videoBtn_      = nullptr;
        QPushButton *cancelBtn_     = nullptr;
        QComboBox *microphoneCombo_ = nullptr;
        QComboBox *cameraCombo_     = nullptr;
        std::vector<std::string> microphones_;
        std::vector<std::string> cameras_;
};

}
