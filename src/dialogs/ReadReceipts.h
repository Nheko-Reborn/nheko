#pragma once

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

class Avatar;

namespace dialogs {

class ReceiptItem : public QWidget
{
        Q_OBJECT

public:
        ReceiptItem(QWidget *parent,
                    const QString &user_id,
                    uint64_t timestamp,
                    const QString &room_id);

protected:
        void paintEvent(QPaintEvent *);

private:
        QString dateFormat(const QDateTime &then) const;

        QHBoxLayout *topLayout_;
        QVBoxLayout *textLayout_;

        Avatar *avatar_;

        QLabel *userName_;
        QLabel *timestamp_;
};

class ReadReceipts : public QFrame
{
        Q_OBJECT
public:
        explicit ReadReceipts(QWidget *parent = nullptr);

public slots:
        void addUsers(const std::multimap<uint64_t, std::string, std::greater<uint64_t>> &users);

protected:
        void paintEvent(QPaintEvent *event) override;
        void hideEvent(QHideEvent *event) override
        {
                userList_->clear();
                QFrame::hideEvent(event);
        }

private:
        QLabel *topLabel_;

        QListWidget *userList_;
};
} // dialogs
