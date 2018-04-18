/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DIALOGWINDOW_H
#define DIALOGWINDOW_H

#include <QtCore>
#include <QMainWindow>
#include <memory>

class QEventLoop;


enum class ModalResult { None, Default, Ok, Cancel, Yes, No, Ignore, Abort, Save, Resume, Suspend, Start, Stop, Error };


class DialogWindow : public QMainWindow
{
    Q_OBJECT
signals:
    // Signaled when the dialog closes. The value the window closed with is passed in result.
    // The signal is only sent for dialog windows shown with showModal() or those with a
    // window modality different from Qt::NonModal.
    // If no result was given, this defaults to QModalResult::Cancel.
    void finished(ModalResult result);
public:
    DialogWindow(QWidget *parent = nullptr, bool resizing = true);
    virtual ~DialogWindow();

    ModalResult showModal();
    void show();

    // Closes the window with result. If a derived class refuses the close by ignoring the
    // event, the passed result will be ignored and the window won't close.
    void modalClose(ModalResult result);
    // Returns the last result set with modalClose(). Windows closed with deleteLater() or by
    // setting their WA_DeleteOnClose attribute cannot safely return a result here. In that
    // case, use the finished() signal instead.
    ModalResult modalResult() const;

public slots:
    // Sets modal result to Ok and closes the window.
    void closeOk();
    // Sets modal result to Cancel and closes the window.
    void closeCancel();
    // Sets modal result to Yes and closes the window.
    void closeYes();
    // Sets modal result to No and closes the window.
    void closeNo();
    // Sets modal result to Ignore and closes the window.
    void closeIgnore();
    // Sets modal result to Abort and closes the window.
    void closeAbort();
    // Sets modal result to Save and closes the window.
    void closeSave();
    // Sets modal result to Resume and closes the window.
    void closeResume();
    // Sets modal result to Suspend and closes the window.
    void closeSuspend();
    // Sets modal result to Start and closes the window.
    void closeStart();
    // Sets modal result to Stop and closes the window.
    void closeStop();
    // Sets modal result to Error and closes the window.
    void closeError();
protected:
    // Overrides from Qt:

    virtual bool event(QEvent *e) override;

    // This function MUST be called as the last executed line, even when ignoring the event in
    // a derived class. Otherwise the modalresult won't be reset to Cancel.
    virtual void closeEvent(QCloseEvent *e) override;

    // Handles the default button.
    virtual void keyPressEvent(QKeyEvent *e) override;

    // Position the dialog window to the center of the parent or the center of the screen.
    virtual void showEvent(QShowEvent *e) override;
private:
    QEventLoop *loop;
    ModalResult res;

    typedef QMainWindow base;
};

#endif // DIALOGWINDOW_H
