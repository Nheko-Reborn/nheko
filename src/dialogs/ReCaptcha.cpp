#include <QDesktopServices>
#include <QLabel>
#include <QPaintEvent>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/ReCaptcha.h"

#include "Config.h"
#include "MatrixClient.h"
#include "ui/FlatButton.h"
#include "ui/RaisedButton.h"
#include "ui/Theme.h"

using namespace dialogs;

ReCaptcha::ReCaptcha(const QString &session, QWidget *parent)
  : QWidget(parent)
{
        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(8);
        buttonLayout->setMargin(0);

        QFont buttonFont;
        buttonFont.setPointSizeF(buttonFont.pointSizeF() * conf::modals::BUTTON_TEXT_SIZE_RATIO);

        openCaptchaBtn_ = new FlatButton("OPEN reCAPTCHA", this);
        openCaptchaBtn_->setFont(buttonFont);

        confirmBtn_ = new RaisedButton(tr("CONFIRM"), this);
        confirmBtn_->setFont(buttonFont);

        cancelBtn_ = new RaisedButton(tr("CANCEL"), this);
        cancelBtn_->setFont(buttonFont);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(openCaptchaBtn_);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QFont font;
        font.setPointSizeF(font.pointSizeF() * conf::modals::LABEL_MEDIUM_SIZE_RATIO);

        auto label = new QLabel(tr("Solve the reCAPTCHA and press the confirm button"), this);
        label->setFont(font);

        layout->addWidget(label);
        layout->addLayout(buttonLayout);

        connect(openCaptchaBtn_, &QPushButton::clicked, [session]() {
                const auto url = QString("https://%1:%2/_matrix/client/r0/auth/m.login.recaptcha/"
                                         "fallback/web?session=%3")
                                   .arg(QString::fromStdString(http::client()->server()))
                                   .arg(http::client()->port())
                                   .arg(session);

                QDesktopServices::openUrl(url);
        });

        connect(confirmBtn_, &QPushButton::clicked, this, &dialogs::ReCaptcha::closing);
        connect(cancelBtn_, &QPushButton::clicked, this, &dialogs::ReCaptcha::close);
}

void
ReCaptcha::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
