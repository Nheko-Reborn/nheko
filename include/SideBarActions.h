#pragma once

#include <QAction>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QWidget>

#include "FlatButton.h"
#include "Menu.h"

namespace mtx {
namespace requests {
struct CreateRoom;
}
}

class SideBarActions : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)

public:
        SideBarActions(QWidget *parent = nullptr);

        QColor borderColor() const { return borderColor_; }
        void setBorderColor(QColor &color) { borderColor_ = color; }

signals:
        void showSettings();
        void joinRoom(const QString &room);
        void createRoom(const mtx::requests::CreateRoom &request);

protected:
        void resizeEvent(QResizeEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

private:
        QHBoxLayout *layout_;

        Menu *addMenu_;
        QAction *createRoomAction_;
        QAction *joinRoomAction_;

        FlatButton *settingsBtn_;
        FlatButton *createRoomBtn_;
        FlatButton *joinRoomBtn_;

        QColor borderColor_;
};
