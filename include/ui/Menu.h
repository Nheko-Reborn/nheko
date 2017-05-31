#pragma once

#include <QMenu>

class Menu : public QMenu
{
public:
	Menu(QWidget *parent = nullptr)
	    : QMenu(parent)
	{
		setFont(QFont("Open Sans", 10));
		setStyleSheet(
			"QMenu { color: black; background-color: white; margin: 0px;}"
			"QMenu::item { color: black; padding: 7px 20px; border: 1px solid transparent; margin: 2px 0px; }"
			"QMenu::item:selected { color: black; background: rgba(180, 180, 180, 100); }");
	};

protected:
	void leaveEvent(QEvent *e)
	{
		Q_UNUSED(e);

		hide();
	}
};
