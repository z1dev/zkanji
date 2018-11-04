/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include <QtEvents>
#include <QDesktopWidget>
#include <QApplication>
#include <QWindow>
#include "dialogwindow.h"
#include <popupdict.h>
#include <popupkanjisearch.h>

#ifdef Q_OS_LINUX
#include "stayontop_x11.h"
#endif

#ifndef NDEBUG
#ifndef _DEBUG
#define NDEBUG
#define NDEBUG_ADDED_SEPARATELY
#endif
#endif

#include <cassert>

#ifdef NDEBUG_ADDED_SEPARATELY
#undef NDEBUG_ADDED_SEPARATELY
#undef NDEBUG
#endif


//-------------------------------------------------------------


DialogWindow::DialogWindow(QWidget *parent, bool resizing) : base(parent, parent != nullptr ? Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | (resizing ? Qt::WindowMinMaxButtonsHint : Qt::MSWindowsFixedSizeDialogHint) | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint |
    (!parent->windowFlags().testFlag(Qt::WindowStaysOnTopHint) ? (Qt::WindowType)0 : Qt::WindowStaysOnTopHint)
    : resizing ? Qt::WindowFlags() : (Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint)), loop(nullptr), res(ModalResult::Cancel)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowModality(Qt::NonModal);
}

DialogWindow::~DialogWindow()
{
    // If this fails it's programmer error. Must call base::closeEvent to stop the loop.
    assert(!loop || !loop->isRunning());

    //{
    //    loop->quit();
    //    emit finished(res);
    //}
}

ModalResult DialogWindow::showModal()
{
    setWindowModality(Qt::ApplicationModal);
    ModalResult r = ModalResult::Cancel;
    connect(this, &DialogWindow::finished, [&r](ModalResult res) { r = res; });
    show();

    return r;
}

void DialogWindow::show()
{
    base::show();

    if (windowModality() == Qt::NonModal || loop != nullptr)
        return;

    loop = new QEventLoop(this);
    loop->exec();
}

void DialogWindow::modalClose(ModalResult result)
{
    if (!loop || !loop->isRunning())
    {
        res = result;

        close();
        return;
    }

    res = result;
    close();
}

ModalResult DialogWindow::modalResult() const
{
    return res;
}

void DialogWindow::closeOk()
{
    modalClose(ModalResult::Ok);
}

void DialogWindow::closeCancel()
{
    modalClose(ModalResult::Cancel);
}

void DialogWindow::closeYes()
{
    modalClose(ModalResult::Yes);
}

void DialogWindow::closeNo()
{
    modalClose(ModalResult::No);
}

void DialogWindow::closeIgnore()
{
    modalClose(ModalResult::Ignore);
}

void DialogWindow::closeAbort()
{
    modalClose(ModalResult::Abort);
}

void DialogWindow::closeSave()
{
    modalClose(ModalResult::Save);
}

void DialogWindow::closeResume()
{
    modalClose(ModalResult::Resume);
}

void DialogWindow::closeSuspend()
{
    modalClose(ModalResult::Suspend);
}

void DialogWindow::closeStart()
{
    modalClose(ModalResult::Start);
}

void DialogWindow::closeStop()
{
    modalClose(ModalResult::Stop);
}

void DialogWindow::closeError()
{
    modalClose(ModalResult::Error);
}

bool DialogWindow::event(QEvent *e)
{
#ifdef Q_OS_LINUX
    if (e->type() == QEvent::WinIdChange)
    {
        bool r = base::event(e);
        if (windowFlags().testFlag(Qt::WindowStaysOnTopHint))
            x11_window_set_on_top(this, true);
        return r;
    }
#endif

    return base::event(e);
}

void DialogWindow::closeEvent(QCloseEvent *e)
{
    if (!e->isAccepted())
    {
        res = ModalResult::Cancel;
        return;
    }

    base::closeEvent(e);
    if (loop && loop->isRunning())
    {
        loop->quit();
        emit finished(res);
    }
}

void DialogWindow::keyPressEvent(QKeyEvent *e)
{
    /* Some parts are taken directly from QDialog::keyPressEvent() */

#ifdef Q_OS_MAC
    if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period)
    {
        close();
        return;
    }
#endif

    if (!e->modifiers() || (e->modifiers() & Qt::KeypadModifier && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)))
    {
        switch (e->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return: {
            // If a button is focused, it'll be clicked. If no button is focused, the first
            // button with the Default property set to true is clicked. If no such button is
            // found, the key press is ignored.
            QList<QPushButton*> list = findChildren<QPushButton*>();
            QPushButton *b = nullptr;
            for (QPushButton *btn : list)
            {
                if (btn->window() != this || !btn->isVisibleTo(this))
                    continue;

                if (btn->hasFocus())
                {
                    b = btn;
                    break;
                }
                if (b == nullptr && btn->isDefault())
                    b = btn;
            }

            if (b != nullptr)
                b->click();
            break;
        }
        case Qt::Key_Escape:
            close();
            break;
        }
    }

    base::keyPressEvent(e);
}

void DialogWindow::showEvent(QShowEvent *e)
{
    if (!e->spontaneous() && !testAttribute(Qt::WA_Moved))
    {
        Qt::WindowStates  state = windowState();

        QWidget *w = parentWidget() == nullptr ? nullptr : parentWidget()->window();
        // Position the window

        // Part of Qt still in development. Uncomment when QPA becomes available (if ever.)

        //if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        //    if (theme->themeHint(QPlatformTheme::WindowAutoPlacement).toBool())
        //        return;

        // Center window above the parent or if the parent is the locked popup dictionary or
        // popup kanji search, center of screen.

        int screen = w != nullptr ? qApp->desktop()->screenNumber(w) : qApp->desktop()->isVirtualDesktop() ? qApp->desktop()->screenNumber(QCursor::pos()) : qApp->desktop()->screenNumber(this);
        QRect scr = qApp->desktop()->availableGeometry(screen);

        QPoint mid;
        if (w != nullptr && w != PopupDictionary::getInstance() && w != PopupKanjiSearch::getInstance())
            mid = w->frameGeometry().center();
        else
            mid = scr.center();

        if (w != nullptr)
            screen = qApp->desktop()->screenNumber(w);

        if (QWindow *h = windowHandle())
            h->setScreen(QGuiApplication::screens().at(screen));

        QRect fg = frameGeometry();
        move(
            std::max(scr.left(), std::min(mid.x() - fg.width() / 2, scr.left() + scr.width() - fg.width())),
            std::max(scr.top(), std::min(mid.y() - fg.height() / 2, scr.top() + scr.height() - fg.height()))
        );

        // End window positioning.

        setAttribute(Qt::WA_Moved, false);
        if (state != windowState())
            setWindowState(state);
    }
}


//-------------------------------------------------------------
