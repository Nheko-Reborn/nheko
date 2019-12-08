#include <QPainter>
#include <QStyleOption>
#include <QTabWidget>
#include <QTimer>

#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "UserMentions.h"
//#include "timeline/TimelineItem.h"

using namespace popups;

UserMentions::UserMentions(QWidget *parent)
  : QWidget{parent}
{
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);

        tab_layout_ = new QTabWidget(this);

        top_layout_ = new QVBoxLayout(this);
        top_layout_->setSpacing(0);
        top_layout_->setMargin(0);

        local_scroll_area_ = new QScrollArea(this);
        local_scroll_area_->setWidgetResizable(true);
        local_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        local_scroll_widget_ = new QWidget(this);
        local_scroll_widget_->setObjectName("local_scroll_widget");

        all_scroll_area_ = new QScrollArea(this);
        all_scroll_area_->setWidgetResizable(true);
        all_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        all_scroll_widget_ = new QWidget(this);
        all_scroll_widget_->setObjectName("all_scroll_widget");

        // Height of the typing display.
        QFont f;
        f.setPointSizeF(f.pointSizeF() * 0.9);
        const int bottomMargin = QFontMetrics(f).height() + 6;

        local_scroll_layout_ = new QVBoxLayout(local_scroll_widget_);
        local_scroll_layout_->setContentsMargins(4, 0, 15, bottomMargin);
        local_scroll_layout_->setSpacing(0);
        local_scroll_layout_->setObjectName("localscrollarea");

        all_scroll_layout_ = new QVBoxLayout(all_scroll_widget_);
        all_scroll_layout_->setContentsMargins(4, 0, 15, bottomMargin);
        all_scroll_layout_->setSpacing(0);
        all_scroll_layout_->setObjectName("allscrollarea");

        local_scroll_area_->setWidget(local_scroll_widget_);
        local_scroll_area_->setAlignment(Qt::AlignBottom);

        all_scroll_area_->setWidget(all_scroll_widget_);
        all_scroll_area_->setAlignment(Qt::AlignBottom);

        tab_layout_->addTab(local_scroll_area_, tr("This Room"));
        tab_layout_->addTab(all_scroll_area_, tr("All Rooms"));
        top_layout_->addWidget(tab_layout_);

        setLayout(top_layout_);
}

void
UserMentions::initializeMentions(const QMap<QString, mtx::responses::Notifications> &notifs)
{
        nhlog::ui()->debug("Initializing " + std::to_string(notifs.size()) + " notifications.");

        for (const auto &item : notifs) {
                for (const auto notif : item.notifications) {
                        const auto event_id = QString::fromStdString(utils::event_id(notif.event));

                        try {
                                const auto room_id = QString::fromStdString(notif.room_id);
                                const auto user_id = utils::event_sender(notif.event);
                                const auto body    = utils::event_body(notif.event);

                                pushItem(event_id,
                                         user_id,
                                         body,
                                         room_id,
                                         ChatPage::instance()->currentRoom());

                        } catch (const lmdb::error &e) {
                                nhlog::db()->warn("error while sending desktop notification: {}",
                                                  e.what());
                        }
                }
        }
}

void
UserMentions::showPopup()
{
        for (auto widget : all_scroll_layout_->findChildren<QWidget *>()) {
                delete widget;
        }
        for (auto widget : local_scroll_layout_->findChildren<QWidget *>()) {
                delete widget;
        }

        auto notifs = cache::client()->getTimelineMentions();

        initializeMentions(notifs);
        show();
}

void
UserMentions::pushItem(const QString &event_id,
                       const QString &user_id,
                       const QString &body,
                       const QString &room_id,
                       const QString &current_room_id)
{
        (void)event_id;
        (void)user_id;
        (void)body;
        (void)room_id;
        (void)current_room_id;
        //        setUpdatesEnabled(false);
        //
        //        // Add to the 'all' section
        //        TimelineItem *view_item = new TimelineItem(
        //          mtx::events::MessageType::Text, user_id, body, true, room_id,
        //          all_scroll_widget_);
        //        view_item->setEventId(event_id);
        //        view_item->hide();
        //
        //        all_scroll_layout_->addWidget(view_item);
        //        QTimer::singleShot(0, this, [view_item, this]() {
        //                view_item->show();
        //                view_item->adjustSize();
        //                setUpdatesEnabled(true);
        //        });
        //
        //        // if it matches the current room... add it to the current room as well.
        //        if (QString::compare(room_id, current_room_id, Qt::CaseInsensitive) == 0) {
        //                // Add to the 'local' section
        //                TimelineItem *local_view_item = new
        //                TimelineItem(mtx::events::MessageType::Text,
        //                                                                 user_id,
        //                                                                 body,
        //                                                                 true,
        //                                                                 room_id,
        //                                                                 local_scroll_widget_);
        //                local_view_item->setEventId(event_id);
        //                local_view_item->hide();
        //                local_scroll_layout_->addWidget(local_view_item);
        //
        //                QTimer::singleShot(0, this, [local_view_item]() {
        //                        local_view_item->show();
        //                        local_view_item->adjustSize();
        //                });
        //        }
}

void
UserMentions::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
