#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QWidget>

#include "../AvatarProvider.h"
#include "../Cache.h"
#include "../ChatPage.h"
#include "PopupItem.h"

class ReplyPopup : public QWidget
{
        Q_OBJECT

public:
        explicit ReplyPopup(QWidget *parent = nullptr);

public slots:
        void setReplyContent(const QString &user, const QString &msg, const QString &srcEvent);

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void userSelected(const QString &user);

private:
        QVBoxLayout *layout_;

};
