#pragma once

#include <QFrame>
#include <QLabel>
#include <QListWidgetItem>
#include <QStringList>

class FlatButton;
class TextField;
class QListWidget;

namespace dialogs {

class InviteUsers : public QFrame
{
        Q_OBJECT
public:
        explicit InviteUsers(QWidget *parent = nullptr);

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void closing(bool isLeaving, QStringList invitees);

private slots:
        void removeInvitee(QListWidgetItem *item);

private:
        void addUser();
        QStringList invitedUsers() const;

        FlatButton *confirmBtn_;
        FlatButton *cancelBtn_;

        TextField *inviteeInput_;
        QLabel *errorLabel_;

        QListWidget *inviteeList_;
};
} // dialogs
