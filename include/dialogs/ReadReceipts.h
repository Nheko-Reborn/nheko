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
        ReceiptItem(QWidget *parent, const QString &user_id, uint64_t timestamp);

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
        void addUsers(const std::multimap<uint64_t, std::string> &users);

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        QLabel *topLabel_;

        QListWidget *userList_;
};
} // dialogs
