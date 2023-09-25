#include "ShortcutRegistry.h"

ShortcutRegistry *ShortcutRegistry::s_instance = nullptr;

EditableShortcut::EditableShortcut(QObject *parent)
    : QObject{parent}
{
}

const QStringList EditableShortcut::shortcuts() const
{
    QStringList dest;
    dest.resize(m_shortcuts.size());
    std::transform(m_shortcuts.begin(), m_shortcuts.end(), dest.begin(), [](const auto &shortcut) {
        return shortcut.toString();
    });
    return dest;
}

void EditableShortcut::setName(const QString &name)
{
    if (name == m_name)
        return;
    m_name = name;
    emit nameChanged();
}

void EditableShortcut::setDescription(const QString &description)
{
    if (description == m_description)
        return;
    m_description = description;
    emit descriptionChanged();
}

void EditableShortcut::setShortcut(const QString &shortcut)
{
    setShortcuts({shortcut});
}

void EditableShortcut::setShortcuts(const QStringList &shortcuts)
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

EditableShortcut::EditableShortcut(const QString &name, const QString &description, QObject *parent)
  : QObject{parent}
  , m_name{name}
  , m_description{description}
{
    ShortcutRegistry::instance()->registerShortcut(this);
}

ShortcutRegistry *
ShortcutRegistry::instance()
{
    return s_instance;
}

ShortcutRegistry *ShortcutRegistry::create(QQmlEngine *qmlEngine, QJSEngine *)
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

QHash<int, QByteArray> ShortcutRegistry::roleNames() const
{
    return {{Roles::Name, "name"},
            {Roles::Description, "description"},
            {Roles::Shortcut, "shortcut"}};
}

QVariant ShortcutRegistry::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_shortcuts.size() || index.row() < 0)
        return {};

    switch (role)
    {
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

bool ShortcutRegistry::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_shortcuts.size() || index.row() < 0)
        return false;

    switch (role)
    {
    case Roles::Shortcut:
        if (auto shortcut = QKeySequence(value.toString()); !shortcut.isEmpty()) {
            m_shortcuts[index.row()]->setShortcut(shortcut.toString());
            return true;
        } else
            return false;
    default:
        return false;
    }
}

ShortcutRegistry::ShortcutRegistry(QObject *parent)
  : QAbstractListModel{parent}
{
    s_instance = this;
}

void ShortcutRegistry::registerShortcut(EditableShortcut *action)
{
    m_shortcuts.push_back(action);
}
