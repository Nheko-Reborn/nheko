// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QQuickItem>

class NhekoDropArea : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString roomid READ roomid WRITE setRoomid NOTIFY roomidChanged)
public:
    NhekoDropArea(QQuickItem *parent = nullptr);

signals:
    void roomidChanged(QString roomid);

public slots:
    void setRoomid(QString roomid)
    {
        if (roomid_ != roomid) {
            roomid_ = roomid;
            emit roomidChanged(roomid);
        }
    }
    QString roomid() const { return roomid_; }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QString roomid_;
};
