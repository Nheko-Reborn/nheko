#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "../ui/FlatButton.h"
#include "../ui/TextLabel.h"

struct RelatedInfo;
class UserItem;

class ReplyPopup : public QWidget
{
        Q_OBJECT

public:
        explicit ReplyPopup(QWidget *parent = nullptr);

public slots:
        void setReplyContent(const RelatedInfo &related);

protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

signals:
        void userSelected(const QString &user);
        void clicked(const QString &text);
        void cancel();

private:
        QHBoxLayout *topLayout_;
        QVBoxLayout *mainLayout_;
        QHBoxLayout *buttonLayout_;

        UserItem *userItem_;
        FlatButton *closeBtn_;
        TextLabel *msgLabel_;
        QLabel *eventLabel_;

        int buttonSize_;
};
