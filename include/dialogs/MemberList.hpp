#pragma once

#include <QFrame>
#include <QListWidget>

class Avatar;
class FlatButton;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;

struct RoomMember;

template<class T>
class QSharedPointer;

namespace dialogs {

class MemberItem : public QWidget
{
        Q_OBJECT

public:
        MemberItem(const RoomMember &member, QWidget *parent);

private:
        QHBoxLayout *topLayout_;
        QVBoxLayout *textLayout_;

        Avatar *avatar_;

        QLabel *userName_;
        QLabel *userId_;
};

class MemberList : public QFrame
{
        Q_OBJECT
public:
        MemberList(const QString &room_id, QWidget *parent = nullptr);

public slots:
        void addUsers(const std::vector<RoomMember> &users);

protected:
        void paintEvent(QPaintEvent *event) override;
        void hideEvent(QHideEvent *event) override
        {
                list_->clear();
                QFrame::hideEvent(event);
        }

private:
        void moveButtonToBottom();

        QString room_id_;
        QLabel *topLabel_;
        QListWidget *list_;
        FlatButton *moreBtn_;
};
} // dialogs
