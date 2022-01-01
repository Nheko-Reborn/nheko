// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

// A DelegateChooser like the one, that was added to Qt5.12 (in labs), but compatible with older Qt
// versions see KDE/kquickitemviews see qtdeclarative/qqmldelagatecomponent

#pragma once

#include <QQmlComponent>
#include <QQmlIncubator>
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

    [[nodiscard]] QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    [[nodiscard]] QVariant roleValue() const;
    [[nodiscard]] const QVariant &roleValueRef() const { return roleValue_; }
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
    Q_PROPERTY(QVariant roleValue READ roleValue WRITE setRoleValue NOTIFY roleValueChanged)
    Q_PROPERTY(QQuickItem *child READ child NOTIFY childChanged)

    QQmlListProperty<DelegateChoice> choices();

    [[nodiscard]] QVariant roleValue() const;
    void setRoleValue(const QVariant &value);

    [[nodiscard]] QQuickItem *child() const { return child_; }

    void recalcChild();
    void componentComplete() override;

signals:
    void roleChanged();
    void roleValueChanged();
    void childChanged();

private:
    struct DelegateIncubator : public QQmlIncubator
    {
        DelegateIncubator(DelegateChooser &parent)
          : QQmlIncubator(QQmlIncubator::AsynchronousIfNested)
          , chooser(parent)
        {}
        void statusChanged(QQmlIncubator::Status status) override;

        DelegateChooser &chooser;
    };

    QVariant roleValue_;
    QList<DelegateChoice *> choices_;
    QQuickItem *child_ = nullptr;
    DelegateIncubator incubator{*this};

    static void appendChoice(QQmlListProperty<DelegateChoice> *, DelegateChoice *);
    static int choiceCount(QQmlListProperty<DelegateChoice> *);
    static DelegateChoice *choice(QQmlListProperty<DelegateChoice> *, int index);
    static void clearChoices(QQmlListProperty<DelegateChoice> *);
};
