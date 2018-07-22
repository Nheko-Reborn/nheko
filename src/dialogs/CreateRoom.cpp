#include <QComboBox>
#include <QLabel>
#include <QStyleOption>
#include <QVBoxLayout>

#include "dialogs/CreateRoom.h"

#include "Config.h"
#include "ui/FlatButton.h"
#include "ui/TextField.h"
#include "ui/Theme.h"
#include "ui/ToggleButton.h"

using namespace dialogs;

CreateRoom::CreateRoom(QWidget *parent)
  : QFrame(parent)
{
        setMinimumSize(conf::modals::MIN_WIDGET_WIDTH, conf::modals::MIN_WIDGET_HEIGHT);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setMargin(0);

        QFont buttonFont;
        buttonFont.setPointSizeF(buttonFont.pointSizeF() * conf::modals::BUTTON_TEXT_SIZE_RATIO);

        confirmBtn_ = new FlatButton("CREATE", this);
        confirmBtn_->setFont(buttonFont);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFont(buttonFont);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QFont font;
        font.setPixelSize(conf::headerFontSize);

        nameInput_ = new TextField(this);
        nameInput_->setLabel(tr("Name"));

        topicInput_ = new TextField(this);
        topicInput_->setLabel(tr("Topic"));

        aliasInput_ = new TextField(this);
        aliasInput_->setLabel(tr("Alias"));

        auto visibilityLayout = new QHBoxLayout;
        visibilityLayout->setContentsMargins(0, 10, 0, 10);

        auto presetLayout = new QHBoxLayout;
        presetLayout->setContentsMargins(0, 10, 0, 10);

        auto visibilityLabel = new QLabel(tr("Room Visibility"), this);
        visibilityCombo_     = new QComboBox(this);
        visibilityCombo_->addItem("Private");
        visibilityCombo_->addItem("Public");

        visibilityLayout->addWidget(visibilityLabel);
        visibilityLayout->addWidget(visibilityCombo_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto presetLabel = new QLabel(tr("Room Preset"), this);
        presetCombo_     = new QComboBox(this);
        presetCombo_->addItem("Private Chat");
        presetCombo_->addItem("Public Chat");
        presetCombo_->addItem("Trusted Private Chat");

        presetLayout->addWidget(presetLabel);
        presetLayout->addWidget(presetCombo_, 0, Qt::AlignBottom | Qt::AlignRight);

        auto directLabel_ = new QLabel(tr("Direct Chat"), this);
        directToggle_     = new Toggle(this);
        directToggle_->setActiveColor(QColor("#38A3D8"));
        directToggle_->setInactiveColor(QColor("gray"));
        directToggle_->setState(true);

        auto directLayout = new QHBoxLayout;
        directLayout->setContentsMargins(0, 10, 0, 10);
        directLayout->addWidget(directLabel_);
        directLayout->addWidget(directToggle_, 0, Qt::AlignBottom | Qt::AlignRight);

        layout->addWidget(nameInput_);
        layout->addWidget(topicInput_);
        layout->addWidget(aliasInput_);
        layout->addLayout(visibilityLayout);
        layout->addLayout(presetLayout);
        layout->addLayout(directLayout);
        layout->addLayout(buttonLayout);

        connect(confirmBtn_, &QPushButton::clicked, this, [this]() {
                request_.name            = nameInput_->text().toStdString();
                request_.topic           = topicInput_->text().toStdString();
                request_.room_alias_name = aliasInput_->text().toStdString();

                emit closing(true, request_);

                clearFields();
        });

        connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
                emit closing(false, request_);

                clearFields();
        });

        connect(visibilityCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [this](const QString &text) {
                        if (text == "Private") {
                                request_.visibility = mtx::requests::Visibility::Private;
                        } else {
                                request_.visibility = mtx::requests::Visibility::Public;
                        }
                });

        connect(presetCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                [this](const QString &text) {
                        if (text == "Private Chat") {
                                request_.preset = mtx::requests::Preset::PrivateChat;
                        } else if (text == "Public Chat") {
                                request_.preset = mtx::requests::Preset::PublicChat;
                        } else {
                                request_.preset = mtx::requests::Preset::TrustedPrivateChat;
                        }
                });

        connect(directToggle_, &Toggle::toggled, this, [this](bool isDisabled) {
                request_.is_direct = !isDisabled;
        });
}

void
CreateRoom::clearFields()
{
        nameInput_->clear();
        topicInput_->clear();
        aliasInput_->clear();
}

void
CreateRoom::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
CreateRoom::showEvent(QShowEvent *event)
{
        nameInput_->setFocus();

        QFrame::showEvent(event);
}
