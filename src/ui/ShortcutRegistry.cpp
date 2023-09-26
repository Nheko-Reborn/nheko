// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ShortcutRegistry.h"

ShortcutRegistry *ShortcutRegistry::s_instance = nullptr;

EditableShortcut::EditableShortcut(QObject *parent)
  : QObject{parent}
{
    ShortcutRegistry::instance()->registerShortcut(this);
}

EditableShortcut::EditableShortcut(const QString &name, const QString &description, QObject *parent)
  : QObject{parent}
  , m_name{name}
  , m_description{description}
{
    ShortcutRegistry::instance()->registerShortcut(this);
}

const QStringList
EditableShortcut::shortcuts() const
{
    QStringList dest;
    dest.resize(m_shortcuts.size());
    std::transform(m_shortcuts.begin(), m_shortcuts.end(), dest.begin(), [](const auto &shortcut) {
        return shortcut.toString();
    });
    return dest;
}

void
EditableShortcut::setName(const QString &name)
{
    if (name == m_name)
        return;
    m_name = name;
    emit nameChanged();
}

void
EditableShortcut::setDescription(const QString &description)
{
    if (description == m_description)
        return;
    m_description = description;
    emit descriptionChanged();
}

void
EditableShortcut::setShortcut(const QString &shortcut)
{
    setShortcuts({shortcut});
}

void
EditableShortcut::setShortcuts(const QStringList &shortcuts)
{
    QList<QKeySequence> temp;
    temp.resize(shortcuts.size());
    std::transform(shortcuts.begin(), shortcuts.end(), temp.begin(), [](const auto &shortcut) {
        return QKeySequence(shortcut);
    });

    if (temp == m_shortcuts)
        return;
    m_shortcuts = temp;
    emit shortcutsChanged();
}

ShortcutRegistry::ShortcutRegistry(QObject *parent)
  : QAbstractListModel{parent}
{
    if (s_instance)
        m_shortcuts = s_instance->m_shortcuts;

    s_instance = this;
}

ShortcutRegistry *
ShortcutRegistry::instance()
{
    return s_instance;
}

ShortcutRegistry *
ShortcutRegistry::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    // The instance has to exist before it is used. We cannot replace it.
    Q_ASSERT(s_instance);

    // The engine has to have the same thread affinity as the singleton.
    Q_ASSERT(qmlEngine->thread() == s_instance->thread());

    // There can only be one engine accessing the singleton.
    static QJSEngine *s_engine = nullptr;
    if (s_engine)
        Q_ASSERT(qmlEngine == s_engine);
    else
        s_engine = qmlEngine;

    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QHash<int, QByteArray>
ShortcutRegistry::roleNames() const
{
    return {
      {Roles::Name, "name"}, {Roles::Description, "description"}, {Roles::Shortcut, "shortcut"}};
}

QVariant
ShortcutRegistry::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_shortcuts.size() || index.row() < 0)
        return {};

    switch (role) {
    case Roles::Name:
        return m_shortcuts[index.row()]->name();
    case Roles::Description:
        return m_shortcuts[index.row()]->description();
    case Roles::Shortcut:
        return m_shortcuts[index.row()]->shortcut();
    default:
        return {};
    }
}

void
ShortcutRegistry::changeShortcut(const QString &name, const QString &newShortcut)
{
    for (int i = 0; i < m_shortcuts.size(); ++i) {
        if (m_shortcuts[i]->name() == name) {
            qDebug() << "new:" << newShortcut;
            m_shortcuts[i]->setShortcut(newShortcut);
            emit dataChanged(index(i), index(i), {Roles::Shortcut});
            return;
        }
    }
}

QString
ShortcutRegistry::keycodeToChar(int keycode) const
{
    return QString((char)keycode);
}

void
ShortcutRegistry::registerShortcut(EditableShortcut *action)
{
    beginInsertRows({}, m_shortcuts.size(), m_shortcuts.size());
    m_shortcuts.push_back(action);
    endInsertRows();
}
