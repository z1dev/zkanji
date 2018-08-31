/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZKANJIGRIDVIEW_H
#define ZKANJIGRIDVIEW_H

#include <QAbstractScrollArea>
#include <QMenu>
#include <QBasicTimer>

#include <list>
#include <map>
#include <memory>

#include "smartvector.h"

class QMenu;
class QMimeData;
class QPixmap;
class KanjiGroup;
class KanjiGridModel;
class Dictionary;
class RangeSelection;
class ZStatusBar;
struct Interval;
struct Range;
enum class CommandCategories;

// Widget for displaying a list of kanji in a grid layout. This class does NOT use the
// view/model system of Qt, because there's no native Qt view that needs to display the same
// information. Instead it displays data provided by KanjiGridModel derived classes.
class ZKanjiGridView : public QAbstractScrollArea
{
    Q_OBJECT
signals:
    void currentChanged(int curr, int prev);
    void selectionChanged();
public:
    ZKanjiGridView(QWidget *parent = 0);
    ~ZKanjiGridView();

    void setModel(KanjiGridModel *newmodel);
    KanjiGridModel* model() const;

    void assignStatusBar(ZStatusBar *bar);
    ZStatusBar* statusBar() const;

    Dictionary* dictionary() const;
    void setDictionary(Dictionary *newdict);

    // See UICommands::Commands in globalui.h
    void executeCommand(int command);
    // Returns whether a given command is valid and active in the widget in its current state.
    void commandState(int command, bool &enabled, bool &checked, bool &visible) const;

    int cellSize();
    //void setCellSize(int newsize);

    //void setGroup(KanjiGroup *g);

    // Returns the number of selected cells.
    int selCount() const;
    // Returns whether the cell at index is selected or not.
    bool selected(int index) const;
    // Changes the selection state of a cell at the given index to sel.
    void setSelected(int index, bool sel);
    // Returns whether there are cells selected in the view and there are no gaps in the
    // selection.
    bool continuousSelection() const;

    // Returns the index of the cell at the passed coordinates.
    int cellAt(int x, int y) const;
    // Returns the index of the cell at the passed coordinates.
    int cellAt(const QPoint &p) const;
    // Returns the client rectangle of the cell at index.
    QRect cellRect(int index) const;
    // Returns the row at the passed y coordinate. This function can return a meaningful value
    // when cellAt returns -1.
    int cellRowAt(int y) const;

    // Sets selection to index, deselecting all other cells.
    void select(int index);
    // Toggles the selection of index. If the toggled cell was the only one selected, it won't
    // be deselected. Returns whether the cell is selected after the call.
    bool toggleSelect(int index);
    // Selects the cells between the selection pivot and endindex. If deselect is false, the
    // previous selection is kept, otherwise everything else becomes unselected.
    void multiSelect(int endindex, bool deselect);

    // Returns the index of the pos-th selected item. The order of the items matches the
    // displayed order.
    int selectedCell(int pos) const;

    // Returns the index list of selected cells.
    void selectedCells(std::vector<int> &result) const;
    // Returns a list of the indexes of the selected kanji in the global kanji list.
    void selKanji(std::vector<ushort> &result) const;

    // Returns the selected kanji in a single string.
    QString selString() const;

    // Requests the repainting of the cell at the given index.
    void updateCell(int index);
    // Requests repainting of the selected cells.
    void updateSelected();

    // In case the cell with the passed index is not visible, it is scrolled into view. If the
    // cell is fully outside the view and its distance to the view is at least 1 pixel, or
    // forcemid is set to true, it will be/ shown in the middle of the viewport. Otherwise it
    // is scrolled just to be inside the view on the edge.
    void scrollTo(int index, bool forcemid = false);
    // Scrolls the view to make the cell with the given index visible. If the cell was fully
    // outside the viewport, it will be scrolled until it's visible at the edge.
    void scrollIn(int index);

    int autoScrollMargin() const;
    void setAutoScrollMargin(int val);

    void startAutoScroll();
    void stopAutoScroll();
    void doAutoScroll();

    // Notifies the kanji group shown in the view to remove the kanji from the group which is
    // selected in the view. This is a helper function with the same result as removing the
    // items directly from the group itself.
    void removeSelected();
public slots:
    void settingsChanged();
    void dictionaryToBeRemoved(int oldix, int newix, Dictionary *d);

    virtual void reset();
    // Received when new items were added to the group displayed in the view.
    virtual void itemsInserted(const smartvector<Interval> &intervals);
    // Received when items were removed from the group displayed in the view.
    virtual void itemsRemoved(const smartvector<Range> &ranges);
    // Received when items were removed from the group displayed in the view.
    virtual void itemsMoved(const smartvector<Range> &ranges, int pos);
    // The order of items in the model are about to be changed.
    virtual void layoutToChange();
    // The order of items in the model were changed.
    virtual void layoutChanged();

    virtual void selectAll();
    virtual void clearSelection();

    virtual void modelDestroyed();
protected:
    // Cancels drag or mouse down. Called on focus out and returns whether a meaningful action
    // has been cancelled. What counts as meaningful can be decided in derived classes. The
    // default implementation returns true if a left button was held down.
    virtual bool cancelActions();

    // Makes sure the drag drop indicator is updated while draggong.
    virtual void scrollContentsBy(int dx, int dy) override;

    virtual bool viewportEvent(QEvent *e) override;

    virtual void timerEvent(QTimerEvent *e) override;

    virtual void paintEvent(QPaintEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;

    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void contextMenuEvent(QContextMenuEvent *e) override;

    virtual void focusInEvent(QFocusEvent *e) override;
    virtual void focusOutEvent(QFocusEvent *e) override;

    virtual void dragEnterEvent(QDragEnterEvent *e) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *e) override;
    virtual void dragMoveEvent(QDragMoveEvent *e) override;
    virtual void dropEvent(QDropEvent *e) override;
private:
    // Populates the status bar if it's empty and updates the values to reflect current
    // selection.
    void updateStatus();

    void statusDestroyed();

    // Computes new column and row counts depending on the passed viewport's width.
    void recompute(const QSize &size);
    // Computes scrollbar ranges depending on the passed viewport size and row count.
    void recomputeScrollbar(const QSize &size);

    // Generates data needed to start a drag and drop operation and returns it in a mime data
    // to transfer. Returns null when no data is present.
    QMimeData* dragMimeData();

    // Extracts the dragged kanji indexes from the passed mime data. Returns whether the data
    // contained kanji indexes.
    //bool dragIndexes(const QMimeData *data, std::vector<ushort> &kindexes);

    // Causes a repaint at the right position to show or hide the drag and drop indicator.
    void updateDragIndicator();

    // Returns true if the obj is another view displaying the same group of kanji as this one.
    bool hasSameGroup(QObject *obj);

    // Returns the cell index of a drag and drop operation by the passed point. The returned
    // index might be the cell under the mouse, or the next cell, if the position is near the
    // border to the right.
    int dragCell(QPoint pt) const;

    // Changes the current to index, updating both cells and emitting the currentChanged()
    // signal. Updates the kanji information window with the new current kanji.
    void setAsCurrent(int index);

    // Returns a command category when the widget is visible, it's on a ZKanjiWidget, and that
    // widget it either active, or is the sole widget on the form in the given category. The
    // returned value depends on the current view mode of the ZKanjiWidget. A 0 return value
    // means the parent form's menu shouldn't be changed.
    CommandCategories activeCategory() const;
    // Enables or disables the command actions in the form's main menu.
    void updateCommands() const;

    // Used when clicking and dragging inside the view.
    // None: not dragging.
    // CanDrag: the user clicked inside a bigger selection and might start dragging it. This
    //      state is not used if the user clicks inside a single selection or selects a new
    //      cell.
    // Dragging: Drag and drop over the grid is in progress.
    enum class State { None, CanDrag, Dragging };

    KanjiGridModel *itemmodel;
    // Whether the slots are connected to itemmodel signals.
    bool connected;

    Dictionary *dict;

    // Context menu shown for kanji.
    //QMenu *popup;

    ZStatusBar *status;

    State state;

    // First index marked for removal in beginItemsRemove() and beginItemsMove().
    //int removefirstpos;
    // Last index marked for removal in beginItemsRemove() and beginItemsMovwe().
    //int removelastpos;
    // Index marked as destination in beginItemsMove().
    //int movepos;

    // Width and height of a single grid square.
    int cellsize;

    // Number of pixels near edge of view where auto scroll can start during drag and drop.
    int autoscrollmargin;
    // Amount to scroll when auto scrolling. Gradually increased.
    int autoScrollCount;

    QBasicTimer autoScrollTimer;

    // Number of columns currently shown in the grid. This value
    // automatically changes as the user resizes the widget.
    int cols;
    // Number of rows currently shown in the grid. This value
    // automatically changes as the user resizes the widget.
    int rows;

    // The left mouse button has been pressed over the view and not released yet.
    bool mousedown;

    // The client coordinate of the mouse inside the view when starting to drag a kanji.
    QPoint mousedownpos;

    // Index of the current item. Acts as the item to start out from when navigating the grid.
    int current;

    // Pivot item when selecting a range of cells with the shit key pressed. The items between
    // selpivot and the clicked (new current) item will be selected.
    int selpivot;

    // Object holding the selected cell ranges in the grid.
    std::unique_ptr<RangeSelection> selection;

    int persistentid;

    // Cell index where the last kanji tooltip was shown.
    int kanjitipcell;
    // Kanji character in the global kanjis list which was shown in the last tooltip.
    int kanjitipkanji;

    // When holding shift and page-up/down etc, the index of the active
    // selection shouldn't change, but the selection should expand with
    // each keypress. In those cases lastkeyindex saves the value of the
    // last selected cell. Its value is -1 when the selection starts
    // from the active selection.
    //int lastkeyindex;

    // List of selected cells in the order they were added to the selection.
    //std::vector<int> sellist;
    // A map for fast determining whether a cell is selected.
    // [cell index, sellist indexes].
    //std::map<int, int> selmap;

    // Cell position of the visual indicator showing the target of a drag and drop operation.
    // Dropped kanji will be inserted in front of this cell.
    int dragind;

    // Saved sellist when dragging kanji over the view.
    //std::vector<int> dragsellist;
    // Saved selmap when dragging kanji over the view.
    //std::map<int, int> dragselmap;

    // Saved parameters while drag dropping, to be able to handle auto scrolling that updates
    // the drag and drop indicator position.

    Qt::DropActions savedDropActions;
    const QMimeData *savedMimeData;
    Qt::MouseButtons savedMouseButtons;
    Qt::KeyboardModifiers savedKeyboardModifiers;

    typedef QAbstractScrollArea base;
private slots:
    //  Adds the current selection to a kanji group picked by the user.
    void showKanjiInfo() const;
    void kanjiToGroup() const;
    void collectWords() const;
    void definitionEdit() const;
    void copyKanji() const;
    void appendKanji() const;
};


#endif // ZKANJIGRIDVIEW_H
