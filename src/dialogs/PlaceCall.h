#pragma once

#include <QWidget>

class QPushButton;
class QString;

namespace dialogs {

class PlaceCall : public QWidget
{
        Q_OBJECT

public:
        PlaceCall(const QString &callee,
                  const QString &displayName,
                  const QString &roomName,
                  const QString &avatarUrl,
                  QWidget *parent = nullptr);

signals:
        void voice();
        void cancel();

private:
        QPushButton *voiceBtn_;
        QPushButton *cancelBtn_;
};

}
