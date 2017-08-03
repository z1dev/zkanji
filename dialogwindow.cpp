/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include <QtEvents>
#include "dialogwindow.h"

#ifdef Q_OS_LINUX
#include "stayontop_x11.h"
#endif


//-------------------------------------------------------------


DialogWindow::DialogWindow(QWidget *parent) : base(parent, parent != nullptr ? Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint |
    (!parent->windowFlags().testFlag(Qt::WindowStaysOnTopHint) ? (Qt::WindowType)0 : Qt::WindowStaysOnTopHint)
    : Qt::WindowFlags()), loop(nullptr), res(ModalResult::Cancel)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowModality(Qt::NonModal);
}

DialogWindow::~DialogWindow()
{
    // This is for debugging purposes only. If we throw here, it's best for the program to
    // crash anyway.
    if (loop && loop->isRunning())
        throw "Programmer error. Must call base::closeEvent to stop the loop.";

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
    base::closeEvent(e);
    if (!e->isAccepted())
    {
        res = ModalResult::Cancel;
        return;
    }

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


//-------------------------------------------------------------
