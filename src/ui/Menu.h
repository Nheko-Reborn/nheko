#pragma once

#include <QMenu>

#include "Config.h"

class Menu : public QMenu
{
        Q_OBJECT
public:
        Menu(QWidget *parent = nullptr)
          : QMenu(parent){};

protected:
        void leaveEvent(QEvent *e) override
        {
                hide();

                QMenu::leaveEvent(e);
        }
};
