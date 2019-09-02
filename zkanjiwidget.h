/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZKANJIWIDGET_H
#define ZKANJIWIDGET_H

#include <QWidget>
#include <QMenu>

namespace Ui {
    class ZKanjiWidget;
}


class QAbstractButton;
class DropMenu : public QMenu
{
    Q_OBJECT

public:
    DropMenu(QWidget *parent = nullptr);

    // Must be called once before the menu is shown. Sets the button which will be the
    // position where the menu is shown.
    void setButton(QAbstractButton *btn);
protected:
    void showEvent(QShowEvent *event) override;
private:
    // The menu drops down below the button, or when not enough space, above it.
    QAbstractButton *button;

    typedef QMenu   base;
};

enum class ViewModes { WordSearch, WordGroup, KanjiGroup, KanjiSearch };


class Dictionary;
class WordGroup;
class QMenu;
class QXmlStreamWriter;
class QXmlStreamReader;
class QSignalMapper;
class ZKanjiWidget : public QWidget
{
    Q_OBJECT
public:
    ZKanjiWidget(QWidget *parent = nullptr);
    ~ZKanjiWidget();

    // Returns the first parent with the ZKanjiWidget class of the passed widget, or null, if
    // the widget is not on a ZKanjiWidget parent.
    static ZKanjiWidget* getZParent(QWidget *widget);

    // Saves widget state to the passed xml stream.
    void saveXMLSettings(QXmlStreamWriter &writer) const;
    // Loads the widget state from the passed xml stream.
    void loadXMLSettings(QXmlStreamReader &reader);

    // Returns the currently displayed dictionary. Change it by posting the
    // SetDictEvent event to the window.
    Dictionary* dictionary();
    // Returns the index of the displayed dictionary in the global dictionary list.
    int dictionaryIndex() const;

    void setDictionary(int index);

    // See UICommands::Commands enum in globalui.h
    void executeCommand(int command);

    // Returns whether a given command is valid and active in the widget in its current state.
    void commandState(int command, bool &enabled, bool &checked, bool &visible) const;

    // Choose what's displayed in the widget.
    // 0: Dictionary, 1: Word Groups, 2: Kanji Groups, 3: Kanji Search
    void setMode(ViewModes newmode);

    // Returns what is displayed currently in the widget.
    // 0: Dictionary, 1: Word Groups, 2: Kanji Groups, 3: Kanji Search
    ViewModes mode() const;

    // Called before the widget is placed on a main ZKanjiForm, or when undocked into a
    // floating ZKanjiForm. Updates the popup menu showed for empty parts between widgets.
    //void docking(bool dock);

    // Updates the appearance of the widget to make it recognizably active compared to other
    // ZKanjiWidgets on the main form.
    void showActivated(bool active);

    // Sets a main menu action that will be updated when the docking allowed option changes
    // for the widget.
    //void setDockAllowAction(QAction *a);

    // Adds the allow docking or float menu items to the passed menu. If the menu already has
    // actions in it, first a separator is added.
    void addDockAction(QMenu *dest);

    // Whether moving the widget's window over the main window can result in docking or not.
    //bool dockingAllowed() const;

    // Returns whether the widget is docked into the main form, and is not on a floating
    // window.
    bool docked() const;

    // Returns whether this widget is active on its form. For floating window parents, always
    // returns true.
    bool isActiveWidget() const;
public slots:
    void allowActionTriggered(bool checked);
protected:
    virtual bool event(QEvent *e) override;
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void contextMenuEvent(QContextMenuEvent *e) override;
    //virtual void keyPressEvent(QKeyEvent *e) override;
private slots:
    void allowActionDestroyed();

    //void on_newWindowButton_clicked();
    //void on_deckWindowButton_clicked();
    void on_dictionary_wordDoubleClicked(int windex, int dindex);
    void on_pagesStack_currentChanged(int index);
    //void on_modeButton_clicked();
    //void on_modeCBox_currentIndexChanged(int index);
    //void on_kanjiSearchButton_toggled(bool checked);
    //void on_dictPageButton_toggled(bool checked);
    //void on_wordGroupPageButton_toggled(bool checked);
    //void on_kanjiGroupPageButton_toggled(bool checked);

    void dictionaryAdded();
    void dictionaryRemoved(int index, int order);
    void dictionaryMoved(int from, int to);
    void dictionaryRenamed(const QString &oldname, int index, int order);
    void dictionaryFlagChanged(int index, int order);
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);
private:
    // Change texts of the mode menu to match the currently selected language.
    void setTranslationTexts();

    // Changes what's displayed in the window, depending on the passed action. The action
    // must be one of the actions in the modeMenu.
    void setModeByAction();

    // Changes the current dictionary in the widget, depending on the action calling it from
    // dictMenu.
    void setDictByAction();

    // Only call when the widget is on the main window with other widgets. Notifies the main
    // window to float this widget to its own ZKanjiForm.
    void floatToWindow();

    // Pre-render button icons for the mode change button at the top left corner.
    //static void renderButton(QPixmap &dest, const QPixmap &orig);

    // Callback for adding word to group.
    //static void wordToGroup(WordGroup *g, int w);

    Ui::ZKanjiWidget *ui;

    // Index of the currently selected dictionary in the widget.
    int dictindex;

    // Images used on the mode change button.
    QPixmap dictimg;
    QPixmap wgrpimg;
    QPixmap kgrpimg;
    QPixmap ksrcimg;

    // Menu shown when pressing the mode button.
    DropMenu modemenu;
    // Menu shown when pressing the dictionary selector button.
    DropMenu dictmenu;

    // A popup menu with docking options, activated on right clicking free parts of the
    // widgets.
    QMenu dockmenu;

    // Set to true when the widget is the active widget on a main form and should be painted
    // as such.
    bool paintactive;

    // Whether moving the widget's window over the main window can result in docking or not.
    bool allowdocking;
    // Action in the main menu for allowing or disallowing docking. Only used for floating
    // widgets.
    QAction* allowdockaction;

    typedef QWidget base;
};


#endif // ZKANJIWIDGET_H
