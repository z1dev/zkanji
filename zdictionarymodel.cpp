/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMimeData>
#include <QtEvents>
#include <QColor>
#include <QSet>
//#include "zkanjimain.h"
#include "zdictionarymodel.h"
#include "words.h"
//#include "zstrings.h"
#include "groups.h"
#include "romajizer.h"
#include "zkanjimain.h"
#include "grammar_enums.h"
#include "grammar.h"
#include "furigana.h"
#include "kanji.h"
#include "ranges.h"
#include "dictionarysettings.h"
#include "globalui.h"
#include "colorsettings.h"
#include "generalsettings.h"


//-------------------------------------------------------------


DictionaryItemModel::DictionaryItemModel(QObject *parent) : base(parent), connected(false)
{
    setColumns({ 
        { (int)DictColumnTypes::Frequency, Qt::AlignHCenter, ColumnAutoSize::Stretched, false, -1, QString() },
#if SHOWDEBUGCOLUMN
        { (int)ColumnTypes::DEBUGIndex, Qt::AlignLeft, false, true, 61, QString() },
#endif
        { (int)DictColumnTypes::Kanji, Qt::AlignLeft, ColumnAutoSize::Auto, true, Settings::scaled(50), tr("Written") },
        { (int)DictColumnTypes::Kana, Qt::AlignLeft, ColumnAutoSize::Auto, true, Settings::scaled(70), tr("Kana") },
        { (int)DictColumnTypes::Definition, Qt::AlignLeft, ColumnAutoSize::NoAuto, false, Settings::scaled(6400), tr("Definition") }
    });

    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &DictionaryItemModel::dictionaryToBeRemoved);
}

DictionaryItemModel::~DictionaryItemModel()
{
}

WordGroup* DictionaryItemModel::wordGroup() const
{
    return nullptr;
}

WordEntry* DictionaryItemModel::items(int index) const
{
    int ix = -1;

    if (dictionary() == nullptr || (ix = indexes(index)) == -1)
        return nullptr;

    return dictionary()->wordEntry(ix);
}

void DictionaryItemModel::insertColumn(int index, const DictColumnData &column)
{
    auto tmp = std::move(cols);
    cols.resize(tmp.size() + 1);
    cols[index] = column;

    for (int ix = 0; ix != tmp.size(); ++ix)
        cols[ix + (ix >= index ? 1 : 0)] = tmp[ix];
}

void DictionaryItemModel::setColumn(int index, const DictColumnData &column)
{
    cols[index] = column;
}

void DictionaryItemModel::setColumns(const std::initializer_list<DictColumnData> &columns)
{
    cols.resize(columns.size());
    int ix = 0;
    for (auto c : columns)
        cols[ix++] = c;
}

void DictionaryItemModel::setColumns(const std::vector<DictColumnData> &columns)
{
    cols.resize(columns.size());
    int ix = 0;
    for (auto c : columns)
        cols[ix++] = c;
}

void DictionaryItemModel::addColumn(const DictColumnData &column)
{
    cols.resize(cols.size() + 1);
    cols[cols.size() - 1] = column;
}

void DictionaryItemModel::removeColumn(int index)
{
    auto tmp = std::move(cols);
    cols.resize(tmp.size() - 1);
    for (int ix = 0; ix != tmp.size() - 1; ++ix)
        cols[ix] = tmp[ix + (ix >= index ? 1 : 0)];
}

int DictionaryItemModel::columnByType(int columntype, int n)
{
    for (int ix = 0; ix != cols.size(); ++ix)
        if (cols[ix].type == columntype)
        {
            if (n == 0)
                return ix;
            --n;
        }
    return -1;
}

//bool DictionaryItemModel::canDragEnter(QDragEnterEvent *e, bool samesource) const
//{
//    return false;
//}

int DictionaryItemModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return cols.size();
}

QVariant DictionaryItemModel::data(const QModelIndex &index, int role) const
{
    if (role == (int)DictColumnRoles::Type)
        return headerData(index.column(), Qt::Horizontal, role);

    if (role == (int)DictRowRoles::WordEntry)
    {
        QVariant result;
        result.setValue(items(index.row()));
        return result;
    }

    if (role == (int)DictRowRoles::WordIndex)
        return indexes(index.row());

    // The basic dictionary model doesn't list the definitions separately, and has to return
    // -1 to indicate this.
    if (role == (int)DictRowRoles::DefIndex)
        return -1;

    if (role != Qt::DisplayRole)
        return QVariant(); // base::data(index, role);

    WordEntry *w = items(index.row());

    if (w == nullptr)
        return QVariant();

    switch (headerData(index.column(), Qt::Horizontal, (int)DictColumnRoles::Type).toInt())
    {
    case (int)DictColumnTypes::Frequency:
        return w->freq;
#if SHOWDEBUGCOLUMN
    case ColumnTypes::DEBUGIndex:
        return indexes(index.row());
#endif
    case (int)DictColumnTypes::Kanji:
        return QString(w->kanji.toQStringRaw());
    case (int)DictColumnTypes::Kana:
        return QString(w->kana.toQStringRaw());
    case (int)DictColumnTypes::Definition:
    {
        //QString def;
        //for (int ix = 0; ix < w->defcnt; ++ix)
        //{
        //    if (w->defcnt != 1)
        //        def += QStringLiteral("%1. ").arg(ix + 1);
        //    def += QString(w->defs[ix].def) + "  ";
        //}

        //QVariant result;
        //if (rowCount() > index.row())
        //    result.setValue(DictionaryItemDefinition(dictionary(), indexes(index.row())));
        //return result;
    }
    default:
        return QVariant();
    }

}

QMap<int, QVariant> DictionaryItemModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles = base::itemData(index);
    QVariant variantData = data(index, (int)DictColumnRoles::Type);
    if (variantData.isValid())
        roles.insert((int)DictColumnRoles::Type, variantData);
    variantData = data(index, (int)DictRowRoles::WordEntry);
    if (variantData.isValid())
        roles.insert((int)DictRowRoles::WordEntry, variantData);
    variantData = data(index, (int)DictRowRoles::WordIndex);
    if (variantData.isValid())
        roles.insert((int)DictRowRoles::WordIndex, variantData);
    variantData = data(index, (int)DictRowRoles::DefIndex);
    if (variantData.isValid())
        roles.insert((int)DictRowRoles::DefIndex, variantData);

    return roles;
}

QVariant DictionaryItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::TextAlignmentRole && role != (int)ColumnRoles::DefaultWidth && role != (int)ColumnRoles::AutoSized && role != (int)ColumnRoles::FreeSized && role != (int)DictColumnRoles::Type))
        return base::headerData(section, orientation, role);

    if (role == (int)DictColumnRoles::Type)
    {
        QVariant result;
        if (section >= 0 && section < cols.size())
            result.setValue(cols[section].type);
        else
            result.setValue(-1);
        return result;
    }

    if (role == Qt::TextAlignmentRole)
    {
        QVariant result;
        if (section >= 0 && section < cols.size())
            result.setValue((int)cols[section].align);
        else
            result.setValue((int)Qt::AlignHCenter);

        return result;
    }

    if (role == (int)ColumnRoles::AutoSized)
    {
        if (section >= 0 && section < cols.size())
            return (int)cols[section].autosize;
        return (int)ColumnAutoSize::NoAuto;
    }

    if (role == (int)ColumnRoles::FreeSized)
    {
        if (section >= 0 && section < cols.size())
            return cols[section].sizable;
        return false;
    }

    if (role == (int)ColumnRoles::DefaultWidth)
    {
        // Returns the expected size of a specific column. The height of the returned value is
        // not valid. If a column is auto sized, and the view handles auto sizing, the value
        // is ignored. Negative value means that the width is (-value * row height), while 0
        // means the width defaults to the width of the header text.
        if (section >= 0 && section < cols.size())
            return cols[section].width;
        return 30;
    }

    if (section >= 0 && section < cols.size())
        return cols[section].text;
    return QVariant();
}

QMimeData* DictionaryItemModel::mimeData(const QModelIndexList& ind) const
{
    QMimeData *data = new QMimeData();

    QByteArray arr;
    arr.resize(sizeof(int) * ind.size() + sizeof(intptr_t) * 2);

    // Format of zkanji/words mime data:
    // Pointer to dictionary. intptr_t
    // Pointer to word group. intptr_t
    // Array of word indexes. There's no count just the values. int each.

    *(intptr_t*)arr.data() = (intptr_t)dictionary();
    *((intptr_t*)arr.data() + 1) = (intptr_t)wordGroup();

    int *dat = (int*)(arr.data() + sizeof(intptr_t) * 2);

    // The values after the word group are the word indexes in the group's dictionary.
    for (auto it = ind.cbegin(); it != ind.cend(); ++it)
    {
        *dat = indexes(it->row());
        ++dat;
    }

    data->setData("zkanji/words", arr);

    return data;
}

//QStringList DictionaryItemModel::mimeTypes() const
//{
//    QStringList types;
//    types << "zkanji/words";
//
//    return types;
//}

void DictionaryItemModel::connect()
{
#ifdef _DEBUG
    if (connected)
        throw "double connect";
#endif
    if (connected || dictionary() == nullptr)
        return;

    connect(dictionary(), &Dictionary::entryChanged, this, &DictionaryItemModel::entryChanged);
    //connect(dictionary(), &Dictionary::entryAboutToBeRemoved, this, &DictionaryItemModel::entryAboutToBeRemoved);
    connect(dictionary(), &Dictionary::entryRemoved, this, &DictionaryItemModel::entryRemoved);
    connect(dictionary(), &Dictionary::entryAdded, this, &DictionaryItemModel::entryAdded);

    connected = true;
}

void DictionaryItemModel::disconnect()
{
    if (!connected || dictionary() == nullptr)
        return;

    disconnect(dictionary(), nullptr, this, nullptr);

    connected = false;
}

void DictionaryItemModel::dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict)
{
    if (dict == dictionary())
        disconnect();
}

//void DictionaryItemModel::dictionaryReplaced(int ind, int ordind, void *olddict, Dictionary *dict)
//{
//    if (connected && olddict == dictionary())
//    {
//        connected = false;
//        connect();
//    }
//}


//-------------------------------------------------------------


DictionaryWordListItemModel::DictionaryWordListItemModel(QObject *parent) : base(parent), dict(nullptr)
{

}

DictionaryWordListItemModel::~DictionaryWordListItemModel()
{
}

void DictionaryWordListItemModel::setWordList(Dictionary *d, const std::vector<int> &wordlist)
{
    beginResetModel();
    if (dict != nullptr)
        disconnect();

    dict = d;
    list = wordlist;

    if (dict != nullptr)
        connect();

    endResetModel();
}

void DictionaryWordListItemModel::setWordList(Dictionary *d, std::vector<int> &&wordlist)
{
    beginResetModel();
    if (dict != nullptr)
        disconnect();

    dict = d;
    list.swap(wordlist);

    if (dict != nullptr)
        connect();

    endResetModel();
}

const std::vector<int>& DictionaryWordListItemModel::getWordList() const
{
    return list;
}

Dictionary* DictionaryWordListItemModel::dictionary() const
{
    return dict;
}

int DictionaryWordListItemModel::indexes(int pos) const
{
    if (list.empty())
        return -1;
    return list[pos];
}

int DictionaryWordListItemModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    if (list.empty())
        return 0;
    return list.size();
}

Qt::DropActions DictionaryWordListItemModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

Qt::DropActions DictionaryWordListItemModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    return 0;
}

//void DictionaryWordListItemModel::entryAboutToBeRemoved(int windex)
//{
//    for (int ix = 0; ix != list.size(); ++ix)
//        if (list[ix] == windex)
//        {
//            beginRemoveRows(QModelIndex(), ix, ix);
//            break;
//        }
//}

void DictionaryWordListItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    int wpos = -1;
    for (int ix = 0; ix != list.size(); ++ix)
    {
        if (list[ix] == windex)
            wpos = ix;
        else if (list[ix] > windex)
            --list[ix];
    }

    if (wpos != -1)
    {
        list.erase(list.begin() + wpos);
        signalRowsRemoved({ { wpos, wpos } }); //endRemoveRows();
    }
}

void DictionaryWordListItemModel::entryChanged(int windex, bool studydef)
{
    for (int ix = 0; ix != list.size(); ++ix)
        if (list[ix] == windex)
        {
            emit dataChanged(index(ix, 0), index(ix, columnCount() - 1));
            break;
        }
}

void DictionaryWordListItemModel::entryAdded(int windex)
{

}


//-------------------------------------------------------------


DictionaryDefinitionListItemModel::DictionaryDefinitionListItemModel(QObject *parent) : base(parent), dict(nullptr), index(-1)
{
}

DictionaryDefinitionListItemModel::~DictionaryDefinitionListItemModel()
{
}

void DictionaryDefinitionListItemModel::setWord(Dictionary *d, int windex)
{
    if (dict == d && index == windex)
        return;

    dict = d;
    index = windex;
}

int DictionaryDefinitionListItemModel::getWord()
{
    return index;
}

Dictionary* DictionaryDefinitionListItemModel::dictionary() const
{
    return dict;
}

int DictionaryDefinitionListItemModel::indexes(int pos) const
{
    return index;
}

int DictionaryDefinitionListItemModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() || index == -1 ? 0 : dict->wordEntry(index)->defs.size();
}

QVariant DictionaryDefinitionListItemModel::data(const QModelIndex &index, int role) const
{
    if (role == (int)DictRowRoles::DefIndex)
        return index.row();

    return base::data(index, role);
}

QMap<int, QVariant> DictionaryDefinitionListItemModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> result = base::itemData(index);
    result.insert((int)DictRowRoles::DefIndex, QVariant(index.row()));
    return result;
}

void DictionaryDefinitionListItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    if (windex != index)
        return;
    beginResetModel();
    dict = nullptr;
    index = -1;
    endResetModel();
}

void DictionaryDefinitionListItemModel::entryChanged(int windex, bool studydef)
{
    if (windex != index || studydef)
        return;

    beginResetModel();
    endResetModel();
}

void DictionaryDefinitionListItemModel::entryAdded(int windex)
{
}


//-------------------------------------------------------------


DictionarySearchResultItemModel::DictionarySearchResultItemModel(QObject *parent) : base(parent), sdict(nullptr)
{
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterMoved, this, &DictionarySearchResultItemModel::filterMoved);
}

DictionarySearchResultItemModel::~DictionarySearchResultItemModel()
{

}

void DictionarySearchResultItemModel::search(SearchMode mode, Dictionary *dict, QString searchstr, SearchWildcards wildcards, bool strict, bool inflections, bool studydefs, WordFilterConditions *cond)
{
    if (dict == nullptr || (sdict == dict && smode == mode && swildcards == wildcards && sstrict == strict && sinflections == inflections && sstudydefs == studydefs && ((!scond && !cond) || (!scond == !cond && *scond == *cond)) && ssearchstr == searchstr))
        return;

    if (dict != sdict && sdict != nullptr)
        disconnect();

    smode = mode;

    if (dict != sdict)
    {
        sdict = dict;
        if (sdict != nullptr)
            connect();
    }

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

    list.reset(new WordResultList(dict));

    beginResetModel();
    sdict->findWords(std::move(*list), mode, searchstr, wildcards, strict, inflections, studydefs, nullptr, cond);
    endResetModel();

}

void DictionarySearchResultItemModel::resetFilterConditions()
{
    scond.reset();
}

Dictionary* DictionarySearchResultItemModel::dictionary() const
{
    return sdict;
}

int DictionarySearchResultItemModel::indexes(int pos) const
{
    return list->getIndexes()[pos];
}

int DictionarySearchResultItemModel::rowCount(const QModelIndex &parent) const
{
    if (!list)
        return 0;
    return list->size();
}

QVariant DictionarySearchResultItemModel::data(const QModelIndex &index, int role) const
{
    if (role == (int)DictRowRoles::Inflection)
    {
        auto &inflist = list->getInflections();
        if (inflist.size() <= index.row())
            return 0;
        return QVariant::fromValue((intptr_t)inflist[index.row()]);
    }

    return base::data(index, role);
}

QMap<int, QVariant> DictionarySearchResultItemModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles = base::itemData(index);
    QVariant variantData = data(index, (int)DictRowRoles::Inflection);
    if (variantData.isValid())
        roles.insert((int)DictRowRoles::Inflection, variantData);
    return roles;
}

Qt::DropActions DictionarySearchResultItemModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

Qt::DropActions DictionarySearchResultItemModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    return 0;
}

//void DictionarySearchResultItemModel::entryAboutToBeRemoved(int windex)
//{
//    auto &ind = list->getIndexes();
//    for (int ix = 0; ix != list->size(); ++ix)
//        if (ind[ix] == windex)
//        {
//            beginRemoveRows(QModelIndex(), ix, ix);
//            break;
//        }
//}

void DictionarySearchResultItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    auto &ind = list->getIndexes();
    int wpos = -1;
    for (int ix = 0; ix != list->size(); ++ix)
        if (ind[ix] == windex)
            wpos = ix;
        else if (ind[ix] > windex)
            --ind[ix];

    if (wpos != -1)
    {
        list->removeAt(wpos);
        //endRemoveRows();
        signalRowsRemoved({ { wpos, wpos } });
    }
}

void DictionarySearchResultItemModel::entryChanged(int windex, bool studydef)
{
    if (studydef)
        return;

    std::vector<InfTypes> infs;
    bool match = sdict->wordMatches(windex, smode, ssearchstr, swildcards, sstrict, sinflections, sstudydefs, scond.get(), &infs);

    if (!match)
    {
        auto &ind = list->getIndexes();
        int wpos = -1;
        for (int ix = 0; ix != list->size(); ++ix)
            if (ind[ix] == windex)
            {
                wpos = ix;
                break;
            }

        if (wpos != -1)
        {
            //beginRemoveRows(QModelIndex(), wpos, wpos);
            list->removeAt(wpos);
            //endRemoveRows();
            signalRowsRemoved({ { wpos, wpos } });
        }
        return;
    }

    int oldpos = -1, newpos = -1;
    if (smode == SearchMode::Japanese)
        newpos = list->jpInsertPos(windex, infs, &oldpos);

    if (smode == SearchMode::Definition)
        newpos = list->defInsertPos(ssearchstr, windex, &oldpos);

    if (newpos == oldpos)
    {
        emit dataChanged(index(newpos, 0), index(newpos, columnCount() - 1));
        return;
    }

    int movepos = newpos;
    if (movepos > oldpos && oldpos != -1)
        ++movepos;

    //beginMoveRows(QModelIndex(), oldpos, oldpos, QModelIndex(), movepos);
    if (oldpos != -1)
        list->removeAt(oldpos);
    list->insert(newpos, windex, infs);
    //endMoveRows();
    if (oldpos != -1)
        signalRowsMoved({ { oldpos, oldpos } }, movepos);
    else
        signalRowsInserted({ { newpos, 1 } });
}

void DictionarySearchResultItemModel::filterMoved(int index, int to)
{
    if (!scond)
        return;

    std::vector<Inclusion> &inc = scond->inclusions;
    if (index >= inc.size() && to >= inc.size())
        return;

    if (to > index)
        --to;

    Inclusion moved = index >= inc.size() ? Inclusion::Ignore : inc[index];
    if (index < inc.size())
        inc.erase(inc.begin() + index);

    if (to >= inc.size())
    {
        if (moved == Inclusion::Ignore)
            return;
        while (inc.size() <= to)
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

void DictionarySearchResultItemModel::entryAdded(int windex)
{
    std::vector<InfTypes> infs;
    bool match = sdict->wordMatches(windex, smode, ssearchstr, swildcards, sstrict, sinflections, sstudydefs, scond.get(), &infs);

    if (!match)
        return;

    int newpos = -1;
    if (smode == SearchMode::Japanese)
        newpos = list->jpInsertPos(windex, infs, nullptr);

    if (smode == SearchMode::Definition)
        newpos = list->defInsertPos(ssearchstr, windex, nullptr);

    //beginInsertRows(QModelIndex(), newpos, newpos);
    list->insert(newpos, windex, infs);
    //endInsertRows();
    signalRowsInserted({ { newpos, 1 } });
}


//-------------------------------------------------------------


DictionaryBrowseItemModel::DictionaryBrowseItemModel(Dictionary *dict, BrowseOrder order, QObject *parent) : base(parent), dict(dict), order(order)
{
    connect();
}

DictionaryBrowseItemModel::~DictionaryBrowseItemModel()
{

}

void DictionaryBrowseItemModel::setFilterConditions(WordFilterConditions *fcond)
{
    if (fcond == nullptr || !*fcond)
    {
        if (!cond)
            return;

        if (!*cond)
        {
            cond.reset();
            return;
        }

        beginResetModel();
        cond.reset();
        list.clear();
        endResetModel();
        return;
    }

    if (cond && *fcond == *cond)
        return;

    if (cond)
    {
        // If the passed filter is only different in the ignored items, save it and return.
        if (cond->examples == fcond->examples && cond->groups == fcond->groups)
        {
            int fsiz = fcond->inclusions.size();
            int csiz = cond->inclusions.size();
            
            bool different = false;

            if (fsiz > csiz)
            {
                for (int ix = fsiz - 1; !different && ix != csiz - 1; --ix)
                    if (fcond->inclusions[ix] != Inclusion::Ignore)
                        different = true;
            }
            else if (csiz > fsiz)
            {
                for (int ix = csiz - 1; !different && ix != fsiz - 1; --ix)
                    if (cond->inclusions[ix] != Inclusion::Ignore)
                        different = true;
            }

            if (!different)
            {
                for (int ix = std::min(csiz, fsiz) - 1; !different && ix != -1; --ix)
                    if (cond->inclusions[ix] != fcond->inclusions[ix])
                        different = true;

                if (!different)
                {
                    if (csiz != fsiz)
                        cond->inclusions = fcond->inclusions;

                    return;
                }
            }

        }
    }

    if (!cond)
        cond.reset(new WordFilterConditions);

    *cond = *fcond;
    
    // The dictionary order list must be filtered from scratch.

    beginResetModel();
    list.clear();
    const std::vector<int> &wordlist = dict->wordOrdering(order);

    for (int ix = 0; ix != wordlist.size(); ++ix)
    {
        int wix = wordlist[ix];
        if (ZKanji::wordfilters().match(dict->wordEntry(wix), cond.get()))
            list.push_back(wix);
    }
    endResetModel();
}

BrowseOrder DictionaryBrowseItemModel::browseOrder() const
{
    return order;
}

int DictionaryBrowseItemModel::browseRow(QString searchstr) const
{
    // Update entryAdded() when code changes.

    if (order == BrowseOrder::AIUEO)
    {
        searchstr = hiraganize(searchstr);
        // Remove non-kana.
        for (int ix = searchstr.size() - 1; ix != -1; --ix)
            if (!KANA(searchstr.at(ix).unicode()))
                searchstr[ix] = QChar('*');
        searchstr.remove(QChar('*'));
    }
    else if (order == BrowseOrder::ABCDE)
        searchstr = romanize(searchstr.constData());
    
    auto orderfunc = dict->browseOrderCompareFunc(order);

    const std::vector<int> *wordlist;
    if (!cond)
        wordlist = &dict->wordOrdering(order);
    else
        wordlist = &list;

    auto it = std::lower_bound(wordlist->begin(), wordlist->end(), searchstr.constData(), orderfunc);

    if (it == wordlist->end())
        return wordlist->size() - 1;
    return it - wordlist->begin();
}

int DictionaryBrowseItemModel::browseRow(int windex) const
{
    // Update entryAdded() when code changes.


    WordEntry *e = dict->wordEntry(windex);

    QString searchstr;
    if (order == BrowseOrder::AIUEO)
        searchstr = hiraganize(e->kana.toQString());
    else
        searchstr = e->romaji.toQString();

    int r = browseRow(searchstr);

    if (r == -1)
    {
        if (cond)
        {
            // Error in the normal browse function. To make this still work, we have to go over
            // every word listed by the model and find windex with brute force.

            for (int ix = 0; ix != list.size(); ++ix)
                if (list[ix] == windex)
                {
                    r = ix;
                    break;
                }
        }

        return r;
    }

    // What we found is only the first word with a similar kana form. The real word must be
    // somewhere near.

    const std::vector<int> *wordlist;

    if (!cond)
        wordlist = &dict->wordOrdering(order);
    else
        wordlist = &list;

    auto orderfunc = dict->browseOrderCompareIndexFunc(order);

    for (int ix = r, siz = wordlist->size(); ix != siz; ++ix)
    {
        if (wordlist->operator[](ix) == windex)
        {
            r = ix;
            break;
        }

        // To avoid checking the whole dictionary after the first result, the search can stop
        // if the current word is already higher in the order than the word at windex.
        if (orderfunc(windex, wordlist->operator[](ix), searchstr.constData(), nullptr))
            break;
    }

    return r;
}

Dictionary* DictionaryBrowseItemModel::dictionary() const
{
    return dict;
}

int DictionaryBrowseItemModel::indexes(int pos) const
{
    if (!cond)
        return dict->wordOrdering(order)[pos];

    return list[pos];
}

int DictionaryBrowseItemModel::rowCount(const QModelIndex &parent) const
{
    if (!cond)
        return dict->entryCount();

    return list.size();
}

Qt::DropActions DictionaryBrowseItemModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

Qt::DropActions DictionaryBrowseItemModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    return 0;
}

//void DictionaryBrowseItemModel::entryAboutToBeRemoved(int windex)
//{
//    QModelIndex bix = browseIndex(windex);
//
//    int wpos = bix.isValid() ? bix.row() : -1;
//
//    if (wpos == -1)
//    {
//        const std::vector<int> *wordlist;
//
//        if (!cond || !*cond)
//            wordlist = &dict->wordOrdering(order);
//        else
//            wordlist = &list;
//
//        for (int ix = 0; ix != wordlist->size(); ++ix)
//            if (wordlist->operator[](ix) == windex)
//            {
//                wpos = ix;
//                break;
//            }
//    }
//
//    if (wpos != -1)
//        beginRemoveRows(QModelIndex(), wpos, wpos);
//}

void DictionaryBrowseItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    int wpos = -1;
    if (cond)
    {
        for (int ix = 0, siz = list.size(); ix != siz; ++ix)
            if (list[ix] > windex)
                --list[ix];
            else if (list[ix] == windex)
                wpos = ix;

        if (wpos != -1)
            list.erase(list.begin() + wpos);
    }
    else
    {
        if (order == BrowseOrder::ABCDE)
            wpos = abcdeindex;
        else
            wpos = aiueoindex;
    }

    if (wpos != -1)
        signalRowsRemoved({ { wpos, wpos } });
}

void DictionaryBrowseItemModel::entryChanged(int windex, bool studydef)
{
    if (studydef)
        return;

    int brow = browseRow(windex);

    int wpos = brow;

    if (wpos == -1)
        return;

    emit dataChanged(index(wpos, 0), index(wpos, columnCount() - 1));
}

void DictionaryBrowseItemModel::entryAdded(int windex)
{
    if (!cond)
    {
        //beginInsertRows(QModelIndex(), ix, ix);
        //endInsertRows();
        signalRowsInserted({ { browseRow(windex), 1 } });
        return;
    }

    if (!ZKanji::wordfilters().match(dict->wordEntry(windex), cond.get()))
        return;

    // The entry must be inserted at the same position it is in the source dictionary. Find
    // the entry's position both in the dictionary and its future place in the model.

    // These lines are copied from browseIndex().

    WordEntry *e = dict->wordEntry(windex);

    QString searchstr;
    if (order == BrowseOrder::AIUEO)
        searchstr = hiraganize(e->kana.toQString());
    else
        searchstr = e->romaji.toQString();

    auto orderfunc = dict->browseOrderCompareFunc(order);

    const std::vector<int> &wordlist = dict->wordOrdering(order);

    auto it = std::lower_bound(wordlist.begin(), wordlist.end(), searchstr.constData(), orderfunc);

#ifdef _DEBUG
    if (it == wordlist.end())
        throw "Word should be in the dictionary already.";
#endif

    // The position of the first word found in the ordered word list of the dictionary that
    // matches the kana form of the searched word.
    int dictpos = it - wordlist.begin();

    it = std::lower_bound(list.begin(), list.end(), searchstr.constData(), orderfunc);

    // The position of the first word found in the ordered word list of the model that matches
    // the kana form of the searched word. After stepping through all the words before windex
    // in the dictionary, this will be the insert position of windex in list.
    int inspos = it - list.begin();

    if (it != list.end())
    {
        // Find windex in the dictionary's ordered list by skipping those in front of it, and
        // skip the same words in list too.

        auto orderixfunc = dict->browseOrderCompareIndexFunc(order);

        while (true)
        {
            while (inspos != list.size() && orderixfunc(list[inspos], wordlist[dictpos], nullptr, nullptr))
                ++inspos;

            if (inspos == list.size() || wordlist[dictpos] == windex)
                break;

            while (wordlist[dictpos] != windex && orderixfunc(wordlist[dictpos], list[inspos], nullptr, nullptr))
                ++dictpos;
        }
    }

    //beginInsertRows(QModelIndex(), inspos, inspos);
    list.insert(list.begin() + inspos, windex); //push_back(windex);
    //endInsertRows();
    signalRowsInserted({ { inspos, 1 } });
}


//-------------------------------------------------------------


DictionaryEntryItemModel::DictionaryEntryItemModel(Dictionary *dict, WordEntry *entry, QObject *parent) : base(parent), dict(dict), entry(entry)
{
}

DictionaryEntryItemModel::~DictionaryEntryItemModel()
{
}

Dictionary* DictionaryEntryItemModel::dictionary() const
{
    return dict;
}

int DictionaryEntryItemModel::indexes(int pos) const
{
    return -1;
}

WordEntry* DictionaryEntryItemModel::items(int pos) const
{
    if (pos <= entry->defs.size())
        return entry;

    return nullptr;
}

int DictionaryEntryItemModel::rowCount(const QModelIndex &parent) const
{
    return entry == nullptr ? 0 : entry->defs.size() + 1;
}

Qt::DropActions DictionaryEntryItemModel::supportedDragActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

Qt::DropActions DictionaryEntryItemModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
//    return samesource && e->mimeData()->hasFormat("zkanji/editdef") && e->mimeData()->data(QStringLiteral("zkanji/editdef")).size() == sizeof(int);

    if (samesource && mime->hasFormat("zkanji/editdef"))
        return Qt::MoveAction | Qt::CopyAction;
    return 0;
}

void DictionaryEntryItemModel::entryChanged()
{
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void DictionaryEntryItemModel::changeDefinition(int ix, QString defstr, const WordDefAttrib &defattrib)
{
#ifdef _DEBUG
    if (ix > entry->defs.size())
        throw "Can't edit definition outside range.";
#endif
    
    if (ix == entry->defs.size())
    {
        //beginInsertRows(QModelIndex(), entry->defs.size() + 1, entry->defs.size() + 1);
        entry->defs.resize(entry->defs.size() + 1);
        //endInsertRows();
        signalRowsInserted({ { entry->defs.size(), 1 } });
    }

    entry->defs[ix].def.copy(defstr.constData());
    entry->defs[ix].attrib = defattrib;
    emit dataChanged(index(ix, 0), index(ix, columnCount() - 1));
}

void DictionaryEntryItemModel::deleteDefinition(int ix)
{
#ifdef _DEBUG
    if (ix >= entry->defs.size())
        throw "Can't delete definition outside range.";
#endif

    //beginRemoveRows(QModelIndex(), ix, ix);
    entry->defs.erase(ix);
    //endRemoveRows();
    signalRowsRemoved({ { ix, ix } });
}

void DictionaryEntryItemModel::cloneDefinition(int ix)
{
    //beginInsertRows(QModelIndex(), entry->defs.size(), entry->defs.size());
    int row = entry->defs.size();
    entry->defs.resize(row + 1);
    entry->defs[row] = entry->defs[ix];
    //endInsertRows();
    signalRowsInserted({ { row, 1 } });
}

void DictionaryEntryItemModel::copyFromEntry(WordEntry *src)
{
    beginResetModel();
    ZKanji::cloneWordData(entry, src, true);
    endResetModel();
}

QVariant DictionaryEntryItemModel::data(const QModelIndex &index, int role) const
{
    if (role == (int)DictRowRoles::DefIndex)
        return index.row();

    return base::data(index, role);
}

QMap<int, QVariant> DictionaryEntryItemModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> result = base::itemData(index);
    result.insert((int)DictRowRoles::DefIndex, QVariant(index.row()));
    return result;
}

//bool DictionaryEntryItemModel::canDragEnter(QDragEnterEvent *e, bool samesource) const
//{
//    return samesource && e->mimeData()->hasFormat("zkanji/editdef") && e->mimeData()->data(QStringLiteral("zkanji/editdef")).size() == sizeof(int);
//}

bool DictionaryEntryItemModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    return row != -1 && column != -1 && entry != nullptr && row <= entry->defs.size() &&
            data->hasFormat(QStringLiteral("zkanji/editdef")) &&
            data->data(QStringLiteral("zkanji/editdef")).size() == sizeof(int);
}

bool DictionaryEntryItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    int defix = *(int*)data->data(QStringLiteral("zkanji/editdef")).data();

    auto defcopy = entry->defs[defix];

    int destrow = row;
    if (action == Qt::MoveAction)
    {
        //beginMoveRows(QModelIndex(), defix, defix, QModelIndex(), row);
        entry->defs.erase(defix);

        if (row > defix)
            --row;
    }

    //if (action == Qt::CopyAction)
    //    beginInsertRows(QModelIndex(), row, row);
    entry->defs.insert(row, defcopy);
    if (action == Qt::CopyAction)
        //endInsertRows();
        signalRowsInserted({ { row, 1 } });
    else
        //endMoveRows();
        signalRowsMoved({ { defix, defix } }, destrow);

    return true;
}

QMimeData* DictionaryEntryItemModel::mimeData(const QModelIndexList& indexes) const
{
#ifdef _DEBUG
    if (indexes.size() != 1)
        throw "It's invalid to drag more than one definition.";
#endif

    QMimeData *data = new QMimeData();

    QByteArray arr;
    arr.resize(sizeof(int));
    *(int*)(arr.data()) = indexes.at(0).row();

    data->setData(QStringLiteral("zkanji/editdef"), arr);

    return data;
}

//QStringList DictionaryEntryItemModel::mimeTypes() const
//{
//    QStringList types;
//    types << QStringLiteral("zkanji/editdef");
//
//    return types;
//}


//-------------------------------------------------------------


DictionaryGroupItemModel::DictionaryGroupItemModel(QObject *parent) : base(parent), group(nullptr)
{
}

DictionaryGroupItemModel::~DictionaryGroupItemModel()
{

}

void DictionaryGroupItemModel::setWordGroup(WordGroup *newgroup)
{
    if (group == newgroup)
        return;

    if (group != nullptr)
    {
        disconnect(&group->dictionary()->wordGroups(), nullptr, this, nullptr);
        disconnect();
    }

    beginResetModel();
    group = newgroup;

    if (newgroup != nullptr)
    {
        connect(&group->dictionary()->wordGroups(), &WordGroups::itemsInserted, this, &DictionaryGroupItemModel::itemsInserted);
        //connect(&group->dictionary()->wordGroups(), &WordGroups::beginItemsRemove, this, &DictionaryGroupItemModel::beginItemsRemove);
        connect(&group->dictionary()->wordGroups(), &WordGroups::itemsRemoved, this, &DictionaryGroupItemModel::itemsRemoved);
        //connect(&group->dictionary()->wordGroups(), &WordGroups::beginItemsMove, this, &DictionaryGroupItemModel::beginItemsMove);
        connect(&group->dictionary()->wordGroups(), &WordGroups::itemsMoved, this, &DictionaryGroupItemModel::itemsMoved);

        connect();
        connect(group->dictionary(), &Dictionary::dictionaryReset, this, &DictionaryGroupItemModel::dictionaryReset);
    }

    endResetModel();
}

WordGroup* DictionaryGroupItemModel::wordGroup() const
{
    return group;
}

Dictionary* DictionaryGroupItemModel::dictionary() const
{
    if (group == nullptr)
        return nullptr;
    return group->dictionary();
}

int DictionaryGroupItemModel::indexes(int pos) const
{
    if (group == nullptr)
        return -1;
    return group->getIndexes()[pos];
}

//void DictionaryGroupItemModel::wordToGroup(int windex)
//{
//    if (group == nullptr)
//        throw "Programmer is out of his mind.";
//
//    beginInsertRows(QModelIndex(), group->size(), group->size());
//    group->add(windex);
//    endInsertRows();
//}

int DictionaryGroupItemModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() || group == nullptr)
        return 0;

    return group->size();
}

Qt::DropActions DictionaryGroupItemModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions DictionaryGroupItemModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (group == nullptr)
        return 0;

    QByteArray arr;
    
    if (mime->hasFormat("zkanji/words") &&
        (((arr = mime->data("zkanji/words")).size() - sizeof(intptr_t) * 2) % sizeof(int)) == 0 &&
        *(intptr_t*)arr.constData() == (intptr_t)group->dictionary() &&
        (*((intptr_t*)arr.constData() + 1) == 0 || samesource || (intptr_t)group != *((intptr_t*)arr.constData() + 1)))

    //if (mime->hasFormat(QStringLiteral("zkanji/words")))
        return Qt::CopyAction | Qt::MoveAction;
    return 0;
}

//bool DictionaryGroupItemModel::canDragEnter(QDragEnterEvent *e, bool samesource) const
//{
//    QByteArray arr;
//
//    return e->mimeData()->hasFormat(QStringLiteral("zkanji/words")) &&
//        (((arr = e->mimeData()->data(QStringLiteral("zkanji/words"))).size() - sizeof(intptr_t) * 2) % sizeof(int)) == 0 &&
//        *(intptr_t*)arr.constData() == (intptr_t)group->dictionary() &&
//        (*((intptr_t*)arr.constData() + 1) == 0 || samesource || (intptr_t)group != *((intptr_t*)arr.constData() + 1));
//
//}

bool DictionaryGroupItemModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    return group != nullptr && row != -1 && column != -1 && data->hasFormat("zkanji/words");
}

bool DictionaryGroupItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    const QByteArray arr = data->data(QStringLiteral("zkanji/words"));
    if ((void*)*(intptr_t*)arr.constData() != dictionary())
        return false;

    if (*((intptr_t*)arr.constData() + 1) == (intptr_t)group)
        action = Qt::MoveAction;
    else
        action = Qt::CopyAction;

    int cnt = (arr.size() - sizeof(intptr_t) * 2) / sizeof(int);
    if (cnt * sizeof(int) != arr.size() - sizeof(intptr_t) * 2)
        return false;

    std::vector<int> indexes;
    indexes.reserve(cnt);
    const int *dat = (const int*)(arr.constData() + sizeof(intptr_t) * 2);
    while (cnt)
    {
        indexes.push_back(*dat);
        ++dat;
        --cnt;
    }

    if (action == Qt::MoveAction)
    {
        // Convert from word indexes to group indexes.
        group->indexOf(indexes, indexes);

        smartvector<Range> ranges;
        _rangeFromIndexes(indexes, ranges);

        group->move(ranges, row);
    }
    else
        group->insert(indexes, row);
    
    return true;
}

//void DictionaryGroupItemModel::entryAboutToBeRemoved(int windex)
//{
//    //removalrow = -1;
//
//    auto &list = group->getIndexes();
//    for (int ix = 0; ix != list.size(); ++ix)
//        if (list[ix] == windex)
//        {
//            removalrow = ix;
//            beginRemoveRows(QModelIndex(), ix, ix);
//            break;
//        }
//}

void DictionaryGroupItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    // The group also singals about items removal. Removed entry is handled there.

    //if (removalrow != -1)
    //    endRemoveRows();
    //removalrow = -1;
    ;
}

void DictionaryGroupItemModel::entryChanged(int windex, bool studydef)
{
    // A word in the group's dictionary has changed, but the word might not be part of the
    // group. Only emit dataChanged() if applicable here.

    int row = -1;
    auto &list = group->getIndexes();
    for (int ix = 0; row == -1 && ix != list.size(); ++ix)
    {
        if (list[ix] == windex)
            row = ix;
    }

    if (row == -1)
        return;

    emit dataChanged(index(row, 0), index(row, columnCount() - 1));
}

void DictionaryGroupItemModel::entryAdded(int windex)
{
    ;
}

void DictionaryGroupItemModel::dictionaryReset()
{
    beginResetModel();
    endResetModel();
}

void DictionaryGroupItemModel::itemsInserted(GroupBase *parent, const smartvector<Interval> &intervals)
{
    if (parent != group)
        return;
    //beginInsertRows(QModelIndex(), first, last);
    //endInsertRows();

    signalRowsInserted(intervals);
}

//void DictionaryGroupItemModel::beginItemsRemove(GroupBase *parent, int first, int last)
//{
//    if (parent != group)
//        return;
//
//    beginRemoveRows(QModelIndex(), first, last);
//}

void DictionaryGroupItemModel::itemsRemoved(GroupBase *parent, const smartvector<Range> &ranges/*, int first, int last*/)
{
    if (parent != group)
        return;

    //endRemoveRows();
    signalRowsRemoved(ranges);
}

//void DictionaryGroupItemModel::beginItemsMove(GroupBase *parent, int first, int last, int pos)
//{
//    if (parent != group)
//        return;
//
//    beginMoveRows(QModelIndex(), first, last, QModelIndex(), pos);
//}

void DictionaryGroupItemModel::itemsMoved(GroupBase *parent, const smartvector<Range> &ranges, int pos)
{
    if (parent != group)
        return;

    //endMoveRows();
    signalRowsMoved(ranges, pos);
}


//-------------------------------------------------------------


//DictionaryGroupListItemModel::DictionaryGroupListItemModel(const std::vector<GroupBase*> &grouplist, QWidget *parent) : base(parent), dict(ZKanji::dictionary(0))
//{
//    connect(&dict->wordGroups(), &WordGroups::groupDeleted, this, &DictionaryGroupListItemModel::groupDeleted);
//    connect(&dict->wordGroups(), &WordGroups::itemsInserted, this, &DictionaryGroupListItemModel::itemsInserted);
//    connect(&dict->wordGroups(), &WordGroups::itemsRemoved, this, &DictionaryGroupListItemModel::itemsRemoved);
//    connect(&dict->wordGroups(), &WordGroups::itemsMoved, this, &DictionaryGroupListItemModel::itemsMoved);
//    connect();
//
//    groups.reserve(grouplist.size());
//    for (int ix = 0, siz = grouplist.size(); ix != siz; ++ix)
//    {
//        if (dynamic_cast<WordGroup*>(grouplist[ix]) != nullptr)
//        {
//            if (groups.empty())
//                dict = grouplist[ix]->dictionary();
//#ifdef _DEBUG
//            else if (dict != grouplist[ix]->dictionary())
//                throw "Mixed list of groups.";
//#endif
//            groups.push_back((WordGroup*)grouplist[ix]);
//        }
//    }
//    groups.shrink_to_fit();
//
//    QSet<int> found;
//    poslist.reserve(groups.size());
//    for (WordGroup *g : groups)
//    {
//        poslist.push_back(list.size());
//        const std::vector<int> &wlist = g->getIndexes();
//        list.reserve(list.size() + wlist.size());
//        for (int wix : wlist)
//        {
//            if (found.contains(wix))
//                continue;
//            list.push_back({ g, wix });
//            found.insert(wix);
//        }
//    }
//    list.shrink_to_fit();
//}
//
//Dictionary* DictionaryGroupListItemModel::dictionary() const
//{
//    return dict;
//}
//
//int DictionaryGroupListItemModel::indexes(int pos) const
//{
//    const auto &p = list[pos];
//    return p.first->indexes(p.second);
//}
//
//int DictionaryGroupListItemModel::rowCount(const QModelIndex &parent) const
//{
//    if (parent.isValid())
//        return 0;
//
//    return list.size();
//}
//
//void DictionaryGroupListItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
//{
//    // Hopefully a group signals first that its word has been removed.
//    ;
//}
//
//void DictionaryGroupListItemModel::entryChanged(int windex, bool studydef)
//{
//    auto it = std::find(list.begin(), list.end(), windex);
//    if (it == list.end())
//        return;
//    int ix = it - list.begin();
//    emit dataChanged(index(ix, 0), index(ix, columnCount() - 1));
//}
//
//void DictionaryGroupListItemModel::entryAdded(int windex)
//{
//    ;
//}
//
//void DictionaryGroupListItemModel::groupDeleted(GroupCategoryBase *parent, int index, void *oldaddress)
//{
//    auto it = std::find(groups.begin(), groups.end(), (WordGroup*)oldaddress);
//    if (it == groups.end())
//        return;
//    int pos = it - groups.begin();
//    int begin = poslist[pos];
//    int end = pos < poslist.size() - 1 ? poslist[pos + 1] : list.size();
//
//    if (begin == end)
//        return;
//
//    std::vector<int> removed(list.begin() + begin, list.begin() + end);
//    list.erase(list.begin() + begin, list.begin() + end);
//
//    int cnt = end - begin;
//    for (int ix = pos + 1, siz = poslist.size(); ix != siz; ++ix)
//        poslist[ix] -= cnt;
//    poslist.erase(poslist.begin() + pos);
//    groups.erase(groups.begin() + pos);
//
//    signalRowsRemoved({ { begin, end - 1} });
//
//    std::sort(removed.begin(), removed.end());
//
//    smartvector<Interval> inserted;
//    // [group, group list index, list insert index
//    std::vector<std::tuple<int, int, int>> inserts;
//
//    // Insert all the items in removed that are in the groups after pos.
//    for (int siz = groups.size(); pos != siz && !removed.empty(); ++pos)
//    {
//        const std::vector<int> &glist = groups[pos]->getIndexes();
//        if (glist.empty())
//            continue;
//
//        std::vector<int> order(glist.size());
//        std::iota(order.begin(), order.end(), 0);
//        std::sort(order.begin(), order.end(), [&glist](int a, int b) { return glist[a] < glist[b]; });
//
//        // List of group position indexes that will have to be displayed.
//        std::vector<int> added;
//        added.reserve(removed.size());
//
//        // Current position in order.
//        int opos = 0;
//        int osiz = order.size();
//        int rpos = 0;
//        int rsiz = removed.size();
//        while (rpos != rsiz && opos != osiz)
//        {
//            while (opos < osiz && glist[order[opos]] < removed[rpos])
//                ++opos;
//            if (opos == osiz)
//                break;
//
//            while (rpos < rsiz && glist[order[opos]] > removed[rpos])
//                ++rpos;
//            if (rpos == rsiz)
//                break;
//
//            while (opos < osiz && rpos < rsiz && glist[order[opos]] == removed[rpos])
//            {
//                added.push_back(order[opos]);
//                removed[rpos] = -1;
//                ++rpos;
//                ++opos;
//            }
//        }
//
//        removed.resize(std::remove(removed.begin(), removed.end(), -1) - removed.begin());
//        std::sort(added.begin(), added.end());
//
//        // Found which items in the group should be displayed. Time to look for the positions
//        // where they will be inserted.
//
//        begin = poslist[pos];
//        end = pos < poslist.size() - 1 ? poslist[pos + 1] : list.size();
//
//        inserts.reserve(inserts.size() + added.size());
//
//        int apos = 0;
//        int asiz = added.size();
//        int gpos = 0;
//        int gsiz = glist.size();
//        while (apos != asiz)
//        {
//            while (added[apos] != gpos)
//            {
//                if (begin != end && list[begin] == glist[gpos])
//                    ++begin;
//                ++gpos;
//            }
//            while (apos != asiz && gpos == added[apos])
//            {
//                inserts.push_back(std::make_tuple(pos, gpos, begin));
//                ++gpos;
//                ++apos;
//            }
//        }
//    }
//
//    if (inserts.empty())
//        return;
//
//    inserted.push_back(new Interval);
//    Interval *i = inserted.back();
//
//    i->index = std::get<2>(inserts.back());
//    i->count = 0;
//
//    int lastgroup = std::get<0>(inserts.back());
//    int groupinserts = 0;
//
//    // Insert the words in the groups.
//    for (int ix = inserts.size() - 1; ix != -1; --ix)
//    {
//        int lpos = std::get<2>(inserts[ix]);
//        if (i->index == lpos)
//            ++i->count;
//        else
//        {
//            inserted.insert(inserted.begin(), new Interval);
//            i = inserted.front();
//            i->index = lpos;
//            i->count = 1;
//        }
//        pos = std::get<0>(inserts[ix]);
//        if (pos != lastgroup)
//        {
//            for (int iy = pos + 1, siy = groups.size(); iy != siy; ++iy)
//                poslist[iy] += groupinserts;
//            groupinserts = 0;
//        }
//        
//        list.insert(list.begin() + lpos, groups[pos]->indexes(std::get<1>(inserts[ix])));
//        ++groupinserts;
//    }
//
//    if (!inserted.empty())
//        signalRowsInserted(inserted);
//
//}
//
//void DictionaryGroupListItemModel::itemsInserted(GroupBase *parent, const smartvector<Interval> &intervals)
//{
//
//}
//
//void DictionaryGroupListItemModel::itemsRemoved(GroupBase *parent, const smartvector<Range> &ranges)
//{
//    auto it = std::find(groups.begin(), groups.end(), (WordGroup*)parent);
//    if (it == groups.end())
//        return;
//    int pos = it - groups.begin();
//
//    // Word indexes removed from the parent group.
//    std::vector<int> removed;
//
//    const std::vector<int> &glist = groups[pos]->getIndexes();
//    removed.reserve(glist.size());
//
//    // Fill removed with indexes in list first. It will be used to remove the items and signal
//    // the removal.
//
//    int gpos = 0;
//    int lpos = poslist[pos];
//    int lend = pos < poslist.size() -1 ?  poslist[pos + 1] : list.size();
//    for (int rix = 0, rsiz = ranges.size(); rix != rsiz; ++rix)
//    {
//        const Range *r = ranges[rix];
//        while (gpos != r->first && lpos != lend)
//        {
//            if (list[lpos] == glist[gpos])
//                ++lpos;
//            ++gpos;
//        }
//
//        for (; gpos != r->last + 1; ++gpos)
//        {
//            if (list[lpos] == glist[gpos])
//            {
//                removed.push_back(lpos);
//                ++lpos;
//            }
//        }
//    }
//
//    // Ranges for removal.
//    smartvector<Range> sranges;
//    _rangeFromIndexes(removed, sranges);
//    for (int &val : removed)
//    {
//        int p = val;
//        val = list[val];
//        list[p] = -1;
//    }
//    list.resize(std::remove(list.begin(), list.end(), -1) - list.begin());
//    for (int ix = pos + 1, siz = poslist.size(); ix != siz; ++ix)
//        poslist[ix] -= removed.size();
//    if (!sranges.empty())
//        signalRowsRemoved(sranges);
//
//    // Insert items back into groups coming after pos.
//}
//
//void DictionaryGroupListItemModel::itemsMoved(GroupBase *parent, const smartvector<Range> &ranges, int pos)
//{
//
//}


//-------------------------------------------------------------


KanjiWordsItemModel::KanjiWordsItemModel(QObject *parent) : base(parent), dict(nullptr), kix(-1), rix(-1), onlyex(false)
{
}

KanjiWordsItemModel::~KanjiWordsItemModel()
{
}

void KanjiWordsItemModel::setKanji(Dictionary *d, int kindex, int reading)
{
    if (dict == d && kix == kindex && rix == reading)
        return;

    beginResetModel();

    disconnect();
    if (dict != nullptr)
        disconnect(dict, nullptr, this, nullptr);

    dict = d;
    kix = kindex;
    rix = reading;

    fillList();

    connect();
    if (dict != nullptr)
    {
        connect(dict, &Dictionary::kanjiExampleAdded, this, &KanjiWordsItemModel::kanjiExampleAdded);
        //connect(dict, &Dictionary::kanjiExampleAboutToBeRemoved, this, &KanjiWordsItemModel::kanjiExampleAboutToBeRemoved);
        connect(dict, &Dictionary::kanjiExampleRemoved, this, &KanjiWordsItemModel::kanjiExampleRemoved);
    }
    endResetModel();
}

bool KanjiWordsItemModel::showOnlyExamples() const
{
    return onlyex;
}

void KanjiWordsItemModel::setShowOnlyExamples(bool val)
{
    if (onlyex == val)
        return;

    beginResetModel();
    onlyex = val;

    fillList();

    endResetModel();
}

Dictionary* KanjiWordsItemModel::dictionary() const
{
    return dict;
}

int KanjiWordsItemModel::indexes(int pos) const
{
    return list[pos];
}

int KanjiWordsItemModel::rowCount(const QModelIndex &parent) const
{
    return list.size();
}

QVariant KanjiWordsItemModel::data(const QModelIndex &index, int role) const
{
    if (dict != nullptr && index.isValid() && role == (int)CellRoles::CellColor)
    {
        if (onlyex || dict->isKanjiExample(kix, list[index.row()]))
            return QVariant(Settings::uiColor(ColorSettings::KanjiExBg));
    }

    return base::data(index, role);
}

//void KanjiWordsItemModel::entryAboutToBeRemoved(int windex)
//{
//    int pos = -1;
//    for (int ix = 0; ix != list.size() && pos == -1; ++ix)
//        if (list[ix] == windex)
//            pos = ix;
//
//    if (pos == -1)
//        return;
//
//    beginRemoveRows(QModelIndex(), pos, pos);
//
//}

void KanjiWordsItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    int pos = -1;
    for (int ix = 0; ix != list.size(); ++ix)
        if (list[ix] > windex)
            --list[ix];
        else if (list[ix] == windex)
            pos = ix;

    if (pos == -1)
        return;

    list.erase(list.begin() + pos);

    //endRemoveRows();
    signalRowsRemoved({ { pos, pos } });
}

void KanjiWordsItemModel::entryChanged(int windex, bool studydef)
{
    int pos = -1;
    for (int ix = 0; ix != list.size() && pos == -1; ++ix)
        if (list[ix] == windex)
            pos = ix;

    if (pos == -1)
        return;

    emit dataChanged(index(pos, 0), index(pos, columnCount() - 1));
}

void KanjiWordsItemModel::entryAdded(int windex)
{
    if (dict == nullptr || kix == -1)
        return;

    WordEntry *e = dict->wordEntry(windex);
    KanjiEntry *k = ZKanji::kanjis[kix];

    // No work to do if the word doesn't have the kanji in it or it's not displayed for other
    // reasons.

    //const KanjiDictData *dat = dict->kanjiData(kix);
    //std::find(dat->ex.begin(), dat->ex.end(), windex) == dat->ex.end()
    if (e->kanji.find(ZKanji::kanjis[kix]->ch) == -1 || (rix != -1 && !matchKanjiReading(e->kanji, e->kana, k, rix)) || (onlyex && !dict->isKanjiExample(kix, windex)))
        return;

    auto it = std::upper_bound(list.begin(), list.end(), windex, [this](int ax, int bx) {
        return Dictionary::jpSortFunc(std::make_pair(dict->wordEntry(ax), nullptr), std::make_pair(dict->wordEntry(bx), nullptr));
    });

    int pos = it - list.begin();

    //beginInsertRows(QModelIndex(), pos, pos);
    list.insert(it, windex);
    //endInsertRows();
    signalRowsInserted({ { pos, 1 } });
}

void KanjiWordsItemModel::kanjiExampleAdded(int kindex, int windex)
{
    if (kix != kindex)
        return;

    if (!onlyex)
    {
        int row = wordRow(windex);
        if (row != -1)
            emit dataChanged(index(row, 0), index(row, columnCount() - 1));
        return;
    }

    KanjiEntry *k = ZKanji::kanjis[kix];
    WordEntry *e = dict->wordEntry(windex);
    if (rix != -1 && !matchKanjiReading(e->kanji, e->kana, k, rix))
        return;

    auto it = std::upper_bound(list.begin(), list.end(), windex, [this](int ax, int bx) {
        return Dictionary::jpSortFunc(std::make_pair(dict->wordEntry(ax), nullptr), std::make_pair(dict->wordEntry(bx), nullptr));
    });

    int row = it - list.begin();
    //beginInsertRows(QModelIndex(), row, row);
    list.insert(it, windex);
    //endInsertRows();
    signalRowsInserted({ { row, 1 } });
}

//void KanjiWordsItemModel::kanjiExampleAboutToBeRemoved(int kindex, int windex)
//{
//    if (kix != kindex || !onlyex)
//        return;
//
//    int row = wordRow(windex);
//    if (row == -1)
//        return;
//
//    //if (!onlyex)
//    //{
//    //    emit dataChanged(index(it - list.begin(), 0), index(it - list.begin(), columnCount() - 1));
//    //    return;
//    //}
//
//    beginRemoveRows(QModelIndex(), row, row);
//}

void KanjiWordsItemModel::kanjiExampleRemoved(int kindex, int windex)
{
    if (kix != kindex)
        return;

    int row = wordRow(windex);
    if (row == -1)
        return;

    if (!onlyex)
    {
        emit dataChanged(index(row, 0), index(row, columnCount() - 1));
        return;
    }

    list.erase(list.begin() + row);

    //endRemoveRows();
    signalRowsRemoved({ { row, row } });
}

int KanjiWordsItemModel::wordRow(int windex) const
{
    auto it = std::lower_bound(list.begin(), list.end(), windex, [this](int ax, int bx) {
        return Dictionary::jpSortFunc(std::make_pair(dict->wordEntry(ax), nullptr), std::make_pair(dict->wordEntry(bx), nullptr));
    });

    while (it != list.end() && *it != windex && !Dictionary::jpSortFunc(std::make_pair(dict->wordEntry(windex), nullptr), std::make_pair(dict->wordEntry(*it), nullptr)))
        ++it;

    if (it == list.end() || *it != windex)
        return -1;

    return it - list.begin();
}

//std::function<bool(int, int)> KanjiWordsItemModel::sortFunc() const
//{
//    //return [this](int a, int b)
//    //{
//    //    const WordEntry *aw = dict->wordEntry(a);
//    //    const WordEntry *bw = dict->wordEntry(b);
//
//    //    int afreq = aw->freq;
//    //    int bfreq = bw->freq;
//
//    //    //if (abs(afreq - bfreq) > 1500 || (abs(afreq - bfreq) > 500 && afreq + bfreq < 2400))
//    //    //    return afreq > bfreq;
//
//    //    int afreqdiv = afreq / 700;
//    //    int bfreqdiv = bfreq / 700;
//
//    //    if (std::abs(afreqdiv - bfreqdiv) > 1)
//    //        return afreqdiv > bfreqdiv;
//
//    //    int alen = aw->kana.size();
//    //    int blen = bw->kana.size();
//
//    //    bool ako = (aw->defs[0].attrib.notes & (1 << (int)WordNotes::KanaOnly)) != 0;
//    //    bool bko = (bw->defs[0].attrib.notes & (1 << (int)WordNotes::KanaOnly)) != 0;
//
//    //    if (ako)
//    //        afreq += 440;
//    //    if (bko)
//    //        bfreq += 440;
//    //    afreq += (26 - std::min(alen, 25)) * 10;
//    //    bfreq += (26 - std::min(blen, 25)) * 10;
//
//    //    afreqdiv = afreq / 250;
//    //    bfreqdiv = bfreq / 250;
//
//    //    if (std::abs(afreqdiv - bfreqdiv) > 1)
//    //        return afreqdiv > bfreqdiv;
//
//    //    //if ((ako && !bko && alen < blen) || (bko && !ako && blen < alen))
//    //    //    return alen < blen;
//
//    //    int akanjicnt = 0;
//    //    int avalidcnt = 0;
//    //    int aklen = aw->kanji.size();
//    //    for (int ix = 0; ix < aklen; ++ix)
//    //        if (KANJI(aw->kanji[ix].unicode()))
//    //            ++akanjicnt;
//    //        else if (VALIDCODE(aw->kanji[ix].unicode()))
//    //            ++avalidcnt;
//
//    //    int bkanjicnt = 0;
//    //    int bvalidcnt = 0;
//    //    int bklen = bw->kanji.size();
//    //    for (int ix = 0; ix < bklen; ++ix)
//    //        if (KANJI(bw->kanji[ix].unicode()))
//    //            ++bkanjicnt;
//    //        else if (VALIDCODE(bw->kanji[ix].unicode()))
//    //            ++bvalidcnt;
//
//    //    afreq += (21 - std::min(akanjicnt + avalidcnt, 20)) * 12;
//    //    bfreq += (21 - std::min(bkanjicnt + bvalidcnt, 20)) * 12;
//
//    //    afreqdiv = afreq / 20;
//    //    bfreqdiv = bfreq / 20;
//
//    //    if (std::abs(afreqdiv - bfreqdiv) > 1)
//    //        return afreqdiv > bfreqdiv;
//
//    //    //if (akanjicnt < bkanjicnt && alen - 2 < blen)
//    //    //    return true;
//    //    //if (bkanjicnt <= akanjicnt && blen - 2 <= alen)
//    //    //    return false;
//
//    //    afreq += 20 - std::min(aklen, 19);
//    //    bfreq += 20 - std::min(bklen, 19);
//
//    //    return afreq > bfreq;
//
//    //    //if (afreq != bfreq)
//    //    //    return afreq < bfreq;
//
//    //    //if (alen != blen)
//    //    //    return alen < blen;
//
//    //    //if (aklen != bklen)
//    //    //    return aklen < bklen;
//
//    //    //int r = qcharcmp(aw->kana.data(), bw->kana.data());
//    //    //if (r)
//    //    //    return r < 0;
//    //    //return qcharcmp(aw->kanji.data(), bw->kanji.data()) < 0;
//
//    //};
//}

void KanjiWordsItemModel::fillList()
{
    list.clear();

    //const KanjiDictData *dat = dict->kanjiData(kix);

    if (rix == -1 && !onlyex)
        dict->getKanjiWords(kix, list); // = dat->words;
    else
    {
        KanjiEntry *k = ZKanji::kanjis[kix];
        int siz = dict->kanjiWordCount(kix);
        for (int ix = 0; ix != siz /*dat->words.size()*/; ++ix)
        {
            int wix = dict->kanjiWordAt(kix, ix); //dat->words[ix];
            WordEntry *e = dict->wordEntry(wix);
            if ((rix == -1 || matchKanjiReading(e->kanji, e->kana, k, rix)) && (!onlyex || dict->isKanjiExample(kix, wix) /*std::find(dat->ex.begin(), dat->ex.end(), wix) != dat->ex.end()*/ ))
                list.push_back(wix);
        }
    }


    std::sort(list.begin(), list.end(), [this](int ax, int bx) {
        return Dictionary::jpSortFunc(std::make_pair(dict->wordEntry(ax), nullptr), std::make_pair(dict->wordEntry(bx), nullptr));
    });
}


//-------------------------------------------------------------

