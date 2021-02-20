#include "Manager.h"

#include <QRegularExpression>
#include <QTextDocumentFragment>

#include "EventAccessors.h"
#include "Utils.h"

QString
NotificationsManager::formatNotification(const mtx::events::collections::TimelineEvents &e)
{
        return QTextDocumentFragment::fromHtml(
                 mtx::accessors::formattedBodyWithFallback(e).replace(
                   QRegularExpression("(<mx-reply>.+\\<\\/mx-reply\\>)"), ""))
          .toPlainText();
}
