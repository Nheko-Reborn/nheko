#pragma once

#include <QEvent>
#include <QFrame>
#include <QImage>
#include <QLabel>

#include "Cache.h"

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
        bool eventFilter(QObject *obj, QEvent *event)
        {
                if (event->type() == QEvent::MouseButtonRelease) {
                        emit clicked();
                        return true;
                }

                return QObject::eventFilter(obj, event);
        }
};

/// Convenience class which connects events emmited from threads
/// outside of main with the UI code.
class ThreadProxy : public QObject
{
        Q_OBJECT

signals:
        void error(const QString &msg);
        void avatarChanged(const QImage &img);
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
        void topicEventSent()
        {
                errorField_->hide();
                close();
        }

        void nameEventSent(const QString &name)
        {
                errorField_->hide();
                emit nameChanged(name);
                close();
        }

        void error(const QString &msg)
        {
                errorField_->setText(msg);
                errorField_->show();
        }

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

class RoomSettings : public QFrame
{
        Q_OBJECT
public:
        RoomSettings(const QString &room_id, QWidget *parent = nullptr);

signals:
        void enableEncryptionError(const QString &msg);
        void showErrorMessage(const QString &msg);
        void accessRulesUpdated();

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

        void setAvatar(const QImage &img);
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

        QComboBox *accessCombo     = nullptr;
        Toggle *encryptionToggle_  = nullptr;
        Toggle *keyRequestsToggle_ = nullptr;
};

} // dialogs
