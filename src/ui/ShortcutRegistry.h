#pragma once

#include <QAbstractListModel>
#include <QAction>
#include <QQmlEngine>

class EditableShortcut : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged FINAL)
    Q_PROPERTY(QString shortcut READ shortcut WRITE setShortcut NOTIFY shortcutsChanged FINAL)
    Q_PROPERTY(QStringList shortcuts READ shortcuts WRITE setShortcuts NOTIFY shortcutsChanged FINAL)

public:
    EditableShortcut(QObject *parent = nullptr);
    EditableShortcut(const QString &name, const QString &description, QObject *parent = nullptr);
    EditableShortcut(const QString &name, const QString &description, const QString &text, QObject *parent = nullptr);
    EditableShortcut(const QString &name, const QString &description, const QIcon &icon, const QString &text, QObject *parent = nullptr);

    const QString &name() const { return m_name; }
    const QString &description() const { return m_description; }
    const QString shortcut() const
    {
        return m_shortcuts.size() > 0 ? m_shortcuts.first().toString() : QString{};
    }
    const QStringList shortcuts() const;

    void setName(const QString &name);
    void setDescription(const QString &description);
    void setShortcut(const QString &shortcut);
    void setShortcuts(const QStringList &shortcuts);

signals:
    void nameChanged();
    void descriptionChanged();
    void shortcutsChanged();

private:
    QString m_name;
    QString m_description;
    QList<QKeySequence> m_shortcuts;
};

class ShortcutRegistry : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Roles
    {
        Name,
        Description,
        Shortcut,
    };

    static ShortcutRegistry *instance();
    static ShortcutRegistry *create(QQmlEngine *qmlEngine, QJSEngine *);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_shortcuts.size();
    }
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

private:
    explicit ShortcutRegistry(QObject *parent = nullptr);

    void registerShortcut(EditableShortcut *action);

    static ShortcutRegistry *s_instance;
    QList<EditableShortcut *> m_shortcuts;

    friend EditableShortcut;
};
