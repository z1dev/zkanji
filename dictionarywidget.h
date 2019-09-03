/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef DICTIONARYWIDGET_H
#define DICTIONARYWIDGET_H

#include <QWidget>
#include <QStringListModel>
#include <QBasicTimer>

#include <memory>
#include <functional>

namespace Ui {
    class DictionaryWidget;
}

enum class CommandCategories;

class QModelIndex;
class Dictionary;
class DictionaryItemModel;
class DictionaryBrowseItemModel;
class DictionarySearchResultItemModel;
class DictionarySearchFilterProxyModel;
class MultiLineDictionaryItemModel;
struct WordEntry;
class WordGroup;
class ZAbstractTableModel;
class QXmlStreamWriter;
class QXmlStreamReader;
class ZDictionaryListView;
class ZComboBox;
class QStringListModel;
class QSignalMapper;
class QMenu;
class ZStatusBar;
struct DictionaryWidgetData;
enum class SearchMode : uchar;
enum class BrowseOrder : uchar;

enum class Inclusion : uchar;
enum class ListSelectionType;

enum class DictColumnTypes;

struct WordFilterConditions;

// (Model, Column index, row A, row B) - The rows are for the source model before filtering,
// no further conversion needed.
typedef std::function<bool(DictionaryItemModel*, int, int, int)> ProxySortFunction;

class DictionaryWidget : public QWidget
{
    Q_OBJECT
signals:
    // Sent when the row selection changed for single selection dictionary tables.
    void rowChanged(int row, int oldrow);
    // Sent when the selection changed.
    void rowSelectionChanged();
    void wordDoubleClicked(int windex, int dindex);
    void sortIndicatorChanged(int index, Qt::SortOrder order);

    void customizeContextMenu(QMenu *menu, QAction *insertpos, Dictionary *dict, DictColumnTypes coltype, QString selstr, const std::vector<int> &windexes, const std::vector<ushort> &kindexes);
public:
    // Determines where the displayed words come from and how they are looked up.
    // DictSearch: The displayed words are taken from dictionary searches. The browse mode
    //      displays every word in the dictionary. Entering a search string in J-E and E-J
    //      lookups will search for words according to their Japanese or English definition.
    // Filtered: The displayed words are taken from a model passed to setModel() and filtered
    //      by user input with a filter model. The browse mode is hidden, and every word are
    //      visible in both J-E and E-J lookups unless filtered.
    enum ListMode : uchar { DictSearch, Filtered };

    DictionaryWidget(QWidget *parent = nullptr);
    ~DictionaryWidget();

    // Saves widget state to the passed xml stream.
    void saveXMLSettings(QXmlStreamWriter &writer) const;
    // Loads widget state from the passed xml stream.
    void loadXMLSettings(QXmlStreamReader &reader);

    void saveState(DictionaryWidgetData &data) const;
    void restoreState(const DictionaryWidgetData &data);

    // Pass a statusbar to use it to display data about the listed words.
    void assignStatusBar(ZStatusBar *bar);
    // The statusbar is only shown on the main window and the popup dictionary by default.
    // Restores the hidden status bar and assigns it to the dictionary widget.
    void showStatusBar();

    bool isSavingColumnData() const;
    void setSaveColumnData(bool save);

    // Inserts a margin on the left side of the toolbar for the window selector button above
    // the widget outside the normal layout.
    //void makeModeSpace(const QSize &size);

    ZDictionaryListView* view();
    const ZDictionaryListView* view() const;

    Dictionary* dictionary();
    const Dictionary* dictionary() const;

    void setDictionary(Dictionary *newdict);
    void setDictionary(int index);

    // See UICommands::Commands in globalui.h
    void executeCommand(int command);

    // Returns whether a given command is valid and active in the widget in its current state.
    void commandState(int command, bool &enabled, bool &checked, bool &visible) const;

    //BrowseOrder browseOrder() const;
    //void setBrowseOrder(BrowseOrder bo);

    ListMode listMode() const;
    void setListMode(ListMode newmode);

    SearchMode searchMode() const;
    void setSearchMode(SearchMode mode);

    ListSelectionType selectionType() const;
    void setSelectionType(ListSelectionType seltype);

    bool acceptWordDrops() const;
    void setAcceptWordDrops(bool droptarget);

    bool wordDragEnabled() const;
    void setWordDragEnabled(bool enablestate);

    bool isSortIndicatorVisible() const;
    void setSortIndicatorVisible(bool showit);

    // Sets a function to use when sorting in the dictionary by the current sort indicator. To
    // sort the dictionary call sortByIndicator(). Pass null in func to reset the sort order.
    // Setting a different model or changing the display mode resets the sort function.
    void setSortFunction(ProxySortFunction func);

    // Sets the displayed position and direction of the sort indicator in the horizontal
    // header. Does not sort the model. Use sortByIndicator() for sorting after the position
    // is set correctly.
    void setSortIndicator(int index, Qt::SortOrder order);

    // Sorts or disables sorting in the dictionary view for the header view's indicator
    // column. Only works when displaying a custom model, and not in dictionary search mode.
    void sortByIndicator();

    int checkBoxColumn() const;
    void setCheckBoxColumn(int newcol);

    // Adds a widget to the toolbar left of the other widgets. Pass true in separate to add a
    // separator line between the new widget and the existing contents.
    void addFrontWidget(QWidget *widget, bool separated = false);
    // Adds a widget to the toolbar at the far right end. Pass true in separate to add a
    // separator line between the new widget and the existing contents.
    void addBackWidget(QWidget *widget, bool separated = false);

    // Returns the model currently set in the table. This can differ from the data set by
    // setModel(), which can be a filtered source model.
    //ZAbstractTableModel* currentModel();

    // Sets list mode to Filtered and updates the widget to display the passed model.
    void setModel(DictionaryItemModel *newmodel);

    // Uses setModel() to change the displayed model in the widget to a group's items.
    void setGroup(WordGroup *newgroup);

    // Returns the word listed in the words table at row.
    WordEntry* words(int row) const;

    // Returns the index of the word in the dictionary at row.
    int wordIndex(int row) const;

    // Returns the word of the current item in the dictionary list.
    WordEntry* currentWord() const;

    // Returns the dictionary index of the current item in the dictionary list.
    int currentIndex() const;

    // Returns the current row in the dictionary.
    int currentRow() const;

    // Returns whether there are any rows displayed as selected in the table.
    bool hasSelection() const;

    // Returns the number of rows selected.
    int selectionSize() const;
    // Returns dictionary words indexes of selected rows in indexes.
    void selectedIndexes(std::vector<int> &indexes) const;
    // Returns the selected rows in the dictionary.
    void selectedRows(std::vector<int> &rows) const;
    // Converts row indexes in filteredrows to the row indexes in the base model before
    // filtering. The order of the result is not defined and can be random.
    void unfilteredRows(const std::vector<int> &filteredrows, std::vector<int> &result);
    // Returns whether the displayed items are the result of a search.
    bool filtered() const;

    // Returns the count of the currently displayed rows. In multiline mode, the lines are
    // counted separately.
    int rowCount() const;

    // Returns true if the table contains selected rows, and there are no gaps in the
    // selection.
    bool continuousSelection() const;

    // Returns whether the selection contains the row. The temporary range selection is also
    // considered, not just the saved selections. In multiline mode, the lines are counted
    // separately.
    bool rowSelected(int rowindex) const;

    // Sets the search text to the passed string, searching the dictionary and populating the
    // words table with the results. If a given word with both kana and kanji specified should
    // be selected in the results list, use search() instead.
    void setSearchText(const QString &str);

    // Searches for a word with kanji and kana, setting the search text to one of them, and
    // selecting the matching word. In Japanese mode the kanji is used, while in Browse mode,
    // the kana will be the search text. If a matching word is not found, the top result will
    // be selected.
    //void search(const QString &kanji, const QString &kana);

    // Returns the text in the currently shown line edit.
    QString searchText() const;

    // Returns whether the conjugation/inflection button is show.
    bool inflButtonVisible() const;
    // Set the visibility of the conjugation/inflection button.
    void setInflButtonVisible(bool visible);

    // Returns whether the button for word examples and the examples panel itself are shown or
    // hidden.
    bool examplesVisible() const;
    // Set the visibility of the button for the word examples and the examples panel.
    void setExamplesVisible(bool shown);
    // Set whether to allow or prevent the word examples panel to switch to another word by
    // clicking inside the text area.
    void setExamplesInteraction(bool allow);

    // Returns whether the button to set the display mode of the dictionary to multi line is
    // visible or not.
    bool multilineVisible() const;
    // Changes the visibility of the button to set the display mode of the dictionary to multi
    // line.
    void setMultilineVisible(bool shown);

    // Returns whether the button for the word filters is shown after the search text input.
    bool wordFiltersVisible() const;
    // Changes the visibility of the word filters button after the search text input.
    void setWordFiltersVisible(bool shown);

    // Turns on filtering, setting the filter with the given index to included or excluded.
    // Ignore is currently not supported.
    void turnOnWordFilter(int index, Inclusion inc);

    // Set whether the the dictionary list view displays user defined study definitions
    // instead of the dictionary definition. If a user defined study definition is not found,
    // the original definition will be displayed but without word tags or meaning numbers.
    // Translation to Japanese searches will try to use the dictionary's study definition tree
    // for the search.
    void setStudyDefinitionUsed(bool study);

    // Returns whether the the dictionary list view displays user defined study definitions
    // instead of the dictionary definition. If a user defined study definition is not found,
    // the original definition will be displayed but without word tags or meaning numbers.
    // Translation to Japanese searches will try to use the dictionary's study definition tree
    // for the search.
    bool isStudyDefinitionUsed() const;

    // Sets the search text to the kana form of the word at windex, selecting the row of the
    // word. If the current search mode is Definition, it's changed to Japanese.
    // Only valid to call when the list mode is DictSearch.
    void browseTo(Dictionary *d, int windex);

    // Usually the dictionary widget updates only when it's shown. Calling this will force
    // the populating of the displayed model once even while the widget is hidden. Does not
    // update the model by itself.
    void forceUpdate();

    // Scrolls back to the start of the words list.
    void resetScrollPosition();
    // Scrolls horizontally back to the start of the words list.
    void resetHorizontalScroll();
protected:
    virtual void showEvent(QShowEvent *e) override;
    virtual void timerEvent(QTimerEvent *e) override;
    virtual bool event(QEvent *e) override;
    virtual bool eventFilter(QObject *obj, QEvent *e) override;
private slots:
    // Emitted when the current row changes in the table.
    void tableRowChanged(int row, int prev);

    // The user clicked a word on the currently displayed example sentence.
    // Look up the word in the dictionary. The arguments identify the sentence
    // and the word clicked in it.
    void exampleWordSelected(ushort block, uchar line, int wordpos, int wordform);

    // The user right clicked the dictionary filter button and a context menu
    // should be shown above it.
    void showFilterContext(const QPoint &pos);

    // The include state of a filter condition has changed.
    void filterInclude(int index, Inclusion oldinclude);
    // A condition (i.e. words in example) in the filter conditions has changed.
    void conditionChanged();

    // Removes the filter at index, repopulating the word list table if the change affects
    // one of the included filters.
    void filterErased(int index);

    // Repopulates the word list table if the change affects one of the included filters.
    void filterChanged(int index);

    // Moves the condition at index in front of the new position to.
    void filterMoved(int index, int to);

    void showContextMenu(QMenu *menu, QAction *insertpos, Dictionary *dict, DictColumnTypes coltype, QString selstr, const std::vector<int> &windexes, const std::vector<ushort> &kindexes);

    // Starts a timer when editing the dictionary search so the entered search text is saved
    // in the drop down list after the timeout is over.
    void searchEdited();

    void settingsChanged();

    // When in dictionary search mode, updates the search results. Otherwise it's the model
    // owner's responsibility to react to dictionary resets.
    void dictionaryReset();
    void dictionaryRemoved(int index, int order, void *oldaddress);
    void dictionaryFlagChanged(int index, int order);
    void dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int index);

    void on_jpBeforeButton_clicked(bool checked);
    void on_jpAfterButton_clicked(bool checked);
    void on_enAfterButton_clicked(bool checked);
    void on_jpStrictButton_clicked(bool checked);
    void on_enStrictButton_clicked(bool checked);
    void on_examplesButton_clicked(bool checked);
    void on_inflButton_clicked(bool checked);
    void on_filterButton_clicked(bool checked);
private:
    // Creates the dictionary widget actions the widget is on. Only valid when the widget is
    // not placed on a zkanji form. For example when it's in its own dialog.
    void installCommands();
    // Adds a single command action to the widget.
    void addCommandShortcut(const QKeySequence &keyseq, int command);

    void updateSearchMode();

    // Updates the words displayed in the table. If the widget is hidden, only updates the
    // words if updateforced is true (set elsewhere, e.g. by calling forceUpdate()), otherwise
    // sets updatepending to true and returns.
    void updateWords();

    // Multi-line dictionary button was toggled. Updates the table.
    void updateMultiline();

    // Updates the model of the table, setting it to the new model, and also
    // restoring the selection model and its connections.
    void setTableModel(ZAbstractTableModel *newmodel);

    // When listing words of a group, the list is filtered by some search string.
    //bool groupFiltered();

    // Call when some setting or filter changes which can cause the dictionary browse model to
    // be repopulated. Deletes the browse model, and if the current list mode is browse, sets
    // model to hold every word in the dictionary. Must call updateWord() somewhere after this
    // function returns.
    //void resetBrowseModel();

    // Returns whether filtering conditions are set that influences the listing of words in
    // any way. Including the check of the checked state of the filter button.
    bool conditionsEmpty() const;

    // Returns a list of word indexes in the current dictionary shown when in browsing mode.
    // When filtered is true the returned list only holds indexes to words that fit the filter
    // conditions. The conditions are not checked and shouldn't be empty.
    //std::vector<int> browseList(bool filtered) const;
    // Returns the index of the search string in the words table when in browsing mode. If an
    // exact match is not found, returns the index of the first word that would come after
    // the search string. Uses the current browse order.
    //int browseIndex(const QString &search) const;

    void storeLastSearch(ZComboBox *box);

    static QStringListModel jpsearches;
    static QStringListModel ensearches;

    // Returns a command category when the widget is visible, it's on a ZKanjiWidget, and that
    // widget it either active, or is the sole widget on the form in the given category. The
    // returned value depends on the current view mode of the ZKanjiWidget. A 0 return value
    // means the parent form's menu shouldn't be changed.
    CommandCategories activeCategory() const;

    Ui::DictionaryWidget *ui;

    Dictionary *dict;

    // The model currently set as the model in the words list, either directly, or indirectly
    // as a source model in the filtermodel. A model is never deleted by this reference.
    DictionaryItemModel *model;

    QSignalMapper *commandmap;

    // Provides items to be listed in the widget's words table when searching the dictionary.
    std::unique_ptr<DictionarySearchResultItemModel> searchmodel;

    // Provides items shown when browsing the dictionary.
    std::unique_ptr<DictionaryBrowseItemModel> browsemodel;

    // Used in place of model when listing a group and a search string is set for filtering.
    // A filtermodel is only created and set as the displayed model when filtering is needed.
    std::unique_ptr<DictionarySearchFilterProxyModel> filtermodel;

    // Whether the displayed word list needs updating. Calling updateWords() while the widget
    // is not visible will set this value to true, and only updates when the widget is shown.
    bool updatepending;
    // When set before calling updateWords(), the word listing is updated even if the widget
    // is hidden.
    bool updateforced;

    // Whether column order or sorting are saved when saving widget state in XML. Default is
    // true.
    bool savecolumndata;

    ListMode listmode;

    SearchMode mode;
    BrowseOrder browseorder;

    // List of active filters and whether to show words in examples or groups only.
    std::unique_ptr<WordFilterConditions> conditions;

    // Function used to determine sort order when sorting is requested.
    ProxySortFunction sortfunc;

    QBasicTimer historytimer;

    typedef QWidget base;
};

#endif // DICTIONARYWIDGET_H
