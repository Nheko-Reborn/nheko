#pragma once

#include <QObject>

class TimelineModel;

class InputBar : public QObject {
        Q_OBJECT

public:
        InputBar(TimelineModel *parent)
          : QObject()
          , room(parent)
        {}

public slots:
        void send();
        bool paste(bool fromMouse);
        void updateState(int selectionStart, int selectionEnd, int cursorPosition, QString text);

private:
        TimelineModel *room;
        QString text;
        int selectionStart = 0, selectionEnd = 0, cursorPosition = 0;
};
