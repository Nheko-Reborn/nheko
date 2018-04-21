#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QWidget>

class Avatar;
struct SearchResult;

class PopupItem : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor)
        Q_PROPERTY(bool hovering READ hovering WRITE setHovering)

public:
        PopupItem(QWidget *parent, const QString &user_id);

        QString user() const { return user_id_; }
        QColor hoverColor() const { return hoverColor_; }
        void setHoverColor(QColor &color) { hoverColor_ = color; }

        bool hovering() const { return hovering_; }
        void setHovering(const bool hover) { hovering_ = hover; };

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

        //! Set if the item is currently being hovered during tab completion (cycling).
        bool hovering_;
};

class SuggestionsPopup : public QWidget
{
        Q_OBJECT

public:
        explicit SuggestionsPopup(QWidget *parent = nullptr);

public slots:
        void addUsers(const QVector<SearchResult> &users);
        void selectHoveredSuggestion();
        //! Move to the next available suggestion item.
        void selectNextSuggestion();
        //! Move to the previous available suggestion item.
        void selectPreviousSuggestion();
        //! Remove hovering from all items.
        void resetHovering();
        //! Set hovering to the item in the given layout position.
        void setHovering(int pos);

signals:
        void itemSelected(const QString &user);

private:
        void hoverSelection();
        void resetSelection() { selectedItem_ = -1; }
        void selectFirstItem() { selectedItem_ = 0; }
        void selectLastItem() { selectedItem_ = layout_->count() - 1; }

        QVBoxLayout *layout_;

        //! Counter for tab completion (cycling).
        int selectedItem_ = -1;
};
