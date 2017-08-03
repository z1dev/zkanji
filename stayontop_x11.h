/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef STAYONTOP_X11_H
#define STAYONTOP_X11_H

#include <QWidget>

#ifdef Q_OS_LINUX
void x11_window_set_on_top(/*Display* display, Window xid*/ QWidget *w, bool ontop);
#endif // Q_OS_LINUX

#endif // STAYONTOP_X11_H
