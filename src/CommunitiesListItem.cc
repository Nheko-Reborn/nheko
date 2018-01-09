#include "CommunitiesListItem.h"

CommunitiesListItem::CommunitiesListItem(QSharedPointer<Community> community,
                                         QString community_id,
                                         QWidget *parent)
  : QWidget(parent)
  , community_(community)
  , communityId_(community_id)
{
        // menu_ = new Menu(this);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFixedHeight(ui::sidebar::CommunitiesSidebarSize);
        setFixedWidth(ui::sidebar::CommunitiesSidebarSize);
}

CommunitiesListItem::~CommunitiesListItem() {}

void
CommunitiesListItem::setCommunity(QSharedPointer<Community> community)
{
        community_ = community;
}

void
CommunitiesListItem::setPressedState(bool state)
{
        if (isPressed_ != state) {
                isPressed_ = state;
                update();
        }
}

void
CommunitiesListItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() == Qt::RightButton) {
                QWidget::mousePressEvent(event);
                return;
        }

        emit clicked(communityId_);

        setPressedState(true);
}

void
CommunitiesListItem::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::Antialiasing);

        if (isPressed_)
                p.fillRect(rect(), highlightedBackgroundColor_);
        else if (underMouse())
                p.fillRect(rect(), hoverBackgroundColor_);
        else
                p.fillRect(rect(), backgroundColor_);

        QFont font;
        font.setPixelSize(conf::fontSize);

        p.setPen(QColor("#333"));

        QRect avatarRegion((width() - IconSize) / 2, (height() - IconSize) / 2, IconSize, IconSize);

        font.setBold(false);
        p.setPen(Qt::NoPen);

        // We using the first letter of room's name.
        if (communityAvatar_.isNull()) {
                QBrush brush;
                brush.setStyle(Qt::SolidPattern);
                brush.setColor("#eee");

                p.setPen(Qt::NoPen);
                p.setBrush(brush);

                p.drawEllipse(avatarRegion.center(), IconSize / 2, IconSize / 2);

                font.setPixelSize(conf::roomlist::fonts::bubble);
                p.setFont(font);
                p.setPen(QColor("#000"));
                p.setBrush(Qt::NoBrush);
                p.drawText(
                  avatarRegion.translated(0, -1), Qt::AlignCenter, QChar(community_->getName()[0]));
        } else {
                p.save();

                QPainterPath path;
                path.addEllipse(
                  (width() - IconSize) / 2, (height() - IconSize) / 2, IconSize, IconSize);
                p.setClipPath(path);

                p.drawPixmap(avatarRegion, communityAvatar_);
                p.restore();
        }

        // TODO: Discord-style community ping counts?
        /*if (unreadMsgCount_ > 0) {
                QColor textColor("white");
                QColor backgroundColor("#38A3D8");

                QBrush brush;
                brush.setStyle(Qt::SolidPattern);
                brush.setColor(backgroundColor);

                if (isPressed_)
                        brush.setColor(textColor);

                QFont unreadCountFont;
                unreadCountFont.setPixelSize(conf::roomlist::fonts::badge);
                unreadCountFont.setBold(true);

                p.setBrush(brush);
                p.setPen(Qt::NoPen);
                p.setFont(unreadCountFont);

                int diameter = 20;

                QRectF r(
                  width() - diameter - 5, height() - diameter - 5, diameter, diameter);

                p.setPen(Qt::NoPen);
                p.drawEllipse(r);

                p.setPen(QPen(textColor));

                if (isPressed_)
                        p.setPen(QPen(backgroundColor));

                p.setBrush(Qt::NoBrush);
                p.drawText(
                  r.translated(0, -0.5), Qt::AlignCenter, QString::number(unreadMsgCount_));
        }*/
}

void
CommunitiesListItem::contextMenuEvent(QContextMenuEvent *event)
{
        Q_UNUSED(event);

        // menu_->popup(event->globalPos());
}

WorldCommunityListItem::WorldCommunityListItem(QWidget *parent)
  : CommunitiesListItem(QSharedPointer<Community>(), "", parent)
{}

WorldCommunityListItem::~WorldCommunityListItem() {}

void
WorldCommunityListItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() == Qt::RightButton) {
                QWidget::mousePressEvent(event);
                return;
        }

        emit CommunitiesListItem::clicked("world");

        setPressedState(true);
}

void
WorldCommunityListItem::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        static QPixmap worldIcon(":/icons/icons/ui/world.png");

        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::Antialiasing);

        if (isPressed())
                p.fillRect(rect(), highlightedBackgroundColor_);
        else if (underMouse())
                p.fillRect(rect(), hoverBackgroundColor_);
        else
                p.fillRect(rect(), backgroundColor_);

        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        brush.setColor("#FFFFFF");

        p.setPen(Qt::NoPen);
        p.setBrush(brush);

        QRect avatarRegion((width() - IconSize) / 2, (height() - IconSize) / 2, IconSize, IconSize);
        p.drawEllipse(avatarRegion.center(), IconSize / 2, IconSize / 2);
        QPainterPath path;
        path.addEllipse((width() - IconSize) / 2, (height() - IconSize) / 2, IconSize, IconSize);
        p.setClipPath(path);

        p.drawPixmap(avatarRegion, worldIcon);
}
