// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "KeySequenceRegistry.h"

KeySequenceRegistry *KeySequenceRegistry::s_instance = nullptr;

KeySequenceImpl::KeySequenceImpl(const QString &name,
                                 const QStringList &keySequences,
                                 QObject *parent)
  : QObject{parent}
  , m_name{name}
  , m_keySequences{keySequences}
{
}

void
KeySequenceImpl::setKeySequences(const QStringList &keySequences)
{
    if (keySequences == m_keySequences)
        return;
    m_keySequences = keySequences;
    emit keySequencesChanged();
}

EditableKeySequence::EditableKeySequence(QObject *parent)
  : QObject{parent}
{
    KeySequenceRegistry::instance()->registerKeySequence(this);
}

EditableKeySequence::EditableKeySequence(const QString &name, QObject *parent)
  : QObject{parent}
  , m_name{name}
{
    KeySequenceRegistry::instance()->registerKeySequence(this);
}

const QString
EditableKeySequence::keySequence() const
{
    return (m_impl && m_impl->keySequences().size() > 0) ? m_impl->keySequences().first()
                                                         : defaultKeySequence();
}

const QStringList
EditableKeySequence::keySequences() const
{
    return m_impl ? m_impl->keySequences() : defaultKeySequences();
}

const QString
EditableKeySequence::defaultKeySequence() const
{
    return m_defaultKeySequences.size() > 0 ? m_defaultKeySequences.first().toString() : QString{};
}

const QStringList
EditableKeySequence::defaultKeySequences() const
{
    QStringList dest;
    dest.resize(m_defaultKeySequences.size());
    std::transform(m_defaultKeySequences.begin(),
                   m_defaultKeySequences.end(),
                   dest.begin(),
                   [](const auto &keySequence) { return keySequence.toString(); });
    return dest;
}

void
EditableKeySequence::setName(const QString &name)
{
    if (name == m_name)
        return;
    m_name = name;
    emit nameChanged();
    KeySequenceRegistry::instance()->registerKeySequence(this);
}

void
EditableKeySequence::setKeySequence(const QString &keySequence)
{
    setKeySequences({keySequence});
}

void
EditableKeySequence::setKeySequences(const QStringList &keySequences)
{
    m_impl->setKeySequences(keySequences);
}

void
EditableKeySequence::setDefaultKeySequence(const QString &keySequence)
{
    setDefaultKeySequences({keySequence});
}

void
EditableKeySequence::setDefaultKeySequences(const QStringList &keySequences)
{
    QList<QKeySequence> temp;
    temp.resize(keySequences.size());
    std::transform(keySequences.begin(),
                   keySequences.end(),
                   temp.begin(),
                   [](const auto &keySequence) { return QKeySequence(keySequence); });

    if (temp == m_defaultKeySequences)
        return;
    m_defaultKeySequences = temp;
    emit defaultKeySequencesChanged();

    if (m_impl && m_impl->keySequences().isEmpty())
        m_impl->setKeySequences(keySequences);
}

KeySequenceRegistry::KeySequenceRegistry(QObject *parent)
  : QAbstractListModel{parent}
{
    s_instance = this;
}

KeySequenceRegistry *
KeySequenceRegistry::instance()
{
    if (!s_instance)
        s_instance = new KeySequenceRegistry;
    return s_instance;
}

KeySequenceRegistry *
KeySequenceRegistry::create(QQmlEngine *qmlEngine, QJSEngine *)
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
KeySequenceRegistry::roleNames() const
{
    return {{Roles::Name, "name"}, {Roles::KeySequence, "keySequence"}};
}

QVariant
KeySequenceRegistry::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_keySequences.size() || index.row() < 0)
        return {};

    switch (role) {
    case Roles::Name:
        return m_keySequences.at(index.row())->name();
    case Roles::KeySequence: {
        const auto &data = m_keySequences.at(index.row())->keySequences();
        return data.size() > 0 ? data.first() : QString{};
    }
    default:
        return {};
    }
}

void
KeySequenceRegistry::changeKeySequence(const QString &name, const QString &newKeySequence)
{
    for (int i = 0; i < m_keySequences.size(); ++i) {
        if (m_keySequences.at(i)->name() == name) {
            m_keySequences.at(i)->setKeySequences({newKeySequence});
            emit dataChanged(index(i), index(i), {Roles::KeySequence});
            return;
        }
    }
}

QString
KeySequenceRegistry::keycodeToChar(int keycode) const
{
    return QString((char)keycode);
}

QMap<QString, QStringList>
KeySequenceRegistry::dumpBindings() const
{
    QMap<QString, QStringList> bindings;
    for (const auto sequence : m_keySequences)
        bindings[sequence->name()] = sequence->keySequences();
    return bindings;
}

void
KeySequenceRegistry::restoreBindings(const QMap<QString, QStringList> &bindings)
{
    for (const auto &[name, keySequences] : bindings.asKeyValueRange()) {
        if (auto it = std::find_if(m_keySequences.begin(),
                                   m_keySequences.end(),
                                   [&name](const auto &impl) { return impl->name() == name; });
            it != m_keySequences.end())
            (*it)->setKeySequences(keySequences);
        else
            m_keySequences.push_back(new KeySequenceImpl{name, keySequences});
    }
}

void
KeySequenceRegistry::registerKeySequence(EditableKeySequence *action)
{
    if (action->name().isEmpty())
        return;

    KeySequenceImpl *impl = nullptr;
    if (auto it =
          std::find_if(m_keySequences.begin(),
                       m_keySequences.end(),
                       [action](const auto &impl) { return impl->name() == action->name(); });
        it != m_keySequences.end())
        impl = *it;
    else {
        impl = new KeySequenceImpl{action->name(), action->keySequences()};
        m_keySequences.push_back(impl);
    }

    action->m_impl = impl;
    connect(impl,
            &KeySequenceImpl::keySequencesChanged,
            action,
            &EditableKeySequence::keySequencesChanged);
    emit action->keySequencesChanged();
}
