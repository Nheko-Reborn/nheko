// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlListProperty>
#include <QQmlProperty>
#include <QQmlPropertyValueSource>
#include <QVariantList>

class NhekoMenuVisibilityFilter
  : public QObject
  , public QQmlPropertyValueSource
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueSource)
    Q_CLASSINFO("DefaultProperty", "items")
    Q_PROPERTY(QQmlListProperty<QQmlComponent> items READ items CONSTANT FINAL)
    Q_PROPERTY(QQmlComponent *delegate MEMBER delegate FINAL)
    Q_PROPERTY(QVariantList model MEMBER model FINAL)
    QML_ELEMENT

public:
    NhekoMenuVisibilityFilter(QObject *parent = nullptr)
      : QObject(parent)
    {
    }

    QQmlListProperty<QQmlComponent> items();

    void setTarget(const QQmlProperty &prop) override;

private:
    QQmlProperty targetProperty;
    QList<QQmlComponent *> items_;
    QQmlComponent *delegate = nullptr;
    QVariantList model;

    static void appendItem(QQmlListProperty<QQmlComponent> *, QQmlComponent *);
    static qsizetype itemCount(QQmlListProperty<QQmlComponent> *);
    static QQmlComponent *getItem(QQmlListProperty<QQmlComponent> *, qsizetype index);
    static void clearItems(QQmlListProperty<QQmlComponent> *);
    static void replaceItem(QQmlListProperty<QQmlComponent> *, qsizetype index, QQmlComponent *);
    static void removeLast(QQmlListProperty<QQmlComponent> *);

public slots:
    // call this before showing the menu. We don't want to update elsewhere to prevent jumping menus
    // and useless work
    void updateTarget();
};
