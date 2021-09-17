// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/TextLabel.h"

#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QEvent>
#include <QWheelEvent>

#include "Utils.h"

bool
ContextMenuFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        emit contextMenuIsOpening();
        return true;
    }

    return QObject::eventFilter(obj, event);
}

TextLabel::TextLabel(QWidget *parent)
  : TextLabel(QString(), parent)
{}

TextLabel::TextLabel(const QString &text, QWidget *parent)
  : QTextBrowser(parent)
{
    document()->setDefaultStyleSheet(QString("a {color: %1; }").arg(utils::linkColor()));

    setText(text);
    setOpenExternalLinks(true);

    // Make it look and feel like an ordinary label.
    setReadOnly(true);
    setFrameStyle(QFrame::NoFrame);
    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    setPalette(pal);

    // Wrap anywhere but prefer words, adjust minimum height on the fly.
    setLineWrapMode(QTextEdit::WidgetWidth);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    connect(document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            this,
            &TextLabel::adjustHeight);
    document()->setDocumentMargin(0);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setFixedHeight(0);

    connect(this, &TextLabel::linkActivated, this, &TextLabel::handleLinkActivation);

    auto filter = new ContextMenuFilter(this);
    installEventFilter(filter);
    connect(filter, &ContextMenuFilter::contextMenuIsOpening, this, [this]() {
        contextMenuRequested_ = true;
    });
}

void
TextLabel::focusOutEvent(QFocusEvent *e)
{
    QTextBrowser::focusOutEvent(e);

    // We keep the selection available for the context menu.
    if (contextMenuRequested_) {
        contextMenuRequested_ = false;
        return;
    }

    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
}

void
TextLabel::mousePressEvent(QMouseEvent *e)
{
    link_ = (e->button() & Qt::LeftButton) ? anchorAt(e->pos()) : QString();
    QTextBrowser::mousePressEvent(e);
}

void
TextLabel::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() & Qt::LeftButton && !link_.isEmpty() && anchorAt(e->pos()) == link_) {
        emit linkActivated(link_);
        return;
    }

    QTextBrowser::mouseReleaseEvent(e);
}

void
TextLabel::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}

void
TextLabel::handleLinkActivation(const QUrl &url)
{
    auto parts          = url.toString().split('/');
    auto defaultHandler = [](const QUrl &url) { QDesktopServices::openUrl(url); };

    if (url.host() != "matrix.to" || parts.isEmpty())
        return defaultHandler(url);

    try {
        using namespace mtx::identifiers;
        parse<User>(parts.last().toStdString());
    } catch (const std::exception &) {
        return defaultHandler(url);
    }

    emit userProfileTriggered(parts.last());
}
