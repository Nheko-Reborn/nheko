#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "dialogs/CreateRoom.h"

#include "Config.h"
#include "ui/TextField.h"
#include "ui/ToggleButton.h"

using namespace dialogs;

CreateRoom::CreateRoom(QWidget *parent)
  : QFrame(parent)
{
        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose, true);

        QFont largeFont;
        largeFont.setPointSizeF(largeFont.pointSizeF() * 1.5);

        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        setMinimumHeight(conf::modals::MIN_WIDGET_HEIGHT);
        setMinimumWidth(conf::window::minModalWidth);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(15);

        confirmBtn_ = new QPushButton(tr("Create room"), this);
        confirmBtn_->setDefault(true);
        cancelBtn_ = new QPushButton(tr("Cancel"), this);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(cancelBtn_);
        buttonLayout->addWidget(confirmBtn_);

        QFont font;
        font.setPointSizeF(font.pointSizeF() * 1.3);

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
        directToggle_->setState(false);
        directToggle_->setChecked(false);

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

                emit createRoom(request_);

                clearFields();
                emit close();
        });

        connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
                clearFields();
                emit close();
        });

        connect(visibilityCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged),
                [this](const QString &text) {
                        if (text == "Private") {
                                request_.visibility = mtx::requests::Visibility::Private;
                        } else {
                                request_.visibility = mtx::requests::Visibility::Public;
                        }
                });

        connect(presetCombo_,
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged),
                [this](const QString &text) {
                        if (text == "Private Chat") {
                                request_.preset = mtx::requests::Preset::PrivateChat;
                        } else if (text == "Public Chat") {
                                request_.preset = mtx::requests::Preset::PublicChat;
                        } else {
                                request_.preset = mtx::requests::Preset::TrustedPrivateChat;
                        }
                });

        connect(directToggle_, &Toggle::toggled, this, [this](bool isEnabled) {
                request_.is_direct = isEnabled;
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
CreateRoom::showEvent(QShowEvent *event)
{
        nameInput_->setFocus();

        QFrame::showEvent(event);
}
