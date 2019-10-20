// A DelegateChooser like the one, that was added to Qt5.12 (in labs), but compatible with older Qt versions
// see KDE/kquickitemviews
// see qtdeclarative/qqmldelagatecomponent

#pragma once

#include <QQmlComponent>
#include <QQmlListProperty>
#include <QQuickItem>
#include <QtCore/QObject>
#include <QtCore/QVariant>

class QQmlAdaptorModel;

class DelegateChoice : public QObject
{
        Q_OBJECT
        Q_CLASSINFO("DefaultProperty", "delegate")

public:
        Q_PROPERTY(QVariant roleValue READ roleValue WRITE setRoleValue NOTIFY roleValueChanged)
        Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)

        QQmlComponent *delegate() const;
        void setDelegate(QQmlComponent *delegate);

        QVariant roleValue() const;
        void setRoleValue(const QVariant &value);

signals:
        void delegateChanged();
        void roleValueChanged();
        void changed();

private:
        QVariant roleValue_;
        QQmlComponent *delegate_ = nullptr;
};

class DelegateChooser : public QQuickItem
{
	Q_OBJECT
	Q_CLASSINFO("DefaultProperty", "choices")

public:
        Q_PROPERTY(QQmlListProperty<DelegateChoice> choices READ choices CONSTANT)
        Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged)
        Q_PROPERTY(QVariant roleValue READ roleValue WRITE setRoleValue NOTIFY roleValueChanged)

        QQmlListProperty<DelegateChoice> choices();

        QString role() const;
        void setRole(const QString &role);

        QVariant roleValue() const;
        void setRoleValue(const QVariant &value);

        QQmlComponent *delegate(QQmlAdaptorModel *adaptorModel, int row, int column = 0) const;

        void recalcChild();
        void componentComplete() override;

signals:
        void roleChanged();
        void roleValueChanged();

private:
        QString role_;
        QVariant roleValue_;
        QList<DelegateChoice *> choices_;
        QQuickItem *child;

        static void appendChoice(QQmlListProperty<DelegateChoice> *, DelegateChoice *);
        static int choiceCount(QQmlListProperty<DelegateChoice> *);
        static DelegateChoice *choice(QQmlListProperty<DelegateChoice> *, int index);
        static void clearChoices(QQmlListProperty<DelegateChoice> *);
};

