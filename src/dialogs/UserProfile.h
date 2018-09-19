#pragma once

#include <QString>
#include <QWidget>

class Avatar;
class FlatButton;
class QLabel;
class QListWidget;
class Toggle;

struct DeviceInfo
{
        QString device_id;
        QString display_name;
};

Q_DECLARE_METATYPE(std::vector<DeviceInfo>)

class Proxy : public QObject
{
        Q_OBJECT

signals:
        void done(const QString &user_id, const std::vector<DeviceInfo> &devices);
};

namespace dialogs {

class DeviceItem : public QWidget
{
        Q_OBJECT

public:
        explicit DeviceItem(DeviceInfo device, QWidget *parent);

private:
        DeviceInfo info_;

        // Toggle *verifyToggle_;
};

class UserProfile : public QWidget
{
        Q_OBJECT
public:
        explicit UserProfile(QWidget *parent = nullptr);

        void init(const QString &userId, const QString &roomId);

private slots:
        void updateDeviceList(const QString &user_id, const std::vector<DeviceInfo> &devices);

private:
        void resetToDefaults();

        Avatar *avatar_;

        QLabel *userIdLabel_;
        QLabel *displayNameLabel_;

        FlatButton *banBtn_;
        FlatButton *kickBtn_;
        FlatButton *ignoreBtn_;
        FlatButton *startChat_;

        QLabel *devicesLabel_;
        QListWidget *devices_;
};

} // dialogs
