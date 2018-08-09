#pragma once

#include <QFrame>
#include <QImage>

#include "Cache.h"

class Avatar;
class FlatButton;
class QComboBox;
class QHBoxLayout;
class QShowEvent;
class LoadingIndicator;
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
        void showErrorMessage(const QString &msg);
        void accessRulesUpdated();

protected:
        void paintEvent(QPaintEvent *event) override;
        void showEvent(QShowEvent *event) override;

private:
        //! Whether the user has enough power level to send m.room.join_rules events.
        bool canChangeJoinRules(const std::string &room_id, const std::string &user_id) const;
        //! Whether the user has enough power level to send m.room.name & m.room.topic events.
        bool canChangeNameAndTopic(const std::string &room_id, const std::string &user_id) const;
        void updateAccessRules(const std::string &room_id,
                               const mtx::events::state::JoinRules &,
                               const mtx::events::state::GuestAccess &);
        void stopLoadingSpinner();
        void startLoadingSpinner();
        void resetErrorLabel();

        void setAvatar(const QImage &img) { avatarImg_ = img; }
        void setupEditButton();
        //! Retrieve the current room information from cache.
        void retrieveRoomInfo();
        void enableEncryption();

        Avatar *avatar_;

        bool usesEncryption_ = false;
        QHBoxLayout *btnLayout_;

        FlatButton *editFieldsBtn_;

        RoomInfo info_;
        QString room_id_;
        QImage avatarImg_;

        QLabel *errorLabel_;
        LoadingIndicator *spinner_;

        QComboBox *accessCombo;
        Toggle *encryptionToggle_;
        Toggle *keyRequestsToggle_;
};

} // dialogs
