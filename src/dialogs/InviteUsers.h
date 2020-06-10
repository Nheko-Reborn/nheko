#pragma once

#include <QFrame>
#include <QStringList>

class QPushButton;
class QLabel;
class TextField;
class QListWidget;
class QListWidgetItem;

namespace dialogs {

class InviteUsers : public QFrame
{
        Q_OBJECT
public:
        explicit InviteUsers(QWidget *parent = nullptr);

protected:
        void showEvent(QShowEvent *event) override;

signals:
        void sendInvites(QStringList invitees);

private slots:
        void removeInvitee(QListWidgetItem *item);

private:
        void addUser();
        QStringList invitedUsers() const;

        QPushButton *confirmBtn_;
        QPushButton *cancelBtn_;

        TextField *inviteeInput_;
        QLabel *errorLabel_;

        QListWidget *inviteeList_;
};
} // dialogs
