/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include <QApplication>
//#include <QElapsedTimer>

#include "zproxytablemodel.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "ranges.h"

#include "checked_cast.h"

//-------------------------------------------------------------


DictionarySearchFilterProxyModel::DictionarySearchFilterProxyModel(QObject *parent) : base(parent), sortcolumn(-1), sortorder(Qt::AscendingOrder)/*, sdict(nullptr)*/
{
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterMoved, this, &DictionarySearchFilterProxyModel::filterMoved);
}

DictionarySearchFilterProxyModel::~DictionarySearchFilterProxyModel()
{
    for (auto p : list)
        delete p.second;
}

//void DictionarySearchFilterProxyModel::setFilterList(WordResultList &&filter)
//{
//    list.clear();
//
//    // Get the words list of both the filter and the original model, and sort them by
//    // the pointer of the word entries. Once sorted, words that are found in both lists
//    // are retrieved and the list is filled with the sorted indexes in the source model.
//
//    std::vector<WordEntry*> sortedfilter;
//    std::vector<std::pair<int, WordEntry*>> sortedmodel;
//
//    for (int ix = 0; ix != filter.size(); ++ix)
//        sortedfilter.push_back(filter.items(ix));
//
//    std::sort(sortedfilter.begin(), sortedfilter.end(), [](WordEntry* a, WordEntry *b) {
//        return (intptr_t)a < (intptr_t)b;
//    });
//
//    for (int ix = 0; ix != sourceModel()->rowCount(); ++ix)
//        sortedmodel.push_back(std::make_pair(ix, sourceModel()->items(ix)));
//
//    std::sort(sortedmodel.begin(), sortedmodel.end(), [](const std::pair<int, WordEntry*> &a, const std::pair<int, WordEntry*> &b) {
//        return (intptr_t)a.second < (intptr_t)b.second;
//    });
//
//    int filterpos = 0;
//    int modelpos = 0;
//    int filtersize = sortedfilter.size();
//    int modelsize = sortedmodel.size();
//
//    WordEntry** filterdata = sortedfilter.data();
//    std::pair<int, WordEntry*> *modeldata = sortedmodel.data();
//
//    while (filterpos != filtersize && modelpos != modelsize)
//    {
//        if (filterdata[filterpos] == modeldata[modelpos].second)
//        {
//            list.push_back(modeldata[modelpos].first);
//            ++filterpos;
//            ++modelpos;
//            continue;
//        }
//
//        while (filterpos != filtersize && filterdata[filterpos] < modeldata[modelpos].second)
//            ++filterpos;
//        if (filterpos == filtersize)
//            break;
//        while (modelpos != modelsize && filterdata[filterpos] > modeldata[modelpos].second)
//            ++modelpos;
//    }
//
//    std::sort(list.begin(), list.end());
//    invalidateFilter();
//}

void DictionarySearchFilterProxyModel::filter(SearchMode mode, QString searchstr, SearchWildcards wildcards, bool strict, bool inflections, bool studydefs, WordFilterConditions *cond)
{
    if (sourceModel() == nullptr || sourceModel()->dictionary() == nullptr || (/*sdict == sourceModel()->dictionary() &&*/ smode == mode && swildcards == wildcards && sstrict == strict && sinflections == inflections && sstudydefs == studydefs && ((!scond && !cond) || (!scond == !cond && *scond == *cond)) && ssearchstr == searchstr))
        return;

    smode = mode;
    //sdict = sourceModel()->dictionary();
    swildcards = wildcards;
    sstrict = strict;
    sinflections = inflections;
    sstudydefs = studydefs;
    if (cond)
    {
        if (!scond)
            scond.reset(new WordFilterConditions);
        *scond = *cond;
    }
    else
        scond.reset();

    ssearchstr = searchstr;

    fillLists(sourceModel());
}

void DictionarySearchFilterProxyModel::sortBy(int column, Qt::SortOrder order, ProxySortFunction func)
{
    if (sortcolumn == column && sortorder == order && sortfunc.target<bool(*)(DictionaryItemModel*, int, int, int)>() == func.target<bool(*)(DictionaryItemModel*, int, int, int)>())
    {
        preparedsortfunc = sortfunc;
        return;
    }

    if (sourceModel() == nullptr)
    {
        sortcolumn = column;
        sortorder = order;
        preparedsortfunc = sortfunc = func;

        return;
    }

    // Sorting means layout change.

    emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint);

    // Source model indexes are persistent when we are only sorting the lists, so they
    // are suitable to convert old persistent indexes to new ones.

    QModelIndexList oldprs = persistentIndexList();
    QModelIndexList newprs;
    newprs.reserve(oldprs.size());
    for (const QModelIndex &ind : oldprs)
        newprs.push_back(mapToSource(ind));

    // Sorting.

    sortcolumn = column;
    sortorder = order;
    preparedsortfunc = sortfunc = func;


    // Rebuilding srclist.
    auto endit = srclist.begin() + list.size();
    std::iota(srclist.begin(), endit, 0);

    if (!func)
        std::sort(list.begin(), list.end(), [](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) { return a.first < b.first; });
    else
    {
        DictionaryItemModel *source = sourceModel();
        std::sort(list.begin(), list.end(), [this, source](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) {
            return sortfunc(source, sortcolumn, a.first, b.first) != (sortorder == Qt::DescendingOrder);
        });

        std::sort(srclist.begin(), srclist.begin() + list.size(), [this](int a, int b) { return list[a].first < list[b].first; });
    }
   
    // Fixing persistent indexes.

    for (QModelIndex &ind : newprs)
        ind = mapFromSource(ind);
    changePersistentIndexList(oldprs, newprs);

    emit layoutChanged();
}

void DictionarySearchFilterProxyModel::prepareSortFunction(ProxySortFunction func)
{
    preparedsortfunc = func;
}

DictionaryItemModel* DictionarySearchFilterProxyModel::sourceModel()
{
    return (DictionaryItemModel*)base::sourceModel();
}

const DictionaryItemModel* DictionarySearchFilterProxyModel::sourceModel() const
{
    return (DictionaryItemModel*)base::sourceModel();
}

void DictionarySearchFilterProxyModel::setSourceModel(DictionaryItemModel *newmodel)
{
    DictionaryItemModel *smodel = sourceModel();
    if (smodel == newmodel)
        return;

    if (smodel != nullptr)
    {
        disconnect(smodel, nullptr, this, nullptr);
        //    disconnect(smodel, &DictionaryItemModel::dataChanged, this, &DictionarySearchFilterProxyModel::groupModelChanged);
        //    disconnect(smodel, &DictionaryItemModel::modelReset, this, &DictionarySearchFilterProxyModel::groupModelChanged);
        //    disconnect(smodel, &DictionaryItemModel::rowsInserted, this, &DictionarySearchFilterProxyModel::groupModelChanged);
        //    disconnect(smodel, &DictionaryItemModel::rowsMoved, this, &DictionarySearchFilterProxyModel::groupModelChanged);
        //    disconnect(smodel, &DictionaryItemModel::rowsRemoved, this, &DictionarySearchFilterProxyModel::groupModelChanged);
    }

    sortfunc = nullptr;
    preparedsortfunc = nullptr;
    fillLists(newmodel);
    base::setSourceModel(newmodel);

    connect(newmodel, &DictionaryItemModel::dataChanged, this, &DictionarySearchFilterProxyModel::sourceDataChanged);
    connect(newmodel, &DictionaryItemModel::headerDataChanged, this, &DictionarySearchFilterProxyModel::sourceHeaderDataChanged);
    connect(newmodel, &DictionaryItemModel::modelAboutToBeReset, this, &DictionarySearchFilterProxyModel::sourceAboutToBeReset);
    connect(newmodel, &DictionaryItemModel::modelReset, this, &DictionarySearchFilterProxyModel::sourceReset);
    //connect(model, &DictionaryItemModel::rowsAboutToBeInserted, this, &DictionarySearchFilterProxyModel::sourceRowsAboutToBeInserted);
    connect(newmodel, &DictionaryItemModel::rowsWereInserted, this, &DictionarySearchFilterProxyModel::sourceRowsInserted);
    //connect(model, &DictionaryItemModel::rowsAboutToBeRemoved, this, &DictionarySearchFilterProxyModel::sourceRowsAboutToBeRemoved);
    connect(newmodel, &DictionaryItemModel::rowsWereRemoved, this, &DictionarySearchFilterProxyModel::sourceRowsRemoved);
    //connect(model, &DictionaryItemModel::rowsAboutToBeMoved, this, &DictionarySearchFilterProxyModel::sourceRowsAboutToBeMoved);
    connect(newmodel, &DictionaryItemModel::rowsWereMoved, this, &DictionarySearchFilterProxyModel::sourceRowsMoved);

    connect(newmodel, &DictionaryItemModel::columnsAboutToBeInserted, this, &DictionarySearchFilterProxyModel::sourceColumnsAboutToBeInserted);
    connect(newmodel, &DictionaryItemModel::columnsInserted, this, &DictionarySearchFilterProxyModel::sourceColumnsInserted);
    connect(newmodel, &DictionaryItemModel::columnsAboutToBeRemoved, this, &DictionarySearchFilterProxyModel::sourceColumnsAboutToBeRemoved);
    connect(newmodel, &DictionaryItemModel::columnsRemoved, this, &DictionarySearchFilterProxyModel::sourceColumnsRemoved);
    connect(newmodel, &DictionaryItemModel::columnsAboutToBeMoved, this, &DictionarySearchFilterProxyModel::sourceColumnsAboutToBeMoved);
    connect(newmodel, &DictionaryItemModel::columnsMoved, this, &DictionarySearchFilterProxyModel::sourceColumnsMoved);

    connect(newmodel, &DictionaryItemModel::layoutAboutToBeChanged, this, &DictionarySearchFilterProxyModel::sourceLayoutAboutToBeChanged);
    connect(newmodel, &DictionaryItemModel::layoutChanged, this, &DictionarySearchFilterProxyModel::sourceLayoutChanged);
    connect(newmodel, &DictionaryItemModel::statusChanged, this, &DictionarySearchFilterProxyModel::sourceStatusChanged);
}

QModelIndexList DictionarySearchFilterProxyModel::mapListToSource(const QModelIndexList &indexes)
{
    QModelIndexList result;
    result.reserve(indexes.size());
    for (auto it = indexes.begin(); it != indexes.end(); ++it)
        result << mapToSource(*it);

    return result;
}

QModelIndex DictionarySearchFilterProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        return QModelIndex();
    return createIndex(row, column, nullptr);
}

QModelIndex DictionarySearchFilterProxyModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int DictionarySearchFilterProxyModel::rowCount(const QModelIndex &index) const
{
    if (index.isValid() /*|| index.model() != this*/)
        return 0;
    return tosigned(list.size());
}

int DictionarySearchFilterProxyModel::columnCount(const QModelIndex &index) const
{
    if (index.isValid() /*|| index.model() != this*/)
        return 0;
    return sourceModel()->columnCount();
}

QModelIndex DictionarySearchFilterProxyModel::mapToSource(const QModelIndex &index) const
{
    if (!index.isValid() || index.model() != this || index.row() >= tosigned(list.size()))
        return QModelIndex();

    return sourceModel()->index(list[index.row()].first, index.column());
}

QModelIndex DictionarySearchFilterProxyModel::mapFromSource(const QModelIndex &index) const
{
    if (!index.isValid() || index.model() != sourceModel())
        return QModelIndex();

    // The size of srclist is not necessarily the same as list, so an artificial end is used.
    auto endit = srclist.begin() + list.size();
    auto it = std::lower_bound(srclist.begin(), endit, index.row(), [this](int src, int val) { return list[src].first < val; });
    if (it == endit)
        return QModelIndex();
    if (list[*it].first != index.row())
        return QModelIndex();

    return createIndex(*it, index.column(), nullptr);
}

QVariant DictionarySearchFilterProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == (int)DictRowRoles::Inflection)
    {
        auto inflist = list[index.row()].second;
        return QVariant::fromValue((intptr_t)inflist);
    }

    return base::data(index, role);
}

QMap<int, QVariant> DictionarySearchFilterProxyModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles = base::itemData(index);
    QVariant variantData = data(index, (int)DictRowRoles::Inflection);
    if (variantData.isValid())
        roles.insert((int)DictRowRoles::Inflection, variantData);
    return roles;
}

Qt::DropActions DictionarySearchFilterProxyModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    auto actions = sourceModel()->supportedDropActions(samesource, mime);
    actions &= ~Qt::MoveAction;

    return actions;
}

void DictionarySearchFilterProxyModel::sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right, const QVector<int> &roles)
{
#ifdef _DEBUG
    if (!source_top_left.isValid() || source_top_left.model() != sourceModel() || !source_bottom_right.isValid() || source_bottom_right.model() != sourceModel() || source_top_left.row() > source_bottom_right.row() || source_top_left.column() > source_bottom_right.column())
        throw "Invalid range.";
#endif

    if (sourceModel() == nullptr || !source_top_left.isValid() || !source_bottom_right.isValid())
        return;

    bool filtering = !ssearchstr.isEmpty() || !condEmpty();
    if (base::sourceModel() == nullptr || (!filtering && !sortfunc))
    {
        emit dataChanged(index(source_top_left.row(), source_top_left.column()), index(source_bottom_right.row(), source_bottom_right.column()), roles);
        return;
    }

    DictionaryItemModel *source = sourceModel();

    Dictionary *dict = source->dictionary();

    // Data change can cause words to be excluded from list, other words to be included, and
    // the rest to be sorted differently.

    // [list index] Items in the list that should be deleted as they don't match new
    // conditions.
    std::vector<int> toremove;
    // [source model index, inflections] Words not listed yet, but added here.
    std::vector<std::pair<int, InfVector*>> toadd;
    // [source model index, inflections] Words listed that must be updated.
    std::vector<std::pair<int, InfVector*>> toupdate;

    int top = source_top_left.row();
    int bottom = source_bottom_right.row();
    auto srcit = std::lower_bound(srclist.begin(), srclist.end(), top, [this](int src, int top) { return list[src].first < top; });

    // Check what to do with the changed rows and put them in the appropriate list.
    for (int ix = top, last = bottom + 1; ix != last; ++ix)
    {
        std::vector<InfTypes> winfs;
        bool match = !filtering || dict->wordMatches(source->indexes(ix), smode, ssearchstr, swildcards, sstrict, sinflections, sstudydefs, scond.get(), &winfs);

        bool found = srcit != srclist.end() && list[*srcit].first == ix;

        if (!match && found)
            toremove.push_back(*srcit);
        else if (match && !found)
            toadd.emplace_back(ix, winfs.empty() ? nullptr : new InfVector(winfs));
        else if (match)
            toupdate.emplace_back(ix, winfs.empty() ? nullptr : new InfVector(winfs));

        if (found)
            ++srcit;
    }

    // Delete rows that should not be listed any more.

    // The indexes in toremove are only out of order if sorting is active.
    if (sortfunc)
        std::sort(toremove.begin(), toremove.end());

    smartvector<Range> removed;
    for (int pos = tosigned(toremove.size()) - 1, prev = pos - 1; pos != -1; --prev)
    {
        if (prev == -1 || toremove[prev + 1] - toremove[prev] > 1)
        {
            int rfirst = toremove[prev + 1];
            int rlast = toremove[pos];

            //beginRemoveRows(QModelIndex(), rfirst, rlast);

            for (int iy = rfirst; iy != rlast + 1; ++iy)
                delete list[iy].second;
            list.erase(list.begin() + rfirst, list.begin() + (rlast + 1));

            //endRemoveRows();

            removed.insert(removed.begin(), { rfirst, rlast });

            pos = prev;
        }
    }

    // Every value in srclist would be accessed. It's easier to rebuild it.
    srclist.resize(list.size());
    for (int iy = 0, sizy = tosigned(srclist.size()); iy != sizy; ++iy)
        srclist[iy] = iy;
    if (sortfunc)
        std::sort(srclist.begin(), srclist.begin() + list.size(), [this](int a, int b) { return list[a].first < list[b].first; });

    if (!removed.empty())
        signalRowsRemoved(removed);

    // Find the new position of rows that changed, using the layout change begin/end block to
    // be able to freely modify the lists. This means when layout change starts, objects that
    // need to keep indexes intact will create persistent indexes. Those must be updated
    // before the layout change ends.

    if (!toupdate.empty() && sortfunc)
    {
        emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint);

        // Source model indexes are persistent when we are only sorting the lists, so they
        // are suitable to convert old persistent indexes to new ones.

        QModelIndexList oldprs = persistentIndexList();
        QModelIndexList newprs;
        newprs.reserve(oldprs.size());
        for (const QModelIndex &ind : oldprs)
            newprs.push_back(mapToSource(ind));

        // Values needing an update are removed from the lists and put back in at the correct
        // positions.

        // [list indexes] Holds indexes to list for each item in toupdate.
        std::vector<int> srcupdatepos;
        srcupdatepos.reserve(toupdate.size());
        srcit = srclist.begin();
        for (int ix = 0, siz = tosigned(toupdate.size()); ix != siz; ++ix)
        {
            srcit = std::lower_bound(srcit, srclist.end(), toupdate[ix].first, [this](int src, int val) { return list[src].first < val; });
            srcupdatepos.push_back(*srcit);
        }

        // Removing everything from list that are in toupdate.
        for (int ix = 0, siz = tosigned(srcupdatepos.size()); ix != siz; ++ix)
        {
            int listpos = srcupdatepos[ix];
            list[listpos].first = -1;
            delete list[listpos].second;
        }
        list.resize(std::remove_if(list.begin(), list.end(), [](const std::pair<int, InfVector*> &val) { return val.first == -1; }) - list.begin());

        // Finding positions of the items in toupdate to be placed back in list. It's assumed
        // that the remaining values in list are correct, and they were ordered by sortfunc.

        std::sort(toupdate.begin(), toupdate.end(), [source, this](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) {
            bool v = sortfunc(source, sortcolumn, a.first, b.first);
            if (sortorder == Qt::DescendingOrder)
                return !v;
            return v;
        });

        for (int ix = 0, siz = tosigned(toupdate.size()); ix != siz; ++ix)
        {
            auto listit = std::upper_bound(list.begin(), list.end(), toupdate[ix].first, [this, source](int srcindex, const std::pair<int, InfVector*> &listitem) {
                return sortfunc(source, sortcolumn, srcindex, listitem.first) != (sortorder == Qt::DescendingOrder);
            });
            list.insert(listit, toupdate[ix]);
        }

        // Rebuilding srclist.

        srclist.resize(list.size());
        for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
            srclist[ix] = ix;

        std::sort(srclist.begin(), srclist.begin() + list.size(), [this](int a, int b) { return list[a].first < list[b].first; });


        // Fixing persistent indexes.

        for (QModelIndex &ind : newprs)
            ind = mapFromSource(ind);
        changePersistentIndexList(oldprs, newprs);

        emit layoutChanged();
    }
    else if (!sortfunc)
    {
        // No sorting, check if anything is to be updated and signal it.

        // Rows between source index top and bottom must be signaled. Because there's no
        // sorting, the rows in list are already in good order.

        int first = std::lower_bound(list.begin(), list.end(), top, [](const std::pair<int, InfVector*> &item, int top) { return item.first < top; }) - list.begin();
        int last = std::upper_bound(list.begin(), list.end(), bottom, [](int bottom, const std::pair<int, InfVector*> &item) { return bottom < item.first; }) - list.begin();

        if (first < last)
            emit dataChanged(index(first, 0), index(last - 1, columnCount() - 1), roles);
    }


    if (toadd.empty())
        return;

    // Inserting items from toadd.

    smartvector<Interval> inserted;

    if (sortfunc)
    {
        //srclist.resize(list.size() + toadd.size());

        std::sort(toadd.begin(), toadd.end(), [source, this](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) {
            return sortfunc(source, sortcolumn, a.first, b.first) != (sortorder == Qt::DescendingOrder);
        });

        auto endit = std::upper_bound(list.begin(), list.end(), toadd.back().first, [this, source](int srcindex, const std::pair<int, InfVector*> &listitem) { 
            return sortfunc(source, sortcolumn, srcindex, listitem.first) != (sortorder == Qt::DescendingOrder);
        });
        for (int pos = tosigned(toadd.size()) - 1, prev = pos - 1; pos != -1; --prev)
        {
            auto newendit = prev == -1 ? endit : std::upper_bound(list.begin(), endit, toadd[prev].first, [this, source](int srcindex, const std::pair<int, InfVector*> &listitem) {
                return sortfunc(source, sortcolumn, srcindex, listitem.first) != (sortorder == Qt::DescendingOrder);
            });

            if (prev == -1 || newendit != endit)
            {
                inserted.push_back({ tosigned(endit - list.begin()), pos - prev });
                list.insert(endit, toadd.begin() + (prev + 1), toadd.begin() + (pos + 1));

                pos = prev;
            }
        }


        //int addtop = 0;
        //int cnt = 0;

        //int listpos = 0;

        //for (int ix = 0, siz = toadd.size(); ix != siz + 1; ++ix)
        //{
        //    // Insert position of current item.
        //    int pos = 0;

        //    if (ix != siz)
        //    {
        //        //if (sortfunc)
        //        //{
        //        listpos = std::upper_bound(list.begin() + listpos, list.end(), toadd[ix].first, [this, model](int srcindex, const std::pair<int, InfVector*> &listitem) {
        //            return sortfunc(model, sortcolumn, sortorder, srcindex, listitem.first);
        //        }) - list.begin();
        //        pos = listpos;
        //        //}
        //        //else
        //        //{
        //        //    listit = std::upper_bound(listit, list.end(), toadd[ix].first, [this, model](int srcindex, const std::pair<int, InfVector*> &listitem) {
        //        //        return srcindex < listitem.first;
        //        //    });
        //        //    pos = listit - list.begin();
        //        //}

        //        if (cnt == 0)
        //            addtop = pos;
        //    }

        //    if (ix == siz || pos != addtop)
        //    {
        //        beginInsertRows(QModelIndex(), addtop, addtop + cnt - 1);
        //        list.insert(list.begin() + addtop, toadd.begin() + (ix - cnt), toadd.begin() + ix);

        //        // Every item in srclist has to be accessed to fix the values. Instead,
        //        // it's easier to just rebuild it.
        //        for (int ix = 0, siz = list.size(); ix != siz; ++ix)
        //            srclist[ix] = ix;

        //        //if (sortfunc)
        //        std::sort(srclist.begin(), srclist.begin() + list.size(), [this](int a, int b) { return list[a].first < list[b].first; });

        //        endInsertRows();
        //        cnt = 1;
        //        addtop = pos;
        //    }
        //    else
        //        ++cnt;
        //}
    }
    else // No sort function
    {
        //beginInsertRows(QModelIndex(), pos, pos + toadd.size() - 1);

        auto endit = std::upper_bound(list.begin(), list.end(), toadd.back().first, [this, source](int srcindex, const std::pair<int, InfVector*> &listitem) { return srcindex < listitem.first; });
        for (int pos = tosigned(toadd.size()) - 1, prev = pos - 1; pos != -1; --prev)
        {
            auto newendit = prev == -1 ? endit : std::upper_bound(list.begin(), endit, toadd[prev].first, [this, source](int srcindex, const std::pair<int, InfVector*> &listitem) { return srcindex < listitem.first; });

            if (prev == -1 || newendit != endit)
            {
                inserted.push_back({ tosigned(endit - list.begin()), pos - prev });
                list.insert(endit, toadd.begin() + (prev + 1), toadd.begin() + (pos + 1));

                pos = prev;
            }
        }

        //endInsertRows();

    }

    srclist.resize(list.size());
    for (int iy = 0, sizy = tosigned(srclist.size()); iy != sizy; ++iy)
        srclist[iy] = iy;
    if (sortfunc)
        std::sort(srclist.begin(), srclist.begin() + list.size(), [this](int a, int b) { return list[a].first < list[b].first; });

    if (!inserted.empty())
        signalRowsInserted(inserted);
}

void DictionarySearchFilterProxyModel::sourceHeaderDataChanged(Qt::Orientation orientation, int start, int end)
{
    if (orientation == Qt::Horizontal)
    {
        emit headerDataChanged(orientation, start, end);
        return;
    }

    // Vertical header change is generally not supported. Emit change for the whole header
    // as a safety belt.

    emit headerDataChanged(orientation, 0, tosigned(list.size()) - 1);
}

void DictionarySearchFilterProxyModel::sourceAboutToBeReset()
{
    beginResetModel();
}

void DictionarySearchFilterProxyModel::sourceReset()
{
    fillLists(sourceModel());
    endResetModel();
}

//void DictionarySearchFilterProxyModel::sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
//{
//    if (base::sourceModel() == nullptr || (ssearchstr.isEmpty() && condEmpty() && !sortfunc))
//    {
//        beginInsertRows(QModelIndex(), start, end);
//        return;
//    }
//}

void DictionarySearchFilterProxyModel::sourceRowsInserted(const smartvector<Interval> &intervals/*const QModelIndex &source_parent, int start, int end*/)
{
    if (sourceModel() == nullptr || intervals.empty())
        return;

    int insertcnt = _intervalSize(intervals);

    bool filtering = !ssearchstr.isEmpty() || !condEmpty();
    if (!filtering && !sortfunc)
    {
        list.reserve(list.size() + insertcnt);
        srclist.reserve(srclist.size() + insertcnt);

        for (int ix = 0; ix != insertcnt; ++ix)
        {
            list.emplace_back(tosigned(list.size()), nullptr);
            srclist.push_back(tosigned(srclist.size()));
        }

        signalRowsInserted(intervals);

        //endInsertRows();

        return;

    }

    //QMessageBox::information(qApp->activeWindow(), "bbb", "1", QMessageBox::Ok);

    DictionaryItemModel *source = sourceModel();
    Dictionary *dict = source->dictionary();
    //int insertcnt = end - start + 1;

    //qint64 startuptime = 0;
    //qint64 sorttime = 0;
    //qint64 begintime = 0;
    //qint64 endtime = 0;
    //qint64 betweentime = 0;
    //qint64 insorttime = 0;

    //QElapsedTimer t;
    //t.start();


    // [source model index, inflection] list of rows that were accepted by the search filters.
    std::vector<std::pair<int, InfVector*>> toadd;

    // Filling toadd list with indexes and inflections to add to list below.

    if (!filtering || insertcnt < 100)
    {
        // For small lists, check that every item matches the filters separately.

        std::unique_ptr<InfVector> winfs;
        if (filtering)
            winfs.reset(new InfVector);

        // Delta position of the current interval. Each interval start at the original line
        // where they were inserted.
        int ipos = 0;
        for (int ix = 0, siz = tosigned(intervals.size()); ix != siz; ++ix)
        {
            const Interval *i = intervals[ix];
            for (int iy = 0; iy != i->count; ++iy)
            {
                int index = ipos + i->index + iy;
                if (!filtering || dict->wordMatches(source->indexes(index), smode, ssearchstr, swildcards, sstrict, sinflections, sstudydefs, scond.get(), winfs.get()))
                {
                    toadd.emplace_back(index, !winfs || winfs->empty() ? nullptr : winfs.release());
                    if (filtering && !winfs->empty())
                        winfs.reset(new InfVector);
                }
            }
            ipos += i->count;
        }
    }
    else
    {
        // Finding the inserted rows and their inflections that match the word filters.

        WordResultList wlist(dict);
        // Index of words in dictionary that were inserted.
        std::vector<int> wpool;
        wpool.reserve(insertcnt);
        // [word index, source model index]
        std::vector<std::pair<int, int>> worder;

        // Delta position of the current interval. Each interval start at the original line
        // where they were inserted.
        int ipos = 0;
        for (int ix = 0, siz = tosigned(intervals.size()); ix != siz; ++ix)
        {
            const Interval *i = intervals[ix];
            for (int iy = 0; iy != i->count; ++iy)
            {
                int index = ipos + i->index + iy;
                int windex = source->indexes(index);
                wpool.push_back(windex);
                worder.emplace_back(windex, index);
            }
            ipos += i->count;
        }

        std::sort(worder.begin(), worder.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.first < b.first; });

        dict->findWords(wlist, smode, ssearchstr, swildcards, sstrict, sinflections, sstudydefs, &wpool, scond.get());

        const auto &ixs = wlist.getIndexes();
        auto &infs = wlist.getInflections();
        for (int ix = 0, siz = tosigned(wlist.size()); ix != siz; ++ix)
        {
            bool hasinf = tosigned(infs.size()) > ix;
            toadd.emplace_back(ixs[ix], hasinf ? infs[ix] : nullptr);
            if (hasinf)
                infs[ix] = nullptr;
        }
        infs.clear();

        // The first value in toadd pairs is currently the word index in the dictionary, but
        // we need source model indexes instead. Map the word indexes using worder.

        std::sort(toadd.begin(), toadd.end(), [](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) { return a.first < b.first; });

        int orderpos = 0;
        for (int ix = 0, siz = tosigned(toadd.size()); ix != siz; ++ix)
        {
            while (worder[orderpos].first != toadd[ix].first)
                ++orderpos;
            toadd[ix].first = worder[orderpos].second;
        }
    }


    if (!sortfunc)
    {
        // Updating source model indexes in list before insertion.

        int iindex = tosigned(intervals.size()) - 1;
        const Interval *i = intervals[iindex];
        // Number of source model indexes that were inserted before the current list item.
        int iinsert = insertcnt;

        for (int ix = tosigned(list.size()) - 1; ix != -1; --ix)
        {
            while (iindex != -1 && list[ix].first < i->index)
            {
                iinsert -= i->count;
                --iindex;
                if (iindex != -1)
                    i = intervals[iindex];
            }
            if (iindex == -1)
                break;
            list[ix].first += iinsert;
        }

        if (toadd.empty())
            return;

        smartvector<Interval> inserted;

        // Inserting items of toadd to list at the correct positions and filling the inserted
        // list for signalling.

        std::sort(toadd.begin(), toadd.end(), [](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) { return a.first < b.first; });
        // Insert position of toadd items in list.
        int insertpos = std::upper_bound(list.begin(), list.end(), toadd.back().first, [](int start, const std::pair<int, InfVector*> &item) { return start < item.first; }) - list.begin();
        for (int pos = tosigned(toadd.size()) - 1, prev = pos - 1; pos != -1; --prev)
        {
            if (prev == -1 || (insertpos != 0 && toadd[prev].first < list[insertpos - 1].first))
            {
                list.insert(list.begin() + insertpos, toadd.begin() + (prev + 1), toadd.begin() + (pos + 1));
                inserted.insert(inserted.begin(), { insertpos, pos - prev });
                if (prev != -1)
                    insertpos = std::upper_bound(list.begin(), list.begin() + insertpos, toadd[prev].first, [](int start, const std::pair<int, InfVector*> &item) { return start < item.first; }) - list.begin();
                pos = prev;
            }
        }

        while (srclist.size() != list.size())
            srclist.push_back(tosigned(srclist.size()));

        if (!inserted.empty())
            signalRowsInserted(inserted);

        //// Update list and srclist to reflect the original state before insertion. Objects
        //// reacting to beginInsert should get the same source as before.

        //int pos = std::lower_bound(list.begin(), list.end(), start, [](const std::pair<int, InfVector*> &item, int val) {
        //    return item.first < val;
        //}) - list.begin();
        //while (pos != list.size())
        //    list[pos++].first += insertcnt;

        //if (toadd.empty())
        //    return;

        //std::sort(toadd.begin(), toadd.end(), [](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) { return a.first < b.first; });

        //start = std::upper_bound(list.begin(), list.end(), toadd.front().first, [](int start, const std::pair<int, InfVector*> &item) { return start < item.first; }) - list.begin();


        //beginInsertRows(QModelIndex(), start, start + toadd.size() - 1);
        //list.insert(list.begin() + start, toadd.begin(), toadd.end());
        //srclist.reserve(list.size());
        //while (srclist.size() != list.size())
        //    srclist.push_back(srclist.size());
        //endInsertRows();

        return;
    }

    //startuptime = t.nsecsElapsed();
    //t.invalidate();
    //t.start();

    // Updating source model indexes in list before insertion.

    int iindex = tosigned(intervals.size()) - 1;
    const Interval *i = intervals[iindex];
    // Number of source model indexes that were inserted before the current list item.
    int iinsert = insertcnt;

    for (int ix = tosigned(srclist.size()) - 1; ix != -1; --ix)
    {
        while (iindex != -1 && list[srclist[ix]].first < i->index)
        {
            iinsert -= i->count;
            --iindex;
            if (iindex != -1)
                i = intervals[iindex];
        }
        if (iindex == -1)
            break;
        list[srclist[ix]].first += iinsert;
    }


    //for (int ix = 0; ix != list.size(); ++ix)
    //{
    //    if (list[ix].first >= start)
    //        list[ix].first += insertcnt;
    //}

    //int addtop = 0;
    //int cnt = 0;

    //int listpos = 0;

    //sorttime = t.nsecsElapsed();
    //t.invalidate();
    //t.start();

    if (toadd.empty())
        return;

    std::sort(toadd.begin(), toadd.end(), [source, this](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) {
        bool v = sortfunc(source, sortcolumn, a.first, b.first);
        if (sortorder == Qt::DescendingOrder)
            return !v;
        return v;
    });

    smartvector<Interval> inserted;

    // Inserting items of toadd to list at the correct positions and filling the inserted
    // list for signalling.

    // Insert position of toadd items in list.
    int insertpos = std::upper_bound(list.begin(), list.end(), toadd.back().first, [source, this](int start, const std::pair<int, InfVector*> &item) {
        bool v = sortfunc(source, sortcolumn, start, item.first);
        if (sortorder == Qt::DescendingOrder)
            return !v;
        return v;
    }) - list.begin();

    for (int pos = tosigned(toadd.size()) - 1, prev = pos - 1; pos != -1; --prev)
    {
        if (prev == -1 || (insertpos != 0 && (sortfunc(source, sortcolumn, toadd[prev].first, list[insertpos - 1].first) != (sortorder == Qt::DescendingOrder))))
        {
            list.insert(list.begin() + insertpos, toadd.begin() + (prev + 1), toadd.begin() + (pos + 1));
            inserted.insert(inserted.begin(), { insertpos, pos - prev });
            if (prev != -1)
                insertpos = std::upper_bound(list.begin(), list.begin() + insertpos, toadd[prev].first, [this, source](int start, const std::pair<int, InfVector*> &item) { 
                return sortfunc(source, sortcolumn, start, item.first) != (sortorder == Qt::DescendingOrder);
            }) - list.begin();
            pos = prev;
        }
    }

    // Rebuilding srclist from scratch.
    srclist.resize(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        srclist[ix] = ix;
    std::sort(srclist.begin(), srclist.begin() + list.size(), [this](int a, int b) { return list[a].first < list[b].first; });

    if (!inserted.empty())
        signalRowsInserted(inserted);


    //std::sort(toadd.begin(), toadd.end(), [model, this](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) {
    //    return sortfunc(model, sortcolumn, sortorder, a.first, b.first);
    //});
    //for (int ix = 0, siz = toadd.size(); ix != siz + 1; ++ix)
    //{
    //    // Insert position of current item.
    //    int pos = 0;

    //    if (ix != siz)
    //    {
    //        listpos = std::upper_bound(list.begin() + listpos, list.end(), toadd[ix].first, [this, model](int srcindex, const std::pair<int, InfVector*> &listitem) {
    //            return sortfunc(model, sortcolumn, sortorder, srcindex, listitem.first);
    //        }) - list.begin();
    //        pos = listpos;

    //        if (cnt == 0)
    //            addtop = pos;
    //    }

    //    if (ix == siz || pos != addtop)
    //    {
    //        //betweentime += t.nsecsElapsed();
    //        //t.invalidate();
    //        //t.start();

    //        beginInsertRows(QModelIndex(), addtop, addtop + cnt - 1);

    //        //begintime += t.nsecsElapsed();
    //        //t.invalidate();
    //        //t.start();

    //        list.insert(list.begin() + addtop, toadd.begin() + (ix - cnt), toadd.begin() + ix);

    //        //betweentime += t.nsecsElapsed();
    //        //t.invalidate();
    //        //t.start();

    //        // Every item in srclist has to be accessed to fix the values. Instead, it's
    //        // easier to just rebuild it.
    //        for (int ix = 0, siz = list.size(); ix != siz; ++ix)
    //            srclist[ix] = ix;
    //        std::sort(srclist.begin(), srclist.begin() + list.size(), [this](int a, int b) { return list[a].first < list[b].first; });

    //        //insorttime += t.nsecsElapsed();
    //        //t.invalidate();
    //        //t.start();

    //        endInsertRows();

    //        //endtime += t.nsecsElapsed();
    //        //t.invalidate();
    //        //t.start();

    //        cnt = 1;
    //        addtop = pos;
    //    }
    //    else
    //        ++cnt;
    //}
    

    //betweentime += t.nsecsElapsed();
    //t.invalidate();

    //QMessageBox::information(qApp->activeWindow(), "blblb", QString("startup: %1,\nsort: %2,\nbegin: %3,\nend: %4,\nbetween: %5,\ninsort: %6")
    //    .arg(startuptime).arg(sorttime).arg(begintime).arg(endtime).arg(betweentime).arg(insorttime), QMessageBox::Ok);
}

//void DictionarySearchFilterProxyModel::sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
//{
//    bool filtering = !ssearchstr.isEmpty() || !condEmpty();
//    if (base::sourceModel() == nullptr || (!filtering && !sortfunc))
//    {
//        beginRemoveRows(QModelIndex(), start, end);
//        return;
//    }
//
//    if (!sortfunc)
//    {
//        int top = std::lower_bound(list.begin(), list.end(), start, [](const std::pair<int, InfVector*> &item, int start) { return item.first < start; }) - list.begin();
//        int bottom = std::upper_bound(list.begin() + top, list.end(), end, [](int end, const std::pair<int, InfVector*> &item) { return end < item.first; }) - list.begin();
//
//        if (top != bottom)
//            beginRemoveRows(QModelIndex(), top, bottom - 1);
//
//        return;
//    }
//
//    //qint64 sorttime = 0;
//    //qint64 begintime = 0;
//    //qint64 endtime = 0;
//    //qint64 betweentime = 0;
//    //qint64 insorttime = 0;
//
//    //QElapsedTimer t;
//    //t.start();
//
//    // When the lists are sorted, there can be possibly many beginRemove/endRemove blocks, and
//    // it's late to do it all in sourceRowsRemoved() where other objects can randomly access
//    // rows already removed.
//    // To avoid that, all the removal are done here in as many blocks as needed. In the
//    // endRemove part only the source row indexes are updated.
//
//    // [list index] list of rows to be removed.
//    std::vector<int> toremove;
//
//    auto srcit = std::lower_bound(srclist.begin(), srclist.end(), start, [this](int src, int start) { return list[src].first < start; });
//    while (srcit != srclist.end() && list[*srcit].first <= end)
//    {
//        toremove.push_back(*srcit);
//        ++srcit;
//    }
//
//    std::sort(toremove.begin(), toremove.end());
//
//    //sorttime = t.nsecsElapsed();
//    //t.invalidate();
//    //t.start();
//
//    for (int ix = toremove.size() - 1, rpos = ix - 1; ix != -1; --rpos)
//    {
//        if (rpos == -1 || toremove[rpos + 1] - toremove[rpos] > 1)
//        {
//            int rfirst = toremove[rpos + 1];
//            int rlast = toremove[ix];
//
//            //betweentime += t.nsecsElapsed();
//            //t.invalidate();
//            //t.start();
//            beginRemoveRows(QModelIndex(), rfirst, rlast);
//            //begintime += t.nsecsElapsed();
//            //t.invalidate();
//            //t.start();
//            for (int iy = rfirst; iy != rlast + 1; ++iy)
//                delete list[iy].second;
//            list.erase(list.begin() + rfirst, list.begin() + (rlast + 1));
//
//            //betweentime += t.nsecsElapsed();
//            //t.invalidate();
//            //t.start();
//
//            int cnt = rlast - rfirst + 1;
//            for (int iy = 0, siz = srclist.size(); iy != siz; ++iy)
//            {
//                int &val = srclist[iy];
//                if (val >= rfirst)
//                {
//                    if (val <= rlast)
//                        val = -1;
//                    else
//                        val -= cnt;
//                }
//            }
//            std::remove(srclist.begin(), srclist.end(), -1);
//
//            //insorttime += t.nsecsElapsed();
//            //t.invalidate();
//            //t.start();
//            endRemoveRows();
//            //endtime += t.nsecsElapsed();
//            //t.invalidate();
//            //t.start();
//            ix = rpos;
//        }
//    }
//
//    srclist.resize(list.size());
//    //betweentime += t.nsecsElapsed();
//    //t.invalidate();
//
//    //QMessageBox::information(qApp->activeWindow(), "blblb", QString("sort: %1,\nbegin: %2,\nend: %3,\nbetween: %4,\ninsort: %5")
//    //    .arg(sorttime).arg(begintime).arg(endtime).arg(betweentime).arg(insorttime), QMessageBox::Ok);
//
//}

void DictionarySearchFilterProxyModel::sourceRowsRemoved(const smartvector<Range> &ranges/*const QModelIndex &source_parent, int start, int end*/)
{
    if (sourceModel() == nullptr || ranges.empty())
        return;

    //int removed = end - start + 1;
    int removedcnt = _rangeSize(ranges);

    bool filtering = !ssearchstr.isEmpty() || !condEmpty();
    if (!filtering && !sortfunc)
    {
        list.resize(list.size() - removedcnt);
        srclist.resize(list.size());

        //endRemoveRows();

        signalRowsRemoved(ranges);

        return;
    }

    if (!sortfunc)
    {
        // Updating source model indexes in list, marking with -1 those that are to be removed.

        const Range *r = ranges.back();
        // Index of current range.
        int rindex = tosigned(ranges.size()) - 1;
        // Number of items to decrement source model index in list at pos.
        int rcnt = removedcnt;
        // Position of the last item in list to be removed.
        int removepos = -1;
        for (int ix = tosigned(list.size()) - 1; ix != -1; --ix)
        {
            int &lfirst = list[ix].first;

            // Find the next range that comes in front of the current item.
            while (r->first > lfirst)
            {
                rcnt -= r->last - r->first + 1;
                --rindex;
                if (rindex == -1)
                    break;
                r = ranges[rindex];
            }

            if (rindex == -1)
                break;

            if (r->last >= lfirst)
            {
                removepos = std::max(removepos, ix);
                lfirst = -1;
                continue;
            }

            lfirst -= rcnt;
        }

        // Nothing was removed.
        if (removepos == -1)
            return;

        smartvector<Range> removed;

        // Build removed list and update list.
        for (int pos = removepos, prev = pos - 1; pos != -1; --prev)
        {
            if (prev == -1 || list[prev].first != -1)
            {
                removed.insert(removed.begin(), { prev + 1, pos });
                list.erase(list.begin() + (prev + 1), list.begin() + (pos + 1));

                // Find the next item to remove.
                while (prev != -1 && list[prev].first != -1)
                    --prev;
                pos = prev;
            }
        }

        srclist.resize(list.size());

        if (!removed.empty())
            signalRowsRemoved(removed);

        return;
    }

    // Updating source model indexes in list, marking with -1 those that are to be removed.

    const Range *r = ranges.back();
    // Index of current range.
    int rindex = tosigned(ranges.size()) - 1;
    // Number of items to decrement source model index in list at pos.
    int rcnt = removedcnt;
    // Position of the last item in list to be removed.
    int removepos = -1;
    for (int ix = tosigned(srclist.size()) - 1; ix != -1; --ix)
    {
        int &lfirst = list[srclist[ix]].first;

        // Find the next range that comes in front of the current item.
        while (r->first > lfirst)
        {
            rcnt -= r->last - r->first + 1;
            --rindex;
            if (rindex == -1)
                break;
            r = ranges[rindex];
        }

        if (rindex == -1)
            break;

        if (r->last >= lfirst)
        {
            removepos = std::max(removepos, srclist[ix]);
            lfirst = -1;
            continue;
        }

        lfirst -= rcnt;
    }

    // Nothing was removed.
    if (removepos == -1)
        return;

    smartvector<Range> removed;

    // Build removed list and update list.
    for (int pos = removepos, prev = pos - 1; pos != -1; --prev)
    {
        if (prev == -1 || list[prev].first != -1)
        {
            removed.insert(removed.begin(), { prev + 1, pos });
            list.erase(list.begin() + (prev + 1), list.begin() + (pos + 1));

            // Find the next item to remove.
            while (prev != -1 && list[prev].first != -1)
                --prev;
            pos = prev;
        }
    }

    // Rebuilding srclist from scratch.
    srclist.resize(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        srclist[ix] = ix;
    std::sort(srclist.begin(), srclist.begin() + list.size(), [this](int a, int b) { return list[a].first < list[b].first; });

    if (!removed.empty())
        signalRowsRemoved(removed);

    //if (!sortfunc)
    //{
    //    int top = std::lower_bound(list.begin(), list.end(), start, [](const std::pair<int, InfVector*> &item, int start) { return item.first < start; }) - list.begin();
    //    int bottom = std::upper_bound(list.begin() + top, list.end(), end, [](int end, const std::pair<int, InfVector*> &item) { return end < item.first; }) - list.begin();

    //    for (int ix = bottom, siz = list.size(); ix != siz; ++ix)
    //        list[ix].first -= removed;

    //    if (top == bottom)
    //        return;

    //    for (int ix = top; ix != bottom; ++ix)
    //        delete list[ix].second;
    //    list.erase(list.begin() + top, list.begin() + bottom);

    //    srclist.resize(list.size());

    //    endRemoveRows();
    //    return;
    //}

    //// When the list is sorted, the items are already removed in sourceRowsAboutToBeRemoved().
    //// The only thing left to do is to fix the source indexes in list.

    //int srcpos = std::upper_bound(srclist.begin(), srclist.end(), end, [this](int end, int src) { return end < list[src].first; }) - srclist.begin();
    //while (srcpos < srclist.size())
    //    list[srcpos++].first -= removed;
}

//void DictionarySearchFilterProxyModel::sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int start, int end, const QModelIndex &destinationParent, int dest)
//{
//    bool filtering = !ssearchstr.isEmpty() || !condEmpty();
//    if (base::sourceModel() == nullptr || (!filtering && !sortfunc))
//    {
//        beginMoveRows(QModelIndex(), start, end, QModelIndex(), dest);
//        return;
//    }
//
//    // Some operations are a bit expensive so the parameters are changed here if it's possible
//    // to move less items.
//
//    if (dest < start && end - start + 1 > start - dest)
//    {
//        int tmp = start;
//        start = dest;
//        dest = end + 1;
//        end = tmp - 1;
//    }
//    else if (dest > end && end - start + 1 > dest - end - 1)
//    {
//        int tmp = dest;
//        dest = start;
//        start = end + 1;
//        end = tmp - 1;
//    }
//
//    if (!sortfunc)
//    {
//        int top = std::lower_bound(list.begin(), list.end(), start, [](const std::pair<int, InfVector*> &item, int start) { return item.first < start; }) - list.begin();
//        int bottom = std::upper_bound(list.begin() + top, list.end(), end, [](int end, const std::pair<int, InfVector*> &item) { return end < item.first; }) - list.begin();
//        int pos = std::lower_bound(list.begin(), list.end(), dest, [](const std::pair<int, InfVector*> &item, int dest) { return item.first < dest; }) - list.begin();
//
//        if (top != bottom && (pos < top || pos > bottom))
//            beginMoveRows(QModelIndex(), top, bottom - 1, QModelIndex(), pos);
//        return;
//    }
//
//    // Nothing to do here when the lists are sorted. The move can change the sorting, if the
//    // sorting function takes the item positions into account, which will be done in
//    // sourceRowsMoved() only.
//
//}

void DictionarySearchFilterProxyModel::sourceRowsMoved(const smartvector<Range> &ranges, int pos /*const QModelIndex &parent, int start, int end, const QModelIndex &destination, int dest*/)
{
    if (sourceModel() == nullptr || ranges.empty())
        return;

    bool filtering = !ssearchstr.isEmpty() || !condEmpty();
    if (!filtering && !sortfunc)
    {
        signalRowsMoved(ranges, pos);
        return;
    }

    //pos = _fixMovePos(ranges, pos);

    if (!sortfunc)
    {
        smartvector<Range> moved;
        int movepos = std::lower_bound(list.begin(), list.end(), pos, [](const std::pair<int, InfVector*> &p, int pos) { return p.first < pos; }) - list.begin();

        std::vector<int> delta;
        _moveDelta(ranges, pos, delta);

        // Updating source model indexes in list for ranges moved forward, and the items
        // between them that are moved back.

        int first = 0;
        int movedelta = 0;
        for (int ix = 0, siz = tosigned(ranges.size()); ix != siz; ++ix)
        {
            const Range *r = ranges[ix];
            int newfirst = std::lower_bound(list.begin() + first, list.end(), r->first, [](const std::pair<int, InfVector*> &p, int pos) { return p.first < pos; }) - list.begin();

            // Updating indexes in list to point to the correct indexes in the source model.
            for (int iy = first; movedelta != 0 && iy != newfirst; ++iy)
                list[iy].first -= movedelta;

            if (newfirst == tosigned(list.size()) || delta[ix] <= 0)
                break;

            int next = std::upper_bound(list.begin() + newfirst, list.end(), r->last, [](int pos, const std::pair<int, InfVector*> &p) { return pos < p.first; }) - list.begin();
            if (newfirst != next)
                moved.push_back({ newfirst, next - 1 });

            movedelta += r->last - r->first + 1;

            // Updating indexes in list to point to the correct indexes in the source model.
            for (int iy = newfirst; iy != next; ++iy)
                list[iy].first += delta[ix];

            first = next;
        }
        // Updating indexes in list to point to the correct indexes in the source model.
        for (int iy = first; movedelta != 0 && iy != movepos; ++iy)
            list[iy].first -= movedelta;

        int movedinspos = tosigned(moved.size());

        // Updating source model indexes in list for ranges moved backward, and the items
        // between them that are moved forward.

        int last = tosigned(list.size());
        movedelta = 0;
        for (int ix = tosigned(ranges.size()) - 1; ix != -1; --ix)
        {
            const Range *r = ranges[ix];
            int newlast = std::upper_bound(list.begin(), list.begin() + last, r->last, [](int pos, const std::pair<int, InfVector*> &p) { return pos < p.first; }) - list.begin();

            // Updating indexes in list to point to the correct indexes in the source model.
            for (int iy = newlast; movedelta != 0 && iy != last; ++iy)
                list[iy].first += movedelta;

            if (newlast == 0 || delta[ix] >= 0)
                break;

            int prev = std::lower_bound(list.begin(), list.begin() + newlast, r->first, [](const std::pair<int, InfVector*> &p, int pos) { return p.first < pos; }) - list.begin();
            if (newlast != prev)
                moved.insert(moved.begin() + movedinspos, { prev, newlast - 1 });

            movedelta += r->last - r->first + 1;

            // Updating indexes in list to point to the correct indexes in the source model.
            for (int iy = prev; iy != newlast; ++iy)
                list[iy].first += delta[ix];

            last = prev;
        }
        // Updating indexes in list to point to the correct indexes in the source model.
        for (int iy = movepos; movedelta != 0 && iy != last; ++iy)
            list[iy].first += movedelta;


        _fixMoveRanges(moved, movepos);

        if (!moved.empty())
        {
            _moveRanges(moved, movepos, list);
            signalRowsMoved(moved, movepos);
        }
        return;
    }

    throw "Moving items in the base model of a SORTED proxy model is currently not supported, because it'd be too time consuming and not used anywhere (I hope).";

    //// The source rows are moved, but the proxy model should show the same order as before,
    //// so the indexes are fixed first. No signal needs to be sent for this, as there is no
    //// visible change from the outside.

    //int srctop = std::lower_bound(srclist.begin(), srclist.end(), start, [this](int src, int start) { return list[src].first < start; }) - srclist.begin();
    //int srcbottom = std::upper_bound(srclist.begin() + srctop, srclist.end(), end, [this](int end, int src) { return end < list[src].first; }) - srclist.begin();
    //int srcdest = std::lower_bound(srclist.begin(), srclist.end(), dest, [this](int src, int dest) { return list[src].first < dest; }) - srclist.begin();

    //if (srctop == srcbottom || srctop == srclist.size() || srcdest == srcbottom ||
    //    (srcdest != srclist.size() && list[srcdest].first >= list[srctop].first &&
    //    (srcbottom != srclist.size() && list[srcdest].first <= list[srcbottom].first)))
    //    return;

    //// Holds the rows to remove and put back when fixing the layout below.
    //// Initially: [list index, word inflection after search]
    //// During layout change: [source model index, word inflection after search]
    //std::vector<std::pair<int, InfVector*>> tomove;
    //tomove.reserve(srcbottom - srctop);

    //if (dest < start)
    //{
    //    for (int ix = srctop; ix != srcbottom; ++ix)
    //    {
    //        list[ix].first -= start - dest;
    //        tomove.emplace_back(ix, list[ix].second);
    //    }
    //    for (int ix = srcdest; ix != srctop; ++ix)
    //        list[ix].first += end - start + 1;

    //    // Do the same move in srclist.
    //    std::vector<int> srcmoved(srclist.begin() + srctop, srclist.begin() + srcbottom);
    //    srclist.erase(srclist.begin() + srctop, srclist.begin() + srcbottom);
    //    srclist.insert(srclist.begin() + srcdest, srcmoved.begin(), srcmoved.end());
    //}
    //else
    //{
    //    for (int ix = srctop; ix != srcbottom; ++ix)
    //    {
    //        list[ix].first += dest - end - 1;
    //        tomove.emplace_back(ix, list[ix].second);
    //    }
    //    for (int ix = srcbottom; ix != srcdest; ++ix)
    //        list[ix].first -= end - start + 1;

    //    // Do the same move in srclist with the only difference that moving down means the
    //    // destination moved also.
    //    std::vector<int> srcmoved(srclist.begin() + srctop, srclist.begin() + srcbottom);
    //    srclist.erase(srclist.begin() + srctop, srclist.begin() + srcbottom);
    //    srclist.insert(srclist.begin() + (srcdest - srcmoved.size()), srcmoved.begin(), srcmoved.end());
    //}

    //// Moving rows equals to layout change as the sort function can take the order into
    //// account. To fix that the moved rows are taken out and put back at their correct place.

    //emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint);

    //// Source model indexes are persistent when we are only sorting the lists, so they
    //// are suitable to convert old persistent indexes to new ones.

    //QModelIndexList oldprs = persistentIndexList();
    //QModelIndexList newprs;
    //newprs.reserve(oldprs.size());
    //for (const QModelIndex &ind : oldprs)
    //    newprs.push_back(mapToSource(ind));



    //// Values needing an update are removed from the lists then put back in at the correct
    //// positions.

    //// Removing everything from list to be moved. These are all in tomove so no delete for
    //// inflections is necessary. The list indexes in tomove are replaced with source model
    //// indexes.
    //for (int ix = 0, siz = tomove.size(); ix != siz; ++ix)
    //{
    //    int listpos = tomove[ix].first;
    //    tomove[ix].first = list[listpos].first;
    //    list[listpos].first = -1;
    //}
    //list.resize(std::remove_if(list.begin(), list.end(), [](const std::pair<int, InfVector*> &val) { return val.first == -1; }) - list.begin());

    //// Finding positions of the items in tomove to be placed back in list. It's assumed that
    //// both list and items in tomove were originally sorted with sortfunc, so the new
    //// positions can be found with sortfunc again.

    //DictionaryItemModel *model = sourceModel();
    //for (int ix = 0, siz = tomove.size(); ix != siz; ++ix)
    //{
    //    auto listit = std::upper_bound(list.begin(), list.end(), tomove[ix].first, [this, model](int srcindex, const std::pair<int, InfVector*> &listitem) {
    //        return sortfunc(model, sortcolumn, sortorder, srcindex, listitem.first);
    //    });
    //    list.insert(listit, tomove[ix]);
    //}


    //// Rebuilding srclist.

    //srclist.resize(list.size());
    //for (int ix = 0; ix != list.size(); ++ix)
    //    srclist[ix] = ix;
    //if (sortfunc)
    //    std::sort(srclist.begin(), srclist.end(), [this](int a, int b) { return list[a].first < list[b].first; });


    //// Fixing persistent indexes.

    //for (QModelIndex &ind : newprs)
    //    ind = mapFromSource(ind);
    //changePersistentIndexList(oldprs, newprs);

    //emit layoutChanged();
}

void DictionarySearchFilterProxyModel::sourceColumnsAboutToBeInserted(const QModelIndex &/*source_parent*/, int start, int end)
{
    beginInsertColumns(QModelIndex(), start, end);
}

void DictionarySearchFilterProxyModel::sourceColumnsInserted(const QModelIndex &/*source_parent*/, int /*start*/, int /*end*/)
{
    endInsertColumns();
}

void DictionarySearchFilterProxyModel::sourceColumnsAboutToBeRemoved(const QModelIndex &/*source_parent*/, int start, int end)
{
    beginRemoveColumns(QModelIndex(), start, end);
}

void DictionarySearchFilterProxyModel::sourceColumnsRemoved(const QModelIndex &/*source_parent*/, int /*start*/, int /*end*/)
{
    endRemoveColumns();
}

void DictionarySearchFilterProxyModel::sourceColumnsAboutToBeMoved(const QModelIndex &/*sourceParent*/, int sourceStart, int sourceEnd, const QModelIndex &/*destinationParent*/, int destinationColumn)
{
    beginMoveColumns(QModelIndex(), sourceStart, sourceEnd, QModelIndex(), destinationColumn);
}

void DictionarySearchFilterProxyModel::sourceColumnsMoved(const QModelIndex &/*parent*/, int /*start*/, int /*end*/, const QModelIndex &/*destination*/, int /*row*/)
{
    endMoveColumns();
}

void DictionarySearchFilterProxyModel::sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &/*parents*/, QAbstractItemModel::LayoutChangeHint hint)
{
    emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), hint);
    persistentsource = persistentIndexList();
    persistentdest.clear();
    persistentdest.reserve(persistentsource.size());
    for (const QModelIndex &ind : persistentsource)
        persistentdest.push_back(mapToSource(ind));
}

void DictionarySearchFilterProxyModel::sourceLayoutChanged(const QList<QPersistentModelIndex> &/*parents*/, QAbstractItemModel::LayoutChangeHint hint)
{
    fillLists(sourceModel());

    QModelIndexList destindexes;
    destindexes.reserve(persistentsource.size());
    for (const QModelIndex &ind : persistentdest)
        destindexes.push_back(mapFromSource(ind));
    changePersistentIndexList(persistentsource, destindexes);
    persistentdest.clear();
    persistentsource.clear();

    emit layoutChanged(QList<QPersistentModelIndex>(), hint);
}

void DictionarySearchFilterProxyModel::sourceStatusChanged()
{
    emit statusChanged();
}

void DictionarySearchFilterProxyModel::filterMoved(int index, int to)
{
    if (!scond)
        return;

    std::vector<Inclusion> &inc = scond->inclusions;
    if (index >= tosigned(inc.size()) && to >= tosigned(inc.size()))
        return;

    if (to > index)
        --to;

    Inclusion moved = index >= tosigned(inc.size()) ? Inclusion::Ignore : inc[index];
    if (index < tosigned(inc.size()))
        inc.erase(inc.begin() + index);

    if (to >= tosigned(inc.size()))
    {
        if (moved == Inclusion::Ignore)
            return;
        while (tosigned(inc.size()) <= to)
            inc.push_back(Inclusion::Ignore);
        inc[to] = moved;
        return;
    }

    inc.insert(inc.begin() + to, moved);
    if (moved == Inclusion::Ignore)
        return;
    while (!inc.empty() && inc.back() == Inclusion::Ignore)
        inc.pop_back();
}

bool DictionarySearchFilterProxyModel::condEmpty() const
{
    return !scond || !*scond;
}

void DictionarySearchFilterProxyModel::fillLists(DictionaryItemModel *source)
{
    for (auto p : list)
        delete p.second;

    sortfunc = preparedsortfunc;
    std::vector<std::pair<int, InfVector*>>().swap(list);
    int cnt = source == nullptr ? 0 : source->rowCount();

    bool filtered = !ssearchstr.isEmpty() || !condEmpty();
    if (!filtered)
    {
        list.reserve(cnt);
        for (int ix = 0; ix != cnt; ++ix)
            list.emplace_back(ix, nullptr);
    }
    else
    {
        Dictionary *dict = source->dictionary();

        WordResultList wlist(dict);
        std::vector<int> wfilter;
        wfilter.reserve(cnt);
        // [word index, source model index]
        std::vector<std::pair<int, int>> worder;
        for (int ix = 0; ix != cnt; ++ix)
        {
            int windex = source->indexes(ix);
            wfilter.push_back(windex);
            worder.emplace_back(windex, ix);
        }
        std::sort(worder.begin(), worder.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) { 
            if (a.first != b.first)
                return a.first < b.first;
            return a.second < b.second;
        });

        if (!ssearchstr.isEmpty())
            dict->findWords(wlist, smode, ssearchstr, swildcards, sstrict, sinflections, sstudydefs, &wfilter, scond.get());
        else if (scond)
        {
            for (int ix = 0, siz = tosigned(wfilter.size()); ix != siz; ++ix)
                if (ZKanji::wordfilters().match(dict->wordEntry(wfilter[ix]), scond.get()))
                    wlist.add(wfilter[ix]);
        }
        std::vector<int>().swap(wfilter);

        const auto &windexes = wlist.getIndexes();
        auto &winfs = wlist.getInflections();

        // [word index, inflection] This list will hold the result from findWords in word
        // index order, used for building list from worder.
        std::vector<std::pair<int, InfVector*>> tmplist;

        for (int ix = 0, siz = tosigned(wlist.size()); ix != siz; ++ix)
        {
            bool hasinf = tosigned(winfs.size()) > ix;
            tmplist.emplace_back(windexes[ix], hasinf ? winfs[ix] : nullptr);
            if (hasinf)
                winfs[ix] = nullptr;
        }
        wlist.clear();

        std::sort(tmplist.begin(), tmplist.end(), [](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) { 
            if (a.first != b.first)
                return a.first < b.first;
            // Make the null inflection come last if present.
            return (intptr_t)a.second > (intptr_t)b.second;
        });

        int wpos = 0;
        bool first;
        InfVector *inf;
        for (int ix = 0, siz = tosigned(tmplist.size()), wsiz = tosigned(worder.size()); ix != siz && wpos != wsiz; ++ix)
        {
            while (worder[wpos].first != tmplist[ix].first)
                ++wpos;
            // Duplicate word indexes are possible in the filtered model (worder), but the
            // inflection list pointer can't be shared between them. When first is false, the
            // inflections are copied.
            first = true;
            inf = tmplist[ix].second;

            // tmplist can have duplicate word indexes where one item has inflection and an
            // other hasn't. It should be shown in dictionary listings, but when filtering a
            // model it's not possible to handle this correctly. The duplicates with
            // inflection are ignored. Above tmplist was sorted with the null inflection
            // coming last. If such item is not present, the last inflection is used.

            while (ix != siz - 1 && tmplist[ix].first == tmplist[ix + 1].first)
            {
                inf = tmplist[ix + 1].second;
                delete tmplist[ix].second;
                ++ix;
            }

            while (wpos != wsiz && worder[wpos].first == tmplist[ix].first)
            {
                list.emplace_back(worder[wpos].second, !inf || first ? inf : new InfVector(*inf));
                ++wpos;
                first = false;
            }
        }

        if (!sortfunc)
            std::sort(list.begin(), list.end(), [](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) { return a.first < b.first; });
    }

    // Rebuilding srclist.
    srclist.resize(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        srclist[ix] = ix;

    if (sortfunc)
    {
        std::sort(list.begin(), list.end(), [this, source](const std::pair<int, InfVector*> &a, const std::pair<int, InfVector*> &b) {
            return sortfunc(source, sortcolumn, a.first, b.first) != (sortorder == Qt::DescendingOrder);
        });

        std::sort(srclist.begin(), srclist.end(), [this](int a, int b) { return list[a].first < list[b].first; });
    }
}


//-------------------------------------------------------------


MultiLineDictionaryItemModel::MultiLineDictionaryItemModel(QObject *parent) : base(parent)
{

}

MultiLineDictionaryItemModel::~MultiLineDictionaryItemModel()
{
}

void MultiLineDictionaryItemModel::setSourceModel(ZAbstractTableModel *source)
{
    ZAbstractTableModel *oldmodel = sourceModel();

    beginResetModel();

    // Disconnect everything
    if (oldmodel != nullptr)
    {
        disconnect(oldmodel, &DictionaryItemModel::dataChanged, this, &MultiLineDictionaryItemModel::sourceDataChanged);
        disconnect(oldmodel, &DictionaryItemModel::headerDataChanged, this, &MultiLineDictionaryItemModel::sourceHeaderDataChanged);
        //disconnect(oldmodel, &DictionaryItemModel::rowsAboutToBeInserted, this, &MultiLineDictionaryItemModel::sourceRowsAboutToBeInserted);
        disconnect(oldmodel, &DictionaryItemModel::rowsWereInserted, this, &MultiLineDictionaryItemModel::sourceRowsInserted);
        disconnect(oldmodel, &DictionaryItemModel::columnsAboutToBeInserted, this, &MultiLineDictionaryItemModel::sourceColumnsAboutToBeInserted);
        disconnect(oldmodel, &DictionaryItemModel::columnsInserted, this, &MultiLineDictionaryItemModel::sourceColumnsInserted);
        //disconnect(oldmodel, &DictionaryItemModel::rowsAboutToBeRemoved, this, &MultiLineDictionaryItemModel::sourceRowsAboutToBeRemoved);
        disconnect(oldmodel, &DictionaryItemModel::rowsWereRemoved, this, &MultiLineDictionaryItemModel::sourceRowsRemoved);
        disconnect(oldmodel, &DictionaryItemModel::columnsAboutToBeRemoved, this, &MultiLineDictionaryItemModel::sourceColumnsAboutToBeRemoved);
        disconnect(oldmodel, &DictionaryItemModel::columnsRemoved, this, &MultiLineDictionaryItemModel::sourceColumnsRemoved);
        //disconnect(oldmodel, &DictionaryItemModel::rowsAboutToBeMoved, this, &MultiLineDictionaryItemModel::sourceRowsAboutToBeMoved);
        disconnect(oldmodel, &DictionaryItemModel::rowsWereMoved, this, &MultiLineDictionaryItemModel::sourceRowsMoved);
        disconnect(oldmodel, &DictionaryItemModel::columnsAboutToBeMoved, this, &MultiLineDictionaryItemModel::sourceColumnsAboutToBeMoved);
        disconnect(oldmodel, &DictionaryItemModel::columnsMoved, this, &MultiLineDictionaryItemModel::sourceColumnsMoved);
        disconnect(oldmodel, &DictionaryItemModel::layoutAboutToBeChanged, this, &MultiLineDictionaryItemModel::sourceLayoutAboutToBeChanged);
        disconnect(oldmodel, &DictionaryItemModel::layoutChanged, this, &MultiLineDictionaryItemModel::sourceLayoutChanged);
        disconnect(oldmodel, &DictionaryItemModel::modelAboutToBeReset, this, &MultiLineDictionaryItemModel::sourceModelAboutToBeReset);
        disconnect(oldmodel, &DictionaryItemModel::modelReset, this, &MultiLineDictionaryItemModel::sourceModelReset);
    }


    resetData(source);

    base::setSourceModel(source);

    // Connect everything.
    if (source != nullptr)
    {
        connect(source, &DictionaryItemModel::dataChanged, this, &MultiLineDictionaryItemModel::sourceDataChanged);
        connect(source, &DictionaryItemModel::headerDataChanged, this, &MultiLineDictionaryItemModel::sourceHeaderDataChanged);
        //connect(source, &DictionaryItemModel::rowsAboutToBeInserted, this, &MultiLineDictionaryItemModel::sourceRowsAboutToBeInserted);
        connect(source, &DictionaryItemModel::rowsWereInserted, this, &MultiLineDictionaryItemModel::sourceRowsInserted);
        connect(source, &DictionaryItemModel::columnsAboutToBeInserted, this, &MultiLineDictionaryItemModel::sourceColumnsAboutToBeInserted);
        connect(source, &DictionaryItemModel::columnsInserted, this, &MultiLineDictionaryItemModel::sourceColumnsInserted);
        //connect(source, &DictionaryItemModel::rowsAboutToBeRemoved, this, &MultiLineDictionaryItemModel::sourceRowsAboutToBeRemoved);
        connect(source, &DictionaryItemModel::rowsWereRemoved, this, &MultiLineDictionaryItemModel::sourceRowsRemoved);
        connect(source, &DictionaryItemModel::columnsAboutToBeRemoved, this, &MultiLineDictionaryItemModel::sourceColumnsAboutToBeRemoved);
        connect(source, &DictionaryItemModel::columnsRemoved, this, &MultiLineDictionaryItemModel::sourceColumnsRemoved);
        //connect(source, &DictionaryItemModel::rowsAboutToBeMoved, this, &MultiLineDictionaryItemModel::sourceRowsAboutToBeMoved);
        connect(source, &DictionaryItemModel::rowsWereMoved, this, &MultiLineDictionaryItemModel::sourceRowsMoved);
        connect(source, &DictionaryItemModel::columnsAboutToBeMoved, this, &MultiLineDictionaryItemModel::sourceColumnsAboutToBeMoved);
        connect(source, &DictionaryItemModel::columnsMoved, this, &MultiLineDictionaryItemModel::sourceColumnsMoved);
        connect(source, &DictionaryItemModel::layoutAboutToBeChanged, this, &MultiLineDictionaryItemModel::sourceLayoutAboutToBeChanged);
        connect(source, &DictionaryItemModel::layoutChanged, this, &MultiLineDictionaryItemModel::sourceLayoutChanged);
        connect(source, &DictionaryItemModel::modelAboutToBeReset, this, &MultiLineDictionaryItemModel::sourceModelAboutToBeReset);
        connect(source, &DictionaryItemModel::modelReset, this, &MultiLineDictionaryItemModel::sourceModelReset);
    }

    endResetModel();
}

ZAbstractTableModel* MultiLineDictionaryItemModel::sourceModel() const
{
    return (ZAbstractTableModel*)base::sourceModel();
}

QModelIndexList MultiLineDictionaryItemModel::uniqueSourceIndexes(const QModelIndexList& indexes) const
{
    QModelIndexList result;

    QModelIndex prev;
    for (auto it = indexes.cbegin(); it != indexes.cend(); ++it)
    {
        QModelIndex tmp = mapToSource(*it);
        if (tmp == prev)
            continue;

        prev = tmp;
        result << tmp;
    }

    return result;
}

QModelIndex	MultiLineDictionaryItemModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid() || rowcount == 0)
        return QModelIndex();

    return createIndex(mapFromSourceRow(sourceIndex.row()), sourceIndex.column());
}

QModelIndex	MultiLineDictionaryItemModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid() || rowcount == 0)
        return QModelIndex();

    return sourceModel()->index(mapToSourceRow(proxyIndex.row()), proxyIndex.column());
}

int MultiLineDictionaryItemModel::mapFromSourceRow(int sourcerow) const
{
    if (sourcerow < 0 || rowcount == 0)
        return -1;

    return list[sourcerow];
}

int MultiLineDictionaryItemModel::mapToSourceRow(int proxyrow) const
{
    if (proxyrow < 0 || rowcount == 0)
        return -1;

    // Binary search for the row value in list, or the highest value not larger than row.
    cachePosition(proxyrow);

    return cachepos;
}

int MultiLineDictionaryItemModel::mappedRowSize(int sourcerow) const
{
#ifdef _DEBUG
    if (sourcerow < 0 || sourcerow >= tosigned(list.size()))
        throw "Source row out of range.";
#endif
    return (sourcerow == tosigned(list.size()) - 1 ? rowcount : list[sourcerow + 1]) - list[sourcerow];
}

int MultiLineDictionaryItemModel::roundRow(int proxyrow) const
{
    if (proxyrow == rowcount)
        return rowcount;

    int row = mapToSourceRow(proxyrow);

    // Distance from the word starting row.
    int start = proxyrow - list[row];
    // Distance from the starting row of the next word, or to the end of the items.
    int end = (row < tosigned(list.size()) - 1 ? list[row + 1] : rowcount) - proxyrow;
    if (start <= end)
        return list[row];
    return row < tosigned(list.size()) - 1 ? list[row + 1] : rowcount;
}

int MultiLineDictionaryItemModel::rowCount(const QModelIndex &/*parent*/) const
{
    return rowcount;
}

int MultiLineDictionaryItemModel::columnCount(const QModelIndex &parent) const
{
    if (sourceModel() == nullptr)
        return 0;

    return sourceModel()->columnCount(parent);
}

QModelIndex MultiLineDictionaryItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= rowcount)
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex	MultiLineDictionaryItemModel::parent(const QModelIndex &/*index*/) const
{
    return QModelIndex();
}

QVariant MultiLineDictionaryItemModel::data(const QModelIndex &index, int role) const
{
    if (role != (int)DictRowRoles::DefIndex)
    {
        QAbstractItemModel *m = sourceModel();
        if (!m)
            return base::data(index, role);
        return m->data(mapToSource(index), role);
    }

    return QVariant(definitionIndex(index.row()));
}

QVariant MultiLineDictionaryItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QAbstractItemModel *m = sourceModel();
    if (!m || orientation == Qt::Vertical)
        return base::headerData(section, orientation, role);
    return m->headerData(section, orientation, role);
}

QMap<int, QVariant> MultiLineDictionaryItemModel::itemData(const QModelIndex &index) const
{
    QAbstractItemModel *m = sourceModel();
    QMap<int, QVariant> result = m != nullptr ? m->itemData(mapToSource(index)) : base::itemData(index);
    result.insert((int)DictRowRoles::DefIndex, QVariant(definitionIndex(index.row())));
    return result;
}

QMimeData* MultiLineDictionaryItemModel::mimeData(const QModelIndexList& indexes) const
{
    return sourceModel()->mimeData(uniqueSourceIndexes(indexes));
}

void MultiLineDictionaryItemModel::cachePosition(int index) const
{
    if (cachepos >= tosigned(list.size()))
        cachepos = tosigned(list.size()) / 2;

    // We were lucky and requested the same value as before.
    if (list[cachepos] == index || (list[cachepos] < index && (cachepos == tosigned(list.size()) - 1 || list[cachepos + 1] > index)))
        return;

    // Feeling lucky, guessing that the next or previous row is what we were looking for.
    if (cachepos != tosigned(list.size()) - 1 && (list[cachepos + 1] == index || (list[cachepos + 1] < index && (cachepos + 1 == tosigned(list.size()) - 1 || list[cachepos + 2] > index))))
    {
        ++cachepos;
        return;
    }
    if (cachepos != 0 && (list[cachepos - 1] == index || (list[cachepos] > index && list[cachepos - 1] < index)))
    {
        --cachepos;
        return;
    }

    // Finally get the value with binary search.

    int left = list[cachepos] > index ? 0 : cachepos;
    int right = list[cachepos] > index ? cachepos : tosigned(list.size()) - 1;
    auto it = std::upper_bound(list.begin() + left, list.begin() + (right + 1), index);
    if (it == list.begin() + (right + 1))
        cachepos = tosigned(list.size()) - 1;
    else
        cachepos = (it - list.begin()) - 1;
}

void MultiLineDictionaryItemModel::resetData(ZAbstractTableModel *m)
{
    if (m != nullptr)
        rowcount = m->rowCount();
    else
        rowcount = 0;

    list.clear();

    if (rowcount != 0)
    {
        list.reserve(rowcount);
        int prev = 0;
        list.push_back(0);
        for (int ix = 1; ix != rowcount; ++ix)
        {
            prev += tosigned(m->data(m->index(ix - 1, 0), (int)DictRowRoles::WordEntry).value<WordEntry*>()->defs.size());
            list.push_back(prev);
        }

        cachepos = tosigned(list.size()) / 2;

        rowcount = list.back() + tosigned(m->data(m->index(rowcount - 1, 0), (int)DictRowRoles::WordEntry).value<WordEntry*>()->defs.size());
    }
}

int MultiLineDictionaryItemModel::definitionIndex(int index) const
{
    if (index < 0 || index >= rowcount)
        return 0;
    cachePosition(index);

    return index - list[cachepos];
}

void MultiLineDictionaryItemModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if (sourceModel() == nullptr || !topLeft.isValid() || !bottomRight.isValid() || topLeft.parent().isValid() || bottomRight.parent().isValid())
        return;

    QAbstractItemModel *m = sourceModel();

    int top = topLeft.row();
    int bottom = bottomRight.row();
    int left = topLeft.column();
    int right = bottomRight.column();

    if (roles.isEmpty() || roles.indexOf((int)DictRowRoles::WordEntry) != -1)
    {
        // Must recalculate number of definitions and emit signals if number of rows change.

        smartvector<Interval> inserted;
        smartvector<Range> removed;

        // There can be both removals and insertions. The list is first updated after removals
        // and the remove signal is sent. The inserted list is filled but it's only signalled
        // below.
        int dif = 0;
        for (int ix = top, siz = tosigned(list.size()); ix != bottom + 1; ++ix)
        {
            WordEntry *e = m->data(m->index(top, 0), (int)DictRowRoles::WordEntry).value<WordEntry*>();
            int oldsiz = (ix == siz - 1 ? rowcount : list[ix + 1]) - list[ix];
            int newsiz = tosigned(e->defs.size());

            list[ix] -= dif;
            if (newsiz < oldsiz)
            {
                removed.push_back({ list[ix] + dif + newsiz, list[ix] + dif + oldsiz - 1 });
                dif += oldsiz - newsiz;
            }
            if (newsiz > oldsiz)
                inserted.push_back({ list[ix] + oldsiz, newsiz - oldsiz });
        }
        for (int ix = bottom + 1, siz = tosigned(list.size()); ix != siz; ++ix)
            list[ix] -= dif;
        rowcount -= dif;

        if (!removed.empty())
            signalRowsRemoved(removed);

        if (!inserted.empty())
        {
            dif = _intervalSize(inserted);
            int inspos = tosigned(inserted.size()) - 1;
            rowcount += dif;
            for (int ix = tosigned(list.size()) - 1; ix != -1 && dif != 0; --ix)
            {
                if (list[ix] < inserted[inspos]->index)
                {
                    dif -= inserted[inspos]->count;
                    --inspos;
                }
                list[ix] += dif;
            }

            signalRowsInserted(inserted);
        }

        //// Difference between original row index and new row index at the row
        //// currently processed.
        //int dif = 0;

        //int pos = list[top];

        //// When data changes it's possible that the number of word definitions of that data
        //// have changed as well. To keep the views updated, data change signals must be
        //// emitted for the updated rows, but also insert and delete row signals.

        //// Top position of the first data which has changed but its data changed signal hasn't
        //// been emited yet. Used for batch signalling data change of word rows, when the
        //// number of definitions stay the same.
        //int changepos = pos;

        //for (; top != list.size(); ++top)
        //{
        //    if (top <= bottom)
        //    {
        //        WordEntry *e = m->data(m->index(top, 0), (int)DictRowRoles::WordEntry).value<WordEntry*>();

        //        int nextpos;
        //        if (top != list.size() - 1)
        //            nextpos = list[top + 1] + dif;
        //        else
        //            nextpos = rowcount + dif;

        //        int oldcnt = nextpos - pos;
        //        int newcnt = e->defs.size();
        //        int sizedif = newcnt - oldcnt;

        //        emit dataChanged(createIndex(changepos, left), createIndex(pos + std::min(newcnt, oldcnt) - 1, right), roles);
        //        if (sizedif != 0)
        //        {
        //            if (sizedif > 0)
        //                beginInsertRows(QModelIndex(), pos + oldcnt, pos + newcnt - 1);
        //            else /* sizedif < 0 */
        //                beginRemoveRows(QModelIndex(), pos + newcnt, pos + oldcnt - 1);
        //        }

        //        dif += sizedif;
        //        nextpos += sizedif;

        //        pos = nextpos;
        //        if (top != list.size() - 1)
        //            list[top + 1] = nextpos;

        //        if (sizedif != 0)
        //        {
        //            changepos = pos;

        //            if (sizedif > 0)
        //                endInsertRows();
        //            else /* sizedif < 0 */
        //                endRemoveRows();
        //        }
        //        continue;
        //    }

        //    if (changepos != pos)
        //    {
        //        emit dataChanged(createIndex(changepos, left), createIndex(pos - 1, right), roles);
        //        changepos = pos;
        //    }

        //    if (dif == 0)
        //        break;

        //    list[top] += dif;
        //}

        //rowcount += dif;

        //return;
    }

    emit dataChanged(createIndex(0/*list[top]*/, left), createIndex(rowcount /*(bottom == rowcount - 1 ? rowcount : list[bottom + 1])*/ - 1, right), roles);
}

void MultiLineDictionaryItemModel::sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    emit headerDataChanged(orientation, list[first], (last != tosigned(list.size()) - 1 ? list[last + 1] : rowcount) - 1);
}

//void MultiLineDictionaryItemModel::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
//{
//    // We don't know how many rows this insertion will result in. Ignore the
//    // signal and try to handle it in sourceRowsInserted().
//}

void MultiLineDictionaryItemModel::sourceRowsInserted(const smartvector<Interval> &intervals/*const QModelIndex &parent, int first, int last*/)
{
    if (intervals.empty())
        return;

    int listsiz = tosigned(list.size());

    // Adding fake item to make the algorithm simpler. It is removed afterwards.
    list.push_back(rowcount);

    smartvector<Interval> inserted;

    ZAbstractTableModel *source = sourceModel();

    int dif = 0;
    int idif = 0;
    for (int ix = 0, siz = tosigned(intervals.size()); ix != siz; ++ix)
    {
        const Interval *i = intervals[ix];

        inserted.push_back({ i->index == listsiz ? rowcount : (list[i->index + idif] - dif), 0 });

        for (int iy = 0; iy != i->count; ++iy)
        {
            WordEntry *e = source->data(source->index(i->index + idif + iy, 0), (int)DictRowRoles::WordEntry).value<WordEntry*>();
            int cnt = tosigned(e->defs.size());

            inserted.back()->count += cnt;
            list.insert(list.begin() + (i->index + idif + iy + 1), list[i->index + idif + iy] + cnt);

            dif += cnt;
        }

        idif += i->count;

        int next = ix == siz - 1 ? tosigned(list.size()) : (intervals[ix + 1]->index + idif + 1);
        for (int iy = i->index + idif + 1; iy != next; ++iy)
            list[iy] += dif;
    }

    rowcount += dif;
    list.pop_back();

    if (!inserted.empty())
        signalRowsInserted(inserted);
    emit selectionWasInserted(intervals);

    //// Get the number of inserted rows and emit signals for them.

    //int top = first;
    //int bottom = last;

    //int where = first != list.size() ? list[first] : rowcount;

    //QAbstractItemModel *m = sourceModel();

    //std::vector<int> inserted;
    //inserted.reserve(last - first + 1);

    //int addpos = where;

    //for (; top != bottom + 1; ++top)
    //{
    //    inserted.push_back(addpos);

    //    WordEntry *e = m->data(m->index(top, 0), (int)DictRowRoles::WordEntry).value<WordEntry*>();
    //    addpos += e->defs.size();
    //}

    //beginInsertRows(QModelIndex(), where, addpos - 1);

    //int cnt = addpos - where;
    //rowcount += cnt;

    //for (top = first; top != list.size(); ++top)
    //    list[top] += cnt;

    //list.insert(list.begin() + first, inserted.begin(), inserted.end());

    //endInsertRows();
}

void MultiLineDictionaryItemModel::sourceColumnsAboutToBeInserted(const QModelIndex &/*parent*/, int start, int end)
{
    beginInsertColumns(QModelIndex(), start, end);
}

void MultiLineDictionaryItemModel::sourceColumnsInserted(const QModelIndex &/*parent*/, int /*first*/, int /*last*/)
{
    endInsertColumns();
}

//void MultiLineDictionaryItemModel::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
//{
//    // Compute the number of visible rows removed and emit the signal.
//    int top = list[first];
//    int bottom = (last != list.size() - 1 ? list[last + 1] : rowcount) - 1;
//
//    beginRemoveRows(QModelIndex(), top, bottom);
//}

void MultiLineDictionaryItemModel::sourceRowsRemoved(const smartvector<Range> &ranges/*const QModelIndex &parent, int first, int last*/)
{
    if (ranges.empty())
        return;

    smartvector<Range> removed;

    int rnextpos = 1;
    const Range *r = ranges.front();
    const Range *rnext = ranges.size() == 1 ? nullptr : ranges[1];

    int f = list[r->first];
    int l = (r->last + 1 < tosigned(list.size()) ? list[r->last + 1] : rowcount) - 1;
    removed.push_back({ list[r->first], (r->last + 1 < tosigned(list.size()) ? list[r->last + 1] : rowcount) - 1 });

    int cnt = l - f + 1;

    for (int ix = r->first, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        if (rnext && ix == rnext->first)
        {
            r = rnext;

            f = list[r->first];
            l = (r->last + 1 < tosigned(list.size()) ? list[r->last + 1] : rowcount) - 1;
            removed.push_back({ f, l });

            cnt += l - f + 1;
            ++rnextpos;
            rnext = tosigned(ranges.size()) <= rnextpos ? nullptr : ranges[rnextpos];
        }
        if (r != nullptr && ix <= r->last)
            list[ix] = -1;
        else
            list[ix] -= cnt;
        continue;
    }

    list.resize(std::remove(list.begin(), list.end(), -1) - list.begin());
    rowcount -= cnt;

    if (!removed.empty())
        signalRowsRemoved(removed);
    emit selectionWasRemoved(ranges);

    //// Update the data once the rows have been removed and emit the signal.

    //int top = list[first];
    //int bottom = (last != list.size() - 1 ? list[last + 1] : rowcount);
    //int cnt = bottom - top;

    //list.erase(list.begin() + first, list.begin() + (last + 1));
    //for (; first != list.size(); ++first)
    //    list[first] -= cnt;

    //rowcount -= cnt;

    //endRemoveRows();
}

void MultiLineDictionaryItemModel::sourceColumnsAboutToBeRemoved(const QModelIndex &/*parent*/, int first, int last)
{
    beginRemoveColumns(QModelIndex(), first, last);
}

void MultiLineDictionaryItemModel::sourceColumnsRemoved(const QModelIndex &/*parent*/, int /*first*/, int /*last*/)
{
    endRemoveColumns();
}

//void MultiLineDictionaryItemModel::sourceRowsAboutToBeMoved(const QModelIndex &srcParent, int first, int last, const QModelIndex &destParent, int destpos)
//{
//    int top = list[first];
//    int bottom = (last != list.size() - 1 ? list[last + 1] : rowcount) - 1;
//
//    int pos = (destpos == list.size() ? rowcount : list[destpos]);
//
//    beginMoveRows(QModelIndex(), top, bottom, QModelIndex(), pos);
//}

void MultiLineDictionaryItemModel::sourceRowsMoved(const smartvector<Range> &ranges, int pos/*const QModelIndex &parent, int first, int last, const QModelIndex &destination, int destpos*/)
{
    if (ranges.empty())
        return;

    smartvector<Range> moved;
    int movepos = pos >= tosigned(list.size()) ? rowcount : list[pos];

    for (int ix = 0, siz = tosigned(ranges.size()); ix != siz; ++ix)
    {
        const Range *r = ranges[ix];
        moved.push_back({ list[r->first], (r->last + 1 == tosigned(list.size()) ? rowcount : list[r->last + 1]) - 1 });
    }

    // Convert list to hold the number of definitions of each source row temporarily.
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        list[ix] = (ix == siz - 1 ? rowcount : list[ix + 1]) - list[ix];

    _moveRanges(ranges, pos, list);

    // Restore list after the move.
    int prev = 0;
    std::swap(prev, list[0]);
    for (int ix = 1, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        std::swap(prev, list[ix]);
        list[ix] += list[ix - 1];
    }

    if (!moved.empty())
        signalRowsMoved(moved, movepos);
    emit selectionWasMoved(ranges, pos);

    //if (destpos > first)
    //{
    //    // Moving rows down has the same result as moving the skipped rows above the first
    //    // moved row. Change the arguments and do the same thing.
    //    int tmpfirst = last + 1;
    //    int tmplast = destpos - 1;

    //    destpos = first;

    //    first = tmpfirst;
    //    last = tmplast;
    //}

    //int top = list[first];
    //int bottom = (last != list.size() - 1 ? list[last + 1] : rowcount);
    //int cnt = bottom - top;

    //int pos = (destpos == list.size() ? rowcount : list[destpos]);

    //std::vector<int> moved{ list.begin() + first, list.begin() + last + 1 };

    //if (moved.empty())
    //{
    //    // In this unlikely event do nothing.
    //    endMoveRows();
    //    return;
    //}

    //list.erase(list.begin() + first, list.begin() + last + 1);

    //int movecnt = moved.front() - pos;
    //for (int &val : moved)
    //    val -= movecnt;

    //for (int ix = destpos; ix != first; ++ix)
    //    list[ix] += cnt;

    //list.insert(list.begin() + destpos, moved.begin(), moved.end());


    //endMoveRows();
}

void MultiLineDictionaryItemModel::sourceColumnsAboutToBeMoved(const QModelIndex &/*sourceParent*/, int sourceStart, int sourceEnd, const QModelIndex &/*destinationParent*/, int destinationColumn)
{
    beginMoveColumns(QModelIndex(), sourceStart, sourceEnd, QModelIndex(), destinationColumn);
}

void MultiLineDictionaryItemModel::sourceColumnsMoved(const QModelIndex &/*parent*/, int /*start*/, int /*end*/, const QModelIndex &/*destination*/, int /*column*/)
{
    endMoveColumns();
}

void MultiLineDictionaryItemModel::sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &/*parents*/, QAbstractItemModel::LayoutChangeHint hint)
{
    emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), hint);

    persistentsource = persistentIndexList();
    persistentdest.clear();
    persistentdest.reserve(persistentsource.size());
    for (const QModelIndex &ind : persistentsource)
    {
        QModelIndex srcind = mapToSource(ind);
        int indpos = ind.row() - mapFromSourceRow(srcind.row());
        persistentdest.push_back(std::make_pair(QPersistentModelIndex(srcind), indpos));
    }
}

void MultiLineDictionaryItemModel::sourceLayoutChanged(const QList<QPersistentModelIndex> &/*parents*/, QAbstractItemModel::LayoutChangeHint hint)
{
    resetData(sourceModel());

    QModelIndexList destindexes;
    destindexes.reserve(persistentsource.size());
    for (const std::pair<QPersistentModelIndex, int> &ind : persistentdest)
    {
        QModelIndex dest;
        // mapFromSourceRow(sourceIndex.row()), sourceIndex.column()
        if (ind.first.isValid() && rowcount != 0)
            dest = createIndex(mapFromSourceRow(ind.first.row()) + ind.second, ind.first.column());
        destindexes.push_back(dest);
    }
    changePersistentIndexList(persistentsource, destindexes);

    emit layoutChanged(QList<QPersistentModelIndex>(), hint);
}

void MultiLineDictionaryItemModel::sourceModelAboutToBeReset()
{
    beginResetModel();
}

void MultiLineDictionaryItemModel::sourceModelReset()
{
    ZAbstractTableModel *m = sourceModel();
    if (m != nullptr)
        resetData(m);
    endResetModel();
}


//-------------------------------------------------------------
