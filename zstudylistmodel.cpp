/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMimeData>
#include <memory>
#include "zstudylistmodel.h"
#include "zkanjimain.h"
#include "worddeck.h"
#include "wordtodeckform.h"
#include "zui.h"
#include "words.h"
#include "globalui.h"
#include "ranges.h"
#include "dialogs.h"
#include "generalsettings.h"

//-------------------------------------------------------------


std::vector<DictColumnData> StudyListModel::queuedata;
std::vector<DictColumnData> StudyListModel::studieddata;
std::vector<DictColumnData> StudyListModel::testeddata;
int StudyListModel::scale = -1;

const std::vector<DictColumnData>& StudyListModel::queueData()
{
    if (scale != Settings::general.scale)
        updateColData();
    return queuedata;
}
const std::vector<DictColumnData>& StudyListModel::studiedData()
{
    if (scale != Settings::general.scale)
        updateColData();
    return studieddata;
}
const std::vector<DictColumnData>& StudyListModel::testedData()
{
    if (scale != Settings::general.scale)
        updateColData();
    return testeddata;
}

void StudyListModel::updateColData()
{
    scale = Settings::general.scale;
    queuedata = {
        { (int)DeckColumnTypes::Index, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "#") },
        { (int)DeckColumnTypes::AddedDate, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(75), QT_TRANSLATE_NOOP("StudyListModel", "Added") },
        { (int)DeckColumnTypes::Priority, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Priority") },
        { (int)DeckColumnTypes::StudiedPart, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(45), QT_TRANSLATE_NOOP("StudyListModel", "Studied") },
        { (int)DictColumnTypes::Kanji, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(80), QT_TRANSLATE_NOOP("StudyListModel", "Written") },
        { (int)DictColumnTypes::Kana, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(100), QT_TRANSLATE_NOOP("StudyListModel", "Kana") },
        { (int)DictColumnTypes::Definition, Qt::AlignLeft, ColumnAutoSize::NoAuto, false, Settings::scaled(6400), QT_TRANSLATE_NOOP("StudyListModel", "Definition") }
    };
    studieddata = {
        { (int)DeckColumnTypes::Index, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "#") },
        { (int)DeckColumnTypes::AddedDate, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(75), QT_TRANSLATE_NOOP("StudyListModel", "Added") },
        { (int)DeckColumnTypes::Level, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Level") },
        { (int)DeckColumnTypes::Tries, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Tries") },
        { (int)DeckColumnTypes::FirstDate, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(75), QT_TRANSLATE_NOOP("StudyListModel", "First") },
        { (int)DeckColumnTypes::LastDate, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(75), QT_TRANSLATE_NOOP("StudyListModel", "Last") },
        { (int)DeckColumnTypes::NextDate, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(75), QT_TRANSLATE_NOOP("StudyListModel", "Next") },
        { (int)DeckColumnTypes::Interval, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(65), QT_TRANSLATE_NOOP("StudyListModel", "Interval") },
        { (int)DeckColumnTypes::Multiplier, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(45), QT_TRANSLATE_NOOP("StudyListModel", "Multiplier") },
        { (int)DeckColumnTypes::StudiedPart, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(45), QT_TRANSLATE_NOOP("StudyListModel", "Studied") },
        { (int)DictColumnTypes::Kanji, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(80), QT_TRANSLATE_NOOP("StudyListModel", "Written") },
        { (int)DictColumnTypes::Kana, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(100), QT_TRANSLATE_NOOP("StudyListModel", "Kana") },
        { (int)DictColumnTypes::Definition, Qt::AlignLeft, ColumnAutoSize::NoAuto, false, Settings::scaled(6400), QT_TRANSLATE_NOOP("StudyListModel", "Definition") }
    };

    testeddata = {
        { (int)DeckColumnTypes::Index, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "#") },
        { (int)DeckColumnTypes::AddedDate, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(75), QT_TRANSLATE_NOOP("StudyListModel", "Added") },
        { (int)DeckColumnTypes::OldLevel, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Old Lv.") },
        { (int)DeckColumnTypes::Level, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Level") },
        { (int)DeckColumnTypes::RetryCount, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Retried") },
        { (int)DeckColumnTypes::WrongCount, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Wrong") },
        { (int)DeckColumnTypes::EasyCount, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Easy") },
        { (int)DeckColumnTypes::NextDate, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(75), QT_TRANSLATE_NOOP("StudyListModel", "Next") },
        { (int)DeckColumnTypes::Interval, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(65), QT_TRANSLATE_NOOP("StudyListModel", "Interval") },
        { (int)DeckColumnTypes::OldMultiplier, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(50), QT_TRANSLATE_NOOP("StudyListModel", "Old mul.") },
        { (int)DeckColumnTypes::Multiplier, Qt::AlignRight, ColumnAutoSize::NoAuto, true, Settings::scaled(45), QT_TRANSLATE_NOOP("StudyListModel", "Multiplier") },
        { (int)DeckColumnTypes::StudiedPart, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, Settings::scaled(45), QT_TRANSLATE_NOOP("StudyListModel", "Studied") },
        { (int)DictColumnTypes::Kanji, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(80), QT_TRANSLATE_NOOP("StudyListModel", "Written") },
        { (int)DictColumnTypes::Kana, Qt::AlignLeft, ColumnAutoSize::NoAuto, true, Settings::scaled(100), QT_TRANSLATE_NOOP("StudyListModel", "Kana") },
        { (int)DictColumnTypes::Definition, Qt::AlignLeft, ColumnAutoSize::NoAuto, false, Settings::scaled(6400), QT_TRANSLATE_NOOP("StudyListModel", "Definition") }
    };
}

int StudyListModel::queueColCount()
{
    if (scale != Settings::general.scale)
        updateColData();
    return queuedata.size();
}

int StudyListModel::studiedColCount()
{
    if (scale != Settings::general.scale)
        updateColData();
    return studieddata.size();
}

int StudyListModel::testedColCount()
{
    if (scale != Settings::general.scale)
        updateColData();
    return testeddata.size();
}

//-------------------------------------------------------------


void StudyListModel::defaultColumnWidths(DeckItemViewModes mode, std::vector<int> &result)
{
    if (mode == DeckItemViewModes::Queued)
    {
        result.resize(queueColCount() - 1);
        int ix = 0;
        for (const DictColumnData &dat : queueData())
        {
            if (ix == result.size())
                break;
            result[ix++] = dat.width;
        }
    }
    else if (mode == DeckItemViewModes::Studied)
    {
        result.resize(studiedColCount() - 1);
        int ix = 0;
        for (const DictColumnData &dat : studiedData())
        {
            if (ix == result.size())
                break;
            result[ix++] = dat.width;
        }
    }
    else if (mode == DeckItemViewModes::Tested)
    {
        result.resize(testedColCount() - 1);
        int ix = 0;
        for (const DictColumnData &dat : testedData())
        {
            if (ix == result.size())
                break;
            result[ix++] = dat.width;
        }
    }
}

int StudyListModel::defaultColumnWidth(DeckItemViewModes mode, int columnindex)
{
    if (mode == DeckItemViewModes::Queued)
    {
        if (columnindex < 0 || columnindex >= queueColCount())
            return -1;
        return (queueData().begin() + columnindex)->width;
    }
    else if (mode == DeckItemViewModes::Studied)
    {
        if (columnindex < 0 || columnindex >= studiedColCount())
            return -1;
        return (studiedData().begin() + columnindex)->width;
    }
    else if (mode == DeckItemViewModes::Tested)
    {
        if (columnindex < 0 || columnindex >= testedColCount())
            return -1;
        return (testedData().begin() + columnindex)->width;
    }

    return -1;
}

StudyListModel::StudyListModel(WordDeck *deck, QObject *parent) : base(parent), deck(deck), mode(DeckItemViewModes::Studied), kanjion(true), kanaon(true), defon(true)
{
    setViewMode(DeckItemViewModes::Queued);
    connect();

    connect(deck, &WordDeck::itemsQueued, this, &StudyListModel::itemsQueued);
    connect(deck, &WordDeck::itemsRemoved, this, [this](const std::vector<int> &indexes, bool queued) { itemsRemoved(indexes, queued); });
    connect(deck, &WordDeck::itemDataChanged, this, &StudyListModel::itemDataChanged);
}

StudyListModel::~StudyListModel()
{

}

void StudyListModel::reset()
{
    beginResetModel();
    if (mode == DeckItemViewModes::Queued)
        setColumns(queueData());
    else if (mode == DeckItemViewModes::Studied)
        setColumns(studiedData());
    else if (mode == DeckItemViewModes::Tested)
        setColumns(testedData());
    fillList(list);
    endResetModel();
}

void StudyListModel::setViewMode(DeckItemViewModes newmode)
{
    if (mode == newmode)
        return;

    beginResetModel();

    mode = newmode;
    if (mode == DeckItemViewModes::Queued)
        setColumns(queueData());
    else if (mode == DeckItemViewModes::Studied)
        setColumns(studiedData());
    else if (mode == DeckItemViewModes::Tested)
        setColumns(testedData());
    fillList(list);

    endResetModel();
}

DeckItemViewModes StudyListModel::viewMode() const
{
    return mode;
}

bool StudyListModel::sortOrder(int column, int rowa, int rowb)
{
    int ixa = rowa;
    int ixb = rowb;
    rowa = list[rowa];
    rowb = list[rowb];

    if (mode == DeckItemViewModes::Queued)
    {
        FreeWordDeckItem *itema = deck->queuedItems(rowa);
        FreeWordDeckItem *itemb = deck->queuedItems(rowb);
        switch (headerData(column, Qt::Horizontal, (int)DictColumnRoles::Type).toInt())
        {
        case (int)DeckColumnTypes::Index:
            return rowa < rowb;
        case (int)DeckColumnTypes::AddedDate:
            if (itema->added != itemb->added)
                return itema->added < itemb->added;
            return rowa < rowb;
        case (int)DeckColumnTypes::Priority:
            if (itema->priority != itemb->priority)
                return itema->priority > itemb->priority;
            return rowa < rowb;
        case (int)DeckColumnTypes::StudiedPart:
        {
            uchar vala = (uchar)itema->questiontype;
            uchar valb = (uchar)itemb->questiontype;
            if (vala != valb)
                return vala < valb;
            vala = (uchar)itema->mainhint;
            valb = (uchar)itemb->mainhint;
            if (itema->mainhint != itemb->mainhint)
                return vala < valb;
            return rowa < rowb;
        }
        case (int)DictColumnTypes::Kanji:
        {
            int dif = qcharcmp(dictionary()->wordEntry(itema->data->index)->kanji.data(), dictionary()->wordEntry(itemb->data->index)->kanji.data());
            if (dif != 0)
                return dif < 0;
            return rowa < rowb;
        }
        case (int)DictColumnTypes::Kana:
        {
            int dif = qcharcmp(dictionary()->wordEntry(itema->data->index)->kana.data(), dictionary()->wordEntry(itemb->data->index)->kana.data());
            if (dif != 0)
                return dif < 0;
            return rowa < rowb;
        }
        default:
            return rowa < rowb;
        }
    }
    else if (mode == DeckItemViewModes::Studied)
    {
        LockedWordDeckItem *itema = deck->studiedItems(rowa);
        LockedWordDeckItem *itemb = deck->studiedItems(rowb);
        StudyDeck *study = deck->getStudyDeck();
        //const StudyCard *card = ZKanji::student()->getDeck(deck->deckId())->cards(item->cardid);

        switch (headerData(column, Qt::Horizontal, (int)DictColumnRoles::Type).toInt())
        {
        case (int)DeckColumnTypes::Index:
            return rowa < rowb;
        case (int)DeckColumnTypes::AddedDate:
            if (itema->added != itemb->added)
                return itema->added < itemb->added;
            return rowa < rowb;
        case (int)DeckColumnTypes::Level:
        {
            int levela = study->cardLevel(itema->cardid);
            int levelb = study->cardLevel(itemb->cardid);
            if (levela != levelb)
                return levela < levelb;
            // To the next case:
        }
        case (int)DeckColumnTypes::Tries:
        {
            int ta = study->cardInclusion(itema->cardid);
            int tb = study->cardInclusion(itemb->cardid);
            if (ta != tb)
                return ta < tb;
            // To the next case:
        }
        case (int)DeckColumnTypes::FirstDate:
        {
            QDate daya = study->cardFirstStats(itema->cardid).day;
            QDate dayb = study->cardFirstStats(itemb->cardid).day;
            if (daya != dayb)
                return daya < dayb;
            // To the next case:
        }
        case (int)DeckColumnTypes::LastDate:
        {
            QDateTime da = study->cardTestDate(itema->cardid);
            QDateTime db = study->cardTestDate(itemb->cardid);
            if (da != db)
                return da < db;
            // To the next case:
        }
        case (int)DeckColumnTypes::NextDate:
        {
            QDateTime nda = study->cardNextTestDate(itema->cardid);
            QDateTime ndb = study->cardNextTestDate(itemb->cardid);
            if (nda != ndb)
                return nda < ndb;
            // To the next case:
        }
        case (int)DeckColumnTypes::Interval:
        {
            auto ia = study->cardSpacing(itema->cardid);
            auto ib = study->cardSpacing(itemb->cardid);
            if (ia != ib)
                return ia < ib;
            return rowa < rowb;
        }
        case (int)DeckColumnTypes::Multiplier:
        {
            auto ia = study->cardMultiplier(itema->cardid);
            auto ib = study->cardMultiplier(itemb->cardid);
            if (ia != ib)
                return ia < ib;
            return rowa < rowb;
        }
        case (int)DeckColumnTypes::StudiedPart:
        {
            WordPartBits vala = itema->questiontype;
            WordPartBits valb = itemb->questiontype;
            if (vala != valb)
                return vala < valb;
            return rowa < rowb;
        }
        case (int)DictColumnTypes::Kanji:
        {
            int dif = qcharcmp(dictionary()->wordEntry(itema->data->index)->kanji.data(), dictionary()->wordEntry(itemb->data->index)->kanji.data());
            if (dif != 0)
                return dif < 0;
            return rowa < rowb;
        }
        case (int)DictColumnTypes::Kana:
        {
            int dif = qcharcmp(dictionary()->wordEntry(itema->data->index)->kana.data(), dictionary()->wordEntry(itemb->data->index)->kana.data());
            if (dif != 0)
                return dif < 0;
            return rowa < rowb;
        }
        default:
            return rowa < rowb;
        }
    }
    else if (mode == DeckItemViewModes::Tested)
    {
        LockedWordDeckItem *itema = deck->studiedItems(rowa);
        LockedWordDeckItem *itemb = deck->studiedItems(rowb);
        StudyDeck *study = deck->getStudyDeck();
        //const StudyCard *card = ZKanji::student()->getDeck(deck->deckId())->cards(item->cardid);

        switch (headerData(column, Qt::Horizontal, (int)DictColumnRoles::Type).toInt())
        {
        case (int)DeckColumnTypes::Index:
            return ixa < ixb;
        case (int)DeckColumnTypes::AddedDate:
            if (itema->added != itemb->added)
                return itema->added < itemb->added;
            return ixa < ixb;
        case (int)DeckColumnTypes::OldLevel:
        {
            int levela = study->cardLevelOld(itema->cardid);
            int levelb = study->cardLevelOld(itemb->cardid);
            if (levela != levelb)
                return levela < levelb;
            // To the next case:
        }
        case (int)DeckColumnTypes::Level:
        {
            int levela = study->cardLevel(itema->cardid);
            int levelb = study->cardLevel(itemb->cardid);
            if (levela != levelb)
                return levela < levelb;
            // To the next case:
        }
        case (int)DeckColumnTypes::RetryCount:
        {
            uchar ta = study->cardTries(itema->cardid)[0];
            uchar tb = study->cardTries(itemb->cardid)[0];
            if (ta != tb)
                return ta < tb;
            // To the next case:
        }
        case (int)DeckColumnTypes::WrongCount:
        {
            uchar ta = study->cardTries(itema->cardid)[2];
            uchar tb = study->cardTries(itemb->cardid)[2];
            if (ta != tb)
                return ta < tb;
            // To the next case:
        }
        case (int)DeckColumnTypes::EasyCount:
        {
            uchar ta = study->cardTries(itema->cardid)[3];
            uchar tb = study->cardTries(itemb->cardid)[3];
            if (ta != tb)
                return ta < tb;
            // To the next case:
        }
        case (int)DeckColumnTypes::NextDate:
        {
            QDateTime nda = study->cardNextTestDate(itema->cardid);
            QDateTime ndb = study->cardNextTestDate(itemb->cardid);
            if (nda != ndb)
                return nda < ndb;
            // To the next case:
        }
        case (int)DeckColumnTypes::Interval:
        {
            auto ia = study->cardSpacing(itema->cardid);
            auto ib = study->cardSpacing(itemb->cardid);
            if (ia != ib)
                return ia < ib;
            return ixa < ixb;
        }
        case (int)DeckColumnTypes::OldMultiplier:
        {
            auto ia = study->cardMultiplierOld(itema->cardid);
            auto ib = study->cardMultiplierOld(itemb->cardid);
            if (ia != ib)
                return ia < ib;
            return ixa < ixb;
        }
        case (int)DeckColumnTypes::Multiplier:
        {
            auto ia = study->cardMultiplier(itema->cardid);
            auto ib = study->cardMultiplier(itemb->cardid);
            if (ia != ib)
                return ia < ib;
            return ixa < ixb;
        }
        case (int)DeckColumnTypes::StudiedPart:
        {
            WordPartBits vala = itema->questiontype;
            WordPartBits valb = itemb->questiontype;
            if (vala != valb)
                return vala < valb;
            return ixa < ixb;
        }
        case (int)DictColumnTypes::Kanji:
        {
            int dif = qcharcmp(dictionary()->wordEntry(itema->data->index)->kanji.data(), dictionary()->wordEntry(itemb->data->index)->kanji.data());
            if (dif != 0)
                return dif < 0;
            return ixa < ixb;
        }
        case (int)DictColumnTypes::Kana:
        {
            int dif = qcharcmp(dictionary()->wordEntry(itema->data->index)->kana.data(), dictionary()->wordEntry(itemb->data->index)->kana.data());
            if (dif != 0)
                return dif < 0;
            return ixa < ixb;
        }
        default:
            return ixa < ixb;
        }
    }
    return false;
}

void StudyListModel::setShownParts(bool showkanji, bool showkana, bool showdefinition)
{
    if (kanjion == showkanji && kanaon == showkana && defon == showdefinition)
        return;

    std::swap(kanjion, showkanji);
    std::swap(kanaon, showkana);
    std::swap(defon, showdefinition);

    if (!list.empty() && ((!kanjion && showkanji) || (!kanaon && showkana) || (!defon && showdefinition)))
    {
        smartvector<Range> removed;

        WordPartBits question = mode == DeckItemViewModes::Queued ? deck->queuedItems(list.back())->questiontype : deck->studiedItems(list.back())->questiontype;

        bool lastmatch = !matchesFilter(question);
        for (int pos = list.size() - 1, prev = pos - 1; pos != -1; --prev)
        {
            bool match = prev == -1 ? !lastmatch : !matchesFilter(mode == DeckItemViewModes::Queued ? deck->queuedItems(list[prev])->questiontype : deck->studiedItems(list[prev])->questiontype);
            if (prev != -1 && lastmatch == match)
                continue;

            if (lastmatch)
            {
                removed.insert(removed.begin(), { prev + 1, pos });
                list.erase(list.begin() + (prev + 1), list.begin() + (pos + 1));
            }
            lastmatch = !lastmatch;
            pos = prev;
        }

        if (!removed.empty())
            signalRowsRemoved(removed);
    }
  
    if (((kanjion && !showkanji) || (kanaon && !showkana) || (defon && !showdefinition)))
    {
        smartvector<Interval> inserted;

        // Current position in the deck.
        int pos = (mode == DeckItemViewModes::Queued ? deck->queueSize() : mode == DeckItemViewModes::Studied ? deck->studySize() : deck->testedSize()) - 1;
        // Insert position in list.
        int inspos = list.size() - 1;

        while (pos != -1)
        {
            while (pos != -1 && ((inspos != -1 && list[inspos] == (mode == DeckItemViewModes::Tested ? deck->testedIndex(pos) : pos)) ||
                !matchesFilter(mode == DeckItemViewModes::Queued ? deck->queuedItems(pos)->questiontype : mode == DeckItemViewModes::Studied ? deck->studiedItems(pos)->questiontype : deck->testedItems(pos)->questiontype)))
            {
                if (inspos != -1 && list[inspos] == (mode == DeckItemViewModes::Tested ? deck->testedIndex(pos) : pos))
                    --inspos;
                --pos;
            }

            if (pos == -1)
                break;

            int prev = pos - 1;
            
            while (prev != -1 && (inspos == -1 || list[inspos] != (mode == DeckItemViewModes::Tested ? deck->testedIndex(prev) : prev)) &&
                matchesFilter(mode == DeckItemViewModes::Queued ? deck->queuedItems(prev)->questiontype : mode == DeckItemViewModes::Studied ? deck->studiedItems(prev)->questiontype : deck->testedItems(prev)->questiontype))
                --prev;

            if (inserted.empty() || inserted.front()->index != inspos + 1)
                inserted.insert(inserted.begin(), { inspos + 1, pos - prev });
            else
                inserted.front()->count += pos - prev;
            list.insert(list.begin() + (inspos + 1), pos - prev, 0);
            for (int ix = 0, siz = pos - prev; ix != siz; ++ix)
            {
                int val = prev + 1 + ix;
                list[inspos + 1 + ix] = (mode == DeckItemViewModes::Tested ? deck->testedIndex(val) : val);
            }

            pos = prev;
        }

        if (!inserted.empty())
            signalRowsInserted(inserted);
    }
}

Dictionary* StudyListModel::dictionary() const
{
    return deck->dictionary();
}

// Returns the index of a word in its dictionary at the passed row.
int StudyListModel::indexes(int pos) const
{
    pos = list[pos];
    if (mode == DeckItemViewModes::Queued)
        return deck->queuedItems(pos)->data->index;
    return deck->studiedItems(pos)->data->index;
}

int StudyListModel::rowCount(const QModelIndex &parent) const
{
    //if (mode == DeckItemViewModes::Queued)
    //    return deck->queueSize();
    //return deck->studySize();
    return list.size();
}

QVariant StudyListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int itemindex = list[index.row()];
    
    if (role == (int)DeckRowRoles::DeckIndex)
    {
        //if (mode == DeckItemViewModes::Tested)
        //    return deck->testedIndex(itemindex);
        return itemindex;
    }
    if (role == (int)DeckRowRoles::ItemQuestion || role == (int)DeckRowRoles::ItemHint)
    {
        WordDeckItem *item = mode == DeckItemViewModes::Queued ? (WordDeckItem*)deck->queuedItems(itemindex) : (WordDeckItem*)deck->studiedItems(itemindex);
        if (role == (int)DeckRowRoles::ItemQuestion)
            return (int)item->questiontype;
        return (int)item->mainhint;
    }

    if (role != Qt::DisplayRole)
        return base::data(index, role);

    QVariant result;

    if (mode == DeckItemViewModes::Queued)
    {
        FreeWordDeckItem *item = deck->queuedItems(itemindex);
        switch (headerData(index.column(), Qt::Horizontal, (int)DictColumnRoles::Type).toInt())
        {
        case (int)DeckColumnTypes::Index:
            return QString::number(itemindex);
        case (int)DeckColumnTypes::AddedDate:
            return formatDate(item->added);
        case (int)DeckColumnTypes::Priority:
            return QString::number(item->priority);
        case (int)DeckColumnTypes::StudiedPart:
            return shortHintString(item) + QStringLiteral(" ") + QChar(0x2192) + QStringLiteral(" ") + shortQuestionString(item);
        case (int)DictColumnTypes::Kanji:
        case (int)DictColumnTypes::Kana:
        case (int)DictColumnTypes::Definition:
            return base::data(index, role);
        }
        return result;
    }
    else if (mode == DeckItemViewModes::Studied)
    {
        LockedWordDeckItem *item = deck->studiedItems(itemindex);
        StudyDeck *study = deck->getStudyDeck();
        //const StudyCard *card = ZKanji::student()->getDeck(deck->deckId())->cards(item->cardid);

        switch (headerData(index.column(), Qt::Horizontal, (int)DictColumnRoles::Type).toInt())
        {
        case (int)DeckColumnTypes::Index:
            return QString::number(itemindex);
        case (int)DeckColumnTypes::AddedDate:
            return formatDate(item->added);
        //case (int)DeckColumnTypes::Score:
        //    return QString::number(int(card->score));
        case (int)DeckColumnTypes::Level:
            return QString::number(int(study->cardLevel(item->cardid)));
        case (int)DeckColumnTypes::Tries:
            return QString::number(int(study->cardInclusion(item->cardid)));
        case (int)DeckColumnTypes::FirstDate:
            return formatDate(study->cardFirstStats(item->cardid).day, tr("Never"));
            //return ZKanji::student()->getDeck(deck->deckId())->cards(item->cardid)->stats[0].day;
        case (int)DeckColumnTypes::LastDate:
            return fixFormatPastDate(study->cardTestDate(item->cardid));
        case (int)DeckColumnTypes::NextDate:
            return fixFormatNextDate(study->cardNextTestDate(item->cardid));
        case (int)DeckColumnTypes::Interval:
            return formatSpacing(study->cardSpacing(item->cardid));
        case (int)DeckColumnTypes::Multiplier:
            return QString::number(study->cardMultiplier(item->cardid), 'f', 2);
        case (int)DeckColumnTypes::StudiedPart:
            return shortHintString(item) + QStringLiteral(" ") + QChar(0x2192) + QStringLiteral(" ") + shortQuestionString(item);
        case (int)DictColumnTypes::Kanji:
        case (int)DictColumnTypes::Kana:
        case (int)DictColumnTypes::Definition:
            return base::data(index, role);
        }
        return result;
    }
    else // if (mode == DeckItemViewModes::Tested)
    {
        LockedWordDeckItem *item = deck->studiedItems(itemindex);
        StudyDeck *study = deck->getStudyDeck();
        //const StudyCard *card = ZKanji::student()->getDeck(deck->deckId())->cards(item->cardid);

        switch (headerData(index.column(), Qt::Horizontal, (int)DictColumnRoles::Type).toInt())
        {
        case (int)DeckColumnTypes::Index:
            return QString::number(itemindex) /*deck->studiedIndex(item))*/;
        case (int)DeckColumnTypes::AddedDate:
            return formatDate(item->added);
            //case (int)DeckColumnTypes::Score:
            //    return QString::number(int(card->score));
        case (int)DeckColumnTypes::OldLevel:
            return QString::number(int(study->cardLevelOld(item->cardid)));
        case (int)DeckColumnTypes::Level:
            return QString::number(int(study->cardLevel(item->cardid)));
        case (int)DeckColumnTypes::RetryCount:
            return QString::number(int(study->cardTries(item->cardid)[0]));
        case (int)DeckColumnTypes::WrongCount:
            return QString::number(int(study->cardTries(item->cardid)[2]));
        case (int)DeckColumnTypes::EasyCount:
            return QString::number(int(study->cardTries(item->cardid)[3]));
        case (int)DeckColumnTypes::NextDate:
            return fixFormatNextDate(study->cardNextTestDate(item->cardid));
        case (int)DeckColumnTypes::Interval:
            return formatSpacing(study->cardSpacing(item->cardid));
        case (int)DeckColumnTypes::OldMultiplier:
            return QString::number(study->cardMultiplierOld(item->cardid), 'f', 2);
        case (int)DeckColumnTypes::Multiplier:
            return QString::number(study->cardMultiplier(item->cardid), 'f', 2);
        case (int)DeckColumnTypes::StudiedPart:
            return shortHintString(item) + QStringLiteral(" ") + QChar(0x2192) + QStringLiteral(" ") + shortQuestionString(item);
        case (int)DictColumnTypes::Kanji:
        case (int)DictColumnTypes::Kana:
        case (int)DictColumnTypes::Definition:
            return base::data(index, role);
        }
        return result;
    }
}

Qt::DropActions StudyListModel::supportedDragActions() const
{
    return 0;
}

Qt::DropActions StudyListModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (mode != DeckItemViewModes::Queued)
        return 0;
    if (mime->hasFormat("zkanji/words"))
        return Qt::CopyAction;
    return 0;
}

//QStringList StudyListModel::mimeTypes() const
//{
//    QStringList result;
//    result << QStringLiteral("zkanji/words");
//    return result;
//}

bool StudyListModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    return mode == DeckItemViewModes::Queued && row == -1 && column == -1 && !parent.isValid() && data->formats().contains(QStringLiteral("zkanji/words")) &&
            *(intptr_t*)data->data(QStringLiteral("zkanji/words")).constData() == (intptr_t)deck->dictionary();
}

bool StudyListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    const QByteArray &arr = data->data(QStringLiteral("zkanji/words"));

    int cnt = (arr.size() - sizeof(intptr_t) * 2) / sizeof(int);
    if (cnt <= 0 || cnt * sizeof(int) + sizeof(intptr_t) * 2 != arr.size())
        return false;

    const int *dat = (const int*)(arr.constData() + sizeof(intptr_t) * 2);
    std::vector<int> indexes;
    indexes.reserve(cnt);
    for (int ix = 0; ix != cnt; ++ix, ++dat)
        indexes.push_back(*dat);

    addWordsToDeck(dictionary(), deck, indexes, (QWidget*)gUI->activeMainForm());

    return true;
}

void StudyListModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    // Let the deck handle this and notify the model.

    //if (deck->wordFromIndex(windex) == nullptr)
    //    return;

    //std::vector<int> changes;

    //// Try to locate the word in the list. If not found, there's nothing to remove.
    //for (int ix = 0, siz = list.size(); ix != siz; ++ix)
    //{
    //    int pos = list[ix];
    //    if ((mode == DeckItemViewModes::Queued && deck->queuedItems(pos)->data->index == windex) ||
    //        (/*mode == DeckItemViewModes::Studied &&*/ deck->studiedItems(pos)->data->index == windex) /*||
    //        (mode == DeckItemViewModes::Tested && deck->testedItems(pos)->data->index == windex)*/)
    //    {
    //        changes.push_back(ix);
    //    }
    //}

    //if (changes.empty())
    //    return;

    ////std::sort(changes.begin(), changes.end());
    //smartvector<Range> ranges;
    //_rangeFromIndexes(changes, ranges);

    //for (int ix = ranges.size() - 1; ix != -1; --ix)
    //{
    //    const Range *r = ranges[ix];
    //    list.erase(list.begin() + r->first, list.begin() + r->last + 1);
    //}

    //signalRowsRemoved(ranges);

    //// Updates the item index column.
    //emit dataChanged(base::index(0, 0), base::index(list.size(), 0), { Qt::DisplayRole });
}

void StudyListModel::entryChanged(int windex, bool studydef)
{
    // There can be multiple versions of the same item in the queue. The whole lists must be
    // checked in case the rows must be updated.

    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
    {
        int pos = list[ix];
        if ((mode == DeckItemViewModes::Queued && deck->queuedItems(pos)->data->index == windex) ||
            (/*mode == DeckItemViewModes::Studied &&*/ deck->studiedItems(pos)->data->index == windex) /*||
            (mode == DeckItemViewModes::Tested && deck->testedItems(pos)->data->index == windex)*/)
            emit dataChanged(index(ix, 0), index(ix, columnCount() - 1));
    }

    //if (mode == DeckItemViewModes::Queued)
    //{
    //    // Try to locate the word in the queue.
    //    for (int ix = 0; /*!entryremoving &&*/ ix != deck->queueSize(); ++ix)
    //        if (deck->queuedItems(ix)->data->index == windex)
    //            emit dataChanged(index(ix, 0), index(ix, columnCount() - 1));
    //}
    //else
    //{
    //    // Try to locate the word in the study queue.
    //    for (int ix = 0; /*!entryremoving &&*/ ix != deck->studySize(); ++ix)
    //        if (deck->studiedItems(ix)->data->index == windex)
    //            emit dataChanged(index(ix, 0), index(ix, columnCount() - 1));
    //}
}

void StudyListModel::entryAdded(int windex)
{
    // New dictionary entry. Nothing to do here.
}

void StudyListModel::itemsQueued(int count)
{
    if (mode != DeckItemViewModes::Queued || count == 0)
        return;

    // Items added to the queue.

    int lsiz = list.size();
    if (kanjion && kanaon && defon)
    {
        int newsiz = deck->queueSize();
        list.reserve(newsiz);
        while (list.size() != newsiz)
            list.push_back(list.size());
    }
    else
    {
        for (int ix = deck->queueSize() - count, siz = deck->queueSize(); ix != siz; ++ix)
            if (matchesFilter(deck->queuedItems(ix)->questiontype))
                list.push_back(ix);
    }

    if (lsiz == list.size())
        return;

    //beginInsertRows(QModelIndex(), lsiz, list.size() - 1);
    //endInsertRows();

    signalRowsInserted({ { lsiz, (int)list.size() - lsiz } });
}

void StudyListModel::itemsRemoved(const std::vector<int> &indexes, bool queued, bool decrement)
{
    if (queued != (mode == DeckItemViewModes::Queued))
        return;

    // Items is first filled with indexes in list to be removed. When showing items from the
    // last test, items will only hold the same items in the same order as list which are also
    // in indexes.

    std::vector<int> items;
    std::vector<int> order;

    if (mode == DeckItemViewModes::Tested)
    {
        // Adding list indexes to items that are both in list and indexes.

        // First an ordering of list is built, and the values in indexes are matched those in
        // list in increasing order.

        order.reserve(list.size());
        while (order.size() != list.size())
            order.push_back(order.size());
        std::sort(order.begin(), order.end(), [this](int a, int b) { return list[a] < list[b]; });

        for (int ix = 0, siz = order.size(), pos = 0, isiz = indexes.size(); ix != siz && pos != isiz; ++ix)
        {
            while (pos < isiz && list[order[ix]] > indexes[pos])
                ++pos;
            if (pos < isiz && list[order[ix]] == indexes[pos])
                items.push_back(order[ix]);
        }
        std::sort(items.begin(), items.end());
    }
    else
    {
        // Filling items with values in indexes that are found in list.
        for (int ix = 0, siz = list.size(), pos = 0, isiz = indexes.size(); ix != siz && pos != isiz; ++ix)
        {
            while (pos < isiz && list[ix] > indexes[pos])
                ++pos;
            if (pos < isiz && list[ix] == indexes[pos])
                items.push_back(ix);
        }
    }

    smartvector<Range> removed;
    for (int pos = items.size() - 1, prev = pos - 1; pos != -1; --prev)
    {
        if (prev == -1 || items[prev] != items[prev + 1] - 1)
        {
            if (decrement)
            {
                if (mode != DeckItemViewModes::Tested)
                {
                    for (int ix = list.size() - 1, last = items[pos]; ix != last; --ix)
                        list[ix] -= (pos - prev);
                }
                else
                {
                    std::vector<int> sorted(list.begin() + items[prev + 1], list.begin() + items[pos] + 1);
                    std::sort(sorted.begin(), sorted.end());
                    for (int ix = 0, siz = order.size(), sortpos = 0; ix != siz; ++ix)
                    {
                        while (sortpos < sorted.size() && list[order[ix]] >= sorted[sortpos])
                            ++sortpos;

                        if (sortpos)
                            list[order[ix]] -= sortpos;

                        if (order[ix] > pos)
                            order[ix] -= pos - prev;
                        else if (order[ix] > prev)
                            order[ix] = -1;
                    }

                    order.resize(std::remove(order.begin(), order.end(), -1) - order.begin());
                }
            }

            list.erase(list.begin() + items[prev + 1], list.begin() + items[pos] + 1);
            removed.insert(removed.begin(), { items[prev + 1], items[pos] });
            pos = prev;
        }
    }

    signalRowsRemoved(removed);
    //beginResetModel();
    //fillList(list);
    //endResetModel();
}

void StudyListModel::itemDataChanged(const std::vector<int> &indexes, bool queue)
{
    if (queue != (mode == DeckItemViewModes::Queued) || indexes.empty())
        return;

    if (mode == DeckItemViewModes::Tested && deck->getStudyDeck()->cardLevel(deck->studiedItems(indexes.front())->cardid) == 0)
    {
        // Only items that are reset will have a level of 0. These items are removed from the
        // test list, so instead of data changed, we have to process removal.

        itemsRemoved(indexes, false, false);
        return;
    }

    std::vector<int> ordered;
    ordered.reserve(list.size());
    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
        ordered.push_back(ix);
    std::sort(ordered.begin(), ordered.end(), [this](int a, int b) { return list[a] < list[b]; });

    for (int pos = 0, siz = ordered.size(), ipos = 0, isiz = indexes.size(); pos != siz;)
    {
        while (ipos != isiz && indexes[ipos] < ordered[pos])
            ++ipos;
        while (pos != siz && (ipos == isiz || ordered[pos] < indexes[ipos]))
        {
            ordered[pos] = -1;
            ++pos;
        }
        while (pos != siz && ipos != isiz && ordered[pos] == indexes[ipos])
            ++pos, ++ipos;
    }

    std::sort(ordered.begin(), ordered.end());

    for (int pos = 0, next = pos + 1, siz = ordered.size(); pos != siz; ++next)
    {
        if (ordered[pos] == -1)
        {
            ++pos;
            continue;
        }

        if (next == siz || ordered[next] - ordered[next - 1] != 1)
        {
            emit dataChanged(index(ordered[pos], 0), index(ordered[next - 1], columnCount() - 1));
            pos = next;
        }
    }
}

void StudyListModel::fillList(std::vector<int> &dest)
{
    dest.clear();
    int cnt = mode == DeckItemViewModes::Queued ? deck->queueSize() : mode == DeckItemViewModes::Studied ? deck->studySize() : deck->testedSize();

    if (kanjion && kanaon && defon)
    {
        dest.reserve(cnt);
        while (dest.size() != cnt)
            dest.push_back(mode == DeckItemViewModes::Tested ? deck->testedIndex(dest.size()) : dest.size());
    }
    else
    {
        for (int ix = 0; ix != cnt; ++ix)
        {
            if ((mode == DeckItemViewModes::Queued && matchesFilter(deck->queuedItems(ix)->questiontype)) ||
                (mode == DeckItemViewModes::Studied && matchesFilter(deck->studiedItems(ix)->questiontype)) ||
                (mode == DeckItemViewModes::Tested && matchesFilter(deck->testedItems(ix)->questiontype)))
            {
                dest.push_back(mode == DeckItemViewModes::Tested ? deck->testedIndex(ix) : ix);
            }
        }
    }
}

bool StudyListModel::matchesFilter(WordPartBits part) const
{
    return (kanjion && part == WordPartBits::Kanji) || (kanaon && part == WordPartBits::Kana) || (defon && part == WordPartBits::Definition);
}

QString StudyListModel::shortHintString(WordDeckItem *item) const
{

    WordParts val = item->mainhint;
    if (val == WordParts::Default)
        val = deck->resolveMainHint(item);

    switch (val)
    {
    case WordParts::Kanji:
        return tr("W");
    case WordParts::Kana:
        return tr("K");
    case WordParts::Definition:
        return tr("D");
    default:
        return QStringLiteral("-");
    }
}

QString StudyListModel::shortQuestionString(WordDeckItem *item) const
{
    switch (item->questiontype)
    {
    case WordPartBits::Kanji:
        return tr("W");
    case WordPartBits::Kana:
        return tr("K");
    case WordPartBits::Definition:
        return tr("D");
    default:
        return QStringLiteral("-");
    }
}

QString StudyListModel::definitionString(int index) const
{
    const fastarray<WordDefinition> &defs = deck->dictionary()->wordEntry(index)->defs;
    int cnt = defs.size();

    QString result;

    for (int ix = 0; ix != cnt; ++ix)
    {
        result += defs[ix].def.toQString();
        if (ix != cnt - 1)
            result += QStringLiteral("; ");
    }

    return result;
}


//-------------------------------------------------------------

