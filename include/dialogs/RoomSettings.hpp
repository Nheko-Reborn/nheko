#pragma once

#include <QFrame>
#include <QImage>

#include "Cache.h"

class FlatButton;
class TextField;
class QHBoxLayout;
class Avatar;
class QPixmap;
class QLayout;
class QLabel;
class QComboBox;
class TextField;
class QLabel;

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

class TopSection : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)

public:
        TopSection(const RoomInfo &info, const QImage &img, QWidget *parent = nullptr);
        QSize sizeHint() const override;
        void setRoomName(const QString &name);

        QColor textColor() const { return textColor_; }
        void setTextColor(QColor &color) { textColor_ = color; }

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        static constexpr int AvatarSize = 72;
        static constexpr int Padding    = 5;

        RoomInfo info_;
        QPixmap avatar_;
        QColor textColor_;
};

namespace dialogs {

class RoomSettings : public QFrame
{
        Q_OBJECT
public:
        RoomSettings(const QString &room_id, QWidget *parent = nullptr);

signals:
        void closing();

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

        //! Whether the user would be able to change the name or the topic of the room.
        bool hasEditRights_ = true;
        QHBoxLayout *editLayout_;

        // Button section
        FlatButton *saveBtn_;
        FlatButton *cancelBtn_;

        FlatButton *editFieldsBtn_;

        RoomInfo info_;
        QString room_id_;
        QImage avatarImg_;

        TopSection *topSection_;

        QComboBox *accessCombo;
};

} // dialogs
