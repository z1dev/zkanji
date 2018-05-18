/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZKANJIFORM_H
#define ZKANJIFORM_H

#include <QMainWindow>
#include <QSignalMapper>

namespace Ui {
    class ZKanjiForm;
}

enum class Commands {
    FromJapanese, ToJapanese, BrowseJapanese, ToggleExamples,
    ToggleAnyStart, ToggleAnyEnd, ToggleDeinflect, ToggleStrict,
    ToggleFilter, EditFilters, ToggleMultiline,

    WordsToGroup, StudyWord, WordToDict, EditWord, DeleteWord,
    RevertWord, CreateNewWord, CopyWordDef, CopyWordKanji,
    CopyWordKana, CopyWord, AppendWordKanji, AppendWordKana,

    ResetKanjiSearch, StrokeFilter, JLPTFilter, MeaningFilter,
    ReadingFilter, JouyouFilter, RadicalsFilter, IndexFilter,
    SKIPFilter,

    ShowKanjiInfo, KanjiToGroup, EditKanjiDef, CollectKanjiWords,
    CopyKanji, AppendKanji,

    Count
};

enum class CommandCategories {
    NoCateg = 0,

    SearchCateg = 0x10000,
    GroupCateg = 0x20000,

    CommandMask = 0xffff,
    CategMask = 0xf0000,
};

enum class CommandLimits {
    SearchBegin = (int)Commands::FromJapanese,
    SearchEnd = (int)Commands::ToggleMultiline + 1,

    WordGroupBegin = (int)Commands::WordsToGroup,
    WordGroupEnd = (int)Commands::AppendWordKana + 1,

    KanjiSearchBegin = (int)Commands::ResetKanjiSearch,
    KanjiSearchEnd = (int)Commands::SKIPFilter + 1,

    KanjiGroupBegin = (int)Commands::ShowKanjiInfo,
    KanjiGroupEnd = (int)Commands::AppendKanji + 1,

    WordsBegin = SearchBegin,
    WordsEnd = WordGroupEnd,

    KanjiBegin = KanjiSearchBegin,
    KanjiEnd = KanjiGroupEnd,
};

int makeCommand(Commands command, CommandCategories categ = CommandCategories::NoCateg);

class ZDockOverlay : public QWidget
{
public:
    ZDockOverlay(QWidget *parent = nullptr);
    ~ZDockOverlay();
protected:
    virtual void paintEvent(QPaintEvent *e) override;
private:
    typedef QWidget base;
};


class QLayout;
class ZKanjiWidget;
class QXmlStreamWriter;
class QXmlStreamReader;
class Dictionary;
class QActionGroup;
enum class ViewModes;
// Form holding ZKanjiWidgets, both as the main window containing sliders and widgets, and as
// a floating window above the main window.
class ZKanjiForm : public QMainWindow
{
    Q_OBJECT
signals:
    void activated(ZKanjiForm*, bool active);
    // Signals minimized/restored window state change. Used in GlobalUI to install shortcuts
    // and remove/set kanji information window.
    void stateChanged(bool minimized);
public:
    // Constructs a form with a newly created ZKanjiWidget. Set mainform to true for the first
    // created form, and false for the floating windows.
    ZKanjiForm(bool mainform, QWidget *parent = nullptr);
    // Constructs a form with the passed ZKanjiWidget. The form will always be created as a
    // floating window.
    ZKanjiForm(ZKanjiWidget *w, QWidget *parent = nullptr);
    virtual ~ZKanjiForm();

    // Saves form state to the passed xml settings reader. Iterates over every splitter and
    // ZKanjiWidget on the form and creates the same hierarchy in XML.
    void saveXMLSettings(QXmlStreamWriter &writer) const;
    // Reads form state from xml settings that was saved with saveXMLSettings(). If the form
    // has splitter states set but is not a main form, it raises an error in the reader.
    void loadXMLSettings(QXmlStreamReader &reader);

    // Only returns true for the main window of the program.
    bool mainForm() const;

    // Returns true for the main form if it holds more than a single ZKanjiWidget.
    bool hasDockChild() const;

    // Creates a new ZKanjiForm with the widget as its central widget floating over the main
    // window.
    void floatWidget(ZKanjiWidget* w);

    // Updates the position of the window to be placed on screen with the passed number if
    // possible. Tries to place at the same position as it was on the current screen, but
    // moves within screen limits.
    void moveToScreen(int screennum);

    // Shows window as maximized instead if restoremaximized is true.
    virtual void setVisible(bool vis) override;

    // Returns the zkanji widget which is active in the window, or the zkanji widget which was
    // last active, if the window is not active. If this is a floating window, returns the
    // sole widget found on it.
    ZKanjiWidget* activeWidget() const;

    // Updates the main menu on the form, showing or hiding a menu action associated with the
    // passed command.
    void showCommand(int command, bool show);
    // Updates the main menu on the form by enabling or disabling the menu action associated
    // with the passed command.
    void enableCommand(int command, bool enable = true);
    // Updates the main menu on the form by checking or unchecking the menu action associated
    // with the passed command.
    void checkCommand(int command, bool check = true);
    // Updates which menu items are visibled, checked and enabled in the main menu, depending
    // on the widgets of the form.
    void updateMainMenu();
    // Helper for updateMainMenu();
    void updateSubMenu(ZKanjiWidget *w, QMenu *menu, int from, int to, CommandCategories categ);
    // Updates the menu text to show which dictionary is being translated from and to. Pass
    // widgets being active for the search and group menu items, or they will be looked up.
    void updateDictionaryMenu(ZKanjiWidget *srcw = nullptr, ZKanjiWidget *grpw = nullptr);

    // Executes a command from the main menu.
    void executeCommand(int command);
    // Changes view mode for the currently active widget. Either dictionary, group modes or
    // kanji search.
    void setViewMode(int mode);
    // Changes the currently selected dictionary in all the widgets on the form.
    void switchToDictionary(int index);
protected:
    void showDictionaryInfo();

    //virtual void focusInEvent(QFocusEvent *e) override;
    virtual bool event(QEvent *e) override;
    virtual void moveEvent(QMoveEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
    //virtual void contextMenuEvent(QContextMenuEvent *e) override;
    virtual void changeEvent(QEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;

#ifdef Q_OS_WIN
    // Checkes for WM_USER + 999 to wake up when another instance is started
    virtual bool nativeEvent(const QByteArray &etype, void *msg, long *result) override;
#endif
private slots:
    void appFocusChanged(QWidget *prev, QWidget *current);
    void startDockDrag();

    void dictionaryAdded();
    void dictionaryRemoved(int index, int order, void *oldaddress);
    void dictionaryMoved(int from, int to);
    void dictionaryRenamed(const QString &oldname, int index, int order);
    void dictionaryFlagChanged(int index, int order);

    void menuwidgetDestroyed(QObject *o);
private:
    //void installCommands();

    // Installs a command shortcut on non-main forms.
    //void addCommandShortcut(QSignalMapper *map, const QKeySequence &keyseq, int command);

    // Populates the main menu when the form is created.
    void fillMainMenu();
    // Updates the main menu texts on language change events.
    void retranslateMainMenu();
    // Populates a word list search menu either for dictionary searches or word group lists.
    void fillSearchMenu(QSignalMapper *commandmap, QActionGroup *group, QMenu *menu, CommandCategories categ);
    // Updates the search menu texts on language change events.
    void retranslateSearchMenu(QMenu *menu, CommandCategories categ, int from = 0);
    // Populates a kanji list search menu either for dictionary searches or kanji group lists.
    void fillKanjiMenu(QSignalMapper *commandmap, QMenu *menu, CommandCategories categ);
    // Updates the kanji menu texts on language change events.
    void retranslateKanjiMenu(QMenu *menu, CommandCategories categ, int from = 0);

    // Returns whether the main ZKanjiForm at the passed global position is not covered by
    // any other windows.
    bool validDockPos(QPoint gpos);

    // Finds the dock position at the passed local coordinate on the main form. If a
    // ZkanjiForm is passed in what, its contents are docked at the position, otherwise an
    // overlay widget is created and shown where a docking window would go. If a ZKanjiWidget
    // was docked, the original form must delete itself. Only valid to call for the main form.
    void dockAt(QPoint lpos, ZKanjiForm *what = nullptr);
    // Hides the dock overlay widget if it's shown after a call to dockAt().
    void hideDockOverlay();

    // Returns the widget containing or closest to the local lpos coordinates, which is either
    // a ZKanjiWidget or a QSplitter.
    QWidget* getDockPosOwnerAt(QPoint lpos);

    Ui::ZKanjiForm *ui;

    bool mainform;
    // The widget shown as active when this is the main form. The value is not set to null
    // when the application loses focus or all widgets deactivate.
    ZKanjiWidget *activewidget;
    // The open page on the last activated zkanji widget.
    //int activepage;
    // The dictionary last used on the active widget.
    //Dictionary* activedict;

    // A widget is being placed on the form, after the user confirmed the dock position. Set
    // to avoid application focus changed event handling.
    bool docking;

    // True when UpdateDictionaryMenuEvent is sent but hasn't been handled yet. Only one of
    // this event is ever sent when calling updateDictionaryMenu() multiple times.
    bool menupdatepending;
    // Dictionary search widget passed to the last call to updateDictionaryMenu().
    ZKanjiWidget *menusearchwidget;
    // Group widget passed to the last call to updateDictionaryMenu().
    ZKanjiWidget *menugroupwidget;

    // Rectangle of the frameGeometry saved when pressing the mouse button on the non-client
    // area of a form. If its value changes, the form is not dragged but resized.
    //QRect framerect;

    // Overlay widget shown on the main form while dragging a window above the main form, to
    // show the destination of a dock operation.
    ZDockOverlay *overlay;

    // Position and size gained from pos() and size() every time the window is resized as long
    // as it's not maximized or minimized. Used for saving and restoring the window state in
    // the settings.
    QRect settingsrect;
    // The window is either maximized, or was maximized before it got minimized. Used when
    // zkanji starts minimized.
    bool restoremaximized;

    // Set to true if the changeEvent should ignore the window's minimized/maximized state
    // changes.
    bool skipchange;

    // Set to true to prevent updateMainMenu() calls during startup.
    bool skipmenu;

    QMenu *dictmenu;
    QMenu *viewmenu;
    QMenu *wordsearchmenu;
    QMenu *wordgroupmenu;
    QMenu *kanjisearchmenu;
    QMenu *kanjigroupmenu;
    QMenu *helpmenu;
    QSignalMapper *dictmap;
    QSignalMapper *commandmap;
    QActionGroup *viewgroup;
    QActionGroup *dictgroup;
    QActionGroup *searchgroup;
    QActionGroup *wordsgroup;

    typedef QMainWindow base;
};


#endif // ZKANJIFORM_H
