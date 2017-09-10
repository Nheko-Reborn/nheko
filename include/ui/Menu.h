#pragma once

#include <QMenu>

#include "Config.h"

class Menu : public QMenu
{
public:
        Menu(QWidget *parent = nullptr)
          : QMenu(parent)
        {
                QFont font;
                font.setPixelSize(conf::fontSize);

                setFont(font);
                setStyleSheet(
                  "QMenu { color: black; background-color: white; margin: 0px;}"
                  "QMenu::item { color: black; padding: 7px 20px; border: 1px solid transparent; "
                  "margin: "
                  "2px 0px; }"
                  "QMenu::item:selected { color: black; background: rgba(180, 180, 180, 100); }");
        };

protected:
        void leaveEvent(QEvent *e)
        {
                Q_UNUSED(e);

                hide();
        }
};
