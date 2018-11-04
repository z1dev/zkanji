/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef POPUPKANJISEARCH_H
#define POPUPKANJISEARCH_H

#include "zwindow.h"

namespace Ui {
    class PopupKanjiSearch;
}

class PopupKanjiSearch : public ZWindow
{
    Q_OBJECT
signals :
    // Emited by the closeEvent() function.
    void beingClosed();
public:
    virtual ~PopupKanjiSearch();

    static void popup(int screen);
    static void hidePopup();
    static PopupKanjiSearch* getInstance();
protected:
    void doPopup(int screen);

    virtual QWidget* captionWidget() const override;

    virtual void keyPressEvent(QKeyEvent *e) override;

    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void moveEvent(QMoveEvent *e) override;

    virtual void closeEvent(QCloseEvent *e) override;

    virtual bool event(QEvent *e) override;
private slots:
    // Hides the popup window when the application loses the active state.
    void appStateChange(Qt::ApplicationState state);
    void on_pinButton_clicked(bool checked);

    void settingsChanged();
private:
    PopupKanjiSearch(QWidget *parent = nullptr);

    static PopupKanjiSearch *instance;

    Ui::PopupKanjiSearch *ui;

    // The point relative to the popup window where the user pressed the mouse button for
    // moving the window.
    QPoint grabpos;

    // True while adjusting the window size, to prevent the change saved in the settings.
    bool ignoreresize;

    typedef ZWindow base;
};

#endif // POPUPKANJISEARCH_H

