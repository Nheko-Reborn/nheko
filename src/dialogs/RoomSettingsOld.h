#pragma once

#include <QFrame>
#include <QImage>

#include <mtx/events/guest_access.hpp>

#include "CacheStructs.h"

class Avatar;
class FlatButton;
class QPushButton;
class QComboBox;
class QHBoxLayout;
class QShowEvent;
class LoadingIndicator;
class QLayout;
class QPixmap;
class TextField;
class TextField;
class Toggle;
class QLabel;
class QEvent;

class ClickableFilter : public QObject
{
        Q_OBJECT

public:
        explicit ClickableFilter(QWidget *parent)
          : QObject(parent)
        {}

signals:
        void clicked();

protected:
        bool eventFilter(QObject *obj, QEvent *event) override;
};

/// Convenience class which connects events emmited from threads
/// outside of main with the UI code.
class ThreadProxya : public QObject
{
        Q_OBJECT

signals:
        void error(const QString &msg);
        void avatarChanged();
        void nameEventSent(const QString &);
        void topicEventSent();
};

class EditModal : public QWidget
{
        Q_OBJECT

public:
        EditModal(const QString &roomId, QWidget *parent = nullptr);

        void setFields(const QString &roomName, const QString &roomTopic);

signals:
        void nameChanged(const QString &roomName);

private slots:
        void topicEventSent();
        void nameEventSent(const QString &name);
        void error(const QString &msg);

        void applyClicked();

private:
        QString roomId_;
        QString initialName_;
        QString initialTopic_;

        QLabel *errorField_;

        TextField *nameInput_;
        TextField *topicInput_;

        QPushButton *applyBtn_;
        QPushButton *cancelBtn_;
};

namespace dialogs {

class RoomSettingsOld : public QFrame
{
        Q_OBJECT
public:
        RoomSettingsOld(const QString &room_id, QWidget *parent = nullptr);

signals:
        void enableEncryptionError(const QString &msg);
        void showErrorMessage(const QString &msg);
        void accessRulesUpdated();
        void notifChanged(int index);

protected:
        void showEvent(QShowEvent *event) override;

private slots:
        //! The file dialog opens so the user can select and upload a new room avatar.
        void updateAvatar();

private:
        //! Whether the user has enough power level to send m.room.join_rules events.
        bool canChangeJoinRules(const std::string &room_id, const std::string &user_id) const;
        //! Whether the user has enough power level to send m.room.name & m.room.topic events.
        bool canChangeNameAndTopic(const std::string &room_id, const std::string &user_id) const;
        //! Whether the user has enough power level to send m.room.avatar event.
        bool canChangeAvatar(const std::string &room_id, const std::string &user_id) const;
        void updateAccessRules(const std::string &room_id,
                               const mtx::events::state::JoinRules &,
                               const mtx::events::state::GuestAccess &);
        void stopLoadingSpinner();
        void startLoadingSpinner();
        void resetErrorLabel();
        void displayErrorMessage(const QString &msg);

        void setAvatar();
        void setupEditButton();
        //! Retrieve the current room information from cache.
        void retrieveRoomInfo();
        void enableEncryption();

        Avatar *avatar_ = nullptr;

        bool usesEncryption_ = false;
        QHBoxLayout *btnLayout_;

        FlatButton *editFieldsBtn_ = nullptr;

        RoomInfo info_;
        QString room_id_;
        QImage avatarImg_;

        QLabel *roomNameLabel_     = nullptr;
        QLabel *errorLabel_        = nullptr;
        LoadingIndicator *spinner_ = nullptr;

        QComboBox *notifCombo      = nullptr;
        QComboBox *accessCombo     = nullptr;
        Toggle *encryptionToggle_  = nullptr;
        Toggle *keyRequestsToggle_ = nullptr;
};

} // dialogs
