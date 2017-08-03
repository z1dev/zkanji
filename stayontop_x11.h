#ifndef STAYONTOP_X11_H
#define STAYONTOP_X11_H

#include <QWidget>

#ifdef Q_OS_LINUX
void x11_window_set_on_top(/*Display* display, Window xid*/ QWidget *w, bool ontop);
#endif // Q_OS_LINUX

#endif // STAYONTOP_X11_H
