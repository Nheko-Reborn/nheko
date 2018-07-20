#pragma once

#include <QString>
#include <QWidget>

class Avatar;
class FlatButton;
class QLabel;
class QListWidget;

namespace dialogs {

class DeviceItem : public QWidget
{
        Q_OBJECT

public:
        explicit DeviceItem(QWidget *parent, QString deviceName);

private:
        QString name_;

        // Toggle *verifyToggle_;
};

class UserProfile : public QWidget
{
        Q_OBJECT
public:
        explicit UserProfile(QWidget *parent = nullptr);

        void init(const QString &userId, const QString &roomId);

protected:
        void paintEvent(QPaintEvent *) override;

private:
        Avatar *avatar_;

        QString displayName_;
        QString userId_;

        QLabel *userIdLabel_;
        QLabel *displayNameLabel_;

        FlatButton *banBtn_;
        FlatButton *kickBtn_;
        FlatButton *ignoreBtn_;
        FlatButton *startChat_;

        QListWidget *devices_;
};
} // dialogs
