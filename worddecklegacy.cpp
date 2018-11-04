/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QDataStream>

#include "worddeck.h"
#include "zkanjimain.h"
#include "words.h"


// Class with the only purpose to skip TextSearchTreeBase loading in a stream.
class SkipTreeInStream
{
public:
    SkipTreeInStream(int version, bool blacklisting = false);
    void skip(QDataStream &stream) const;
private:
    void skipNode(QDataStream &stream) const;
    int version;
    bool blacklisting;
};

QDataStream& operator>>(QDataStream& stream, const SkipTreeInStream &);
void skipTreeInStream(QDataStream& stream, int version, bool blacklisting = false);


//-------------------------------------------------------------


SkipTreeInStream::SkipTreeInStream(int version, bool blacklisting) : version(version), blacklisting(blacklisting)
{

}

void SkipTreeInStream::skip(QDataStream &stream) const
{
    qint32 nodecnt = 0;

    if (version <= 1)
    {
        int nversion = 4;
        char tmp[6];
        tmp[4] = 0;
        stream.readRawData(tmp, 4);

        if (strcmp(tmp, "zsct")) // Version above 4 - added 2011.04.04
            throw "ERROR: Not supported"; // Replace these with something useful.

        stream.readRawData(tmp, 3);
        tmp[3] = 0;
        nversion = strtol(tmp, 0, 10);

        if (nversion < 5)
            throw "Error, using throw!";
        stream >> nodecnt;
    }

    if (version >= 2)
    {
        quint16 cnt;
        stream >> cnt;
        nodecnt = cnt;
    }

    for (int ix = 0; ix < nodecnt; ++ix)
    {
        //nodes.addNode();

        //nodes.items(ix)->parent = nullptr;
        //nodes.items(ix)->load(stream, version, version <= 1);
        skipNode(stream);
    }

    //if (version <= 1)
    //    nodes.sort();

    if (version >= 2 && !blacklisting)
        return;

    //int bcnt;
    if (version <= 1)
    {
        quint8 blcnt;
        stream >> blcnt;

        //bcnt = blcnt;
        if (!blacklisting && blcnt != 0)
            throw "Cannot have skipped words in a non-blacklisting tree!"; // Do something with throws already.

    }
    else
    {
        quint32 blcnt;
        stream >> blcnt;

        //bcnt = blcnt;
    }
}

void SkipTreeInStream::skipNode(QDataStream &stream) const
{
    quint32 cnt;

    //label.clear();
    QCharString tmp;
    stream >> make_zstr(tmp, ZStrFormat::Byte);

    stream >> cnt;
    int sum = cnt;

    //#ifdef _DEBUG
    //    if (!lines.empty())
    //        throw "THIS IS NOT AN ERROR, REMOVE THIS AND THE LINE ABOVE IF WE HIT THIS AND IT TURNS OUT TO BE USEFUL!";
    //#endif

    //lines.reserve(cnt);

    //for (int ix = 0; ix < sum; ++ix)
    //{
    //    qint32 val;
    //    stream >> val;
    //    lines.push_back(val);
    //}
    stream.skipRawData(sizeof(qint32) * sum);

    if (version <= 1)
        stream >> cnt;
    else
    {
        quint16 u16;
        stream >> u16;
        cnt = u16;
    }

    //nodes.reserve(cnt);
    for (quint32 ix = 0; ix != cnt; ++ix)
    {
        //TextNode *n = new TextNode(this);
        //nodes.addNode(n, false);

        skipNode(stream);
        //n->load(stream, version, oldver);
        //sum += n->sum;
    }

    //if (oldver)
    //    nodes.sort();
}

QDataStream& operator>>(QDataStream& stream, const SkipTreeInStream &tree)
{
    tree.skip(stream);
    return stream;
}

void skipTreeInStream(QDataStream& stream, int version, bool blacklisting)
{
    SkipTreeInStream tree(version, blacklisting);
    stream >> tree;
}


//-------------------------------------------------------------


template<typename T> void  WordDeckItems<T>::loadLegacy(QDataStream &stream)
{
    //base::load(stream, 1);

    double d;
    qint32 cnt;
    qint32 val;
    stream >> cnt;

    list.reserve(cnt);

    item_type *item;
    for (int ix = 0; ix != cnt; ++ix)
    {
        item = new item_type;
        list.push_back(item);

        stream >> d;

        item->added = ZKanji::QDateTimeUTCFromTDateTime(d);

        stream >> val;
        WordParts questiontype = (WordParts)val;

        // Fix for an error in some corrupted data.
        if ((uint)val > (uint)WordParts::Definition)
            questiontype = WordParts::Kanji;

        stream >> val;
        item->mainhint = (WordParts)val;

        // Fix for an error in some corrupted data.
        if ((uint)val > (uint)WordParts::Default)
            item->mainhint = WordParts::Default;

        switch (questiontype)
        {
        case WordParts::Kanji:
            item->questiontype = WordPartBits::Kanji;
            break;
        case WordParts::Kana:
            item->questiontype = WordPartBits::Kana;
            break;
        case WordParts::Definition:
            item->questiontype = WordPartBits::Definition;
            break;
        default:
            break;
        }

        stream >> val;
        item->data = owner->wordData(val);
        item->data->types |= (int)item->questiontype;

        loadItemLegacy(stream, item, ix);
        //if (ftype == witFree)
        //    fread(&((TFreeWordStudyItem*)item)->priority, sizeof(byte), 1, f);
    }

    //if (ftype == witLocked && fowner->studyid)
    //{
    //    TRepetitionListBase* tmplist = fowner->category->fetch_study(fowner->studyid);
    //    if (list->Count != tmplist->Count)
    //        THROW(L"Error in database! Bug!");
    //    for (int ix = 0; ix < tmplist->Count; ++ix)
    //    {
    //        tmplist->Items[ix]->pt = list->Items[ix];
    //        ((TLockedWordStudyItem*)list->Items[ix])->ritem = tmplist->Items[ix];
    //        if (((TLockedWordStudyItem*)list->Items[ix])->def->repgroup == nullptr)
    //            ((TLockedWordStudyItem*)list->Items[ix])->def->repgroup = ((TLockedWordStudyItem*)list->Items[ix])->ritem;
    //    }
    //}
}

template<>
void WordDeckItems<FreeWordDeckItem>::loadItemLegacy(QDataStream &stream, item_type *item, int /*index*/)
{
    uchar b;
    stream >> b;
    item->priority = b;
}

template<>
void WordDeckItems<LockedWordDeckItem>::loadItemLegacy(QDataStream &/*stream*/, item_type *item, int /*index*/)
{
    item->cardid = nullptr;

    //StudyDeck *deck = owner->getStudyDeck();
    //if (deck == nullptr)
    //    return;

    //item->cardid = deck->cardId(index);
    //deck->setCardData(item->cardid, (intptr_t)item);

    //if (item->data->groupid == nullptr)
    //    item->data->groupid = item->cardid;
}

void ReadingTestList::loadLegacy(QDataStream &stream)
{
    qint32 cnt;
    stream >> cnt;
    list.reserve(cnt);

    qint32 val;
    qint8 b;

    bool bb;

    for (int ix = 0; ix != cnt; ++ix)
    {
        KanjiReadingItem *item = new KanjiReadingItem;
        stream >> val;
        item->kanjiindex = val;
        stream >> b;
        item->reading = b;

        qint32 lcnt;

        stream >> lcnt;
        item->words.reserve(lcnt);

        for (int iy = 0; iy != lcnt; ++iy)
        {
            KanjiReadingWord *wrd = new KanjiReadingWord;
            item->words.push_back(wrd);
            stream >> val;
            wrd->windex = owner->studiedItems(val)->data->index;
            stream >> b;
            //wrd->kanjipos = b;
            stream >> bb;
            wrd->newword = bb;
            stream >> bb;
            wrd->failed = bb;
        }
        list.push_back(item);
    }


    stream >> cnt;

    //tested.reserve(cnt);
    for (int ix = 0; ix != cnt; ++ix)
    {
        //TestedKanjiReading *item = new TestedKanjiReading;
        stream >> val;
        //item->kanjiindex = val;
        stream >> b;
        //item->reading = b;

        stream >> val;
        //item->itemindex = val;

        //tested.push_back(item);
        words.insert(owner->studiedItems(val)->data->index);
    }

    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        for (int iy = 0, sizy = tosigned(list[ix]->words.size()); iy != sizy; ++iy)
            words.insert(list[ix]->words[iy]->windex);
    }
}

void WordDeck::loadLegacy(QDataStream &stream)
{
    // The original zkanji only had 1 study list and no name was saved with it.
    name = qApp->translate("", "Deck %1").arg(1);

    double d;
    //quint32 uval;
    qint32 val;
    QString def;

    owner()->dictionary()->studyDecks()->removeDeck(deckid);

    deckid = owner()->dictionary()->studyDecks()->skipIdLegacy(stream);
    //stream >> deckid;

    stream >> val;
    lastday = ZKanji::QDateTimeUTCFromTDateTime(val).date();
    stream >> val;
    lastcnt = val;

    qint32 cnt;
    stream >> cnt;
    for (int ix = 0; ix != cnt; ++ix)
    {
        WordDeckWord *data = new WordDeckWord;
        list.push_back(data);

        quint8 b;
        stream >> val;
        data->index = val;
        stream >> b;
        // This is filled when loading the freeitems and lockitems.
        data->types = 0;// b;

        stream >> d;
        data->lastinclude = ZKanji::QDateTimeUTCFromTDateTime(d);

        stream >> make_zstr(def, ZStrFormat::Word);

        dictionary()->setWordStudyDefinition(data->index, def);

        //stream >> make_zstr(data->definition, ZStrFormat::Word);

        data->groupid = nullptr;
    }

    // Temporary skipping of tree data from old formats.
    SkipTreeInStream skip(1);
    stream >> skip;

    freeitems.loadLegacy(stream);

    // Temporary skipping of tree data from old formats.
    stream >> skip;

    lockitems.loadLegacy(stream);

    StudyDeck *deck = studyDeck();
    if (deck != nullptr)
    {
        deck->fixResizeCardId(tosigned(lockitems.size()));
        for (int ix = 0, siz = tosigned(lockitems.size()); ix != siz; ++ix)
        {
            LockedWordDeckItem *item = lockitems.items(ix);
            item->cardid = deck->cardId(ix);
            deck->setCardData(item->cardid, (intptr_t)item);

            if (item->data->groupid == nullptr)
                item->data->groupid = item->cardid;
        }
    }

    testreadings.loadLegacy(stream);

    std::sort(list.begin(), list.end(), [](WordDeckWord *a, WordDeckWord *b){
        return a->index < b->index;
    });

    duelist.reserve(lockitems.size());
    for (int ix = 0, siz = tosigned(lockitems.size()); ix != siz; ++ix)
        duelist.push_back(ix);

    std::vector<int> ints;

    stream >> cnt;
    failedlist.reserve(cnt);
    for (int ix = 0; ix != cnt; ++ix)
    {
        stream >> val;
        ints.push_back(val);
        failedlist.push_back(val);
    }

    std::sort(ints.begin(), ints.end());

    for (int ix = tosigned(ints.size()) - 1; ix != -1; --ix)
        duelist.erase(duelist.begin() + ints[ix]);

    sortDueList();
#ifdef _DEBUG
    checkDueList();
#endif

    stream >> val;
    newcnt = val;

    //fread(&newcount, sizeof(int), 1, f);

//    if (lockitems.size() && !deck->dayStats().empty() && deck->dayStats().back().itemcount != lockitems.size())
//#error is this the fixDayStats we are looking for? it was just rewritten.
//        deck->fixDayStats();

    // Remove unused word definitions (Shouldn't do anything after v0.73 groupfile version 17).
    for (int ix = tosigned(list.size()) - 1; ix != -1; --ix)
        if (list[ix]->types == 0)
            list.erase(list.begin() + ix);

}
