#include "Manager.h"

#include <QTextDocumentFragment>

#include "Utils.h"

QString
NotificationsManager::formatNotification(const QString &text)
{
        return QTextDocumentFragment::fromHtml(utils::markdownToHtml(text)).toPlainText();
}
