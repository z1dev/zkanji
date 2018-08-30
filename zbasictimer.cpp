/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include "zbasictimer.h"

ZBasicTimer::ZBasicTimer(QObject *parent) : base(parent), valid(true)
{
    connect(qApp, &QApplication::aboutToQuit, this, &ZBasicTimer::appQuit);
}

void ZBasicTimer::start(int msec, QObject *object)
{
    if (!valid)
        return;

    timer.start(msec, object);
}

void ZBasicTimer::stop()
{
    timer.stop();
}

int ZBasicTimer::timerId() const
{
    return timer.timerId();
}

bool ZBasicTimer::isActive() const
{
    return valid && timer.isActive();
}

void ZBasicTimer::appQuit()
{
    valid = false;
    stop();
}
