/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef POPUPDICT_H
#define POPUPDICT_H

#include <QMainWindow>
#include "zwindow.h"
#include "zevents.h"

namespace Ui {
    class PopupDictionary;
}

// Notifies a main window to change its active dictionary to another.
class ShowPopupEvent : public EventTBase<ShowPopupEvent>
{
private:
    typedef EventTBase<ShowPopupEvent>  base;
public:
    ShowPopupEvent(bool screen, bool translatefrom) : base(), scr(screen), val(translatefrom) { ; }
    bool from() { return val; }
    bool screen() { return scr; }
private:
    bool scr;
    bool val;

};

class PopupDictionary : public ZWindow
{
    Q_OBJECT
signals:
    // Emited by the closeEvent() function.
    void beingClosed();
public:
    virtual ~PopupDictionary();

    static void popup(int screen, bool fromjapanese);
    static void hidePopup();
    static PopupDictionary* getInstance();

    // The index of the currently used dictionary. Returns the dictionary used in the instance
    // or if no instance is present, the dictionary to be set for the instance.
    static int dictionaryIndex();
protected:
    void doPopup(int screen, bool translatefrom);

    virtual QWidget* captionWidget() const override;
    
    virtual void keyPressEvent(QKeyEvent *e) override;

    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void moveEvent(QMoveEvent *e) override;

    virtual bool event(QEvent *e) override;

    virtual void closeEvent(QCloseEvent *e) override;
private slots:
    // Hides the popup window when the application loses the active state.
    void appStateChange(Qt::ApplicationState state);
    void on_pinButton_clicked(bool checked);
    void on_floatButton_clicked(bool checked);
    void on_dictionary_wordDoubleClicked(int windex, int dindex);

    void settingsChanged();
    void dictionaryRemoved(int index);
private:
    PopupDictionary(QWidget *parent = nullptr);

    // Floats or docks the popup form depending on the argument. Pass a screen number, which
    // will be used to show the popup window on, in case it's not yet visible. A value of -1
    // leaves the popup on the current window.
    void floatWindow(bool dofloat, int screen = -1);
    // Changes the width of the window if it should be resized to fit the screen width.
    void resizeToFullWidth();
    // Changes the currently displayed dictionary.
    void setDictionary(int index);

    static PopupDictionary *instance;

    Ui::PopupDictionary *ui;

    bool floating;

    // The point relative to the popup window where the user pressed the mouse button for
    // moving the window.
    QPoint grabpos;
    // Whether the window is being grabbed by its caption.
    bool grabbing;

    // True while adjusting the window size, to prevent the change saved in the settings.
    bool ignoreresize;

    // Index of the currently displayed dictionary.
    int dictindex;

    typedef ZWindow base;
};


#endif // POPUPDICT_H
