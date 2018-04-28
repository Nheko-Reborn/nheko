#pragma once

#include <QFontMetrics>
#include <QPaintDevice>
#include <QPainter>

class Painter : public QPainter
{
public:
        explicit Painter(QPaintDevice *device)
          : QPainter(device)
        {}

        void drawTextLeft(int x, int y, const QString &text)
        {
                QFontMetrics m(fontMetrics());
                drawText(x, y + m.ascent(), text);
        }

        void drawTextRight(int x, int y, int outerw, const QString &text, int textWidth = -1)
        {
                QFontMetrics m(fontMetrics());
                if (textWidth < 0)
                        textWidth = m.width(text);
                drawText((outerw - x - textWidth), y + m.ascent(), text);
        }

        void drawPixmapLeft(int x, int y, const QPixmap &pix, const QRect &from)
        {
                drawPixmap(QPoint(x, y), pix, from);
        }

        void drawPixmapLeft(const QPoint &p, const QPixmap &pix, const QRect &from)
        {
                return drawPixmapLeft(p.x(), p.y(), pix, from);
        }

        void drawPixmapLeft(int x, int y, int w, int h, const QPixmap &pix, const QRect &from)
        {
                drawPixmap(QRect(x, y, w, h), pix, from);
        }

        void drawPixmapLeft(const QRect &r, const QPixmap &pix, const QRect &from)
        {
                return drawPixmapLeft(r.x(), r.y(), r.width(), r.height(), pix, from);
        }

        void drawPixmapLeft(int x, int y, int outerw, const QPixmap &pix)
        {
                Q_UNUSED(outerw);
                drawPixmap(QPoint(x, y), pix);
        }

        void drawPixmapLeft(const QPoint &p, int outerw, const QPixmap &pix)
        {
                return drawPixmapLeft(p.x(), p.y(), outerw, pix);
        }

        void drawPixmapRight(int x, int y, int outerw, const QPixmap &pix, const QRect &from)
        {
                drawPixmap(
                  QPoint((outerw - x - (from.width() / pix.devicePixelRatio())), y), pix, from);
        }

        void drawPixmapRight(const QPoint &p, int outerw, const QPixmap &pix, const QRect &from)
        {
                return drawPixmapRight(p.x(), p.y(), outerw, pix, from);
        }
        void drawPixmapRight(int x,
                             int y,
                             int w,
                             int h,
                             int outerw,
                             const QPixmap &pix,
                             const QRect &from)
        {
                drawPixmap(QRect((outerw - x - w), y, w, h), pix, from);
        }

        void drawPixmapRight(const QRect &r, int outerw, const QPixmap &pix, const QRect &from)
        {
                return drawPixmapRight(r.x(), r.y(), r.width(), r.height(), outerw, pix, from);
        }

        void drawPixmapRight(int x, int y, int outerw, const QPixmap &pix)
        {
                drawPixmap(QPoint((outerw - x - (pix.width() / pix.devicePixelRatio())), y), pix);
        }

        void drawPixmapRight(const QPoint &p, int outerw, const QPixmap &pix)
        {
                return drawPixmapRight(p.x(), p.y(), outerw, pix);
        }

        void drawAvatar(const QPixmap &pix, int w, int h, int d)
        {
                QPainterPath pp;
                pp.addEllipse((w - d) / 2, (h - d) / 2, d, d);

                QRect region((w - d) / 2, (h - d) / 2, d, d);

                setClipPath(pp);
                drawPixmap(region, pix);
        }

        void drawLetterAvatar(const QChar &c,
                              const QColor &penColor,
                              const QColor &brushColor,
                              int w,
                              int h,
                              int d)
        {
                QRect region((w - d) / 2, (h - d) / 2, d, d);

                setPen(Qt::NoPen);
                setBrush(brushColor);

                drawEllipse(region.center(), d / 2, d / 2);

                setBrush(Qt::NoBrush);
                drawEllipse(region.center(), d / 2, d / 2);

                setPen(penColor);
                drawText(region.translated(0, -1), Qt::AlignCenter, c);
        }
};

class PainterHighQualityEnabler
{
public:
        PainterHighQualityEnabler(Painter &p)
          : _painter(p)
        {
                static constexpr QPainter::RenderHint Hints[] = {QPainter::Antialiasing,
                                                                 QPainter::SmoothPixmapTransform,
                                                                 QPainter::TextAntialiasing,
                                                                 QPainter::HighQualityAntialiasing};

                auto hints = _painter.renderHints();
                for (const auto &hint : Hints) {
                        if (!(hints & hint))
                                hints_ |= hint;
                }

                if (hints_)
                        _painter.setRenderHints(hints_);
        }

        ~PainterHighQualityEnabler()
        {
                if (hints_)
                        _painter.setRenderHints(hints_, false);
        }

        PainterHighQualityEnabler(const PainterHighQualityEnabler &other) = delete;
        PainterHighQualityEnabler &operator=(const PainterHighQualityEnabler &other) = delete;

private:
        Painter &_painter;
        QPainter::RenderHints hints_ = 0;
};
