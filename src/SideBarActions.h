#pragma once

#include <QAction>
#include <QHBoxLayout>
#include <QWidget>

namespace mtx {
namespace requests {
struct CreateRoom;
}
}

class Menu;
class FlatButton;
class QResizeEvent;

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
        // void showPublicRooms();
        void joinRoom(const QString &room);
        void createRoom(const mtx::requests::CreateRoom &request);
        // void roomDirectoryRooms(const mtx::requests::PublicRooms &request);

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
        FlatButton *roomDirectory_;

        QColor borderColor_;
};
