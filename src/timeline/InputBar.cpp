#include "InputBar.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

#include "Logging.h"

bool
InputBar::paste(bool fromMouse)
{
        const QMimeData *md = nullptr;

        if (fromMouse) {
                if (QGuiApplication::clipboard()->supportsSelection()) {
                        md = QGuiApplication::clipboard()->mimeData(QClipboard::Selection);
                }
        } else {
                md = QGuiApplication::clipboard()->mimeData(QClipboard::Clipboard);
        }

        if (!md)
                return false;

        if (md->hasImage()) {
                return true;
        } else {
                nhlog::ui()->debug("formats: {}", md->formats().join(", ").toStdString());
                return false;
        }
}

void
InputBar::updateState(int selectionStart_, int selectionEnd_, int cursorPosition_, QString text_)
{
        selectionStart = selectionStart_;
        selectionEnd   = selectionEnd_;
        cursorPosition = cursorPosition_;
        text           = text_;
}

void
InputBar::send()
{
        nhlog::ui()->debug("Send: {}", text.toStdString());
}
