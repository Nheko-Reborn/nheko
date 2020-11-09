#pragma once

#include <QObject>
#include <deque>

class TimelineModel;

class InputBar : public QObject
{
        Q_OBJECT

public:
        InputBar(TimelineModel *parent)
          : QObject()
          , room(parent)
        {}

public slots:
        void send();
        void paste(bool fromMouse);
        void updateState(int selectionStart, int selectionEnd, int cursorPosition, QString text);

signals:
        void insertText(QString text);

private:
        void message(QString body);
        void emote(QString body);
        void command(QString name, QString args);

        TimelineModel *room;
        QString text;
        std::deque<QString> history_;
        std::size_t history_index_ = 0;
        int selectionStart = 0, selectionEnd = 0, cursorPosition = 0;
};
