#include "Manager.h"

#include <QTextDocumentFragment>

#include "Utils.h"

QString
NotificationsManager::formatNotification(const QString &text)
{
        return utils::markdownToHtml(text);
}
