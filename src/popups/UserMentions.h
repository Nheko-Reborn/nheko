#pragma once

#include <mtx/responses.hpp>

#include <QMap>
#include <QPaintEvent>
#include <QScrollArea>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>


namespace popups {

class UserMentions : public QWidget
{
        Q_OBJECT
public:
        UserMentions(QWidget *parent = nullptr);

        void initializeMentions(const QMap<QString, mtx::responses::Notifications> &notifs);
        void showPopup();

protected:
        void paintEvent(QPaintEvent *) override;

private:
        void pushItem(const QString &event_id,
                      const QString &user_id,
                      const QString &body,
                      const QString &room_id,
                      const QString &current_room_id);
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
