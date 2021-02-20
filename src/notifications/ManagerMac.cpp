#include "Manager.h"

#include <QTextDocumentFragment>

#include "EventAccessors.h"
#include "Utils.h"

QString
NotificationsManager::formatNotification(const mtx::events::collections::TimelineEvents &e)
{
        return QTextDocumentFragment::fromHtml(mtx::accessors::formattedBodyWithFallback(e)).toPlainText();
}
