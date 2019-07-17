#include <QTimer>

#include "UserMentions.h"
#include "timeline/TimelineItem.h"

using namespace dialogs;

UserMentions::UserMentions(QWidget *parent)
          : QWidget{parent}
{
        top_layout_ = new QVBoxLayout(this);
        top_layout_->setSpacing(0);
        top_layout_->setMargin(0);

        scroll_area_ = new QScrollArea(this);
        scroll_area_->setWidgetResizable(true);
        scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        scroll_widget_ = new QWidget(this);
        scroll_widget_->setObjectName("scroll_widget");

        // Height of the typing display.
        QFont f;
        f.setPointSizeF(f.pointSizeF() * 0.9);
        const int bottomMargin = QFontMetrics(f).height() + 6;

        scroll_layout_ = new QVBoxLayout(scroll_widget_);
        scroll_layout_->setContentsMargins(4, 0, 15, bottomMargin);
        scroll_layout_->setSpacing(0);
        scroll_layout_->setObjectName("timelinescrollarea");

        scroll_area_->setWidget(scroll_widget_);
        scroll_area_->setAlignment(Qt::AlignBottom);

        top_layout_->addWidget(scroll_area_);

        setLayout(top_layout_);
}

void
UserMentions::pushItem(const QString &event_id, const QString &user_id, const QString &body, const QString &room_id) {
        TimelineItem *view_item =
          new TimelineItem(mtx::events::MessageType::Text,
                           user_id,
                           body,
                           true,
                           room_id,
                           scroll_widget_);
                view_item->setEventId(event_id);
                setUpdatesEnabled(false);
                view_item->hide();

                scroll_layout_->addWidget(view_item);
                QTimer::singleShot(0, this, [view_item, this]() {
                        view_item->show();
                        view_item->adjustSize();
                        setUpdatesEnabled(true);
                });
}