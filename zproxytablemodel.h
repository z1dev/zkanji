/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZPROXYTABLEMODEL_H
#define ZPROXYTABLEMODEL_H

#include <functional>
#include <memory>
#include "zabstracttablemodel.h"
#include "smartvector.h"

class QModelIndex;
enum class InfTypes;
enum class SearchMode : uchar;
enum class SearchWildcard : uchar;
Q_DECLARE_FLAGS(SearchWildcards, SearchWildcard);
struct WordFilterConditions;
class Dictionary;
class WordResultList;
class DictionaryItemModel;
struct Interval;
struct Range;

// (Model, Column index, row A, row B) - The rows are for the source model before filtering,
// no further conversion needed.
typedef std::function<bool(DictionaryItemModel*, int, /*Qt::SortOrder,*/ int, int)> ProxySortFunction;

// Model to filter a DictionaryItemModel. Don't set any other model type as the source.
// Does similar filtering to DictionarySearchResultItemModel. But while that works on the
// dictionary itself, this model filters other models. Don't pass a model of type
// DictionarySearchResultItemModel, which does its own filtering.
class DictionarySearchFilterProxyModel : public ZAbstractProxyTableModel
{
    Q_OBJECT
public:
    DictionarySearchFilterProxyModel(QObject *parent = nullptr);
    virtual ~DictionarySearchFilterProxyModel();

    // Creates a mapping from the passed words to the source model's words. The filter must
    // only contain words found in the source model as well. This WordListProxyModel only
    // does mapping, as the filtering was done elsewhere.
    //void setFilterList(WordResultList &&filter);

    // Filters the source model according to the given dictionary search filter conditions.
    void filter(SearchMode mode, QString searchstr, SearchWildcards wildcards, bool strict, bool inflections, bool studydefs, WordFilterConditions *cond);

    // Sorts the source model by the given parameters using the passed function. Changing the
    // source model disables sorting and the function must be called again. Keeps the model
    // sorted when the source model rows change.
    void sortBy(int column, Qt::SortOrder order, ProxySortFunction func);

    // Sets the sort function without sorting the model. The next time the whole source model
    // is reset, this function will be used. In case sortBy is called first, this function
    // will be ignored.
    void prepareSortFunction(ProxySortFunction func);

    DictionaryItemModel* sourceModel();
    const DictionaryItemModel* sourceModel() const;
    void setSourceModel(DictionaryItemModel *model);

    QModelIndexList mapListToSource(const QModelIndexList &indexes);

    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex &index = QModelIndex()) const override;
    virtual int rowCount(const QModelIndex &index = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &index = QModelIndex()) const override;
    virtual QModelIndex mapToSource(const QModelIndex &index) const override;
    virtual QModelIndex mapFromSource(const QModelIndex &index) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;

    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
    //signals:
    //    void groupModelChanged();
protected:
    //virtual bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
private slots:
    void sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right, const QVector<int> &roles);
    void sourceHeaderDataChanged(Qt::Orientation orientation, int start, int end);
    void sourceAboutToBeReset();
    void sourceReset();
    //void sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
    void sourceRowsInserted(const smartvector<Interval> &intervals/*const QModelIndex &source_parent, int start, int end*/);
    //void sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
    void sourceRowsRemoved(const smartvector<Range> &ranges/*const QModelIndex &source_parent, int start, int end*/);
    //void sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow);
    void sourceRowsMoved(const smartvector<Range> &ranges, int pos/*const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row*/);

    void sourceColumnsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
    void sourceColumnsInserted(const QModelIndex &source_parent, int start, int end);
    void sourceColumnsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
    void sourceColumnsRemoved(const QModelIndex &source_parent, int start, int end);
    void sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationColumn);
    void sourceColumnsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);

    void sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void sourceLayoutChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);

    void sourceStatusChanged();

    virtual void filterMoved(int index, int to);
private:
    typedef std::vector<InfTypes>   InfVector;

    bool condEmpty() const;
    // Rebuilds list and infs from the passed model. Model should match the current
    // sourceModel() or the model which is about to be set as sourceModel().
    void fillLists(DictionaryItemModel *model);

    // [Source model index, Inflection types] Mapping from this model to the source model. The list
    // may be sorted by a sorting function.
    std::vector<std::pair<int, InfVector*>> list;
    // [list index] An ordering of list where the source model indexes are sorted.
    std::vector<int> srclist;
    //// Inflection types corresponding to the items in the same position in list. Only holds
    //// as many items as needed, and is filled with null for list items not having inflections.
    //smartvector<InfVector> infs;

    // Saved search parameters. When calling filter, if these match, the list is not updated.

    std::unique_ptr<WordFilterConditions> scond;
    SearchMode smode;
    //Dictionary *sdict;
    QString ssearchstr;
    SearchWildcards swildcards;
    bool sstrict;
    bool sinflections;
    bool sstudydefs;

    int sortcolumn;
    Qt::SortOrder sortorder;
    ProxySortFunction sortfunc;
    // Sort function to be used the next time the source model is reset. Any change that does
    // not filter the whole source again won't use this function.
    ProxySortFunction preparedsortfunc;

    QModelIndexList persistentsource;
    std::vector<QPersistentModelIndex> persistentdest;

    typedef ZAbstractProxyTableModel   base;
};

// Proxy model for DictionaryItemModel objects. Transforms the source model so each index
// corresponds to a single definition of a word instead of all definitions. Best used after
// filtering and sorting if that's needed, so not all definitions are filtered separately.
class MultiLineDictionaryItemModel : public ZAbstractProxyTableModel
{
    Q_OBJECT
signals:
    void selectionWasInserted(const smartvector<Interval> &intervals);
    void selectionWasRemoved(const smartvector<Range> &ranges);
    void selectionWasMoved(const smartvector<Range> &ranges, int pos);
public:
    MultiLineDictionaryItemModel(QObject *parent = nullptr);
    ~MultiLineDictionaryItemModel();

    void setSourceModel(ZAbstractTableModel *source);
    ZAbstractTableModel* sourceModel() const;

    // Returns a list of source indexes mapped from the proxy indexes. The result doesn't
    // contain consecutive duplicates. The passed indexes should be sorted for the result
    // to not contain duplicates at all.
    QModelIndexList uniqueSourceIndexes(const QModelIndexList& indexes) const;

    virtual QModelIndex	mapFromSource(const QModelIndex &sourceIndex) const override;
    virtual QModelIndex	mapToSource(const QModelIndex &proxyIndex) const override;

    // Returns the top row of a source item in the multi line model.
    int mapFromSourceRow(int sourcerow) const;
    // Returns the row in the source model whose part was mapped to proxyrow.
    int mapToSourceRow(int proxyrow) const;
    // Returns the number of rows of the sourcerow in the base model taken up in this model.
    int mappedRowSize(int sourcerow) const;
    // Returns the nearest row above or below proxyrow which is the first item of a separate
    // source item boundary. 
    int roundRow(int proxyrow) const;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex	parent(const QModelIndex &index) const override;

    // Returns the index of the displayed word definition at the given index.
    // This index is 1 based, because 0 is the default QVariant value.
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    // Calls the same function for the source. The proxy implementation is
    // buggy and the section is -1 when there are no rows in the data.
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    // Adds the index of the displayed word definition at the given index.
    // This index is 1 based, because 0 is the default QVariant value.
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;
    // Filters the indexes list so it only contains indexes for separate words, and forwards
    // the call to the underlying source model. Make sure the passed indexes are sorted.
    virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
private:
    // Searches the list for a matching value and sets cachepos to it. The position set should
    // match the requirement:
    // list[cachepos] <= index and list[cachepos + 1] > index (unless cachepos is at the end
    // of the list.)
    void cachePosition(int index) const;

    // Builds the mapping of this table to model, filling list and rowcount with the correct
    // values.
    void resetData(ZAbstractTableModel *model);

    // Returns the index of the displayed word definition at the given index. This index is 0
    // based.
    int definitionIndex(int index) const;

    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last);
    //void sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsInserted(const smartvector<Interval> &intervals/*const QModelIndex &parent, int first, int last*/);
    void sourceColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void sourceColumnsInserted(const QModelIndex &parent, int first, int last);
    //void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void sourceRowsRemoved(const smartvector<Range> &ranges/*const QModelIndex &parent, int first, int last*/);
    void sourceColumnsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void sourceColumnsRemoved(const QModelIndex &parent, int first, int last);
    //void sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow);
    void sourceRowsMoved(const smartvector<Range> &ranges, int pos/*const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row*/);
    void sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationColumn);
    void sourceColumnsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column);
    void sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void sourceLayoutChanged(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>(), QAbstractItemModel::LayoutChangeHint hint = QAbstractItemModel::NoLayoutChangeHint);
    void sourceModelAboutToBeReset();
    void sourceModelReset();

    // Contains the first row index of each source item in this model. A source item takes up
    // as many rows as its definition count, so multiple row indexes refer to the same item.
    std::vector<int> list;

    // The sum of the count of all rows displayed in the model.
    int rowcount;

    // Cached position in list of the last requested row for fast lookup. Doesn't change the
    // perceivable inner change of the model. If invalid, it's recomputed.
    mutable int cachepos;

    // Persistent indexes saved at the beginning of a layout change to be passed to
    // changePersistentIndexList() as first parameter.
    QModelIndexList persistentsource;
    // persistent index of source model, row of this model relative to original model].
    // Used when mapping persistent local indexes to persistent source model indexes. The
    // second value is the row position inside the multiple rows of the local indexes.
    std::vector<std::pair<QPersistentModelIndex, int>> persistentdest;

    typedef ZAbstractProxyTableModel base;
};

#endif // ZPROXYTABLEMODEL_H