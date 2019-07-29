#pragma once

#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
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
                      const QString &room_id,
                      const QString &current_room_id);

private:
        QTabWidget *tab_layout_;
        QVBoxLayout *top_layout_;
        QVBoxLayout *local_scroll_layout_;
        QVBoxLayout *all_scroll_layout_;

        QScrollArea *local_scroll_area_;
        QWidget *local_scroll_widget_;

        QScrollArea *all_scroll_area_;
        QWidget *all_scroll_widget_;
};

}