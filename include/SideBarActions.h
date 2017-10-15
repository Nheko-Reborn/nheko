#include <QHBoxLayout>
#include <QResizeEvent>

#include <FlatButton.h>

class SideBarActions : public QWidget
{
        Q_OBJECT

public:
        SideBarActions(QWidget *parent = nullptr);
        ~SideBarActions();

protected:
        void resizeEvent(QResizeEvent *event) override;

private:
        QHBoxLayout *layout_;

        FlatButton *settingsBtn_;
        FlatButton *createRoomBtn_;
        FlatButton *joinRoomBtn_;
};
