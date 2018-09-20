#pragma once

#include <QWidget>

class QPushButton;

namespace dialogs {

class ReCaptcha : public QWidget
{
        Q_OBJECT

public:
        ReCaptcha(const QString &session, QWidget *parent = nullptr);

signals:
        void confirmation();

private:
        QPushButton *openCaptchaBtn_;
        QPushButton *confirmBtn_;
        QPushButton *cancelBtn_;
};
} // dialogs
