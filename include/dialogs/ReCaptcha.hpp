#pragma once

#include <QWidget>

class FlatButton;
class RaisedButton;

namespace dialogs {

class ReCaptcha : public QWidget
{
        Q_OBJECT

public:
        ReCaptcha(const QString &session, QWidget *parent = nullptr);

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void closing();

private:
        FlatButton *openCaptchaBtn_;
        RaisedButton *confirmBtn_;
        RaisedButton *cancelBtn_;
};
} // dialogs
