#ifndef UI_OVERLAY_WIDGET_H
#define UI_OVERLAY_WIDGET_H

#include <QEvent>
#include <QObject>
#include <QWidget>

class OverlayWidget : public QWidget
{
	Q_OBJECT

public:
	explicit OverlayWidget(QWidget *parent = 0);
	~OverlayWidget();

protected:
	bool event(QEvent *event) override;
	bool eventFilter(QObject *obj, QEvent *event) override;

	QRect overlayGeometry() const;
};

#endif  // UI_OVERLAY_WIDGET_H
