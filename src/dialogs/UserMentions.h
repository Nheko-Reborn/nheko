#pragma once

#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>

namespace dialogs {

class UserMentions : public QWidget
{
        Q_OBJECT
public:
        UserMentions(QWidget *parent = nullptr);
        void pushItem(const QString &event_id,
                      const QString &user_id,
                      const QString &body,
                      const QString &room_id);

private:
        QVBoxLayout *top_layout_;
        QVBoxLayout *scroll_layout_;

        QScrollArea *scroll_area_;
        QWidget *scroll_widget_;
};

}