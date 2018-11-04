/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZRADICALGRID_H
#define ZRADICALGRID_H

#include <QAbstractScrollArea>
#include <QSet>
#include <set>
#include <unordered_set>
#include "qcharstring.h"
#include "smartvector.h"

class QStylePainter;
class ZRect;

enum class RadicalFilterModes { Parts, Radicals, NamedRadicals };
struct RadicalFilter
{
    RadicalFilterModes mode = RadicalFilterModes::Parts;
    // Can be true in NamedRadicals mode when the filter was selected in grouped display mode.
    bool grouped = false;
    // The values in groups depend on the mode. If mode is Parts or Radicals, the lists
    // contain radical numbers. If mode is NamedRadicals, the lists contain indexes to items
    // in ZKanji::radlist
    std::vector<std::vector<ushort>> groups;
};

bool operator!=(const RadicalFilter &a, const RadicalFilter &b);
bool operator==(const RadicalFilter &a, const RadicalFilter &b);

class KanjiGridModel;
class ZRadicalGrid : public QAbstractScrollArea
{
    Q_OBJECT
public:

    static QChar radsymbols[231];
    static ushort radindexes[231];
    //static QChar partsymbols[267];
    //static ushort partindexes[267];

    ZRadicalGrid(QWidget *parent = nullptr);
    virtual ~ZRadicalGrid();

    void setActiveFilter(const RadicalFilter &rad);

    // Which radicals or parts are displayed.
    RadicalFilterModes displayMode() const;
    // Changes which radicals or parts are displayed.
    void setDisplayMode(RadicalFilterModes newmode);

    // The minimum number of strokes the displayed part or radical should have.
    int strokeLimitMin() const;
    // The maximum number of strokes the displayed part or radical should have. Always equal
    // or greater than strokeLimitMin().
    int strokeLimitMax() const;
    // Sets both the max and min limit for the displayed radicals stroke count to the same
    // value. Set to 0 to erase the limit.
    void setStrokeLimit(int limit);
    // Sets a minimum and maximum limit to the displayed radicals' stroke count. If maxlimit
    // is less than minlimit, it's corrected.
    void setStrokeLimits(int minlimit, int maxlimit);

    // Only used in NamedRadicals mode. Whether the view displays different versions of the
    // same radical as a single item.
    bool groupDisplay() const;
    // Only used in NamedRadicals mode. Set whether the view should display different versions
    // of the same radical as a single item.
    void setGroupDisplay(bool newgroup);

    // Only used in NamedRadicals mode. Whether the view lists the names of the items.
    bool namesDisplay() const;
    // Only used in NamedRadicals mode. Set whether the view should list the names of the
    // given item.
    void setNamesDisplay(bool newnames);

    // Only used in NamedRadicals mode. Only shows radicals that consist or start with the
    // returned string, depending on the value of exactMatch().
    QString searchString() const;
    // Only used in NamedRadicals mode. Set the prefix string of radicals to show. Depending
    // on the value of exactMatch(), the string might have to fully match. The string should
    // be in romaji. Set an empty string to erase the filter.
    void setSearchString(const QString &val);

    // Only used in NamedRadicals mode. Only show radicals that have a name exactly matching
    // the value of searchName().
    bool exactMatch() const;
    // Only used in NamedRadicals mode. Set whether only show radicals that have a name
    // exactly matching the value of searchName().
    void setExactMatch(bool newexact);

    // Adds the current selection to the list of finalized selections. Any of/ the radicals in
    // a single selection can be in a kanji to be a valid result. Each separate selection must
    // match a kanji to be a valid result.
    void finalizeSelection();
    // Removes every finalized selection and resets the current selections as well.
    void clearSelection();

    // Sets the list of kanji that are selected in a parent kanji search window. The list of
    // kanji restricts the shown radicals to the valid ones. The list is taken from the model
    // that represents kanji in a grid view.
    void setFromModel(KanjiGridModel *model);
    // Sets the list of kanji that are selected in a parent kanji search window. The list of
    // kanji restricts the shown radicals to the valid ones.
    void setFromList(const std::vector<ushort> &list);

    //// Clears the previously obtained list via setKanjiList. The list of kanji
    //// restricts the shown radicals to the valid ones.
    //void clearKanjiList();

    // Returns a radicals filter that can be used to filter kanji.
    void activeFilter(RadicalFilter &f);
    // Creates a list of radical indexes from the current selection that can be added to the
    // finalized filters.
    std::vector<ushort> selectionToFilter();
signals:
    // The selection has changed, resulting in new kanji to be displayed.
    void selectionChanged();
    // The selection label showing the current selections should be changed to str. This
    // doesn't mean new kanji must be shown, as it is used when we finalize a selection.
    void selectionTextChanged(const QString &str);
    // Signaled when the meaning of selecting a radical changes. When grouping is true, one
    // new selection represents every form of the radical. Otherwise each radical form can be
    // included in a selection separately.
    void groupingChanged(bool grouping);
protected:
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
private:
    // Computes the correct font sizes used for displaying the radicals, and the radical
    // indexes.
    void init();

    // Collects the radicals that can be shown depending on the current kanji list of the
    // parent kanji form. Any settings like stroke count or radical name set in the radical
    // form are filtered further by calling filter().
    void filterIncluded();

    // Collects the radicals to be shown, filling up list, and widths.
    void filter();

    // Fills the items list with the data of each listed item in the grid. Only those radicals
    // are added that are in list too. If group is true and mode is NamedRadicals, consecutive
    // radicals with the same radical number are included in the data.
    void recomputeItems();

    // Computes new row item distribution and scrollbar sizes from the passed viewport size.
    void recompute(const QSize &size);

    // Updates the vertical scrollbar.
    void recomputeScrollbar(const QSize &size);

    // The radical or part with the given index holds a stroke count,/ and must not come last
    // in a line. The index is for the list, that itself holds indexes to filtered radicals.
    bool blockStarter(int index);

    // The width of an item in the grid. The index is for the list, that itself holds indexes
    // to filtered radicals.
    int itemWidth(int index);

    // Drawing of a single item. The index given is from the list.
    void paintItem(int index, QStylePainter &p, const ZRect &r, QColor bgcol, QColor textcol);

    // Returns the index of the item at the given coordinate in the list. If there is no item
    // at index, or the item is for stroke count, the result is -1.
    int indexAt(int x, int y);
    // Returns the index of the item at the given coordinate in the list. If there is no item
    // at index, or the item is for stroke count, the result is -1.
    int indexAt(const QPoint &pt);
    // Returns the index of the given value in the list.
    int indexOf(ushort val);

    // Returns the rectangle of the item at the index in list.
    ZRect itemRect(int index);

    // Whether the item in list at index is selected.
    bool selected(int index);

    // Returns a textual representation of the current selection and active filters.
    QString generateFiltersText();

    // The character representing the radical at index.
    QChar itemChar(int index);

    // The unified height of items.
    int heights;

    RadicalFilterModes mode;

    // The indexes of radicals shown in the view. Holds indexes in radindexes,
    // ZKanji::radklist or ZKanji::radlist, depending on the mode. When a number converted to
    // short is negative, it represents a stroke count number instead.
    std::vector<ushort> list;

    // The properties of each item in NamedRadicals mode.
    struct ItemData
    {
        // Width of the item including the radical and names when present.
        int width;
        // Number of radicals grouped together if group is true.
        int size;
        // List of names to be shown when present.
        QCharStringList names;
        // Number of names that must be drawn on the top row.
        int namebreak;
    };
    smartvector<ItemData> items;

    // Item placement information. Holds the index of the item after the last of each row.
    // (Indexes of last + 1)
    std::vector<ushort> rows;

    // Radical or index of selected items in list. Holds indexes in partindexes, radindexes
    // or ZKanji::radlist, depending on the mode.
    std::set<ushort> selection;

    // Finalized selections. Holds radical numbers for Parts and Radicals mode, and indexes in
    // ZKanji::radlist in NamedRadicals mode.
    std::vector<std::vector<ushort>> filters;

    // Filters:

    // Stroke min.
    int smin;
    // Stroke max.
    int smax;

    bool group;
    bool names;
    bool exact;
    QString search;

    int radfontsize;
    int namefontsize;
    int notesfontsize;

    // Indexes of kanji to the main kanji list. Radicals can only be included in the grid if
    // they appear in at least one kanji in the list and they are not already in a filter.
    std::vector<ushort> kanjilist;
    // Radicals found in the current kanji list that can be shown.
    QSet<ushort> included;

    typedef QAbstractScrollArea base;
};

class RadicalFiltersModel : public QObject
{
    Q_OBJECT
public:
    typedef size_t  size_type;

    RadicalFiltersModel(QObject *parent = nullptr);
    virtual ~RadicalFiltersModel();

    bool empty() const;
    size_type size() const;

    int add(const RadicalFilter &src);
    void clear();
    void remove(int index);
    void move(int index, bool up);

    const RadicalFilter& filters(int index) const;

    // The filter text is only usable for display. It can't be used to match filters when the
    // filter mode is NamedRadicals, as more radicals have the same character.
    QString filterText(const RadicalFilter &filter) const;
    // The filter text is only usable for display. It can't be used to match filters when the
    // filter mode is NamedRadicals, as more radicals have the same character.
    QString filterText(int ix) const;

    // Returns the index of the stored filter that matches the passed filter or -1 when not
    // found.
    int filterIndex(const RadicalFilter &filter);

    // Reconstructs a filter from string that was generated by filterText.
    //RadicalFilter filterFromText(QString str);

signals:
    void filterAdded();
    void filterRemoved(int index);
    void filterMoved(int index, bool up);
    void cleared();
private:
    smartvector<RadicalFilter> list;

    typedef QObject base;
};

#endif //  ZRADICALGRID_H
