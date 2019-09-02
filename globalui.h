/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GLOBALUI_H
#define GLOBALUI_H

#include <QObject>
#include <QSignalMapper>
#include <QActionGroup>
#include <QAction>
#include <QSet>
#include <QDateTime>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QBasicTimer>

#include <memory>


class ZKanjiForm;
class ZKanjiWidget;
class QMainWindow;
class KanjiForm;
class KanjiInfoForm;
class QXmlStreamReader;
class QXmlStreamWriter;
class Dictionary;
class QWindow;
class QSpacerItem;


// Structure for hiding / showing app windows in a safe way. Calls GlobalUI::hideAppWindows()
// on creation and GlobalUI::showAppWindows() on destruction. Multiple objects of this type
// are safe too.
class HideAppWindowsGuard
{
public:
    HideAppWindowsGuard(const HideAppWindowsGuard&) = delete;
    HideAppWindowsGuard& operator=(const HideAppWindowsGuard&) = delete;
    HideAppWindowsGuard(HideAppWindowsGuard&&) = delete;
    HideAppWindowsGuard& operator=(HideAppWindowsGuard&&) = delete;

    HideAppWindowsGuard();
    ~HideAppWindowsGuard();

    // Shows app windows again. Once called, calling it again or destroying the object won't
    // have an effect.
    void release();
private:
    bool released;
};

// Enables or disables the auto save of dictionary and user data. Call release() or destroy
// the guard to enable auto save again.
class AutoSaveGuard
{
public:
    AutoSaveGuard(const AutoSaveGuard&) = delete;
    AutoSaveGuard& operator=(const AutoSaveGuard&) = delete;
    AutoSaveGuard(AutoSaveGuard&&) = delete;
    AutoSaveGuard& operator=(AutoSaveGuard&&) = delete;

    AutoSaveGuard();
    ~AutoSaveGuard();

    void release();
private:
    bool released;
};

// Guard object to use on flag values that need to take up a different value when the guard
// goes out of scope.
template<typename T>
class FlagGuard
{
private:
    T *what;
    T closeval;
public:
    FlagGuard(const FlagGuard&) = delete;
    FlagGuard& operator=(const FlagGuard&) = delete;
    FlagGuard(FlagGuard&&) = delete;
    FlagGuard& operator=(FlagGuard&&) = delete;

    FlagGuard(T *what, T closeval) : what(what), closeval(closeval) {}
    FlagGuard(T *what, T startval, T closeval) : what(what), closeval(closeval)
    {
        if (what != nullptr)
            *what = startval;
    }
    ~FlagGuard() {
        if (what != nullptr)
            *what = closeval;
    }

    void release(bool updateval = false) {
        if (updateval && what != nullptr)
            *what = closeval;
        what = nullptr; }
};

// Class with the role of creating different Qt objects and be their global parent.
class GlobalUI : public QObject
{
    Q_OBJECT
signals:
    void settingsChanged();
    void kanjiInfoShown();
    // Sent before hideAppWindows() or after showAppWindows() were executed.
    void appWindowsVisibilityChanged(bool shown);

    void sentencesReset();

    void dictionaryAdded();
    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
    void dictionaryRemoved(int index, int orderindex, void *oldaddress);
    void dictionaryMoved(int from, int to);
    void dictionaryRenamed(const QString &oldname, int index, int orderindex);
    // Emitted when a dictionary was replaced during update. Saved dictionary pointers or
    // other data coming from olddict will be invalidated after this call, and the newdict has
    // already been inserted in the dictionary list.
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);

    void dictionaryFlagChanged(int index, int orderindex);
public:
    typedef size_t  size_type;

    static GlobalUI* instance();
    ~GlobalUI();

    // Loads a (main) window state from the XML with reader, creating the window. The window
    // is not shown until the first createWindow(true) is called.
    void loadXMLWindow(bool ismain, QXmlStreamReader &reader);

    // Saves the created kanji information windows' positions and their states in XML with
    // writer.
    void saveXMLKanjiInfo(QXmlStreamWriter &writer);
    // Loads kanji information windows' positions and their states from the XML with reader.
    void loadXMLKanjiInfo(QXmlStreamReader &reader);

    // Saves which study decks had new words added to them last in each dictionary.
    void saveXMLLastDecks(QXmlStreamWriter &writer);
    // Loads which study decks had new words added to them last in each dictionary.
    void loadXMLLastDecks(QXmlStreamReader &reader);

    // Saves which word and kanji groups were last selected in a select group dialog in each
    // dictionary.
    void saveXMLLastGroups(QXmlStreamWriter &writer);
    // Loads which word and kanji groups were last selected in a select group dialog in each
    // dictionary.
    void loadXMLLastGroups(QXmlStreamReader &reader);

    // Saves general settings in dialogs.
    void saveXMLLastSettings(QXmlStreamWriter &writer);
    // Loads general settings in dialogs.
    void loadXMLLastSettings(QXmlStreamReader &reader);

    // Creates and shows a ZKanji window. Pass true in ismain for the first created window.
    // If ismain is true and a main window already exists, the function returns immediately.
    void createWindow(bool ismain = false);

    // Includes the zkanji window in the top level windows list and connects it to slots. Only
    // call for previously created windows that were not created through createWindow.
    void addCreatedWindow(ZKanjiForm *f);

    // Shows a kanji list window. Set useexisting to true if the last opened
    // window should be shown instead if one exists.
    //void createKanjiWindow(bool useexisting = false);

    // Returns the main form of the program, which has a main menu and can be used as a dock
    // target.
    ZKanjiForm* mainForm() const;

    // Returns the form to be used as the parent of opened dialog and tool windows. This can
    // be either the mainForm(), the popup dictionary or kanji search windows, or the topmost
    // modal dialog window, depending on which one is active at the moment.
    QWidget* activeMainForm() const;

    // Raises or restores the application from the tray and activates the active main form.
    void raiseAndActivate();

    // Number of existing forms including the main form with zkanji controls.
    int formCount() const;
    // Returns the existing forms. The first one is always the main form. The others are in
    // order of activation.
    ZKanjiForm* forms(int ix) const;

    // Whether the main form or the popup dictionary/kanji search is visible, and no dialog
    // windows block them.
    bool hasMainAccess() const;

    // The floating form currently being positioned to be docked into the main form. Only set
    // while docking targetting takes place.
    ZKanjiForm* dockForm() const;

    // The last dictionary a word has been added to via the word to dictionary dialog.
    Dictionary* lastWordDestination() const;
    // Set the last dictionary a word has been added to from the word to dictionary dialog.
    void setLastWordDestination(Dictionary *d);

    // Calls either the global wordToGroupSelect() or wordToDictionarySelect() depending on
    // the last window shown and accepted with the ok button.
    void wordToDestSelect(Dictionary *d, int windex, bool showmodal = false);

    enum class LastWordDialog { Group, Dictionary };
    // Changes which dialog the wordToDestSelect will show next.
    void setLastWordToDialog(LastWordDialog last);

    // Hides every top level window apart from the main form, and notifies it to show the dock
    // panels while the mouse is moved.
    void startDockDrag(ZKanjiForm *form);
    // Displays all the windows again.
    void endDockDrag();

    //// Returns the picture of the flag of a country at index position. The flag is the large
    //// version that can be used on the whole button. The flags are generated at the first
    //// call.
    //QPixmap flags(int index);

    // Tries to determine the index of the dictionary used by the current active widget or
    // window. If it can't be determined because the active widgets don't support it, the
    // result is -1.
    int activeDictionaryIndex() const;

    // Emits the settingsChanged() signals to notify controls to update their looks and
    // behavior.
    void applySettings();

    // Sets the application stylesheet to have some stretch sizes and colors.
    void applyStyleSheet();

    // Scales dimensions of child widgets and of the widget itself depending on current
    // scaling.
    void scaleWidget(QWidget *w);

    // Prevents scaling of the passed widget and all its children when itself or its parent is
    // passed to scaleWidget() later.
    void preventWidgetScale(QWidget *w);

    // Copies str to clipboard as plain text, overwriting its contents.
    void clipCopy(const QString &str) const;
    // Appends str to the plain text found in the clipboard. If no plain text is stored there,
    // copies str instead.
    void clipAppend(const QString &str) const;

    void showKanjiInfo(Dictionary *d, int index);
    // Updates the kanji information window if it's visible, to display the kanji from the d
    // dictionary.
    void setInfoKanji(Dictionary *d, int index);

    // Shows or hides the kanji information windows and allows or prevents new ones to open.
    // Set toggleon to true to show and enable, or to false to disallow kanji info windows.
    void toggleKanjiInfo(bool toggleon = true);

    // Returns true if kanji information could be shown by calling showKanjiInfo(). Some
    // operations, like showing a modal window, might block kanji information to be shown.
    bool kanjiInfoAllowed() const;

    // Adds a shortcut to the passed menu via the mapper. The passed UICommand will be
    // executed by executeCommand on the active ZKanjiWidget, when the shortcut is used.
    QAction* addCommandAction(QSignalMapper *map, QMenu *parentmenu, const QString &str, const QKeySequence &keyseq, int command, bool checkable = false, QActionGroup *group = nullptr);

    // Adds a shortcut to the passed menu via the mapper at the passed position. The
    // passed UICommand will be executed by executeCommand on the active ZKanjiWidget,
    // when the shortcut is used.
    QAction* insertCommandAction(QSignalMapper *map, QMenu *parentmenu, int pos, const QString &str, const QKeySequence &keyseq, int command, bool checkable = false, QActionGroup *group = nullptr);

    void minimizeToTray();
    void restoreFromTray();
    bool isInTray() const;

    virtual bool eventFilter(QObject *obj, QEvent *e) override;

    void signalDictionaryAdded();
    void signalDictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
    void signalDictionaryRemoved(int index, int orderindex, void *oldaddress);
    void signalDictionaryMoved(int from, int to);
    void signalDictionaryRenamed(const QString &oldname, int index, int orderindex);
    void signalDictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);

    void signalDictionaryFlagChange(int index);
protected:
    virtual void timerEvent(QTimerEvent *e) override;
    virtual bool event(QEvent *e) override;
public slots:
#ifndef Q_OS_WIN
    void secondAppStarted();
#endif
    void importBaseDict();
    void importExamples();
    void importOldUserData();

    void userExportAction();
    void userImportAction();
    void dictExportAction();
    void dictImportAction();
    void saveUserData();
    void showSettingsWindow();
    // Called at the start of the program to load the scaling value from the ini file. All
    // other values are loaded in loadSettings().
    void loadScalingSetting();
    void saveSettingsAndStates();
    // Load program settings.
    void loadSettings();
    // Load dictionary ordering and window states after the data files were initialized.
    void loadStates();

    // Determines whether we are currently working with a light or dark color palette, and
    // updates the lighttheme value in ColorSettings.
    void checkColorTheme();

    // 0: From Japanese, 1: To Japanese, 2: Kanji popup.
    void showPopup(int which);

    // Closes the main window. If there is no main window (yet?), the call is ignored. In case
    // the main window is hidden because the program is minimized, the application exits.
    void quit();

    // Shows the dialog to practice hiragana and katakana.
    void practiceKana();

    // Shows the long term study list deck listing.
    void showDecks();

    // Hides every currently visible window and saves them in a list to show them later with
    // showAppWindows(). Each call to hideAppWindows() increases a counter, that can only be
    // cleared by calling showAppWindows() the same number of times, before the windows are
    // shown again.
    void hideAppWindows();
    // Restores the windows previously hidden with hideAppWindows(). Each call to
    // hideAppWindows() increases a counter, that can only be cleared by calling
    // showAppWindows() the same number of times, before the windows are shown again.
    void showAppWindows();

    // Displays a simplistic about window for the program.
    void showAbout();

    // Saves user data, user settings and form states.
    void saveBeforeQuit();
private slots:

    void kanjiInfoDestroyed();
    void kanjiInfoLock(bool locked);
    //void appStateChanged(Qt::ApplicationState state);
    void popupClosing();
    void mainStateChanged(bool minimized);
    void appFocusChanged(QWidget *lost, QWidget *received);
    void hiddenWindowDestroyed(QObject *o);
    void trayActivate(QSystemTrayIcon::ActivationReason reason);

    // Called when a window in one of the lists gets deleted.
    void formDestroyed(QObject *form);
    // Called when one of the forms get input focus.
    void formActivated(ZKanjiForm *form, bool active);

    // When called, increments a counter which must be zero to enable auto save. Call
    // enableAutoSave() the same number of times as disableAutoSave() was called to start
    // saving again. Use AutoSaveGuard objects outside this class.
    void disableAutoSave();
    // When called, decrements a counter which must be zero to enable auto save. Call
    // enableAutoSave() the same number of times as disableAutoSave() was called to start
    // saving again. Use AutoSaveGuard objects outside this class.
    void enableAutoSave();

    void scaledWidgetDestroyed(QObject *o);
private:
    GlobalUI(QObject *parent = nullptr);

    // Installs and uninstalls system wide shortcuts for the popup dictionaries.
    void installShortcuts(bool install);

    // Helper for scaleWidget(). Scales spacer items in layouts.
    void _scaleSpacerItem(QSpacerItem *s);
    // Helper for scaleWidget(). Scales layouts.
    void _scaleLayout(QLayout *l);
    // Helper for scaleWidget(). Scales widget and children.
    void _scaleWidget(QWidget *w);
    // Registers widget as scaled to stop scaling it a second time.
    void _registerWidgetScale(QWidget *w);

    // Closes all open zkanji windows.
    void closeAll();

    KanjiInfoForm *kanjiinfo;
    // Blocks showing new kanji info when positive non zero.
    int infoblock;

    // Only instance of the global UI class. Used as the this pointer in every
    // static call where a globalUI pointer is needed.
    static GlobalUI *i;

    // The first object added to the vector is the main form. Holds the forms in their z-order
    // moving the last activated child window to the end of the list.
    /*static*/ std::vector<ZKanjiForm*> mainforms;

    //// Pixmaps of the country flags used on dictionary buttons and in the menu.
    //std::vector<QPixmap> flagimg;

    // Form being dragged for docking.
    ZKanjiForm *dockform;
    // Position of form originally before docking started, in case the dock is cancelled.
    QPoint dockstartpos;

    // Windows hidden when hideAppWindows() or startDockDrag() is called.
    std::vector<QWidget*> appwin;
    // Number of times hideAppWindows() have been called. This is decremented when calling
    // showAppWindows(), and when it reaches 0, the hidden windows are shown again.
    int hiddencounter;
    // Number of times disableAutoSave() was called without a follow up enableAutoSave().
    // Auto save of dictionary and user data is only enabled, if autosavecounter is zero and
    // the settings allow it.
    int autosavecounter;

    // Timer for ui tasks like saving user data. It fires at 2 second intervals.
    QBasicTimer uiTimer;
    // Date and time when the user database was last saved.
    QDateTime lastsave;

    std::unique_ptr<QSystemTrayIcon> trayicon;
    std::unique_ptr<QMenu> traymenu;

    // Maps the popup dictionary keyboard shortcuts and tray icon context menu to showPopup
    // calls.
    QSignalMapper popupmap;

    // Widgets scaled by calling scaleWidget() or preventWidgetScale().
    QSet<QWidget*> scaledwidgets;

    Dictionary *lastworddict;

    LastWordDialog lastworddiag;
    
    friend class HideAppWindowsGuard;
    friend class AutoSaveGuard;
    typedef QObject base;
};

#define gUI GlobalUI::instance()


#endif // GLOBALUI_H
