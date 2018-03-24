#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QWidget>

class Avatar;

struct SearchResult
{
        QString user_id;
        QString display_name;
};

Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(QVector<SearchResult>)

class PopupItem : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor)

public:
        PopupItem(QWidget *parent, const QString &user_id);

        QColor hoverColor() const { return hoverColor_; }
        void setHoverColor(QColor &color) { hoverColor_ = color; }

protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

signals:
        void clicked(const QString &display_name);

private:
        QHBoxLayout *topLayout_;

        Avatar *avatar_;
        QLabel *userName_;
        QString user_id_;

        QColor hoverColor_;
};

class SuggestionsPopup : public QWidget
{
        Q_OBJECT

public:
        explicit SuggestionsPopup(QWidget *parent = nullptr);

public slots:
        void addUsers(const QVector<SearchResult> &users);

signals:
        void itemSelected(const QString &user);

private:
        QVBoxLayout *layout_;
};
