#include <QDateTime>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QtGlobal>

#include "MainWindow.h"
#include "UserMentionsWidget.h"
#include "Utils.h"
#include "ui/Ripple.h"
#include "ui/RippleOverlay.h"

constexpr int MaxUnreadCountDisplayed = 99;

struct WMetrics
{
        int maxHeight;
        int iconSize;
        int padding;
        int unit;

        int unreadLineWidth;
        int unreadLineOffset;

        int inviteBtnX;
        int inviteBtnY;
};

WMetrics
getWMetrics(const QFont &font)
{
        WMetrics m;

        const int height = QFontMetrics(font).lineSpacing();

        m.unit             = height;
        m.maxHeight        = std::ceil((double)height * 3.8);
        m.iconSize         = std::ceil((double)height * 2.8);
        m.padding          = std::ceil((double)height / 2.0);
        m.unreadLineWidth  = m.padding - m.padding / 3;
        m.unreadLineOffset = m.padding - m.padding / 4;

        m.inviteBtnX = m.iconSize + 2 * m.padding;
        m.inviteBtnX = m.iconSize / 2.0 + m.padding + m.padding / 3.0;

        return m;
}

UserMentionsWidget::UserMentionsWidget(QWidget *parent)
  : QWidget(parent)
  , isPressed_(false)
  , unreadMsgCount_(0)
{
        init(parent);

        QFont f;
        f.setPointSizeF(f.pointSizeF());

        const int fontHeight    = QFontMetrics(f).height();
        const int widgetMargin  = fontHeight / 3;
        const int contentHeight = fontHeight * 3;

        setFixedHeight(contentHeight + widgetMargin);

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(widgetMargin);
}

void
UserMentionsWidget::init(QWidget *parent)
{
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);

        setFixedHeight(getWMetrics(QFont{}).maxHeight);

        QPainterPath path;
        path.addRect(0, 0, parent->width(), height());

        ripple_overlay_ = new RippleOverlay(this);
        ripple_overlay_->setClipPath(path);
        ripple_overlay_->setClipping(true);

        unreadCountFont_.setPointSizeF(unreadCountFont_.pointSizeF() * 0.8);
        unreadCountFont_.setBold(true);

        bubbleDiameter_ = QFontMetrics(unreadCountFont_).averageCharWidth() * 3;
}

// void
// UserMentionsWidget::resizeEvent(QResizeEvent *event)
// {
//         Q_UNUSED(event);

//         const auto sz = utils::calculateSidebarSizes(QFont{});

//         if (width() <= sz.small) {
//                 topLayout_->setContentsMargins(0, 0, logoutButtonSize_, 0);

//         } else {
//                 topLayout_->setMargin(5);
//         }

//         QWidget::resizeEvent(event);
// }

void
UserMentionsWidget::setPressedState(bool state)
{
        if (isPressed_ != state) {
                isPressed_ = state;
                update();
        }
}

void
UserMentionsWidget::resizeEvent(QResizeEvent *)
{
        // Update ripple's clipping path.
        QPainterPath path;
        path.addRect(0, 0, width(), height());

        const auto sidebarSizes = utils::calculateSidebarSizes(QFont{});

        if (width() > sidebarSizes.small)
                setToolTip("");
        else
                setToolTip("");

        ripple_overlay_->setClipPath(path);
        ripple_overlay_->setClipping(true);
}

void
UserMentionsWidget::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() == Qt::RightButton) {
                QWidget::mousePressEvent(event);
                return;
        }

        emit clicked();

        setPressedState(true);

        // Ripple on mouse position by default.
        QPoint pos           = event->pos();
        qreal radiusEndValue = static_cast<qreal>(width()) / 3;

        Ripple *ripple = new Ripple(pos);

        ripple->setRadiusEndValue(radiusEndValue);
        ripple->setOpacityStartValue(0.15);
        ripple->setColor(QColor("white"));
        ripple->radiusAnimation()->setDuration(200);
        ripple->opacityAnimation()->setDuration(400);

        ripple_overlay_->addRipple(ripple);
}

void
UserMentionsWidget::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::Antialiasing);

        auto wm = getWMetrics(QFont{});

        QPen titlePen(titleColor_);
        QPen subtitlePen(subtitleColor_);

        QFontMetrics metrics(QFont{});

        if (isPressed_) {
                p.fillRect(rect(), highlightedBackgroundColor_);
                titlePen.setColor(highlightedTitleColor_);
                subtitlePen.setColor(highlightedSubtitleColor_);
        } else if (underMouse()) {
                p.fillRect(rect(), hoverBackgroundColor_);
                titlePen.setColor(hoverTitleColor_);
                subtitlePen.setColor(hoverSubtitleColor_);
        } else {
                p.fillRect(rect(), backgroundColor_);
                titlePen.setColor(titleColor_);
                subtitlePen.setColor(subtitleColor_);
        }

        // Description line with the default font.
        int bottom_y = wm.maxHeight - wm.padding - metrics.ascent() / 2;

        const auto sidebarSizes = utils::calculateSidebarSizes(QFont{});

        if (width() > sidebarSizes.small) {
                QFont headingFont;
                headingFont.setWeight(QFont::Medium);
                p.setFont(headingFont);
                p.setPen(titlePen);

                QFont tsFont;
                tsFont.setPointSizeF(tsFont.pointSizeF() * 0.9);
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
                const int msgStampWidth = QFontMetrics(tsFont).width("timestamp") + 4;
#else
                const int msgStampWidth = QFontMetrics(tsFont).horizontalAdvance("timestamp") + 4;
#endif
                // We use the full width of the widget if there is no unread msg bubble.
                // const int bottomLineWidthLimit = (unreadMsgCount_ > 0) ? msgStampWidth : 0;

                // Name line.
                QFontMetrics fontNameMetrics(headingFont);
                int top_y = 2 * wm.padding + fontNameMetrics.ascent() / 2;

                const auto name = metrics.elidedText(
                  "Mentions",
                  Qt::ElideRight,
                  (width() - wm.iconSize - 2 * wm.padding - msgStampWidth) * 0.8);
                p.drawText(QPoint(2 * wm.padding + wm.iconSize, top_y), name);

                p.setFont(QFont{});
                p.setPen(subtitlePen);

                // The limit is the space between the end of the avatar and the start of the
                // timestamp.
                int usernameLimit =
                  std::max(0, width() - 3 * wm.padding - msgStampWidth - wm.iconSize - 20);
                auto userName =
                  metrics.elidedText("Show Mentioned Messages", Qt::ElideRight, usernameLimit);

                p.setFont(QFont{});
                p.drawText(QPoint(2 * wm.padding + wm.iconSize, bottom_y), userName);

                // We show the last message timestamp.
                p.save();
                if (isPressed_) {
                        p.setPen(QPen(highlightedTimestampColor_));
                } else if (underMouse()) {
                        p.setPen(QPen(hoverTimestampColor_));
                } else {
                        p.setPen(QPen(timestampColor_));
                }

                // p.setFont(tsFont);
                // p.drawText(QPoint(width() - wm.padding - msgStampWidth, top_y), "timestamp");
                p.restore();
        }

        p.setPen(Qt::NoPen);

        if (unreadMsgCount_ > 0) {
                QBrush brush;
                brush.setStyle(Qt::SolidPattern);

                brush.setColor(mentionedColor());

                if (isPressed_)
                        brush.setColor(bubbleFgColor());

                p.setBrush(brush);
                p.setPen(Qt::NoPen);
                p.setFont(unreadCountFont_);

                // Extra space on the x-axis to accomodate the extra character space
                // inside the bubble.
                const int x_width = unreadMsgCount_ > MaxUnreadCountDisplayed
                                      ? QFontMetrics(p.font()).averageCharWidth()
                                      : 0;

                QRectF r(width() - bubbleDiameter_ - wm.padding - x_width,
                         bottom_y - bubbleDiameter_ / 2 - 5,
                         bubbleDiameter_ + x_width,
                         bubbleDiameter_);

                if (width() == sidebarSizes.small)
                        r = QRectF(width() - bubbleDiameter_ - 5,
                                   height() - bubbleDiameter_ - 5,
                                   bubbleDiameter_ + x_width,
                                   bubbleDiameter_);

                p.setPen(Qt::NoPen);
                p.drawEllipse(r);

                p.setPen(QPen(bubbleFgColor()));

                if (isPressed_)
                        p.setPen(QPen(bubbleBgColor()));

                auto countTxt = unreadMsgCount_ > MaxUnreadCountDisplayed
                                  ? QString("99+")
                                  : QString::number(unreadMsgCount_);

                p.setBrush(Qt::NoBrush);
                p.drawText(r.translated(0, -0.5), Qt::AlignCenter, countTxt);
        }

        if (!isPressed_ && hasUnreadMessages_) {
                QPen pen;
                pen.setWidth(wm.unreadLineWidth);
                pen.setColor(highlightedBackgroundColor_);

                p.setPen(pen);
                p.drawLine(0, wm.unreadLineOffset, 0, height() - wm.unreadLineOffset);
        }
}