#pragma once

#include <string>
#include <vector>

#include <QWidget>

class QComboBox;
class QPushButton;
class QString;

namespace dialogs {

class AcceptCall : public QWidget
{
        Q_OBJECT

public:
        AcceptCall(const QString &caller,
                   const QString &displayName,
                   const QString &roomName,
                   const QString &avatarUrl,
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
