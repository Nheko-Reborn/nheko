// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QKeySequence>
#include <QQmlEngine>

class KeySequenceImpl : public QObject
{
    Q_OBJECT

public:
    KeySequenceImpl(const QString &name,
                    const QStringList &keySequences,
                    QObject *parent = nullptr);

    const QString &name() const { return m_name; }
    const QStringList &keySequences() const { return m_keySequences; }

    void setKeySequences(const QStringList &keySequences);

signals:
    void keySequencesChanged();

private:
    const QString m_name;
    QStringList m_keySequences;
};

class EditableKeySequence : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(
      QString keySequence READ keySequence WRITE setKeySequence NOTIFY keySequencesChanged FINAL)
    Q_PROPERTY(QStringList keySequences READ keySequences WRITE setKeySequences NOTIFY
                 keySequencesChanged FINAL)
    Q_PROPERTY(QString defaultKeySequence READ defaultKeySequence WRITE setDefaultKeySequence NOTIFY
                 defaultKeySequencesChanged FINAL)
    Q_PROPERTY(QStringList defaultKeySequences READ defaultKeySequences WRITE setDefaultKeySequences
                 NOTIFY defaultKeySequencesChanged FINAL)

public:
    EditableKeySequence(QObject *parent = nullptr);
    EditableKeySequence(const QString &name, QObject *parent = nullptr);

    const QString &name() const { return m_name; }
    const QString keySequence() const;
    const QStringList keySequences() const;
    const QString defaultKeySequence() const;
    const QStringList defaultKeySequences() const;

    void setName(const QString &name);
    void setKeySequence(const QString &keySequence);
    void setKeySequences(const QStringList &keySequences);
    void setDefaultKeySequence(const QString &keySequence);
    void setDefaultKeySequences(const QStringList &keySequences);

signals:
    void nameChanged();
    void keySequencesChanged();
    void defaultKeySequencesChanged();

private:
    QString m_name;
    QList<QKeySequence> m_defaultKeySequences;

    KeySequenceImpl *m_impl = nullptr;

    friend class KeySequenceRegistry;
};

class KeySequenceRegistry : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Roles
    {
        Name,
        KeySequence,
    };

    static KeySequenceRegistry *instance();
    static KeySequenceRegistry *create(QQmlEngine *qmlEngine, QJSEngine *);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_keySequences.size();
    }
    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void changeKeySequence(const QString &name, const QString &newKeysequence);
    Q_INVOKABLE QString keycodeToChar(int keycode) const;

    QMap<QString, QStringList> dumpBindings() const;
    void restoreBindings(const QMap<QString, QStringList> &bindings);

private:
    explicit KeySequenceRegistry(QObject *parent = nullptr);

    void registerKeySequence(EditableKeySequence *action);

    static KeySequenceRegistry *s_instance;
    QList<KeySequenceImpl *> m_keySequences;

    friend EditableKeySequence;
};
