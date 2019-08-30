#include "TimelineViewManager.h"

TimelineViewManager::TimelineViewManager(QWidget *parent)
{
        view      = new QQuickView();
        container = QWidget::createWindowContainer(view, parent);
        container->setMinimumSize(200, 200);
        view->setSource(QUrl("qrc:///qml/TimelineView.qml"));
        // view->rootContext()->setContextProperty(room);
}
