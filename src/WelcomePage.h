#pragma once

#include <QWidget>

class WelcomePage : public QWidget
{
        Q_OBJECT

public:
        explicit WelcomePage(QWidget *parent = nullptr);

protected:
        void paintEvent(QPaintEvent *) override;

signals:
        // Notify that the user wants to login in.
        void userLogin();

        // Notify that the user wants to register.
        void userRegister();
};
