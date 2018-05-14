/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZDICTIONARYLISTVIEW_H
#define ZDICTIONARYLISTVIEW_H

#include <QMenu>
#include <memory>
#include "zlistview.h"
#include "zlistviewitemdelegate.h"
#include "smartvector.h"

class Dictionary;
class WordGroup;
class MultiLineDictionaryItemModel;
class RangeSelection;
struct WordEntry;
class DictionaryItemModel;
class Dictionary;
class DictionaryListDelegate;
enum class CommandCategories;
enum class DictColumnTypes;

// Table for displaying dictionary entries. Replaces the paint delegate with
// DictionaryListDelegate. Pass models derived from DictionaryItemModel to setModel(), or
// proxy models holding them.
// Allows showing words in multiple table lines by calling setMultiLine(true). The model is
// then replaced with a multi line model, which can be accessed by multiModel(). The original
// model is still available through model(). If no original model was set, the multiModel()
// will return nullptr as well. Setting a new model with setModel() while in multi line mode
// will keep the multi model, and set its source as the one passed to setModel().
// ExtendedSelection mode works differently in multi line mode:
// selecting a line selects all lines of the same word. Selection can be toggled by holding
// SHIFT or CTRL and clicking on items, or navigating while CTRL key is pressed and pressing
// SPACE on words to toggle.
// Every other selection modes allow selecting separate lines within a word.
// ContinuousSelection is not tested and (probably) never used.
class ZDictionaryListView : public ZListView
{
    Q_OBJECT
signals:
    // Sent when a row with a word is double clicked, unless a kanji was under the mouse
    // cursor. windex is set to the index of the word in its dictionary, dindex is the
    // definition index within the word, when available. When listing all definitions on the
    // same line, dindex is set to -1.
    void wordDoubleClicked(int windex, int dindex);

    // Sent when the current row, indicated by a focus rectangle changes to another row. This
    // signal is usually emited before the selection also changes.
    //void currentRowChanged(int current, int previous);

    // Called before the popup menu in the list view is to be shown, giving the opportunity to
    // change the context menu. Passed is the menu to be shown, the position where general
    // actions can be inserted, the type of column the user clicked on, selected string in the
    // kanji or kana field if present, list of word indexes that are selected, list of kanji
    // indexes either in the selected text or in each selected word. The word indexes are
    // valid to the dictionary displayed in the view.
    void contextMenuCreated(QMenu *menu, QAction *insertpos, Dictionary *dict, DictColumnTypes coltype, QString selstr, const std::vector<int> &windexes, const std::vector<ushort> &kindexes);
public:
    ZDictionaryListView(QWidget *parent = nullptr);
    ~ZDictionaryListView();

    // Returns the current item delegate, which is derived from DictionaryListDelegate.
    DictionaryListDelegate* itemDelegate();
    // Replaces the current item delegate with the passed object. The old delegate is deleted.
    void setItemDelegate(DictionaryListDelegate *newdel);

    WordGroup* wordGroup() const;
    Dictionary* dictionary() const;

    // Returns the word index for the word at row. When in multi line mode, the row specifies
    // the item position and not the row.
    int indexes(int row) const;

    // Returns the word indexes for the words at the passed rows in result. When in multi line
    // mode, the rows specify the item positions and not the rows.
    void indexes(const std::vector<int> &rows, std::vector<int> &result) const;

    // See UICommands::Commands in globalui.h
    void executeCommand(int command);
    // Returns whether a given command is valid and active in the widget in its current state.
    void commandState(int command, bool &enabled, bool &checked, bool &visible) const;

    // Set whether the the dictionary list view displays user defined study definitions
    // instead of the dictionary definition. If a user defined study definition is not found,
    // the original definition will be displayed but without word tags or meaning numbers.
    void setStudyDefinitionUsed(bool study);

    // Returns whether the the dictionary list view displays user defined study definitions
    // instead of the dictionary definition. If a user defined study definition is not found,
    // the original definition will be displayed but without word tags or meaning numbers.
    bool isStudyDefinitionUsed() const;

    // Returns whether the passed row is shown as selected. A row can be selected either
    // normally, or, when multi line mode is on, if a different row of the same word is
    // selected in ExtendedSelection mode.
    //bool rowSelected(int row) const;

    // Returns the number of selected rows. When multi line mode is on, rows of a single word
    // are not counted separately.
    //int selectionSize() const;

    // Returns whether the line at index should be painted as current, which is different from
    // selected when multi line mode is on.
    //bool isSelectedWord(const QModelIndex &index) const;

    // Returns whether the row at index is the current row in the view.
    //bool isCurrentRow(int row) const;

    // Returns whether the data is represented by a MultiLineDictionaryItemModel. If true,
    // changing the model with setModel() will create the multi line model instead, and the
    // calls to model() will return that.
    bool isMultiLine() const;
    // Changes the display of the words data to a representation where each word definition
    // gets its own line. If there's a model already set for the view, it's used as the
    // source model for the multi line proxy model.
    // If extended selection is used in the view, when set to multi-line, the selection mode
    // is changed. The selection will then work as a multi select view that can only toggle
    // lines, with the difference that toggle needs ctrl-click and every line of a multi line
    // word is selected or deselected visually.
    void setMultiLine(bool ismulti);

    // Whether the view displays when a word is found in a group. Even when this returns true,
    // the main settings can override it to be hidden.
    bool groupDisplay();
    // Set whether the view should display when a word is found in a group. Even when this
    // returns true, the main settings can override it to be hidden.
    void setGroupDisplay(bool val);

    //void setSelectionMode(QAbstractItemView::SelectionMode mode);
    //QAbstractItemView::SelectionMode selectionMode() const;

    // Returns the indexes of the selected rows in the passed vector. In multi line mode, the
    // rows refer to the rows of the original model.
    //void selectedRows(std::vector<int> &result) const;

    // Override that makes sure the list has a multi line model if isMultiLine() is set to
    // true with the passed new model as its source. Passing null will delete a multi line
    // model, and clear the real model of the view.
    void setModel(ZAbstractTableModel *newmodel);
    // Returns the model last passed to setModel(). The result is not a multi line model even
    // if isMultiLine() returns true, if not a multi line model was passed to setModel().
    ZAbstractTableModel* model() const;
    // Returns the model if it's a DictionaryItemModel, or the first source model of proxy
    // models displayed which is a DictionaryItemModel.
    DictionaryItemModel* baseModel() const;

    // Returns the multi line model if isMultiLine() is set to true and a valid model is used.
    // Otherwise returns null.
    MultiLineDictionaryItemModel* multiModel() const;

    // Returns a model index to the word's item under p point. This function always returns an
    // index valid for model(). If multi line mode is set, use the original indexAt().
    //QModelIndex baseIndexAt(const QPoint &p) const;

    bool isTextSelecting() const;
    QModelIndex textSelectionIndex() const;
    int textSelectionPosition() const;
    int textSelectionLength() const;

    // Stops selecting in kanji or kana text and returns if it has been cancelled.
    virtual bool cancelActions() override;

    virtual void signalSelectionChanged() override;
public slots:
    virtual void settingsChanged() override;

    void multiSelRemoved(const smartvector<Range> &ranges);
    void multiSelInserted(const smartvector<Interval> &intervals);
    void multiSelMoved(const smartvector<Range> &ranges, int pos);
protected:
    virtual void selRemoved(const smartvector<Range> &ranges) override;
    virtual void selInserted(const smartvector<Interval> &intervals) override;
    virtual void selMoved(const smartvector<Range> &ranges, int pos) override;

    virtual int mapToSelection(int row) const override;
    virtual int mapFromSelection(int index, bool top) const override;

    // Returns an index list holding indexes corresponding to the base model. The base model
    // is the model which is not a proxy model, which can be the model() itself, or the model
    // after a chain of sourceModel() calls.
    // Don't call in multi line mode.
    //QModelIndexList baseIndexes(const QModelIndexList &indexes) const;

    // Converts model row indexes to base model indexes. src must hold indexes to rows in
    // model (and not multi model). The resulting list will hold indexes in the base model,
    // which might be the model itself, or if the model is a proxy model, to the source model
    // of model, until the DictionaryItemModel is found.
    void baseRows(const std::vector<int> &src, std::vector<int> &result) const;

    // Calls the remove method of the wordGroup represented by the currently set model for the
    // entries corresponding to rows in indexes. The indexes list must be ordered by row.
    virtual void removeDragRows() override;

    //// Creates a DictionaryListDelegate for the item delegate used in the view.
    //virtual ZListViewItemDelegate* createItemDelegate() override;

    //virtual bool isRowDragSelected(int row) const override;
    virtual QMimeData* dragMimeData() const override;

    virtual int dropRow(int ypos) const override;

    // Inherited Qt functions:

    //virtual QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex &index, const QEvent *e = 0) const override;
    //virtual void toggleRowSelect(int row, Qt::MouseButton button, Qt::KeyboardModifiers modifiers) override;

    //virtual void keyPressEvent(QKeyEvent *event) override;

    virtual bool viewportEvent(QEvent *e) override;

    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;

    virtual bool requestingContextMenu(const QPoint &pos, const QPoint &globalpos, int selindex) override;

    //virtual void contextMenuEvent(QContextMenuEvent *e) override;

    //virtual void dragEnterEvent(QDragEnterEvent *e) override;
//private slots:
    //virtual void selectionChanged(const QItemSelection &index, const QItemSelection &old) override;

    // Slots set for the model() in multi line mode to update the selection.

    //void baseRowsInserted(const QModelIndex &parent, int first, int last);
    //void baseRowsRemoved(const QModelIndex &parent, int first, int last);
    //void baseRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    //void baseModelReset();
    //void baseLayoutWillChange(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    //void baseLayoutChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
private:
    void selectionToGroup() const;
    void wordsToDeck() const;
    void wordsToDeck(const std::vector<int> &windexes) const;
    void wordToDictionary() const;
    void editWord() const;
    void deleteWord() const;
    void deleteWord(int windex) const;
    void revertWord() const;
    void createNewWord() const;

    void copyWordDef() const;
    void copyWordKanji() const;
    void copyWordKana() const;
    void copyWord() const;
    void appendWordKanji() const;
    void appendWordKana() const;
    void copyWordDef(const std::vector<int> &windexes) const;
    void copyWordKanji(const std::vector<int> &windexes) const;
    void copyWordKana(const std::vector<int> &windexes) const;
    void copyWord(const std::vector<int> &windexes) const;
    void appendWordKanji(const std::vector<int> &windexes) const;
    void appendWordKana(const std::vector<int> &windexes) const;

    // Returns a command category when the widget is visible, it's on a ZKanjiWidget, and that
    // widget it either active, or is the sole widget on the form in the given category. The
    // returned value depends on the current view mode of the ZKanjiWidget. A 0 return value
    // means the parent form's menu shouldn't be changed.
    CommandCategories activeCategory() const;

    // Displays a popup menu belonging to the listview at pos global position. The items in
    // the menu depend on the passed parameters.
    void showPopup(const QPoint &pos, QModelIndex index, QString selstr, int kindex);

    // Marks the window area that displays rows for a given word at the position in the base
    // model for draw update. Only works in multi line mode.
    //void updateWordRows(int wpos);
    // Marks the window area that displays selected words of the base model for draw update.
    // Only works in multi line mode.
    //void updateSelected();

    QMenu *popup;

    // Showing multiple lines for each word that have more than one definition.
    bool multi;

    // The selection mode used when multi line is set to false. Turning on multi line changes
    // the selection mode to NoSelection, and the old value is saved here. It's restored if
    // multi line is set to false again.
    //QAbstractItemView::SelectionMode savedselmode;

    //std::unique_ptr<RangeSelection> selection;

    // View should display user defined study definition for words where it's set.
    bool studydefs;

    // Set when manually updating the current row, to ignore row change signals.
    bool ignorechange;

    // Cell index where the last kanji tooltip was shown.
    QModelIndex kanjitipcell;
    // Index of kanji character in its string where the last kanji tooltip was shown.
    int kanjitippos;

    // Whether kanji/kana text selection is enabled.
    bool textselectable;
    // Mouse selection is currently under way.
    bool selecting;
    // Cell where the current selection takes place if selecting is true.
    QModelIndex selcell;
    // Starting character position of the current selection in selcell.
    int selpos;
    // Length of the current selection. When this is 0, nothing is selected. The value is
    // relative to selpos and can be negative.
    int sellen;

    typedef ZListView   base;
};

enum class InfTypes;
class DictionaryListDelegate : public ZListViewItemDelegate
{
public:
    DictionaryListDelegate(ZDictionaryListView *parent = nullptr);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    // Whether the delegate displays when a word is found in a group. Even when this returns
    // true, the main settings can override it to be hidden.
    bool groupDisplay();
    // Set whether the delegate should display when a word is found in a group. Even when this
    // returns true, the main settings can override it to be hidden.
    void setGroupDisplay(bool val);

    // Paints the definition text of an entry. If selected is true, the text is painted with
    // textcolor, otherwise only the main definition is using it.
    virtual void paintDefinition(QPainter *painter, QColor textcolor, QRect r, int y, WordEntry *e, std::vector<InfTypes> *inf, int defix, bool selected) const;

    // Paints the kanji string passed in str with painter at left and baseline y.
    virtual void paintKanji(QPainter *painter, const QModelIndex &index, int left, int top, int basey, QRect r) const;

    // Returns the index of a kanji in the global kanji list at the given point position. The
    // rectangle and index of the item under pos must be provided. Returns -1 if no kanji
    // would be drawn at the passed position, or the column of the index doesn't have the
    // ColumnType::Kanji or ColumnType::Kana type. If a rectangle is passed in kanjirect and
    // the return value is not -1, it'll be set to the coordinates of the kanji under pos.
    int kanjiAt(QPoint pos, QRect itemrect, const QModelIndex &index, QRect *kanjirect = nullptr, int *charpos = nullptr);
    // Returns an index in the kanji or kana text if the pos is in such a column and over
    // the text. The index returned is where a mouse selection should start, up to the size of
    // the string. If there is no text under the position in itemrect, -1 is returned.
    // Set forced to return a valid value instead of -1 in this case. When the mouse cursor is
    // outside the text sideways, it returns 0 or the length of string, depending on the side.
    int characterIndexAt(QPoint pos, QRect itemrect, const QModelIndex &index, bool forced = false);
protected:
    const ZDictionaryListView* owner() const;
private:
    ZDictionaryListView* owner();
    // Draws a text with the current selection from the owner. It is assumed that the text
    // being drawn is the right one. Uses the inverse of the current pen color and background
    // for drawing the selected part. Font and other attributes should already be set.
    void drawSelectionText(QPainter *p, int x, int basey, const QRect &clip, QString str) const;

    bool showgroup;

    typedef ZListViewItemDelegate base;
};

extern const double kanjiRowSize;
extern const double defRowSize;
extern const double notesRowSize;

#endif // ZDICTIONARYLISTVIEW_H
