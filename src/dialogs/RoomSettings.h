#pragma once

#include <QFrame>
#include <QImage>

#include "Cache.h"

class Avatar;
class FlatButton;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QLabel;
class QLayout;
class QPixmap;
class TextField;
class TextField;
class Toggle;

template<class T>
class QSharedPointer;

class EditModal : public QWidget
{
        Q_OBJECT

public:
        EditModal(const QString &roomId, QWidget *parent = nullptr);

        void setFields(const QString &roomName, const QString &roomTopic);

signals:
        void nameChanged(const QString &roomName);
        void nameEventSentCb(const QString &newName);
        void topicEventSentCb();
        void stateEventErrorCb(const QString &msg);

private:
        QString roomId_;
        QString initialName_;
        QString initialTopic_;

        QLabel *errorField_;

        TextField *nameInput_;
        TextField *topicInput_;

        FlatButton *applyBtn_;
        FlatButton *cancelBtn_;
};

namespace dialogs {

class RoomSettings : public QFrame
{
        Q_OBJECT
public:
        RoomSettings(const QString &room_id, QWidget *parent = nullptr);

signals:
        void closing();
        void enableEncryptionError(const QString &msg);

protected:
        void paintEvent(QPaintEvent *event) override;

private slots:
        void saveSettings();

private:
        static constexpr int AvatarSize = 64;

        void setAvatar(const QImage &img) { avatarImg_ = img; }
        void setupEditButton();
        //! Retrieve the current room information from cache.
        void retrieveRoomInfo();
        void enableEncryption();

        Avatar *avatar_;

        //! Whether the user would be able to change the name or the topic of the room.
        bool hasEditRights_  = true;
        bool usesEncryption_ = false;
        QHBoxLayout *btnLayout_;

        FlatButton *editFieldsBtn_;

        RoomInfo info_;
        QString room_id_;
        QImage avatarImg_;

        QComboBox *accessCombo;
        Toggle *encryptionToggle_;
};

} // dialogs
