/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZBASICTIMER_H
#define ZBASICTIMER_H

#include <QObject>
#include <QBasicTimer>

class ZBasicTimer : public QObject
{
    Q_OBJECT
public:
    ZBasicTimer(QObject *parent = nullptr);

    void start(int msec, QObject *object);
    void stop();
    int timerId() const;
    bool isActive() const;
private:
    void appQuit();

    QBasicTimer timer;

    bool valid;

    typedef QObject base;
};


#endif // ZBASICTIMER_H

