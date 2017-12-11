#pragma once

#include <QFrame>

#include <mtx.hpp>

class FlatButton;
class TextField;
class QComboBox;
class Toggle;

namespace dialogs {

class CreateRoom : public QFrame
{
        Q_OBJECT
public:
        CreateRoom(QWidget *parent = nullptr);

signals:
        void closing(bool isCreating, const mtx::requests::CreateRoom &request);

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        void clearFields();

        QComboBox *visibilityCombo_;
        QComboBox *presetCombo_;

        Toggle *directToggle_;

        FlatButton *confirmBtn_;
        FlatButton *cancelBtn_;

        TextField *nameInput_;
        TextField *topicInput_;
        TextField *aliasInput_;

        mtx::requests::CreateRoom request_;
};

} // dialogs
