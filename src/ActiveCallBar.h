#pragma once

#include <QWidget>

class QHBoxLayout;
class QLabel;
class QString;
class FlatButton;

class ActiveCallBar : public QWidget
{
        Q_OBJECT

public:
        ActiveCallBar(QWidget *parent = nullptr);

public slots:
        void setCallParty(const QString &userid, const QString &displayName);

private:
        QHBoxLayout *topLayout_ = nullptr;
        QLabel *callPartyLabel_ = nullptr;
        FlatButton *muteBtn_    = nullptr;
        int buttonSize_         = 32;
        bool muted_             = false;
};
