/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QThreadPool>
#include <QSet>
#include <chrono>
#include <thread>
#include "words.h"
#include "kanji.h"
#include "worddeck.h"
#include "zkanjimain.h"
#include "romajizer.h"
#include "zui.h"
#include "furigana.h"
#include "studysettings.h"
#include "ranges.h"
//#include "groupstudy.h"

#include "checked_cast.h"


//-------------------------------------------------------------


QDataStream& operator<<(QDataStream &stream, const WordDeckWord &w)
{
    stream << (qint32)w.index;
    stream << (quint8)w.types;
    stream << make_zdate(w.lastinclude);
    //stream << *w.groupid;

    return stream;
}

QDataStream& operator>>(QDataStream &stream, WordDeckWord &w)
{
    qint32 i;
    quint8 b;
    stream >> i;
    w.index = i;
    stream >> b;
    w.types = b;
    stream >> make_zdate(w.lastinclude);
    //stream >> *w.groupid;

    return stream;
}


//-------------------------------------------------------------


//-------------------------------------------------------------


template<typename T>
WordDeckItems<T>::WordDeckItems(WordDeck *owner) : /*base(/-*false,*-/ false),*/ owner(owner)
{

}

template<typename T>
void WordDeckItems<T>::load(QDataStream &stream)
{
    StudyDeck *study = owner->getStudyDeck();
    qint32 i;
    stream >> i;
    list.reserve(i);
    for (int ix = 0; ix != i; ++ix)
    {
        list.push_back(new T);
        loadItem(stream, list[ix], study);
    }
}

template<typename T>
void WordDeckItems<T>::clear()
{
    list.clear();
}

template<typename T>
bool WordDeckItems<T>::empty() const
{
    return list.empty();
}

template<typename T>
void WordDeckItems<T>::applyChanges(const std::map<int, int> &changes, const std::map<int, WordDeckWord*> &mainword, const std::map<WordDeckWord*, int> &kept)
{
    auto it = list.begin();
    while (it != list.end())
    {
        auto keptit = kept.find((*it)->data);
        
        int question = (int)(*it)->questiontype;
        if (keptit == kept.end() || (keptit->second & question) == 0)
            it = list.erase(it);
        else
        {
            int newindex = changes.at(keptit->first->index);
            (*it)->data = mainword.at(newindex);
            ++it;
        }
    }
    //reexpand();
}

template<typename T>
void WordDeckItems<T>::copy(WordDeckItems<T> *src, const std::map<CardId*, CardId*> &cardmap, const std::map<WordDeckWord*, WordDeckWord*> &wordmap)
{
    if (src == this)
        return;

    list.clear();
    list.reserve(src->list.size());
    for (int ix = 0, siz = tosigned(src->list.size()); ix != siz; ++ix)
    {
        item_type *i = new item_type;
        *i = *src->list[ix];
        i->data = wordmap.find(src->list[ix]->data)->second;
        list.push_back(i);

        _copy(i, src->list[ix], cardmap);
    }
}

template<typename T>
int WordDeckItems<T>::add(T *item)
{
    //expand(owner->wordRomaji(item->data->index), list.size(), false);
    list.push_back(item);
    return tosigned(list.size()) - 1;
}

template<typename T>
void WordDeckItems<T>::remove(int index)
{
    //removeLine(index, true);
    list.erase(list.begin() + index);
}

template<typename T>
int WordDeckItems<T>::indexOf(T* item) const
{
    auto it = std::find(list.begin(), list.end(), item);
    if (it == list.end())
        return -1;
    return it - list.begin();
}

template<>
void WordDeckItems<FreeWordDeckItem>::_copy(FreeWordDeckItem * /*item*/, FreeWordDeckItem * /*srcitem*/, const std::map<CardId*, CardId*> &/*cardmap*/)
{
    ;
}
template<>

void WordDeckItems<LockedWordDeckItem>::_copy(LockedWordDeckItem *item, LockedWordDeckItem *srcitem, const std::map<CardId*, CardId*> &cardmap)
{
    item->cardid = cardmap.find(srcitem->cardid)->second;
    if (item->data->groupid == nullptr)
        item->data->groupid = item->cardid;
}

//template<typename T>
//bool WordDeckItems<T>::isKana() const
//{
//    return true;
//}
//
//template<typename T>
//bool WordDeckItems<T>::isReversed() const
//{
//    return false;
//}

template<>
void WordDeckItems<FreeWordDeckItem>::loadItem(QDataStream &stream, FreeWordDeckItem *item, StudyDeck * /*study*/)
{
    owner->loadFreeDeckItem(stream, item);
}

template<>
void WordDeckItems<LockedWordDeckItem>::loadItem(QDataStream &stream, LockedWordDeckItem *item, StudyDeck *study)
{
    owner->loadLockedDeckItem(stream, item, study);
}

template<>
void WordDeckItems<FreeWordDeckItem>::saveItem(QDataStream &stream, const FreeWordDeckItem *item, StudyDeck * /*study*/) const
{
    owner->saveFreeDeckItem(stream, item);
}

template<>
void WordDeckItems<LockedWordDeckItem>::saveItem(QDataStream &stream, const LockedWordDeckItem *item, StudyDeck *study) const
{
    owner->saveLockedDeckItem(stream, item, study);
}

template<>
void WordDeckItems<FreeWordDeckItem>::save(QDataStream &stream) const
{
    qint32 lsiz = tosigned(list.size());
    stream << (qint32)lsiz;
    for (int ix = 0; ix != lsiz; ++ix)
        saveItem(stream, list[ix], nullptr);
}

template<>
void WordDeckItems<LockedWordDeckItem>::save(QDataStream &stream) const
{
    StudyDeck *study = owner->getStudyDeck();
    qint32 lsiz = tosigned(list.size());
    stream << (qint32)lsiz;
    for (int ix = 0; ix != lsiz; ++ix)
        saveItem(stream, list[ix], study);
}


//-------------------------------------------------------------


QDataStream& operator<<(QDataStream &stream, const KanjiReadingWord &w)
{
    stream << (qint32)w.windex;
    stream << (qint8)w.newword;
    stream << (qint8)w.failed;

    return stream;
}

QDataStream& operator>>(QDataStream &stream, KanjiReadingWord &w)
{
    qint32 i;
    qint8 b;
    stream >> i;
    w.windex = i;
    stream >> b;
    w.newword = b;
    stream >> b;
    w.failed = b;

    return stream;
}

QDataStream& operator<<(QDataStream &stream, const KanjiReadingItem &r)
{
    qint32 wsiz = tosigned(r.words.size());

    stream << (qint32)wsiz;
    for (int ix = 0; ix != wsiz; ++ix)
        stream << *r.words[ix];
    stream << (qint32)r.kanjiindex;
    stream << (quint8)r.reading;

    return stream;
}

QDataStream& operator>>(QDataStream &stream, KanjiReadingItem &r)
{
    qint32 i;
    qint8 b;
    stream >> i;
    r.words.reserve(i);
    for (int ix = 0; ix != i; ++ix)
    {
        KanjiReadingWord *w = new KanjiReadingWord;
        stream >> *w;
        r.words.push_back(w);
    }
    stream >> i;
    r.kanjiindex = i;
    stream >> b;
    r.reading = b;

    return stream;
}


//-------------------------------------------------------------


ReadingTestList::ReadingTestList(WordDeck *owner) : owner(owner), undoindex(-1)
{

}

void ReadingTestList::load(QDataStream &stream)
{
    undoindex = -1;

    qint32 i;
    stream >> i;
    list.reserve(i);
    for (int ix = 0; ix != i; ++ix)
    {
        list.push_back(new KanjiReadingItem);
        stream >> *list.back();
    }
    stream >> i;
    for (int ix = 0; ix != i; ++ix)
    {
        qint32 val;
        stream >> val;
        words.insert(val);
    }
}

void ReadingTestList::save(QDataStream &stream) const
{
    qint32 lsiz = tosigned(list.size());
    stream << (qint32)lsiz;
    for (int ix = 0; ix != lsiz; ++ix)
        stream << *list[ix];
    stream << (qint32)tosigned(words.count());
    for (int i : words)
        stream << (qint32)i;
}

void ReadingTestList::clear()
{
    list.clear();
    words.clear();
    undoindex = -1;
}

bool ReadingTestList::empty() const
{
    return list.empty();
}

ReadingTestList::size_type ReadingTestList::size() const
{
    return list.size();
}

void ReadingTestList::applyChanges(const std::map<int, int> &changes, const std::map<int, WordDeckWord*> &mainword, const std::map<int, bool> &match)
{
    undoindex = -1;
    words.clear();
    auto it = list.begin();
    while (it != list.end())
    {
        smartvector<KanjiReadingWord> &itwords = (*it)->words;
        auto it2 = itwords.begin();
        while (it2 != itwords.end())
        {
            int newindex = changes.at((*it2)->windex);
            if (newindex == -1 || !match.at(newindex) || mainword.at(newindex)->index != (*it2)->windex)
                it2 = itwords.erase(it2);
            else
            {
                words.insert(newindex);
                (*it2)->windex = newindex;
                ++it2;
            }
        }
        if (itwords.empty())
            it = list.erase(it);
        else
            ++it;
    }
}

void ReadingTestList::copy(ReadingTestList *src)
{
    words = src->words;
    list.clear();
    list.reserve(src->list.size());
    for (int ix = 0, siz = tosigned(src->list.size()); ix != siz; ++ix)
    {
        KanjiReadingItem *item = new KanjiReadingItem;
        KanjiReadingItem *srcitem = src->list[ix];
        list.push_back(item);
        item->kanjiindex = srcitem->kanjiindex;
        item->reading = srcitem->reading;

        for (int iy = 0, siy = tosigned(srcitem->words.size()); iy != siy; ++iy)
        {
            KanjiReadingWord *kw = new KanjiReadingWord;
            *kw = *srcitem->words[iy];
            item->words.push_back(kw);
        }
    }
}

void ReadingTestList::processRemovedWord(int windex)
{
    undoindex = -1;

    QSet<int> tmp;
    std::swap(tmp, words);
    for (auto it = tmp.begin(); it != tmp.end(); ++it)
    {
        int index = *it;
        if (index < windex)
            words.insert(index);
        else if (index > windex)
            words.insert(index - 1);
    }

    for (int ix = tosigned(list.size()) - 1; ix != -1; --ix)
    {
        KanjiReadingItem *item = list[ix];
        for (int iy = tosigned(item->words.size()); iy != -1; --iy)
        {
            int &wix = item->words[iy]->windex;
            if (wix == windex)
                item->words.erase(item->words.begin() + iy);
            else if (wix > windex)
                --wix;
        }
        if (item->words.empty())
            list.erase(list.begin() + ix);
    }
}

void ReadingTestList::add(int windex, bool newitem, bool failed, bool undo)
{
    if (undo && undoindex == windex)
    {
        if (!undoexists)
            words.remove(windex);

        for (std::pair<KanjiReadingItem*, std::unique_ptr<KanjiReadingItem>> &p : undolist)
        {
            auto it = std::find(list.begin(), list.end(), p.first);
            list.erase(it);
            if (p.second != nullptr)
                list.push_back(p.second.release());
        }
        undolist.clear();
    }

    undoindex = -1;

    if (Settings::study.readings == StudySettings::None ||
        (Settings::study.readingsfrom == StudySettings::Both && !newitem && !failed) ||
        (Settings::study.readingsfrom == StudySettings::NewOnly && !newitem) ||
        (Settings::study.readingsfrom == StudySettings::MistakeOnly && !failed))
        return;

    bool wordadded = words.contains(windex);
    undoindex = windex;
    undoexists = wordadded;
    undolist.clear();
    if (!newitem && !failed && wordadded)
        return;
    
    WordEntry *e = owner->dictionary()->wordEntry(windex);

    // The furigana of the word is looked up to avoid calling it every time
    // findKanjiReading() is called.
    std::vector<FuriganaData> fdat;
    findFurigana(e->kanji, e->kana, fdat);

    int len = e->kanji.size();

    QSet<std::pair<int, int>> found;
    for (int ix = 0; ix != len; ++ix)
    {
        if (!KANJI(e->kanji[ix].unicode()))
            continue;

        // Look up the kanji and its reading.
        int k = ZKanji::kanjiIndex(e->kanji[ix]);
        int r = findKanjiReading(e->kanji, e->kana, ix, ZKanji::kanjis[k], &fdat);

        // Not a reading from ON or KUN, so we can't test it.
        if (r == 0 || (Settings::study.readings == StudySettings::ON && r > tosigned(ZKanji::kanjis[k]->on.size())) || (Settings::study.readings == StudySettings::Kun && r <= tosigned(ZKanji::kanjis[k]->on.size())) || found.contains(std::make_pair(k, r)))
            continue;

        found.insert(std::make_pair(k, r));

        // Search the kanji/reading in the items already in the readings list.
        int rix = -1;
        for (int iy = 0, sizy = tosigned(list.size()); iy != sizy && rix == -1; ++iy)
            if (list[iy]->kanjiindex == k && list[iy]->reading == r)
                rix = iy;

        KanjiReadingItem *item;
        if (rix == -1)
        {
            // Check if word has been added before and don't add it again.
            if (wordadded)
                continue;

            // Kanji/reading not found. Add as new item.
            item = new KanjiReadingItem;
            item->kanjiindex = k;
            item->reading = r;
            list.push_back(item);

            // Create undo item.
            undolist.push_back(std::pair<KanjiReadingItem*, std::unique_ptr<KanjiReadingItem>>(item, nullptr));
        }
        else
        {
            item = list[rix];

            // Only add item to undolist if it does not exist already.
            auto it = std::find_if(undolist.begin(), undolist.end(), [item](const std::pair<KanjiReadingItem*, std::unique_ptr<KanjiReadingItem>> &p) { return p.first == item; });
            if (it == undolist.end())
            {
                // Create undo item.
                undolist.push_back(std::pair<KanjiReadingItem*, std::unique_ptr<KanjiReadingItem>>(item, std::unique_ptr<KanjiReadingItem>(new KanjiReadingItem)));
                KanjiReadingItem *old = undolist.back().second.get();
                // Copy the item to old so it can be restored.
                old->kanjiindex = item->kanjiindex;
                old->reading = item->reading;
                for (auto w : item->words)
                    old->words.push_back(new KanjiReadingWord(*w));
            }
        }

        for (int iy = 0, sizy = tosigned(item->words.size()); wordadded && iy != sizy; ++iy)
        {
            if (item->words[iy]->windex != windex)
                continue;

            // The same word has already been included in the test for/ today, but it's not
            // tested yet. Update how to display it when it will be tested.
            item->words[iy]->newword = item->words[iy]->newword || newitem;
            item->words[iy]->failed = item->words[iy]->newword || failed;
            break;
        }

        if (wordadded)
            continue;

        KanjiReadingWord *kreading = new KanjiReadingWord;
        kreading->windex = windex;
        //kreading->kanjipos = ix;
        kreading->newword = newitem;
        kreading->failed = failed;

        item->words.push_back(kreading);
    }

    // If the user already tested the reading of the word, the list of readings to test won't
    // hold it any more, but the words list will. This set of words is reset on each test day.
    if (!wordadded)
        words.insert(windex);

    owner->dictionary()->setToUserModified();
}

void ReadingTestList::removeWord(int windex)
{
    auto it = std::find(words.begin(), words.end(), windex);
    bool changed = true;

    if (it != words.end())
    {
        words.erase(it);
        changed = true;
    }

    for (int ix = tosigned(list.size()) - 1; ix != -1; --ix)
    {
        KanjiReadingItem *item = list[ix];
        for (int iy = tosigned(item->words.size()) - 1; iy != -1; --iy)
            if (item->words[iy]->windex == windex)
            {
                changed = true;
                item->words.erase(item->words.begin() + iy);
                break;
            }

        if (item->words.empty())
        {
            list.erase(list.begin() + ix);
            changed = true;
        }
    }

    if (changed)
        owner->dictionary()->setToUserModified();
}

int ReadingTestList::nextKanji()
{
#ifdef _DEBUG
    if (list.empty())
        throw "no kanji to test";
#endif

    return list.front()->kanjiindex;
}

uchar ReadingTestList::nextReading()
{
    return list.front()->reading;
}

bool ReadingTestList::readingMatches(QString answer)
{
    KanjiEntry *k = ZKanji::kanjis[list.front()->kanjiindex];
    uchar r = list.front()->reading;

    // First try with the naive method, assuming the given answer is exact.

    answer = hiraganize(answer);
    if (answer.isEmpty())
        return false;
    if (answer[answer.size() - 1].unicode() == MINITSU)
        answer[answer.size() - 1] = QChar(HIRATSU);
    QString reading = hiraganize(correctReading());
    if (reading[answer.size() - 1].unicode() == MINITSU)
        reading[answer.size() - 1] = QChar(HIRATSU);

    if (answer == reading)
        return true;

    // Answer is not exact match, but a word might have the correct furigana.
    std::vector<FuriganaData> fdat;

    for (int ix = 0, siz = tosigned(list.front()->words.size()); ix != siz; ++ix)
    {
        WordEntry *w = owner->dictionary()->wordEntry(list.front()->words[ix]->windex);
        findFurigana(w->kanji, w->kana, fdat);

        for (int iy = 0, sizy = tosigned(w->kanji.size()); iy != sizy; ++iy)
        {
            // Stepping through every character and trying to find the kanji being
            // tested. Its reading should also be the same as the current one.
            if (w->kanji[iy] != k->ch)
                continue;

            int fpos = 0;
            int fsiz = tosigned(fdat.size());
            for ( ; fpos != fsiz && iy > tosigned(fdat[fpos].kanji.pos); ++fpos)
                ;
            if (fpos == fsiz || tosigned(fdat[fpos].kanji.pos) != iy || fdat[fpos].kanji.len != 1)
                continue;

            if (findKanjiReading(w->kanji, w->kana, iy, k, &fdat) != r)
                continue;

            reading = hiraganize(w->kana.toQString(fdat[fpos].kana.pos, fdat[fpos].kana.len));

            if (reading[answer.size() - 1].unicode() == MINITSU)
                reading[answer.size() - 1] = QChar(HIRATSU);

            if (answer == reading)
                return true;
        }
    }

    return false;
}

QString ReadingTestList::correctReading()
{
    KanjiEntry *k = ZKanji::kanjis[list.front()->kanjiindex];
    uchar r = list.front()->reading;

    --r;
    if (r < k->on.size())
    {
        int mindif = k->on[r][0] == '-' ? 1 : 0;
        return k->on[r].toQString(mindif, k->on[r].size() - mindif);
    }

    r -= k->on.size();

    return k->kun[r].toQString(0, kunLen(k->kun[r].data()));
}

void ReadingTestList::readingAnswered()
{
    list.erase(list.begin());
    owner->dictionary()->setToUserModified();
}

void ReadingTestList::nextWords(std::vector<int> &wlist)
{
    wlist.clear();
    wlist.reserve(list.front()->words.size());
    for (int ix = 0, siz = tosigned(list.front()->words.size()); ix != siz; ++ix)
        wlist.push_back(list.front()->words[ix]->windex);
}


//-------------------------------------------------------------


WordDeckList::WordDeckList(Dictionary *dict) : base(dict), dict(dict), lastdeck(nullptr)
{
    //list.push_back(new WordDeck(this));
}

WordDeckList::~WordDeckList()
{
    ;
}

void WordDeckList::load(QDataStream &stream)
{
    clear();

    qint32 i;
    stream >> i;
    list.reserve(i);
    for (int ix = 0; ix != i; ++ix)
    {
        list.push_back(new WordDeck(this));
        list.back()->load(stream);
    }
}

void WordDeckList::save(QDataStream &stream) const
{
    stream << (qint32)list.size();
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        list[ix]->save(stream);
}

void WordDeckList::clear()
{
    list.clear();
}

void WordDeckList::applyChanges(Dictionary *olddict, std::map<int, int> &changes)
{
    //dict = newdict;

    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        list[ix]->applyChanges(olddict, changes);
}

void WordDeckList::copy(WordDeckList *src)
{
    if (this == src)
        return;

    list.clear();
    list.reserve(src->list.size());
    for (int ix = 0, siz = tosigned(src->list.size()); ix != siz; ++ix)
    {
        list.push_back(new WordDeck(this));
        list.back()->copy(src->list[ix]);
    }
}

Dictionary* WordDeckList::dictionary()
{
    return dict;
}

void WordDeckList::processRemovedWord(int windex)
{
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        list[ix]->processRemovedWord(windex);
}

WordDeckList::size_type WordDeckList::size() const
{
    return list.size();
}

bool WordDeckList::empty() const
{
    return list.empty();
}

WordDeck* WordDeckList::items(int index)
{
    return list[index];
}

WordDeck* WordDeckList::lastSelected() const
{
    if (lastdeck != nullptr || list.empty())
        return lastdeck;
    return list[0];
}

void WordDeckList::setLastSelected(const QString &deckname)
{
    int ix = indexOf(deckname);
    if (ix == -1)
        lastdeck = nullptr;
    else
        lastdeck = list[ix];
}

void WordDeckList::setLastSelected(WordDeck *deck)
{
    if (indexOf(deck) == -1)
        return;
    lastdeck = deck;
}

int WordDeckList::indexOf(WordDeck *deck) const
{
    auto it = std::find(list.begin(), list.end(), deck);
    if (it == list.end())
        return -1;
    return it - list.begin();
}

int WordDeckList::indexOf(const QString &name) const
{
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        if (list[ix]->getName() == name)
            return ix;
    return -1;
}

bool WordDeckList::rename(int index, const QString &name)
{
    QString val = name.trimmed().left(255);
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        if (list[ix]->getName() == val)
            return false;

    list[index]->setName(val);

    dict->setToUserModified();
    emit deckRenamed(list[index], val);
    return true;
}

bool WordDeckList::add(const QString &name)
{
    QString val = name.trimmed().left(255);
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        if (list[ix]->getName() == val)
            return false;

    list.push_back(new WordDeck(this));
    list.back()->setName(val);
    dict->setToUserModified();
    return true;
}

bool WordDeckList::nameTakenOrInvalid(const QString &name)
{
    QString val = name.trimmed().left(255);
    if (val.isEmpty())
        return true;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        if (list[ix]->getName() == val)
            return true;
    return false;
}

void WordDeckList::remove(int index)
{
    void *addr = list[index];
    emit deckToBeRemoved(index, list[index]);

    if (lastdeck == list[index])
        lastdeck = nullptr;

    list.erase(list.begin() + index);

    dict->setToUserModified();
    emit deckRemoved(index, addr);
}

void WordDeckList::move(const smartvector<Range> &ranges, int pos)
{
    if (ranges.empty())
        return;

    if (pos == -1)
        pos = tosigned(list.size());

    _moveRanges(ranges, pos, list /*[this](const Range &r, int pos) { _moveRange(worddecks, r, pos); }*/);

    dict->setToUserModified();
    emit decksMoved(ranges, pos);
}


//-------------------------------------------------------------


WordDeck::WordDeck(WordDeckList *owner) : base(owner), lastcnt(0), newcnt(0), freeitems(this), lockitems(this), testreadings(this)
{
    generatingnext = false;
    abortgenerating = false;

    deckid = owner->dictionary()->studyDecks()->createDeck();
}

WordDeck::~WordDeck()
{
    waitForNextItem();
    owner()->dictionary()->studyDecks()->removeDeck(deckid);
}

void WordDeck::load(QDataStream &stream)
{
    qint32 i;

    owner()->dictionary()->studyDecks()->removeDeck(deckid);

    stream >> make_zstr(name, ZStrFormat::Byte);
    stream >> deckid;
    stream >> make_zdate(lastday);

    stream >> i;
    lastcnt = i;
    stream >> i;
    newcnt = i;

    stream >> i;
    list.reserve(i);

    StudyDeck *study = studyDeck();

    for (int ix = 0; ix != i; ++ix)
    {
        list.push_back(new WordDeckWord);
        stream >> *list.back();
        
        list.back()->groupid = study->loadCardId(stream);
    }

    freeitems.load(stream);
    lockitems.load(stream);

    testreadings.load(stream);

    stream >> make_zvec<qint32, qint32>(duelist);
    stream >> make_zvec<qint32, qint32>(failedlist);
}

void WordDeck::save(QDataStream &stream) const
{
    waitForNextItem();

    stream << make_zstr(name, ZStrFormat::Byte);
    stream << deckid;
    stream << make_zdate(lastday);
    stream << (qint32)lastcnt;
    stream << (qint32)newcnt;

    stream << (qint32)list.size();

    const StudyDeck *study = studyDeck();
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        stream << *list[ix];
        study->saveCardId(stream, list[ix]->groupid);
    }

    freeitems.save(stream);
    lockitems.save(stream);

    testreadings.save(stream);

    stream << make_zvec<qint32, qint32>(duelist);
    stream << make_zvec<qint32, qint32>(failedlist);
}

void WordDeck::saveWordDeckItem(QDataStream &stream, const WordDeckItem *w) const
{
    stream << make_zdate(w->added);
    stream << (qint32)w->data->index;
    stream << (quint8)w->mainhint;
    stream << (quint8)w->questiontype;
}

void WordDeck::loadWordDeckItem(QDataStream &stream, WordDeckItem *w)
{
    qint32 i;
    quint8 b;
    stream >> make_zdate(w->added);
    stream >> i;
    w->data = wordFromIndex(i);
#ifdef _DEBUG
    if (w->data == nullptr)
        throw "No data for this index.";
#endif
    stream >> b;
    w->mainhint = (WordParts)b;
    stream >> b;
    w->questiontype = (WordPartBits)b;
}

void WordDeck::loadFreeDeckItem(QDataStream &stream, FreeWordDeckItem *w)
{
    loadWordDeckItem(stream, w);
    quint8 b;
    stream >> b;
    w->priority = b;
}

void WordDeck::saveFreeDeckItem(QDataStream &stream, const FreeWordDeckItem *w) const
{
    saveWordDeckItem(stream, w);
    stream << (quint8)w->priority;
}

void WordDeck::loadLockedDeckItem(QDataStream &stream, LockedWordDeckItem *w, StudyDeck *study)
{
    loadWordDeckItem(stream, w);
    w->cardid = study->loadCardId(stream);
    //stream >> *w->cardid;
    study->setCardData(w->cardid, (intptr_t)w);
}

void WordDeck::saveLockedDeckItem(QDataStream &stream, const LockedWordDeckItem *w, StudyDeck *study) const
{
    saveWordDeckItem(stream, w);
    //stream << *w->cardid;
    study->saveCardId(stream, w->cardid);
}

void WordDeck::setName(const QString &newname)
{
    name = newname.left(255).trimmed();
}

const QString& WordDeck::getName() const
{
    return name;
}

WordDeckList* WordDeck::owner() const
{
    return (WordDeckList*)parent();
}

WordDeckList* WordDeck::owner()
{
    return (WordDeckList*)parent();
}

Dictionary* WordDeck::dictionary()
{
    return owner()->dictionary();
}

bool WordDeck::empty() const
{
    return freeitems.empty() && lockitems.empty();
}

void WordDeck::applyChanges(Dictionary *olddict, std::map<int, int> &changes)
{
    // Changes are applied in multiple steps. The words are referenced by WordDeckWord objects
    // for the items in the deck. Multiple items can share data for a single word. The change
    // can merge multiple words into a single one, and in that case some items of other words
    // can be lost, but it's also possible they will be merged into the main word.
    // Steps for the update:
    // - Collecting every word data which will be kept in mainword, paired with the new word
    // index they will get after the dictionary update.
    // - Based on mainword, the readings test is updated. It's relatively easy because it only
    // depends on the words, not separate items in the deck.
    // - Figuring out which items to keep for which word data. The items' types are saved as
    // or-ed flag bits in an integer, paired with the word data in the 'kept' map. When word
    // data are merged, it's possible that an item of a given type will be taken from a word
    // data that will be deleted. If many word data of a single merged word contain an item of
    // the same type, only one of those items will be kept.
    // - Removing items not kept from duelist, failedlist and lastlist, and deleting their
    // associated study data.
    // - Deleting the items not kept from freelist and locklist.
    // - Deleting the words from the word data list not in mainword, and updating the word
    // indexes of words that were found in mainword.

    Dictionary *dict = owner()->dictionary();
    if (dict == olddict)
        return;

    StudyDeck *study = studyDeck();
    if (study == nullptr)
        return;

    // [new index, word data]. Used for joining multiple data when they have the same new
    // index. The data stored either refers to a word in the old dictionary with the exact
    // kanji/kana of a new dictionary word, or the old word with the lowest word index to be
    // merged into the new word.
    std::map<int, WordDeckWord*> mainword;
    // [new index, word match]. When the saved word data in 'mainword' has a matching
    // kanji/kana form in the new dictionary and the old dictionary, the item should have
    // priority over any other item with the same new index. When the value is true, a
    // prioritised word has already been found.
    std::map<int, bool> match;

    for (int ix = tosigned(list.size()) - 1; ix != -1; --ix)
    {
        int newindex = changes.at(list[ix]->index);
        if (newindex == -1)
            continue;
        auto it = mainword.find(newindex);
        if (it == mainword.end())
        {
            mainword.insert(std::make_pair(newindex, list[ix]));

            WordEntry *oldw = olddict->wordEntry(list[ix]->index);
            WordEntry *neww = dict->wordEntry(newindex);
            match.insert(std::make_pair(newindex, !qcharcmp(oldw->kanji.data(), neww->kanji.data()) && !qcharcmp(oldw->kana.data(), neww->kana.data())));
        }
        else if (!match.at(newindex))
        {
            WordEntry *oldw = olddict->wordEntry(list[ix]->index);
            WordEntry *neww = dict->wordEntry(newindex);
            bool matches = !qcharcmp(oldw->kanji.data(), neww->kanji.data()) && !qcharcmp(oldw->kana.data(), neww->kana.data());
            if (matches || it->second->index > list[ix]->index)
            {
                match[newindex] = matches;
                it->second = list[ix];
            }
        }
    }

    // Readings test is independent in many ways so it's safe to apply the changes there
    // first.
    testreadings.applyChanges(changes, mainword, match);

    // [word data, question type] Holds the types of items for each word data that will be
    // used. When word data are deleted because they differ from the main data of the same
    // word, 'kept' will store the types of items associated with them which won't get
    // deleted, just merged into the main word.
    std::map<WordDeckWord*, int> kept;

    // Temporary map that holds [new index, answer type] pairs that were already assigned. At
    // the end of the update, holds the type of answers for each item that will get new index.
    std::map<int, int> types;

    // Temporary vector used for retrieving every user data for a given card group.
    std::vector<intptr_t> datalist;

    // Figuring out which item types to keep for which word data. Types in the main word data
    // placed in 'mainword' have priority. Item types without study data might be kept too,
    // but the 'kept' map will only hold those types of items with study data at first.
    for (auto &p : mainword)
    {
        // Items with no study data are skipped here but are checked later.
        if (p.second->groupid == nullptr)
            continue;

        datalist.clear();
        study->groupData(p.second->groupid, datalist);

        int added = 0;
        for (intptr_t data : datalist)
        {
            LockedWordDeckItem *item = (LockedWordDeckItem*)data;
            added |= (int)item->questiontype;
        }
        kept.insert(std::make_pair(p.second, added));
        types.insert(std::make_pair(p.first, added));
    }

    // Adding more word data to the 'kept' map. Word data not in mainword can be associated
    // with items with study data. If the main saved word data from mainword doesn't have that
    // type of item, items of other word data are used.
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        int newindex = changes.at(list[ix]->index);

        // Word is not used in the new dictionary so it can be skipped.
        if (newindex == -1)
            continue;

        // Word data that will be kept.
        WordDeckWord *maindata = mainword.at(newindex);
        // Word data at the current index.
        WordDeckWord *posdata = list[ix];

        // Data in mainword has already been checked above.
        if (maindata == posdata || posdata->groupid == nullptr)
            continue;

        // Get study data for item types not in the main data.

        int &t = types[newindex];

        datalist.clear();
        study->groupData(posdata->groupid, datalist);

        int added = 0;
        for (int iy = 0, sizy = tosigned(datalist.size()); iy != sizy; ++iy)
        {
            LockedWordDeckItem *item = (LockedWordDeckItem*)datalist[iy];
            // The answer type of item wasn't found for this word before.
            if ((t & (int)item->questiontype) == 0)
                added |= (int)item->questiontype;
        }
        kept[posdata] = added;
        t |= added;
    }

    // Not yet studied (free) items are not in the kept list. Collect them, giving priority to
    // the main data of the words.
    for (auto &p : mainword)
    {
        int &t = types[p.first];

        int notused = (p.second->types & ~t);
        kept[p.second] |= notused;
        t |= notused;
    }

    // Do the same for the other word data not used after the update.
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        int newindex = changes.at(list[ix]->index);

        // New index is not set, so the word won't be used later.
        if (newindex == -1)
            continue;

        // Word data that will be kept.
        WordDeckWord *keptdata = mainword.at(newindex);
        // Word data with index ix.
        WordDeckWord *posdata = list[ix];

        // Data in mainword has already been checked above.
        if (keptdata == posdata)
            continue;

        int &t = types[newindex];

        int notused = (posdata->types & ~t);
        kept[posdata] |= notused;
        t |= notused;
    }

    // At this point 'kept' contains which type of items of which word data will still be in
    // use. They will be merged into words in mainword.
    freeitems.applyChanges(changes, mainword, kept);
    newcnt = std::min(newcnt, tosigned(freeitems.size()));

    // Indexes in lockitems that will be deleted after the changes are applied to it.
    std::vector<int> deleted;

    // Delete cards before applying changes, because the locklist doesn't know about them.
    // Also delete the items from duelist, failedlist and lastlist.
    auto it = duelist.begin();
    while (it != duelist.end())
    {
        LockedWordDeckItem *item = lockitems.items(*it);
        auto keptit = kept.find(item->data);
        if (keptit == kept.end() || (keptit->second & (int)item->questiontype) == 0)
        {
            if (item->data->groupid == item->cardid)
                item->data->groupid = study->deleteCard(item->cardid);
            else
                study->deleteCard(item->cardid);

            deleted.push_back(*it);
            it = duelist.erase(it);
        }
        else
            ++it;
    }
    it = failedlist.begin();
    while (it != failedlist.end())
    {
        LockedWordDeckItem *item = lockitems.items(*it);
        auto keptit = kept.find(item->data);
        if (keptit == kept.end() || (keptit->second & (int)item->questiontype) == 0)
        {
            if (item->data->groupid == item->cardid)
                item->data->groupid = study->deleteCard(item->cardid);
            else
                study->deleteCard(item->cardid);

            deleted.push_back(*it);
            it = failedlist.erase(it);
        }
        else
            ++it;
    }

    lockitems.applyChanges(changes, mainword, kept);

    // Decrementing indexes in duelist, failedlist and lastlist to match lockitems after the
    // change.
    std::sort(deleted.begin(), deleted.end());
    deleted.resize(std::unique(deleted.begin(), deleted.end()) - deleted.begin());
    for (int ix = tosigned(deleted.size()) - 1; ix != -1; --ix)
    {
        for (int dix = tosigned(duelist.size()) - 1; dix != -1; --dix)
        {
#ifdef _DEBUG
            if (duelist[dix] == deleted[ix])
                throw "This item should have been deleted.";
#endif
            if (duelist[dix] > deleted[ix])
                --duelist[dix];
        }
        for (int fix = tosigned(failedlist.size()) - 1; fix != -1; --fix)
        {
#ifdef _DEBUG
            if (failedlist[fix] == deleted[ix])
                throw "This item should have been deleted.";
#endif
            if (failedlist[fix] > deleted[ix])
                --failedlist[fix];
        }
    }

    // Merge card data from list that are not kept, with items in 'mainword' and delete the
    // word data.
    auto lit = list.begin();
    while (lit != list.end())
    {
        WordDeckWord *data = *lit;
        int newindex = changes[data->index];
        if (newindex == -1)
        {
#ifdef _DEBUG
            if (data->groupid != nullptr)
                throw "Data cardid should have been deleted above.";
#endif
            lit = list.erase(lit);
            continue;
        }

        WordDeckWord *mainitem = mainword[newindex];
        if (mainitem == data)
        {
            mainitem->index = newindex;
            mainitem->types = types[newindex];
            ++lit;
            continue;
        }
        if (mainitem->lastinclude < data->lastinclude)
            mainitem->lastinclude = data->lastinclude;

        if (mainitem->groupid && data->groupid)
            study->mergeGroups(mainitem->groupid, data->groupid);
        else 
            // One of them is null, their sum is the pointer which is not null.
            mainitem->groupid = data->groupid = (CardId*)((intptr_t)mainitem->groupid + (intptr_t)data->groupid);

        lit = list.erase(lit);
    }
    std::sort(list.begin(), list.end(), [](WordDeckWord *a, WordDeckWord *b){
        return a->index < b->index;
    });

#ifdef _DEBUG
    study->checkDayStats();
#endif
}

void WordDeck::copy(WordDeck *src)
{
    std::map<CardId*, CardId*> cardmap;
    getStudyDeck()->copy(src->getStudyDeck(), cardmap);

    name = src->name;
    lastday = src->lastday;
    lastcnt = src->lastcnt;
    newcnt = src->newcnt;
    duelist = src->duelist;
    failedlist = src->failedlist;

    // [Source word, Copy]
    std::map<WordDeckWord*, WordDeckWord*> wordmap;
    list.clear();
    list.reserve(src->list.size());
    for (int ix = 0, siz = tosigned(src->list.size()); ix != siz; ++ix)
    {
        WordDeckWord *w = new WordDeckWord;
        *w = *src->list[ix];
        w->groupid = nullptr;
        list.push_back(w);
        wordmap.insert(std::make_pair(src->list[ix], w));
    }

    freeitems.copy(&src->freeitems, cardmap, wordmap);
    lockitems.copy(&src->lockitems, cardmap, wordmap);
    testreadings.copy(&src->testreadings);

}

void WordDeck::processRemovedWord(int windex)
{
    // Removing word from the readings test. The readings test is not strongly connected to
    // the other data and can be updated.
    testreadings.processRemovedWord(windex);

    std::vector<int> fremoved;
    // Removing word from the freeitems list. The freeitems are items with no study data yet,
    // and only the basic word data.
    for (int ix = tosigned(freeitems.size()) - 1; ix != -1 && fremoved.size() != 3; --ix)
        if (freeitems.items(ix)->data->index == windex)
        {
            freeitems.remove(ix);
            fremoved.insert(fremoved.begin(), ix);
        }

    newcnt = std::min(newcnt, tosigned(freeitems.size()));

    std::vector<int> lremoved;

    // Find the positions of the word in lockitems, before removing it from duelist,
    // failedlist and lastlist.

    // Number of times the word was found in lockitems;
    int itemcnt = 0;
    int itempos[3] { 0, 0, 0 };

    for (int ix = 0, siz = tosigned(lockitems.size()); ix != siz && itemcnt != 3; ++ix)
    {
        if (lockitems.items(ix)->data->index == windex)
        {
            itempos[itemcnt] = ix;
            ++itemcnt;

            lremoved.push_back(ix);
        }
    }

    // Remove the items from duelist, failedlist and lastlist.

    if (itemcnt == 1)
    {
        removeIndexFromList(itempos[0], duelist);
        removeIndexFromList(itempos[0], failedlist);
    }
    else if (itemcnt != 0)
    {
        for (int ix = tosigned(duelist.size()) - 1; ix != -1; --ix)
        {
            int &i = duelist[ix];
            if (i > itempos[0])
            {
                if (i > itempos[itemcnt - 1])
                    i -= itemcnt;
                else if (itemcnt == 3 && i > itempos[1])
                {
                    if (i == itempos[2])
                        duelist.erase(duelist.begin() + ix);
                    else
                        i -= 2;
                }
                else if (i == itempos[1])
                    duelist.erase(duelist.begin() + ix);
                else
                    --i;
            }
            else if (i == itempos[0])
                duelist.erase(duelist.begin() + ix);
        }
        for (int ix = tosigned(failedlist.size()) - 1; ix != -1; --ix)
        {
            int &i = failedlist[ix];
            if (i > itempos[0])
            {
                if (i > itempos[itemcnt - 1])
                    i -= itemcnt;
                else if (itemcnt == 3 && i > itempos[1])
                {
                    if (i == itempos[2])
                        failedlist.erase(failedlist.begin() + ix);
                    else
                        i -= 2;
                }
                else if (i == itempos[1])
                    failedlist.erase(failedlist.begin() + ix);
                else
                    --i;
            }
            else if (i == itempos[0])
                failedlist.erase(failedlist.begin() + ix);
        }
    }

    // Remove the item from lockitems.

    for (int ix = itemcnt - 1; ix != -1; --ix)
        lockitems.remove(itempos[ix]);

    // Remove the items from the word data and their study cards.

    for (int ix = tosigned(list.size()) - 1; ix != -1; --ix)
    {
        if (list[ix]->index > windex)
            --list[ix]->index;
        else if (list[ix]->index == windex)
        {
            if (list[ix]->groupid != nullptr)
            {
                StudyDeck *study = studyDeck();
                study->deleteCardGroup(list[ix]->groupid);
            }
            list.erase(list.begin() + ix);
        }
    }

    if (!fremoved.empty())
        emit itemsRemoved(fremoved, true);
    if (!lremoved.empty())
        emit itemsRemoved(lremoved, false);
}

void WordDeck::initNewStudy(int num)
{
    if (num == 0)
        return;

    newcnt += std::min(tosigned(freeitems.size()), num);
    dictionary()->setToUserModified();
}

WordDeckWord* WordDeck::wordFromIndex(int windex)
{
    auto it = std::lower_bound(list.begin(), list.end(), windex, [](WordDeckWord *w, int index) {
        return w->index < index;
    });
    if (it == list.end() || (*it)->index != windex)
        return nullptr;
    return *it;
}

void WordDeck::itemsToWords(const std::vector<int> &items, bool queue, std::vector<int> &result)
{
    if (&result != &items)
        result = items;

    QSet<int> added;
    for (int &ix : result)
    {
        int windex = queue ? freeitems.items(ix)->data->index : lockitems.items(ix)->data->index;
        if (added.contains(windex))
        {
            ix = -1;
            continue;
        }

        added << windex;
        ix = windex;
    }

    result.resize(std::remove(result.begin(), result.end(), -1) - result.begin());
}

void WordDeck::startTest()
{
    //sortDueList();

    StudyDeck *study = studyDeck();

    currentix.reset();
    nextix.reset();

    if (!study->startTestDay())
        return;

#ifdef _DEBUG
    //sortDueList();
    checkDueList();
#endif

    // Yesterday's failed items are moved from failedlist to duelist.
    for (int ix : failedlist)
    {
        int dix = dueIndex(ix);
#ifdef _DEBUG
        if (dix >= 0)
            throw "The item can't be both in the failed and in the due list.";
#endif
        duelist.insert(duelist.begin() + (-dix - 1), ix);
    }

    failedlist.clear();

#ifdef _DEBUG
    checkDueList();
#endif

    // Clear kanji readings test data.
    testreadings.clear();

    lastday = study->testDay();

    dictionary()->setToUserModified();
}

int WordDeck::daysSinceLastInclude() const
{
    const StudyDeck *study = studyDeck();

    return study->daysSinceLastInclude();
}

bool WordDeck::firstTest() const
{
    const StudyDeck *study = studyDeck();

    return study->firstTest();
}

int WordDeck::readingsQueued() const
{
    return tosigned(testreadings.size());
}

int WordDeck::wordDataSize() const
{
    return tosigned(list.size());
}

WordDeckWord* WordDeck::wordData(int index)
{
    return list[index];
}

bool WordDeck::wordDataStudied(int index) const
{
    return list[index]->lastinclude.isValid();
}

//StudyDeckId WordDeck::deckId()
//{
//    return deckid;
//}

StudyDeck* WordDeck::getStudyDeck()
{
    return studyDeck();
}

//QString WordDeck::wordRomaji(int windex)
//{
//    return romanize(dict->wordEntry(windex)->kana.data());
//}

int WordDeck::totalTime() const
{
    if (!deckid.valid())
        return 0;
    const StudyDeck *study = studyDeck();
    if (study->dayStatSize() == 0)
        return 0;
    int sum = 0;
    for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
        sum += study->dayStat(ix).timespent;

    return sum;
}

int WordDeck::studyAverage() const
{
    if (!deckid.valid())
        return 0;
    const StudyDeck *study = studyDeck();
    if (study->dayStatSize() == 0)
        return 0;

    int sum = 0;
    int cnt = 0;
    for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
    {
        int spent = study->dayStat(ix).timespent;
        if (spent != 0)
        {
            sum += study->dayStat(ix).timespent;
            ++cnt;
        }
    }

    return cnt == 0 ? 0 : sum / cnt;
}

int WordDeck::answerAverage() const
{
    int sum = totalTime();
    if (totalTime() == 0)
        return 0;

    int itemcnt = 0;

    const StudyDeck *study = studyDeck();
    for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
    {
        if (study->dayStat(ix).timespent != 0)
            itemcnt += study->dayStat(ix).testcount;
    }
    return sum / itemcnt;
}

int WordDeck::queueSize() const
{
    return tosigned(freeitems.size());
}

int WordDeck::dueSize() const
{
    const StudyDeck *study = studyDeck();
    QDate now = ltDay(QDateTime::currentDateTimeUtc());

    auto endit = std::upper_bound(duelist.begin(), duelist.end(), now, [this, study](QDate now, int ix) {
        QDateTime d = study->cardNextTestDate(lockitems.items(ix)->cardid);
        return now < ltDay(d);
    });

    return tosigned(failedlist.size()) + (endit - duelist.begin());

    //int duecnt = 0;
    //for (int ix : duelist)
    //{
    //    QDateTime d = study->cardNextTestDate(lockitems.items(ix)->cardid);

    //    // Duelist is sorted by the date of future tests so we can stop once
    //    // the first future item is encountered.
    //    if (ltDay(d) > now || abortgenerating)
    //        break;
    //    ++duecnt;
    //}
}

quint32 WordDeck::dueEta() const
{
    const StudyDeck *study = studyDeck();
    quint32 r = 0;
    int dsiz = dueSize();
    for (int ix = 0, siz = tosigned(failedlist.size()); ix != siz; ++ix)
        r += study->cardEta(lockitems.items(failedlist[ix])->cardid);
    dsiz -= tosigned(failedlist.size());
    for (int ix = 0; ix != dsiz; ++ix)
        r += study->cardEta(lockitems.items(duelist[ix])->cardid);

    r += study->newCardEta() * newcnt;
    return r;
}

int WordDeck::newSize() const
{
    return newcnt;
}

int WordDeck::failedSize() const
{
    return tosigned(failedlist.size());
}

FreeWordDeckItem* WordDeck::queuedItems(int index)
{
    return freeitems.items(index);
}

void WordDeck::removeQueuedItems(const std::vector<int> &items)
{
    if (items.empty())
        return;

    std::vector<int> ordered = items;
    std::sort(ordered.begin(), ordered.end());
    for (int ix = tosigned(ordered.size()) - 1; ix != -1; --ix)
    {
        FreeWordDeckItem *item = freeitems.items(ordered[ix]);
        WordDeckWord *dat = item->data;
        dat->types &= ~(uchar)item->questiontype;
        freeitems.remove(ordered[ix]);

        if (dat->types == 0)
            removeWordData(dat);
    }

    dictionary()->setToUserModified();
    emit itemsRemoved(ordered, true);
}

void WordDeck::removeStudiedItems(const std::vector<int> &items)
{
    if (items.empty())
        return;

    StudyDeck *study = studyDeck();
    std::vector<int> ordered = items;
    std::sort(ordered.begin(), ordered.end());
    for (int ix = tosigned(ordered.size()) - 1; ix != -1; --ix)
    {
        for (int iy = tosigned(duelist.size()) - 1; iy != -1; --iy)
        {
            if (duelist[iy] > ordered[ix])
                --duelist[iy];
            else if (duelist[iy] == ordered[ix])
                duelist.erase(duelist.begin() + iy);
        }
        for (int iy = tosigned(failedlist.size()) - 1; iy != -1; --iy)
        {
            if (failedlist[iy] > ordered[ix])
                --failedlist[iy];
            else if (failedlist[iy] == ordered[ix])
                failedlist.erase(failedlist.begin() + iy);
        }

        LockedWordDeckItem *item = lockitems.items(ordered[ix]);
        WordDeckWord *dat = item->data;
        dat->types &= ~(uchar)item->questiontype;

        if (dat->groupid == item->cardid)
            dat->groupid = study->deleteCard(item->cardid);
        else
            study->deleteCard(item->cardid);

        if (dat->groupid == nullptr)
            testreadings.removeWord(dat->index);

        lockitems.remove(ordered[ix]);

        if (dat->types == 0)
            removeWordData(dat);
    }
    
    dictionary()->setToUserModified();
    emit itemsRemoved(ordered, false);
}

void WordDeck::requeueStudiedItems(const std::vector<int> &items)
{
    if (items.empty())
        return;

    std::vector<std::pair<int, int>> parts;
    parts.reserve(items.size());
    for (int ix = 0, siz = tosigned(items.size()); ix != siz; ++ix)
    {
        LockedWordDeckItem *item = lockitems.items(items[ix]);
        WordDeckWord *dat = item->data;
        parts.emplace_back(dat->index, (int)item->questiontype);
    }

    removeStudiedItems(items);
    queueWordItems(parts);

    dictionary()->setToUserModified();
}

void WordDeck::queuedPriorities(const std::vector<int> &items, QSet<uchar> &priorities) const
{
    priorities.clear();
    for (int ix : items)
    {
        const FreeWordDeckItem *item = freeitems.items(ix);
        priorities << item->priority;
    }
}

void WordDeck::setQueuedPriority(const std::vector<int> &items, uchar priority)
{
    if (priority < 1 || priority > 9)
        throw "Invalid priority";

    std::vector<int> changed;
    for (int ix : items)
    {
        FreeWordDeckItem *item = freeitems.items(ix);
        if (item->priority != priority)
        {
            item->priority = priority;
            changed.push_back(ix);
        }
    }

    std::sort(changed.begin(), changed.end());
    if (!changed.empty())
    {
        dictionary()->setToUserModified();
        emit itemDataChanged(changed, true);
    }
}

void WordDeck::itemHints(const std::vector<int> &items, bool queue, uchar &hints, uchar &sel) const
{
    hints = 0;
    sel = 0;
    for (int ix : items)
    {
        const WordDeckItem *item = queue ? (WordDeckItem*)freeitems.items(ix) : (WordDeckItem*)lockitems.items(ix);

        hints |= ((int)WordPartBits::AllParts - (int)item->questiontype);
        sel |= (1 << (int)item->mainhint);
        if (hints == ((int)WordPartBits::AllParts | (int)WordPartBits::Default) && sel == ((int)WordPartBits::AllParts | (int)WordPartBits::Default))
            break;
    }
}

void WordDeck::setItemHints(const std::vector<int> &items, bool queue, WordParts hint)
{
    std::vector<int> changed;
    for (int ix : items)
    {
        WordDeckItem *item = queue ? (WordDeckItem*)freeitems.items(ix) : (WordDeckItem*)lockitems.items(ix);

        if ((int)item->questiontype != (1 << (int)hint) && item->mainhint != hint)
        {
            item->mainhint = hint;
            changed.push_back(ix);
        }
    }

    std::sort(changed.begin(), changed.end());
    if (!changed.empty())
    {
        dictionary()->setToUserModified();
        emit itemDataChanged(changed, queue);
    }
}

void WordDeck::increaseSpacingLevel(int ix)
{
    StudyDeck *study = studyDeck();
    study->increaseSpacingLevel(lockitems.items(ix)->cardid);

    dictionary()->setToUserModified();
    emit itemDataChanged({ ix }, false);
}

void WordDeck::decreaseSpacingLevel(int ix)
{
    StudyDeck *study = studyDeck();
    study->decreaseSpacingLevel(lockitems.items(ix)->cardid);

    dictionary()->setToUserModified();
    emit itemDataChanged({ ix }, false);
}

void WordDeck::resetCardStudyData(const std::vector<int> &items)
{
    if (items.empty())
        return;

    StudyDeck *study = studyDeck();
    for (int ix : items)
        study->resetCardStudyData(lockitems.items(ix)->cardid);

    dictionary()->setToUserModified();
    emit itemDataChanged(items, false);
}

int WordDeck::queueWordItems(const std::vector<std::pair<int, int>> &parts)
{
    if (parts.empty())
        return 0;

    int added = 0;
    for (int ix = 0, siz = tosigned(parts.size()); ix != siz; ++ix)
    {
        int windex = parts[ix].first;
        int types = parts[ix].second;
        types = types & (int)WordPartBits::AllParts;

        if (types == 0)
            continue;

        WordDeckWord *w = addWordWithIndex(windex);

        // Keep those bits in types that are not in w->types.
        types = (~w->types & types);

        QDateTime timeadded = QDateTime::currentDateTimeUtc();

        int current = 1;
        while (types != 0 && current != 8)
        {
            if ((types & current) == 0)
            {
                current <<= 1;
                continue;
            }
            ++added;

            FreeWordDeckItem *item = new FreeWordDeckItem;
            freeitems.add(item);

            item->priority = 5;
            item->added = timeadded;
            item->data = w;
            item->questiontype = (WordPartBits)current;
            item->mainhint = WordParts::Default;

            w->types |= current;

            types -= current;
            current <<= 1;
        }
    }

    if (added != 0)
    {
        dictionary()->setToUserModified();
        emit itemsQueued(added);
    }
    return added;
}

int WordDeck::queueUniqueSize() const
{
    QSet<int> uniques;
    for (int ix = 0, siz = tosigned(freeitems.size()); ix != siz; ++ix)
        uniques.insert(freeitems.items(ix)->data->index);
    return uniques.size();
}

void WordDeck::dueItems(std::vector<int> &items) const
{
    items.clear();
    items.reserve(duelist.size() + failedlist.size());
    for (int ix = 0, siz = tosigned(failedlist.size()); ix != siz; ++ix)
        items.push_back(failedlist[ix]);
    for (int ix = 0, siz = tosigned(duelist.size()); ix != siz; ++ix)
        items.push_back(duelist[ix]);
}

int WordDeck::studySize() const
{
    return tosigned(lockitems.size());
}

LockedWordDeckItem* WordDeck::studiedItems(int index)
{
    return lockitems.items(index);
}

int WordDeck::studyLevel(int index) const
{
    const StudyDeck *study = studyDeck();
    return study->cardLevel(lockitems.items(index)->cardid);
}

int WordDeck::testedSize() const
{
    const StudyDeck *study = studyDeck();

    return study->testSize();
}

//int WordDeck::studiedIndex(LockedWordDeckItem *item) const
//{
//    StudyDeck *study = studyDeck();
//    return study->cardIndex(item->cardid);
//}

LockedWordDeckItem* WordDeck::testedItems(int index)
{
    const StudyDeck *study = studyDeck();
    CardId *id = study->testCard(index);
    return (LockedWordDeckItem*)study->cardData(id);
}

int WordDeck::testedIndex(int index) const
{
    const StudyDeck *study = studyDeck();
    CardId *id = study->testCard(index);
    return study->cardIndex(id);
}

QDate WordDeck::lastDay() const
{
    return lastday;
}

QDate WordDeck::firstDay() const
{
    if (!deckid.valid())
        return QDate();
    const StudyDeck *study = studyDeck();
    if (study->dayStatSize() == 0)
        return QDate();
    return study->dayStat(0).day;
}

int WordDeck::testDayCount() const
{
    if (!deckid.valid())
        return 0;
    const StudyDeck *study = studyDeck();
    if (study->dayStatSize() == 0)
        return 0;

    int r = 0;
    for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
    {
        const DeckDayStat &stat = study->dayStat(ix);
        if (stat.testcount != 0)
            ++r;
    }
    return r;
}

int WordDeck::skippedDayCount() const
{
    if (!deckid.valid())
        return 0;
    int r = testDayCount();

    return firstDay().daysTo(ltDay(QDateTime::currentDateTimeUtc())) - r + 1;
}

void WordDeck::sortDueList()
{
    StudyDeck *study = studyDeck();

    // Computing the due time is slow. The map caches the result.
    // [word index, due time]
    std::map<int, QDateTime> datecache;

    std::sort(duelist.begin(), duelist.end(), [this, study, &datecache](int aix, int bix){
        if (aix == bix)
            return false;

        const LockedWordDeckItem *a = lockitems.items(aix);
        const LockedWordDeckItem *b = lockitems.items(bix);

        QDateTime ta;
        auto it = datecache.find(aix);
        if (it == datecache.end())
        {
            ta = study->cardNextTestDate(a->cardid);
            datecache.insert(std::make_pair(aix, ta));
        }
        else
            ta = it->second;

        QDateTime tb;
        it = datecache.find(bix);
        if (it == datecache.end())
        {
            tb = study->cardNextTestDate(b->cardid);
            datecache.insert(std::make_pair(bix, tb));
        }
        else
            tb = it->second;

        if (ta == tb)
        {
            if (a->data->index == b->data->index)
                return (int)a->questiontype < (int)b->questiontype;
            return a->data->index < b->data->index;
        }

        return ta < tb;
    });
}

int WordDeck::dueIndex(int lockindex) const
{
    const StudyDeck *study = studyDeck();

    // Computing the due time is slow. The map caches the result.
    std::map<int, QDateTime> datecache;
    QDateTime lockdate = study->cardNextTestDate(lockitems.items(lockindex)->cardid);
    datecache.insert(std::make_pair(lockindex, lockdate));

    auto it = std::lower_bound(duelist.begin(), duelist.end(), lockindex, [this, study, lockdate, &datecache](int aix, int lockindex){
        if (aix == lockindex)
            return false;

        const LockedWordDeckItem *a = lockitems.items(aix);
        const LockedWordDeckItem *b = lockitems.items(lockindex);

        QDateTime ta;
        auto it = datecache.find(aix);
        if (it == datecache.end())
        {
            ta = study->cardNextTestDate(a->cardid);
            datecache.insert(std::make_pair(aix, ta));
        }
        else
            ta = it->second;

        //QDateTime tb = lockdate;
        //it = datecache.find(bix);
        //if (it == datecache.end())
        //{
        //    tb = study->cardNextTestDate(b->cardid);
        //    datecache.insert(std::make_pair(bix, tb));
        //}
        //else
        //    tb = it->second;

        if (ta == lockdate)
        {
            if (a->data->index == b->data->index)
            {
#ifdef _DEBUG
                // This is an error, items with the same word index shouldn't share the question type.
                if ((int)a->questiontype == (int)b->questiontype)
                    throw "Invalid items, question types match.";
#endif
                return (int)a->questiontype < (int)b->questiontype;
            }
            return a->data->index < b->data->index;
        }
        return ta < lockdate;
    });

    if (it == duelist.end() || *it != lockindex)
        return duelist.begin() - it - 1;
    return it - duelist.begin();
}

#ifdef _DEBUG
void WordDeck::checkDueList()
{
    if (duelist.empty())
        return;

    StudyDeck *study = studyDeck();

    QDateTime prevdate = study->cardNextTestDate(lockitems.items(duelist[0])->cardid);
    for (int ix = 1, siz = tosigned(duelist.size()); ix < siz; ++ix)
    {
        const LockedWordDeckItem *a = lockitems.items(duelist[ix - 1]);
        const LockedWordDeckItem *b = lockitems.items(duelist[ix]);

        QDateTime thisdate = study->cardNextTestDate(lockitems.items(duelist[ix])->cardid);
        if (thisdate == prevdate)
        {
            // This is an error, items with the same word index shouldn't share the question type.
            if (a->data->index == b->data->index)
            {
                if ((int)a->questiontype == (int)b->questiontype)
                    throw "Invalid items, question types match.";
                if ((int)a->questiontype > (int)b->questiontype)
                    throw "Invalid order";
            }
            if (a->data->index > b->data->index)
                throw "Invalid order";
        }
        else if (thisdate < prevdate)
        {
            throw "Later item comes earlier.";
        }

        prevdate = thisdate;
    }
}
#endif

//LockedWordDeckItem* WordDeck::itemOfType(WordDeckWord *worddat, WordPartBits type)
//{
//    if (worddat->groupid == nullptr)
//        return nullptr;
//
//    StudyDeck *study = studyDeck();
//
//    CardId *id = worddat->groupid;
//    CardId *firstid = id;
//    do
//    {
//        LockedWordDeckItem *item = (LockedWordDeckItem*)study->cardData(id);
//        if (item->questiontype == type)
//            return item;
//        id = study->nextCard(id);
//    } while (id != firstid);
//    return nullptr;
//}

WordParts WordDeck::resolveMainHint(const WordDeckItem *item) const
{
    if (item->mainhint != WordParts::Default)
        return item->mainhint;

    switch (item->questiontype)
    {
    case WordPartBits::Kanji:
        return Settings::study.kanjihint;
    case WordPartBits::Kana:
        return Settings::study.kanahint;
    case WordPartBits::Definition:
    default:
        return Settings::study.defhint;
    }
}

int WordDeck::nextIndex()
{
    if (waitForNextItem() == false)
        return -1;

    // When the nextitem is not precomputed, it'll be generated here on the run.
    // While generation runs on a separate thread, we will have to block manually.
    if (!nextix.isValid() && (dueSize() != 0 || newcnt != 0))
    {
        generateNextItem();
        waitForNextItem();
    }

    currentix = nextix;

    if (currentix.isValid())
        return currentItem()->data->index;

    return -1;
}

WordParts WordDeck::nextHint() const
{
    if (currentix.isValid())
        return currentItem()->mainhint;

    throw "Invalid code. Programmer error.";
}

WordPartBits WordDeck::nextQuestion() const
{
    if (currentix.isValid())
        return currentItem()->questiontype;

    throw "Invalid code. Programmer error.";
}

WordParts WordDeck::nextMainHint() const
{
    if (currentix.isValid())
        return resolveMainHint(currentItem());

    throw "Invalid code. Programmer error.";
}

quint32 WordDeck::nextCorrectSpacing() const
{
    if (currentix.free != -1)
        return (quint32)24 * 60 * 60; // 1 day

    if (currentix.locked != -1)
    {
        LockedWordDeckItem *litem = (LockedWordDeckItem*)currentItem();
        // Simulating an answer does not modify the study deck so it's safe to cast here.
        StudyDeck *study = const_cast<StudyDeck*>(studyDeck());
        return study->answer(litem->cardid, StudyCard::Correct, 0, true);
    }

    throw "Invalid code. Programmer error.";
}

quint32 WordDeck::nextEasySpacing() const
{
    if (currentix.free != -1)
        return (quint32)24 * 60 * 60; // 1 day

    if (currentix.locked != -1)
    {
        LockedWordDeckItem *litem = (LockedWordDeckItem*)currentItem();
        // Simulating an answer does not modify the study deck so it's safe to cast here.
        StudyDeck *study = const_cast<StudyDeck*>(studyDeck());
        return study->answer(litem->cardid, StudyCard::Easy, 0, true);
    }

    throw "Invalid code. Programmer error.";
}

bool WordDeck::nextNewCard() const
{
    return (currentix.free != -1);

}

bool WordDeck::nextFailedCard() const
{
    return currentix.locked != -1 && dueIndex(currentix.locked) < 0;
}

//bool WordDeck::nextRetryPostponed()
//{
//    if (currentix.free != -1)
//        return false;
//
//    if (currentix.locked != -1)
//    {
//        LockedWordDeckItem *litem = (LockedWordDeckItem*)currentItem();
//        StudyDeck *study = studyDeck();
//        bool postponed = false;
//        study->answer(litem->cardid, StudyCard::Retry, 0, postponed, true);
//        return postponed;
//    }
//
//    throw "Invalid code. Programmer error.";
//}
//
//bool WordDeck::nextWrongPostponed()
//{
//    if (currentix.free != -1)
//        return false;
//
//    if (currentix.locked != -1)
//    {
//        LockedWordDeckItem *litem = (LockedWordDeckItem*)currentItem();
//        StudyDeck *study = studyDeck();
//        bool postponed = false;
//        study->answer(litem->cardid, StudyCard::Wrong, 0, postponed, true);
//        return postponed;
//    }
//
//    throw "Invalid code. Programmer error.";
//}

void WordDeck::generateNextItem()
{
    abortgenerating = false;
    NextWordDeckItemThread *thread = new NextWordDeckItemThread(this);
    QThreadPool::globalInstance()->start(thread);
}

void WordDeck::answer(StudyCard::AnswerType a, qint64 answertime)
{
    waitForNextItem();

    // If the next item is the same as the current item, it means that the thread looking for
    // the next one couldn't find anything else. As it couldn't tell what the answer will be,
    // it assumed the current item failed and will have to be repeated.
    // If the current item was a success, there is nothing else to test. Set the next one to
    // invalid.
    if (((nextix == currentix && nextix.free == -1) || (currentix.free != -1 && nextix.locked == tosigned(lockitems.size()))) && (a == StudyCard::Correct || a == StudyCard::Easy))
        nextix.reset();

    StudyDeck *study = studyDeck();

    WordDeckItem *current = currentItem();
    current->data->lastinclude = QDateTime::currentDateTimeUtc();

    // If a new item was tested, it has to be "converted" to a locked item and added to
    // lockitems. Existing items will be removed from either the failedlist or duelist.
    // In either case, the item is added back to failedlist or duelist depending on the
    // answer.
    LockedWordDeckItem *item;

    int dueindex;

    // New items have a 0 or positive free index.
    if (currentix.free != -1)
    {
        // Item was taken from the queue of free items. The item must be removed from the
        // queue and a new card created for it on the deck.

        FreeWordDeckItem *freeitem = (FreeWordDeckItem*)current;
        item = new LockedWordDeckItem;
        item->added = freeitem->added;
        item->data = freeitem->data;
        item->mainhint = freeitem->mainhint;
        item->questiontype = freeitem->questiontype;

        // The next item to be shown is the same as the current item. A new next item must be
        // found.
        if (nextix.locked == tosigned(lockitems.size()) /*== currentix*/)
            nextix.reset();

        freeitems.remove(currentix.free);

        item->cardid = study->createCard(item->data->groupid, (intptr_t)item);
        if (item->data->groupid == nullptr)
            item->data->groupid = item->cardid;

        currentix.free = -1;
        currentix.locked = lockitems.add(item);

        --newcnt;
    }
    else
    {
        item = (LockedWordDeckItem*)current;

        dueindex = dueIndex(currentix.locked);

        if (dueindex >= 0)
        {
#ifdef _DEBUG
            if (duelist[dueindex] != currentix.locked)
                throw "Wrong index found";
#endif

            duelist.erase(duelist.begin() + dueindex);
        }
        else
        {
            auto it = std::find(failedlist.begin(), failedlist.end(), currentix.locked);
            if (it == failedlist.end())
                throw "Item should be either in failed list or in due list. Coder error.";
            failedlist.erase(it);
        }
    }

#ifdef _DEBUG
    checkDueList();
#endif
    study->answer(item->cardid, a, answertime);

    if (a == StudyCard::Correct || a == StudyCard::Easy)
    {
        if (nextItem() == item)
            nextix.reset();

        int insertindex = -1 - dueIndex(currentix.locked);
        duelist.insert(duelist.begin() + insertindex, currentix.locked);

        if (study->cardFirstShow(item->cardid))
        {
            bool newitem = study->cardTestLevel(item->cardid) == 0;
            testreadings.add(item->data->index, newitem, false);
        }
    }
    else
    {
        if (study->cardFirstShow(item->cardid))
        {
            bool newitem = study->cardTestLevel(item->cardid) == 0;
            testreadings.add(item->data->index, newitem, true);
        }
        failedlist.push_back(currentix.locked);
    }
#ifdef _DEBUG
    checkDueList();
#endif

    dictionary()->setToUserModified();
}

StudyCard::AnswerType WordDeck::lastAnswer() const
{
    const StudyDeck *study = studyDeck();
    return study->lastAnswer();
}

void WordDeck::changeLastAnswer(StudyCard::AnswerType a)
{
    abortGenerateNextItem();

    StudyDeck *study = studyDeck();

    LockedWordDeckItem *item = (LockedWordDeckItem*)study->cardData(study->lastCard());
    int lastindex = lockitems.indexOf(item);


    int dueindex = dueIndex(lastindex);
    bool failed = false;

    if (dueindex >= 0)
        duelist.erase(duelist.begin() + dueindex);
    else
    {
        auto it = std::find(failedlist.begin(), failedlist.end(), lastindex);
        if (it == failedlist.end())
            throw "Item should be either in failed list or in due list. Coder error.";
        failedlist.erase(it);
        failed = true;
    }

    study->changeLastAnswer(a);

    // Postponed cards are not stored in failedlist even if the answer was incorrect.
    if (a == StudyCard::Correct || a == StudyCard::Easy)
    {
        duelist.insert(duelist.begin() - (dueIndex(lastindex) + 1), lastindex);

        bool newitem = study->cardTestLevel(item->cardid) == 0;
        testreadings.add(item->data->index, newitem, failed, true);
    }
    else
        failedlist.push_back(lastindex);

    generateNextItem();
    dictionary()->setToUserModified();
}

bool WordDeck::canUndo() const
{
    const StudyDeck *study = studyDeck();
    return study->lastCard() != nullptr && (currentix.free != -1 || currentItem() != (WordDeckItem*)study->cardData(study->lastCard()));
}

int WordDeck::nextPracticeKanji()
{
    return testreadings.nextKanji();
}

uchar WordDeck::nextPracticeReading()
{
    return testreadings.nextReading();
}

bool WordDeck::practiceReadingMatches(QString answer)
{
    return testreadings.readingMatches(answer);
}

QString WordDeck::correctReading()
{
    return testreadings.correctReading();
}

void WordDeck::practiceReadingAnswered()
{
    testreadings.readingAnswered();
}

void WordDeck::nextPracticeReadingWords(std::vector<int> &words)
{
    testreadings.nextWords(words);
}

WordDeckWord* WordDeck::addWordWithIndex(int windex)
{
    auto it = std::lower_bound(list.begin(), list.end(), windex, [](WordDeckWord *w, int index) {
        return w->index < index;
    });
    if (it != list.end() && (*it)->index == windex)
        return *it;

    WordDeckWord *w = new WordDeckWord;
    w->index = windex;
    w->types = 0;
    w->groupid = nullptr;

    list.insert(it, w);

    return w;
}

void WordDeck::removeWordData(WordDeckWord *dat)
{
    auto it = std::lower_bound(list.begin(), list.end(), dat->index, [](const WordDeckWord *data, int windex) { return data->index < windex; });
    if (*it != dat)
        throw "Bad program. Really. Tell someone about it.";

    list.erase(it);
}

bool WordDeck::waitForNextItem() const 
{
    if (generatingnext == false)
        return true;

    auto mil1 = std::chrono::milliseconds(1);
    while (generatingnext)
        std::this_thread::sleep_for(mil1 /*std::chrono::milliseconds(1)*/);
    return nextix.locked != -1;
}

void WordDeck::abortGenerateNextItem()
{
    abortgenerating = true;
    waitForNextItem();
    abortgenerating = false;
    nextix.reset();
}

void WordDeck::doGenerateNextItem()
{
    // This function is called from a separate thread and sets nextitem and
    // nextindex. The currently tested item is in currentitem and
    // currentindex. We assume the current item will fail and will be tested
    // again in 10 minutes.

    const int minutestowait = 10;

    WordDeckItem *current = currentItem();

    nextix.reset();

    StudyDeck *study = studyDeck();
    //const StudyCard *c;

    // Holds every item that was tested in the previous 10 minutes in the order
    // they were tested.
    std::vector<LockedWordDeckItem*> past;

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDate testday = study->testDay(); //ltDay(now);

    // Get the number of possible items for today's test for convenience.
    auto dueit = interruptUpperBound(duelist.begin(), duelist.end(), testday, [this, study](QDate &testday, int ix, bool &stop){
        stop = abortgenerating;
        if (stop)
            return false;

        QDateTime d = study->cardNextTestDate(lockitems.items(ix)->cardid);
        return testday < ltDay(d);
    });

    int duecnt = dueit - duelist.begin();

    //int duecnt = 0;
    //for (int ix : duelist)
    //{
    //    QDateTime d = study->cardNextTestDate(lockitems.items(ix)->cardid);

    //    // Duelist is sorted by the date of future tests so we can stop once
    //    // the first future item is encountered.
    //    if (ltDay(d) > testday || abortgenerating)
    //        break;
    //    ++duecnt;
    //}

    // Collect items tested in past 10 minutes. The currentitem is also one
    // of those, even if its data is not updated yet.
    for (int ix = 0; ix != duecnt && !abortgenerating; ++ix)
    {
        LockedWordDeckItem *item = lockitems.items(duelist[ix]);
        if (item == current || item->data->lastinclude.msecsTo(now) < 60 * 1000 * minutestowait)
            past.push_back(item);
    }
    for (int ix = 0, siz = tosigned(failedlist.size()); ix != siz && !abortgenerating; ++ix)
    {
        LockedWordDeckItem *item = lockitems.items(failedlist[ix]);
        if (item == current || item->data->lastinclude.msecsTo(now) < 60 * 1000 * minutestowait)
            past.push_back(item);
    }

    if (abortgenerating)
        return;

    // The past items are sorted in the order their group was last included in
    // the test today. Those come first that were tested the shortest time
    // ago. The current item and its group is the first one even though its
    // lastinclude is not updated yet.
    if (interruptSort(past.begin(), past.end(), [this, current](LockedWordDeckItem *a, LockedWordDeckItem *b, bool &aborted) {
        aborted = abortgenerating;
        if (current != nullptr)
        {
            if (a->data == current->data && b->data != current->data)
                return true;
            if (b->data == current->data)
                return false;
        }
        return a->data->lastinclude > b->data->lastinclude;
    }))
        return;

    // New items are tested first. Look for one with the highest priority that
    // wasn't tested today, then for one not tested in the past 10 minutes. If
    // none are found try to get one that was tested the earliest.
    if (newcnt - (currentix.free != -1 ? 1 : 0) != 0)
    {
        // Count the number of items in each priority group and go from highest to lowest.
        int priorities[9];
        memset(priorities, 0, sizeof(int) * 9);
        for (int ix = 0, siz = tosigned(freeitems.size()); ix != siz && !abortgenerating; ++ix)
        {
#ifdef _DEBUG
            if (freeitems.items(ix)->priority < 1 || freeitems.items(ix)->priority > 9)
                throw "Invalid";
#endif
            ++priorities[freeitems.items(ix)->priority - 1];
        }

        // Index of item in the free items list if found. If not, use previx.
        int foundix = -1;

        // Index of possible word match. It was tested in the past 10 minutes but still
        // earlier than the others.
        int previx = -1;
        // Time a possible result was last tested.
        QDateTime prevtime = now;

        int p = 8;
        while (foundix == -1 && p != -1 && !abortgenerating)
        {
            for (int ix = 0, siz = tosigned(freeitems.size()); ix != siz && priorities[p] != 0 && !abortgenerating; ++ix)
            {
                FreeWordDeckItem *item = freeitems.items(ix);
                if (item == current || item->priority != p + 1)
                    continue;
                --priorities[p];

                // Group was not tested today.
                if ((current == nullptr || item->data != current->data) && ltDay(item->data->lastinclude) < testday)
                {
                    foundix = ix;
                    break;
                }

                // Group was not tested in past 10 minutes.
                if ((current == nullptr || item->data != current->data) && std::find_if(past.begin(), past.end(), [item](LockedWordDeckItem *a) { return (a->data == item->data); }) == past.end())
                    foundix = ix;
                else if (previx == -1 || previx == currentix.free || ((current == nullptr || current->data != item->data) && item->data->lastinclude < prevtime))
                {
                    // Group was tested in the past 10 minutes, save it in case nothing else
                    // is found. The currentitem's lastinclude is not updated yet, make sure
                    // its group is handled correctly.
                    prevtime = (current != nullptr && item->data == current->data ? now : item->data->lastinclude);
                    previx = ix;
                }
            }

            --p;
        }

        if (abortgenerating)
            return;

        if (foundix == -1)
        {
            // Only true if the current item is the only item tested today.
            if (previx == -1)
                previx = currentix.free;
            foundix = previx;
        }


        if (foundix != -1)
            nextix.setFree(foundix);
        else
            nextix = currentix;

        // The current item will be moved to the locked items list. Make sure nextix will be
        // still valid afterwards.
        if (currentix.free != -1 && nextix.free == currentix.free)
        {
            // If the same item is found as the next one, it'll be the last in the locked
            // items list.
            nextix.free = -1;
            nextix.locked = tosigned(lockitems.size());
        }
        else if (currentix.free != -1 && nextix.free != -1 && currentix.free < nextix.free)
        {
            // The current item will be moved out from before nextix.
            --nextix.free;
        }

        //nextitem = (foundix == -1 ? currentitem : freeitems.items(foundix));

        return;
    }

    // Index of next item is only needed when it was taken from freeitems.
    nextix.reset();

    // Get an item to test from failedlist then duelist, that wasn't tested in the past 10
    // minutes and is due. If no such item is found try one that was tested the earliest.

    // Items failed the last time are used in the order their group was last. tested. We
    // select the item from the group tested the longest time ago.
    qint64 failedmsecs = -1;
    int failedindex = -1;
    for (int ix : failedlist)
    {
        LockedWordDeckItem *litem = lockitems.items(ix);

        if (current != nullptr && current->data == litem->data)
            continue;
        if (litem->data->lastinclude.msecsTo(now) > failedmsecs)
        {
            failedindex = ix;
            failedmsecs = litem->data->lastinclude.msecsTo(now);
        }
        if (abortgenerating)
            break;
    }
    if (failedmsecs >= 60 * 1000 * minutestowait || abortgenerating)
    {
        nextix.setLocked(failedindex);
        return;
    }

    QDateTime prevtime = now;
    //LockedWordDeckItem *previtem = nullptr;
    int previndex = -1;

    if (duecnt != 0)
    {
        // Items with longer intervals have a higher probability of being remembered correctly
        // if they survived that long, and there are less of them. Items with medium length
        // intervals can still pile up after using the study for long. Because items with very
        // short intervals must be repeated early, it's safer to show them first.
        // For the above reasons we give 20% chance for the short interval items to be shown
        // no matter what's before them in the due list. This number is not proven to be
        // correct, so it might change in future tests.

        bool shownewest = rnd(1, 10) <= 2;

        if (!shownewest)
        {
            // Show the first item which hasn't been asked today, or if none found, in the
            // previous 10 minutes. Otherwise remember the item that was included the longest
            // time ago.

            for (int ix = 0; ix != duecnt && !abortgenerating; ++ix)
            {
                LockedWordDeckItem *ditem = lockitems.items(duelist[ix]);
                if (current != nullptr && ditem->data == current->data)
                    continue;

                if (ltDay(ditem->data->lastinclude) < testday)
                {
                    nextix.setLocked(duelist[ix]);
                    break;
                }

                if (!nextix.isValid() && ditem->data->lastinclude.msecsTo(now) > 60 * 1000 * minutestowait)
                    nextix.setLocked(duelist[ix]);

                if (!nextix.isValid() && ditem->data->lastinclude < prevtime)
                {
                    //previtem = ditem;
                    previndex = duelist[ix];
                    prevtime = ditem->data->lastinclude;
                }
            }

            if (nextix.isValid() || abortgenerating)
                return;
        }
        else
        {
            // Show the item with the shortest interval that hasn't been asked today, or if
            // not found in the previous 10 minutes if possible. Otherwise remember the item
            // whose group was included the longest time ago.

            quint32 shorttime = (quint32)-1;
            int shortindex = -1;
            quint32 shorttime10 = (quint32)-1;
            int shortindex10 = -1;

            for (int ix = 0; ix != duecnt && !abortgenerating; ++ix)
            {
                LockedWordDeckItem *ditem = lockitems.items(duelist[ix]);

                if (current != nullptr && ditem->data == current->data)
                    continue;

                if (ltDay(ditem->data->lastinclude) < testday &&
                    (shorttime == (quint32)-1 || study->cardSpacing(ditem->cardid) < shorttime))
                {
                    shorttime = study->cardSpacing(ditem->cardid);
                    shortindex = duelist[ix];
                }

                if (ditem->data->lastinclude.msecsTo(now) > 60 * 1000 * minutestowait &&
                    (shorttime10 == (quint32)-1 || study->cardSpacing(ditem->cardid) < shorttime10))
                {
                    shorttime10 = study->cardSpacing(ditem->cardid);
                    shortindex10 = duelist[ix];
                }

                if (ditem->data->lastinclude < prevtime)
                {
                    //previtem = ditem;
                    previndex = duelist[ix];
                    prevtime = ditem->data->lastinclude;
                }
            }

            if (shortindex != -1 || shortindex10 != -1)
            {
                nextix.setLocked(shortindex != -1 ? shortindex : shortindex10);
                return;
            }
        }

        // Previtem holds the item due that was shown the longest time ago, from items outside
        // the current item's group, or null if no such item exists.
        nextix.setLocked(previndex);
        if (nextix.isValid() || abortgenerating)
            return;
    }

    // The currentitem was not the cause that no item was found. We won't find anything even
    // if we try again.
    if (!currentix.isValid() || abortgenerating)
        return;

    // Every item has been checked, apart from those in the current item's group. Show the
    // item that was shown the longest time ago no matter where it's from.
    if (previndex != -1)
        prevtime = study->cardItemDate(lockitems.items(previndex)->cardid);

    // Start with the failed list. The first item was tested the longest time ago so no need
    // to go further, unless it's the current one.
    if (!failedlist.empty() && (failedlist.front() != currentix.locked || failedlist.size() > 1))
    {
        LockedWordDeckItem *fitem = lockitems.items(failedlist.front());
        if (study->cardItemDate(fitem->cardid) < prevtime || previndex == -1)
        {
            //previtem = fitem;
            previndex = failedlist.front();
        }
        if (previndex == currentix.locked)
        {
            previndex = failedlist[1];
            //previtem = lockitems.items(previndex);
        }
        prevtime = study->cardItemDate(lockitems.items(previndex)->cardid);
    }

    // There are still items in the due list but they are probably all related
    // to the current item.
    if (duecnt != 0)
    {
        // Get the item that was tested the longest time ago.
        for (int ix = 0; ix != duecnt && !abortgenerating; ++ix)
        {
            LockedWordDeckItem *litem = lockitems.items(duelist[ix]);
            if (litem == current || litem->data != current->data)
                continue;

            if (study->cardItemDate(litem->cardid) < prevtime)
            {
                //previtem = duelist[ix];
                previndex = duelist[ix];
                prevtime = study->cardItemDate(litem->cardid);
            }
        }
    }

    if (previndex == -1)
    {
        // If nothing was found, set the current item as the next item as well. We have no way
        // to check the answer before it's given, so it must be checked before showing the
        // next item outside this function.
        nextix = currentix;

        if (currentix.free >= 0)
            nextix.setLocked(tosigned(lockitems.size()));
        return;
    }

    nextix.setLocked(previndex);
}

WordDeckItem* WordDeck::currentItem()
{
    if (!currentix.isValid())
        return nullptr;
    if (currentix.free != -1)
        return freeitems.items(currentix.free);
    return lockitems.items(currentix.locked);
}

WordDeckItem* WordDeck::nextItem()
{
    if (!nextix.isValid())
        return nullptr;
    if (nextix.free != -1)
        return freeitems.items(nextix.free);
    return lockitems.items(nextix.locked);
}

const WordDeckItem* WordDeck::currentItem() const
{
    if (!currentix.isValid())
        return nullptr;
    if (currentix.free != -1)
        return freeitems.items(currentix.free);
    return lockitems.items(currentix.locked);
}

const WordDeckItem* WordDeck::nextItem() const
{
    if (!nextix.isValid())
        return nullptr;
    if (nextix.free != -1)
        return freeitems.items(nextix.free);
    return lockitems.items(nextix.locked);
}

const StudyDeck* WordDeck::studyDeck() const
{
    return owner()->dictionary()->studyDecks()->getDeck(deckid);
}

StudyDeck* WordDeck::studyDeck()
{
    return owner()->dictionary()->studyDecks()->getDeck(deckid);
}


//-------------------------------------------------------------


NextWordDeckItemThread::NextWordDeckItemThread(WordDeck *owner) : base(), owner(owner)
{
    owner->generatingnext = true;
}

NextWordDeckItemThread::~NextWordDeckItemThread()
{
    owner->generatingnext = false;
}

void NextWordDeckItemThread::run()
{
    owner->doGenerateNextItem();
}


//-------------------------------------------------------------
