#pragma once

#include <QWidget>

#include "WebRTCSession.h"

class QHBoxLayout;
class QLabel;
class QTimer;
class Avatar;
class FlatButton;

class ActiveCallBar : public QWidget
{
        Q_OBJECT

public:
        ActiveCallBar(QWidget *parent = nullptr);

public slots:
        void update(WebRTCSession::State);
        void setCallParty(const QString &userid,
                          const QString &displayName,
                          const QString &roomName,
                          const QString &avatarUrl);

private:
        QHBoxLayout *layout_    = nullptr;
        Avatar *avatar_         = nullptr;
        QLabel *callPartyLabel_ = nullptr;
        QLabel *stateLabel_     = nullptr;
        QLabel *durationLabel_  = nullptr;
        FlatButton *muteBtn_    = nullptr;
        int buttonSize_         = 22;
        bool muted_             = false;
        qint64 callStartTime_   = 0;
        QTimer *timer_          = nullptr;

        void setMuteIcon(bool muted);
};
