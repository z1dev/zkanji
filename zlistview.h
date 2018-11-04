/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZLISTVIEW_H
#define ZLISTVIEW_H

#include <QTableView>
#include <QAbstractItemModel>
#include <QBasicTimer>
#include <memory>
#include "smartvector.h"

class ZListViewItemDelegate;
class QMimeData;
class ZAbstractTableModel;
class ListViewHeader;
class QXmlStreamWriter;
class QXmlStreamReader;
enum class CheckBoxMouseState;
class RangeSelection;
struct Range;
struct Interval;
struct ListStateData;
class ZStatusBar;

// None: Nothing can ge selected. There's only the current row indicator.
// Single: Only one row can be selected at a time. (default)
// Toggle: Multiselection. Clicking on a row or pressing space toggles a row's selection
//      status, leaving the rest unchanged.
// Extended: Multiselection mode where clicking or moving changes the selection. Holding
//      control while clicking allows toggling rows, holding shift selects the rows between
//      the selection pivot and clicked row. The selection is cleared if the control key is
//      not pressed.
enum class ListSelectionType { None, Single, Toggle, Extended };

// The ZListView can automatically update its row sizes when the global settings changes. The
// values in the enum specify which setting to take into account when updating the row size.
// Main: use the mainsize font setting as a base.
// Popup: use the popsize font setting as a base, mainly used in the popup dictionary.
// Custom: don't change the row size automatically. This should be handled elsewhere.
enum class ListSizeBase { Main, Popup, Custom };

enum class ColumnAutoSize;
// Sizing data of column, used to temporarily save columns to check in resetColumnData().
struct ColumnSizingData
{
    // The stretch mode of the column.
    ColumnAutoSize autosize;
    // The column can be resized or not.
    bool freesized;
    // Default column width.
    int defwidth;
    // Current column width.
    int width;
};


// Implementing drag and drop in models used in ZListView.
//
// Drop:
// Call: setAcceptDrops(true) 
// Implement in models: supportedDropActions(), dropMimeData(), canDropMimeData()
// canDropMimeData() will be called more times depending on its return value:
// - when near the middle of a row, with row and column set to -1 and parent set to a valid
// row to test whether dropping onto that parent is possible.
// - with row set to a value of 0 or above, and parent set to invalid index, it tests whether
// dropping in front of another row or at the end of table is possible.
// - row and column set to -1 and parent set to an invalid index tests whether dropping on the
// table, but not above or into another row should work.
//
// dropMimeData() should either call canDropMimeData() before trying to drop, or check for the
// row and column and parent again.
// When the result for supportedDropActions() is MoveAction and dropping to the source of the
// drag, the model must handle the move. No remove will be called.
//
// Drag:
// Call: setDragEnabled(true)
// Implement in models: supportedDragActions(), mimeData()
//

// List-view type table. The whole row is selected and there is no separate cursor displayed
// for cells. Navigation is only vertically automatic when clicking inside cells. Uses the
// ZListViewItemDelegate as the default delegate for drawing items. Models used in the table
// should either be proxy models or derived from ZAbstractTableModel.
class ZListView : public QTableView
{
    Q_OBJECT
signals:
    // Sent when the current row, indicated by the focus rectangle changes from one row to a
    // different row.
    void currentRowChanged(int curr, int prev);

    // Sent after the row selection changed in the view.
    void rowSelectionChanged();

    void rowDoubleClicked(int row);
public:
    ZListView(QWidget *parent = nullptr);
    ~ZListView();

    // Saves table column state and sizes to the passed xml stream.
    void saveXMLSettings(QXmlStreamWriter &writer) const;
    // Loads table column state and sizes from the passed xml stream.
    void loadXMLSettings(QXmlStreamReader &reader);

    // Saves table column state and sizes in data.
    void saveColumnState(ListStateData &data) const;
    // Restores table column state and sizes in data. Pass a columnmodel to restore the state
    // even when the table has no model set. If there's already a model in the table, the
    // passed columnmodel is not used. The columnmodel must have the exact column count and
    // column types as the model to be set later, or the column state will be lost.
    void restoreColumnState(const ListStateData &data, ZAbstractTableModel *columnmodel = nullptr);

    void setModel(ZAbstractTableModel *newmodel);
    ZAbstractTableModel* model() const;

    void assignStatusBar(ZStatusBar *bar);
    ZStatusBar* statusBar() const;

    virtual QSize sizeHint() const override;
    void setSizeHint(QSize newsizehint);
    // Sets the size hint to fit charwidth * charheight medium sized characters. The size hint
    // is not updated with font changes.
    void setFontSizeHint(int charwidth, int charheight);

    // Which setting determines the row size.
    ListSizeBase sizeBase() const;
    // Set which setting determines the row size.
    virtual void setSizeBase(ListSizeBase newbase);

    // The column where a checkbox might be present. If there is one, pressing space will
    // check or uncheck this checkbox, instead of selecting the current row. By default there
    // is no column set and this value is -1.
    int checkBoxColumn() const;
    // Set the column where a checkbox might be present. If there is one, pressing space will
    // check or uncheck this checkbox, instead of selecting the current row. By default there
    // is no column set and this value is -1.
    void setCheckBoxColumn(int newcol);

    // Emulates a click on the checkbox at index. If it is found within a selection range,
    // every cell in the selection in the same column as index will get the same unchecked/
    // checked state as that checkbox.
    void clickCheckBox(const QModelIndex &index);

    // Returns the column that will be edited if editing is triggered. If the editing is
    // triggered by mouse clicks, the clicked column must match the editColumn().
    int editColumn() const;
    // Set which column can be edited by edit triggers.
    void setEditColumn(int col);

    // Returns true if an editor is open in the view.
    bool editing() const;

    // Returns the current item delegate, which is derived from ZListViewItemDelegate.
    ZListViewItemDelegate* itemDelegate();
    // Replaces the current item delegate with the passed object. The old delegate is deleted.
    void setItemDelegate(ZListViewItemDelegate *newdel);

    // Forces horizontal scroll bars not to change while scrolling by index.
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;

    bool autoSizeColumns();
    void setAutoSizeColumns(bool value);

    // Returned value when calling ZListView::checkBoxMouseState().
    // None: the mouse is not over the checkbox in a cell and the left mouse button was not
    //      pressed on it either.
    // Hover: the mouse is over the checkbox in a cell but the left mouse button is not
    //      pressed. If the mouse button is pressed over elsewhere, this value won't be set
    //      even if the mouse is moved over the checkbox.
    // DownHover: the mouse is over the checkbox in a cell and the left mouse button was
    //      pressed down on it.
    // Down: the mouse is not over the checkbox in a cell but the left mouse button was
    //      pressed down on it, then the mouse moved outside. 
    CheckBoxMouseState checkBoxMouseState(const QModelIndex &index) const;

    // Returns the index of the current row in the list view. The current view acts as
    // starting row for keyboard navigation, and not necesserily selected.
    int currentRow() const;
    // Changes the row displaying a focus rectangle to rowindex. If selection is enabled,
    // changes the selection to rowindex.
    void setCurrentRow(int rowindex);
    // Changes the row displaying a focus rectangle to rowindex without changing the
    // selection.
    void changeCurrentRow(int rowindex);
    // Fills result with row indexes of the current selection. When using the derived
    // ZDictionaryListView this bypasses the multi line model and returns indexes to its
    // source model.
    void selectedRows(std::vector<int> &result) const;

    // Returns a list of model indexes for the first column of the selected rows. Used in
    // functions that require model indexes instead of rows.
    QModelIndexList selectedIndexes() const;

    // Removes the selection from rows, also clearing the temporary range selection. Calling
    // this function will remove any selection, even from the current row.
    void clearSelection();

    // Returns whether there are any rows displayed as selected in the view.
    bool hasSelection() const;

    // Returns whether the selection contains the row. The temporary range selection is also
    // considered, not just the saved selections.
    bool rowSelected(int rowindex) const;

    // Returns the number of rows that are displayed selected. In derived classes where an
    // item is represented by multiple rows, this returns the selected item count.
    int selectionSize() const;
    // Returns the row index of the index-th selected row. The value of index must be between
    // 0 and selectionSize() - 1. The result is undefined if used in a derived class that
    // displays a single item by multiple rows.
    int selectedRow(int index) const;

    ListSelectionType selectionType() const;
    void setSelectionType(ListSelectionType newtype);

    // Returns true if the list view contains selected rows, and there are no gaps in the
    // selection.
    bool continuousSelection() const;

    // Changes the position of the vertical scroll bar to make the passed row visible in the
    // viewport. If the row is already visible, nothing is changed.
    void scrollToRow(int row, ScrollHint hint = QAbstractItemView::EnsureVisible);

    // Returns the visual rectangle in local coordinates of the row with the passed index.
    QRect visualRowRect(int row) const;
    // Returns the visual rectangle in local coordinates of the row with the passed index.
    // Unlike visualRowRect(), does return the full rectangle containing parts not in the
    // viewport.
    QRect rowRect(int row) const;
    // Repaints the passed row if it's visible in the viewport.
    void updateRow(int row);
    // Repaints the visible part of the passed row range in the viewport. The range must be
    // valid. [first, last] is inclusive on both ends.
    void updateRows(int first, int last);
    // Updates the selected rows in the visible region of the list view. Set range to false
    // to ignore the temporary range selection.
    void updateSelection(bool range = true);
    // Commits the temporary range selection between selpivot and currentrow into selection,
    // making it permanent. Resets selpivot so the next range selection will be a new one.
    void commitPivotSelection();

    // Cancels drag and drop before it happens. Only role of this function is to be called
    // from the dictionary widget's event filter, to stop selection in a dictionary list view,
    // but it also must cancel drag and drops that might originate from this list view.
    virtual bool cancelActions();

    // Emits the selection changed signal.
    virtual void signalSelectionChanged();
public slots:
    virtual void settingsChanged();

    //void sectionResized(int ix, int oldsiz, int newsiz);

    virtual void aboutToBeReset(); /* Not declared in base class, no override */
    virtual void reset() override;
    virtual void selectAll() override;

    //void rowsInserted(const QModelIndex &parent, int first, int last);
    //void rowsRemoved(const QModelIndex &parent, int first, int last);
    //void rowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void rowsRemoved(const smartvector<Range> &ranges);
    void rowsInserted(const smartvector<Interval> &intervals);
    void rowsMoved(const smartvector<Range> &ranges, int pos);

    void layoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
    void layoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint);
protected:

    // Own virtual functions.

    // Updates the selection by removing ranges from it and signals the change.
    virtual void selRemoved(const smartvector<Range> &ranges);
    // Updates the selection by inserting empty intervals and signals the change.
    virtual void selInserted(const smartvector<Interval> &intervals);
    // Updates the selection by moving ranges and signals the change.
    virtual void selMoved(const smartvector<Range> &ranges, int pos);

    // In the dictionary list view subclass, multiple rows can represent a single item. These
    // must behave like a single item when selecting also.
    // Returns the selection item index from the row index. The base implementation returns
    // row unchanged.
    virtual int mapToSelection(int row) const;

    // In the dictionary list view subclass, multiple rows can represent a single item. These
    // must behave like a single item when selecting also.
    // Returns a row index from a selection index. Set top to true to return the first row,
    // and to false to return its last row.
    // The base implementation returns index unchanged.
    virtual int mapFromSelection(int index, bool top) const;

    // Called after a drag and drop resulted in move, to allow the derived class to remove any
    // moved item from its model.
    virtual void removeDragRows() { ; }

    // Returns the row where a drag and drop operation would end if the mouse is at the passed
    // y coordinate. Returning -1 can both mean dragging at the position is invalid, or that
    // the destination will be at the end of the items or at an unspecified location.
    virtual int dropRow(int ypos) const;

    //// Creates the item delegate used in the view. The default implementation returns a
    //// ZListViewItemDelegate object. The parent of the created delegate must be the view, and
    //// it shouldn't be shared between multiple views.
    //virtual ZListViewItemDelegate* createItemDelegate();

    // Hack. Returns true if the view displays a given row at row index as selected. This has
    // nothing to do with the real selection in the view. Only used to trick the view into a
    // drag and drop operation if a derived class wants to start drag for non-selected rows.
    // By default returns the real selection state of the row.
    //virtual bool isRowDragSelected(int row) const;

    // Returns the mime data to be dragged when starting drag and drop from this view. The
    // default implementation returns the mime data corresponding to the selected items
    // returned by the model.
    virtual QMimeData* dragMimeData() const;

    // Sets selrow to row and calls QTableView::selectRow() which selects/deselects the
    // clicked row.
    //virtual void toggleRowSelect(int row, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);

    // Changes the current row to row without changing the selection, and emits the signal if
    // necessary.
    void changeCurrent(int row);

    // Called when the user right-clicked inside the list view at pos. The value of selindex
    // is the selection row the user clicked. This can differ from the actual row if the model
    // displays items spread to multiple rows. The item's index is passed in selindex. It can
    // be -1, when pos doesn't point to a row. globalpos is the cursor's position on the
    // screen.
    // The function returns whether the event for the context menu should be accepted. This is
    // false by default.
    virtual bool requestingContextMenu(const QPoint &pos, const QPoint &globalpos, int selindex);

    // Inherited from Qt.

    // Makes sure the drag drop indicator is updated while draggong.
    virtual void scrollContentsBy(int dx, int dy) override;

    virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;

    virtual bool event(QEvent *e) override;

    //virtual QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex &index, const QEvent *e = 0) const override;
    //virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;

    virtual void timerEvent(QTimerEvent *e) override;

    virtual void focusOutEvent(QFocusEvent *e) override;

    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void verticalScrollbarValueChanged(int value) override;
    virtual void horizontalScrollbarValueChanged(int value) override;
    virtual void showEvent(QShowEvent *e) override;

    virtual void keyPressEvent(QKeyEvent *event) override;

    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;

    virtual void dragEnterEvent(QDragEnterEvent *e) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *e) override;
    virtual void dragMoveEvent(QDragMoveEvent *e) override;
    virtual void dropEvent(QDropEvent *e) override;

    virtual void paintEvent(QPaintEvent *e) override;

    // Skips some handling in the base class that would create persistent indexes, and handles
    // leave events.
    virtual bool viewportEvent(QEvent *e) override;
private:
    // Moved here from protected to hide it from derived classes, that should handle the
    // requestingContextMenu signal.
    virtual void contextMenuEvent(QContextMenuEvent *e) override;

    void statusDestroyed();

    //virtual void setSelectionModel(QItemSelectionModel *newselmodel) override final;

    // Resizes the columns automatically depending on the contents shown in the table. If the
    // first and last visible row indexes are unchanged the columns won't be resized, unless
    // forced is set to true. The columns are either resized for auto sized columns when the
    // view is also auto sized, or for stretched columns.
    void autoResizeColumns(bool forced = false);

    // Auto resizes columns depending on their stretch and auto-size values. Either override
    // this function and set the column widths of each column, or create a new item delegate
    // derived from ZListViewItemDelegate and override its sizeHint function to return a
    // meaningful width for auto sized columns.
    virtual void doResizeColumns();

    // Saves a temporary list of column size rules to match in resetColumnData(), to avoid
    // resetting the column sizes when the column count and column rules haven't changed.
    // Must be followed by a call to resetColumnData() to free the temporary list. Has no
    // effect on auto sized columns.
    // Pass a columnmodel with columns to be saved which can be used when there is no model
    // set in the table. It has no effect if a model exists.
    void saveColumnData(ZAbstractTableModel *columnmodel = nullptr);

    // Updates the sizability and width of every column in the table by the model's provided
    // header data. Calls autoResizeColumns() if autosize is set.
    void resetColumnData();

    // Repaints the part of the list where the drag drop indicator should be painted with the
    // current value of dragpos.
    void updateDragIndicator();

    // Changes the active window color scheme to an inactive one to show the drag indicator.
    // The drag indicator uses the highlight color, which must be changed to the disabled
    // color while dragging is in progress.
    void setDragColorScheme(bool turnon);

    void _invalidate();

    // Adds and updates the status bar labels when the current selection changes.
    void updateStatus();

    // Used when clicking and dragging inside the view.
    // None: not dragging.
    // CanDrag: the user clicked inside a selection and might start dragging it.
    // CanDragOrSelect: the user clicked inside a bigger selection and might start dragging
    //      it, but if the dragging doesn't start, select a single item on release.
    // Dragging: Drag and drop over the grid is in progress.
    enum class State { None, CanDrag, CanDragOrSelect, Dragging };

    std::unique_ptr<RangeSelection> selection;
    int currentrow;
    ListSelectionType seltype;
    // Starting row of multiselection. When selecting items with shift pressed, items between
    // the selpivot and the current row will be selected, beside the normal selection.
    int selpivot;

    ZStatusBar *status;

    // Whether auto sizing of columns is enabled or not.
    bool autosize;

    // What setting influences the row size in the view.
    ListSizeBase sizebase;

    // Cached first visible row in the table.
    int firstrow;
    // Cached last visible row in the table.
    int lastrow;

    // The checkbox column. If a checkbox is present in this column, pressing space checks or
    // unchecks it. Otherwise the default selection behavior is used. By default this is set
    // to -1.
    int checkcol;

    State state;

    // When set, the normal mouse click event is ignored.
    bool doubleclick;
    // The cell of the checkbox the mouse is currently over. It's stored when the mouse button
    // is pressed until it's released.
    QModelIndex mousecell;
    // The cell that will be edited when the editing is delayed, waiting for the double click.
    QModelIndex editcell;
    // The point where the left mouse button was clicked over the list view.
    QPoint mousedownpos;
    // The left mouse button is being pressed.
    bool mousedown;

    // The mouse button is currently pressed over the checkbox in mousecell.
    bool pressed;
    // The mouse cursor is currently hovering over the checkbox in mousecell.
    bool hover;

    // Set to true when clicking on a single selected row while no modifier was pressed, to
    // start editing on mouse release.
    bool canclickedit;
    // The column where the editor will be shown in case editing is enabled.
    int editcolumn;

    bool ignorechange;

    // Row position where the mouse is dragging something, and where data will be inserted on
    // drop.
    int dragpos;
    // Whether dragpos refers to the position in front of an item for insertion, or on the
    // item itself.
    bool dragon;
    // Saved background color of highlighted rows. The original is changed while drag dropping
    // and hovering over the list.
    QColor dragbgcol;
    // Saved text color of highlighted rows. The original is changed while drag dropping
    // and hovering over the list.
    QColor dragtextcol;

    // Saved parameters while drag dropping, to be able to handle auto scrolling that updates
    // the drag and drop indicator position.

    Qt::DropActions savedDropActions;
    const QMimeData *savedMimeData;
    Qt::MouseButtons savedMouseButtons;
    Qt::KeyboardModifiers savedKeyboardModifiers;

    // Persistent model indexes used when the model's layout is changing to update the
    // selection. The last added index is always for the current row.
    QList<QPersistentModelIndex> perix;

    // Sizehint replacement
    QSize sizhint;

    std::vector<ColumnSizingData> tmpcolsize;

    QBasicTimer doubleclicktimer;

    using QTableView::selectionModel;
    using QTableView::setSelectionModel;
    using QTableView::currentIndex;
    using QTableView::setSelectionMode;
    using QTableView::selectionMode;

    typedef QTableView  base;

protected slots:
    void currentChanged(const QModelIndex &/*current*/, const QModelIndex &/*previous*/) {}
    //    void curSelRowChanged(const QModelIndex &current, const QModelIndex &previous);
};


#endif
