#pragma once

#include <QFont>
#include <QFontDatabase>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

#include "nlohmann/json.hpp"

#include "Logging.h"
#include "MainWindow.h"
#include "ui/FlatButton.h"

namespace dialogs {

class RawMessage : public QWidget
{
        Q_OBJECT
public:
        RawMessage(QString msg, QWidget *parent = nullptr)
          : QWidget{parent}
        {
                QFont monospaceFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

                auto layout = new QVBoxLayout{this};
                auto viewer = new QTextBrowser{this};
                viewer->setFont(monospaceFont);
                viewer->setText(msg);

                layout->setSpacing(0);
                layout->setMargin(0);
                layout->addWidget(viewer);

                setAutoFillBackground(true);
                setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
                setAttribute(Qt::WA_DeleteOnClose, true);

                QSize winsize;
                QPoint center;

                auto window = MainWindow::instance();
                if (window) {
                        winsize = window->frameGeometry().size();
                        center  = window->frameGeometry().center();

                        move(center.x() - (width() * 0.5), center.y() - (height() * 0.5));
                } else {
                        nhlog::ui()->warn("unable to retrieve MainWindow's size");
                }

                raise();
                show();
        }
};
} // namespace dialogs
