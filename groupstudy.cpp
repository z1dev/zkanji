/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <random>
#include <set>
#include "groupstudy.h"
#include "groups.h"
#include "zui.h"
#include "words.h"
#include "zkanjimain.h"
#include "romajizer.h"

#include "checked_cast.h"

//-------------------------------------------------------------


QDataStream& operator<<(QDataStream &stream, const WordStudyItem &item)
{
    stream << (qint32)item.windex;
    stream << (quint8)item.excluded;

    stream << (qint32)item.correct;
    stream << (qint32)item.incorrect;

    return stream;
}

QDataStream& operator>>(QDataStream &stream, WordStudyItem &item)
{
    qint32 i;
    quint8 c;
    stream >> i;
    item.windex = i;
    stream >> c;
    item.excluded = c;

    stream >> i;
    item.correct = i;
    stream >> i;
    item.incorrect = i;

    return stream;
}


//-------------------------------------------------------------


QDataStream& operator<<(QDataStream& stream, const WordStudySettings &s)
{
    stream << (qint32)s.tested;
    stream << (qint32)s.typed;
    stream << (qint8)s.hidekana;
    stream << (qint8)s.matchkana;
    stream << (quint8)s.answer;
    stream << (quint8)s.method;
    stream << (qint8)s.usetimer;
    stream << (qint32)s.timer;
    stream << (qint8)s.uselimit;
    stream << (qint32)s.limit;
    stream << (qint8)s.usewordlimit;
    stream << (qint32)s.wordlimit;
    stream << (qint8)s.shuffle;
    stream << (qint32)s.gradual.initnum;
    stream << (qint32)s.gradual.incnum;
    stream << (qint32)s.gradual.roundlimit;
    stream << (qint8)s.gradual.roundshuffle;
    stream << (qint8)s.single.prefernew;
    stream << (qint8)s.single.preferhard;

    return stream;
}

QDataStream& operator>>(QDataStream& stream, WordStudySettings &s)
{
    qint32 i;
    qint8 c;
    quint8 b;
    stream >> i;
    s.tested = i;
    stream >> i;
    s.typed = i;
    stream >> c;
    s.hidekana = c;
    stream >> c;
    s.matchkana = c;
    stream >> b;
    s.answer = (WordStudyAnswering)b;
    stream >> b;
    s.method = (WordStudyMethod)b;
    stream >> c;
    s.usetimer = c;
    stream >> i;
    s.timer = i;
    stream >> c;
    s.uselimit = c;
    stream >> i;
    s.limit = i;
    stream >> c;
    s.usewordlimit = c;
    stream >> i;
    s.wordlimit = i;
    stream >> c;
    s.shuffle = c;
    stream >> i;
    s.gradual.initnum = i;
    stream >> i;
    s.gradual.incnum = i;
    stream >> i;
    s.gradual.roundlimit = i;
    stream >> c;
    s.gradual.roundshuffle = c;
    stream >> c;
    s.single.prefernew = c;
    stream >> c;
    s.single.preferhard = c;

    return stream;
}


//-------------------------------------------------------------


WordStudy::WordStudy(WordGroup *owner) : owner(owner)
{
    memset(&settings, 0, sizeof(WordStudySettings));
}

WordStudy::~WordStudy()
{
}

WordStudy::WordStudy(WordStudy &&src) : owner(nullptr)
{
    *this = std::move(src);
}

WordStudy& WordStudy::operator=(WordStudy &&src)
{
    std::swap(owner, src.owner);
    std::swap(settings, src.settings);
    std::swap(list, src.list);
    std::swap(testitems, src.testitems);
    std::swap(state, src.state);

    return *this;
}

void WordStudy::load(QDataStream &stream)
{
    stream >> settings;

    stream >> make_zvec<qint32, WordStudyItem>(list);

    qint32 i;
    stream >> i;
    testitems.resize(i);
    for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
    {
        TestItem &t = testitems[ix];
        stream >> i;
        t.pos = i;
        stream >> i;
        t.tested = (WordStudyQuestion)i;
        stream >> i;
        t.correct = i;
        stream >> i;
        t.incorrect = i;
    }

    qint8 b;
    stream >> b;
    if (b)
    {
        if (settings.method == WordStudyMethod::Gradual)
            state.reset(new WordStudyGradual(settings.gradual));
        else if (settings.method == WordStudyMethod::Single)
            state.reset(new WordStudySingle);

        state->load(stream);
    }
}

void WordStudy::save(QDataStream &stream) const
{
    // TODO: ? Place "guard strings" in the head of every save function everywhere?

    stream << settings;

    stream << make_zvec<qint32, WordStudyItem>(list);

    stream << (qint32)testitems.size();
    for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
    {
        const TestItem &i = testitems[ix];
        stream << (qint32)i.pos;
        stream << (qint32)i.tested;
        stream << (qint32)i.correct;
        stream << (qint32)i.incorrect;
    }

    stream << (qint8)(state != nullptr);

    if (state != nullptr)
        state->save(stream);
}

WordGroup* WordStudy::getOwner()
{
    return owner;
}

void WordStudy::applyChanges(const std::map<int, int> &changes)
{
    std::set<int> found;

    // Fixing list testitems and the testitems list.
    std::vector<WordStudyItem> tmp = std::move(list);
    list.reserve(tmp.size());
    for (int ix = 0, siz = tosigned(tmp.size()); ix != siz; ++ix)
    {
        auto it = changes.find(tmp[ix].windex);
        if (it == changes.end() || it->second == -1 || found.count(it->second))
        {
            if (it != changes.end())
            {
                for (int iy = tosigned(testitems.size()) - 1; iy != -1; --iy)
                {
                    if (testitems[iy].pos > ix)
                        --testitems[iy].pos;
                    else if (testitems[iy].pos == ix)
                    {
                        testitems.erase(testitems.begin() + iy);

                        if (state != nullptr && !state->itemRemoved(iy, testSize()))
                        {
                            // TODO: notify the user that the study of the group has been abandoned.
                            // If possible in a single message about every group.
                            state.reset();
                        }
                    }
                }
                --ix;
            }
            continue;
        }
        found.insert(it->second);
        tmp[ix].windex = it->second;
        list.push_back(tmp[ix]);
    }

    list.shrink_to_fit();
}

void WordStudy::copy(WordStudy *src)
{
    settings = src->settings;
    list = src->list;
    testitems = src->testitems;

    if (src->state == nullptr)
        state.reset();
    else
    {
        if (settings.method == WordStudyMethod::Gradual)
            state.reset(new WordStudyGradual(settings.gradual));
        else if (settings.method == WordStudyMethod::Single)
            state.reset(new WordStudySingle);

        state->copy(src->state.get());
    }

}

void WordStudy::processRemovedWord(int windex)
{
    int indexindex = -1;
    int posindex = -1;

    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        if (list[ix].windex == windex)
            indexindex = ix;
        else if (list[ix].windex > windex)
            --list[ix].windex;
    }

    if (indexindex != -1)
    {
        list.erase(list.begin() + indexindex);

        for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
        {
            TestItem &item = testitems[ix];
            if (item.pos == indexindex)
                posindex = ix;
            else if (item.pos > indexindex)
                --item.pos;
        }
    }

    if (posindex == -1)
        return;

    testitems.erase(testitems.begin() + posindex);

    if (state != nullptr && !state->itemRemoved(posindex, testSize()))
    {
        // TODO: (maybe) notify the user that the study of the group has been abandoned
        state.reset();
    }
}

const std::vector<WordStudyItem>& WordStudy::indexes() const
{
    return list;
}

void WordStudy::correctedIndexes(std::vector<WordStudyItem> &clist) const
{
    // The passed clist might be list. When changing this function make sure list stays
    // correct.

    const std::vector<int> &glist = owner->getIndexes();

    std::vector<WordStudyItem> tmp(glist.size());
    for (int ix = 0, siz = tosigned(tmp.size()); ix != siz; ++ix)
        tmp[ix].windex = glist[ix];

    std::vector<int> gorder(glist.size(), 0);
    for (int ix = 0, siz = tosigned(glist.size()); ix != siz; ++ix)
        gorder[ix] = ix;
    std::sort(gorder.begin(), gorder.end(), [&glist](int a, int b) { return glist[a] < glist[b]; });

    if (clist.empty())
        clist = list;
    std::vector<int> order(clist.size(), 0);
    for (int ix = 0, siz = tosigned(clist.size()); ix != siz; ++ix)
        order[ix] = ix;
    std::sort(order.begin(), order.end(), [&clist](int a, int b) { return clist[a].windex < clist[b].windex; });

    for (int gpos = 0, gsiz = tosigned(glist.size()), pos = 0, siz = tosigned(clist.size()); pos != siz && gpos != gsiz;)
    {
        while (gpos != gsiz && glist[gorder[gpos]] < clist[order[pos]].windex)
            ++gpos;
        if (gpos == gsiz)
            break;
        while (pos != siz && glist[gorder[gpos]] > clist[order[pos]].windex)
            ++pos;
        if (pos == siz)
            break;

        while (gpos != gsiz && pos != siz && glist[gorder[gpos]] == clist[order[pos]].windex)
        {
            tmp[gorder[gpos]] = clist[order[pos]];

            ++gpos;
            ++pos;
        }
    }

    std::swap(clist, tmp);
}

void WordStudy::initTest()
{
    if (state == nullptr)
    {
        // Update WordTestSettingsForm::closeEvent() if this part changes.

        bool onlykanjikana = (settings.tested & ~((int)WordStudyQuestion::Kanji | (int)WordStudyQuestion::Kana)) == 0;

        // Generate testitems.
        testitems.reserve(list.size());
        for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        {
            if (!list[ix].excluded && (!onlykanjikana || hiraganize(dictionary()->wordEntry(list[ix].windex)->kanji) != hiraganize(dictionary()->wordEntry(list[ix].windex)->kana)))
                testitems.push_back(TestItem(ix, (WordStudyQuestion)createRandomTested(ix, settings.tested)));
        }

        if (settings.method == WordStudyMethod::Single && settings.usewordlimit && (settings.single.preferhard || settings.single.prefernew) && tosigned(testitems.size()) > settings.wordlimit)
        {
            int maxnew = -1;
            int minnew = 0;
            int maxhard = -1;
            int minhard = 0;
            for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
            {
                WordStudyItem &item = list[testitems[ix].pos];
                if (minnew < maxnew)
                {
                    minnew = maxnew = item.correct + item.incorrect;
                    minhard = maxhard = item.correct - item.incorrect;
                    continue;
                }
                if (minnew > item.correct + item.incorrect)
                    minnew = item.correct + item.incorrect;
                if (maxnew < item.correct + item.incorrect)
                    maxnew = item.correct + item.incorrect;

                if (minhard > item.correct - item.incorrect)
                    minhard = item.correct - item.incorrect;
                if (maxhard < item.correct - item.incorrect)
                    maxhard = item.correct - item.incorrect;
            }

            // Pairs of [score, testitem index]. The settings.wordlimit number of items with
            // the top score will be tested only.
            std::vector<std::pair<int, int>> scoring;

            scoring.reserve(testitems.size());
            int rndmax = tosigned(testitems.size()) * 20 * (settings.single.preferhard && settings.single.prefernew ? 2 : 1);
            double newdiv = maxnew <= minnew ? 0 : (rndmax / 2.0) / double(maxnew - minnew);
            double harddiv = maxhard <= minhard ? 0 : (rndmax / 2.0) / double(maxhard - minhard);
            for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
            {
                WordStudyItem &item = list[testitems[ix].pos];
                int sum = item.correct + item.incorrect;
                int dif = item.correct - item.incorrect;
                scoring.push_back(std::make_pair(rnd(1, rndmax) - (settings.single.prefernew && maxnew > minnew ? int((sum - minnew) * newdiv) : 0) + (settings.single.preferhard && maxhard > minhard ? int((dif - minnew) * harddiv) : 0), ix));
            }

            std::sort(scoring.begin(), scoring.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
                return a.first > b.first;
            });

            // Scoring holds an ordering of testitems. Anything below settings.wordlimit
            // should be removed from testitems.

            for (int ix = settings.wordlimit, siz = tosigned(scoring.size()); ix != siz; ++ix)
                testitems[scoring[ix].second].pos = -1;

            testitems.resize(std::remove_if(testitems.begin(), testitems.end(), [](const TestItem &item) {
                return item.pos == -1;
            }) - testitems.begin());
        }

        if (settings.shuffle)
            std::shuffle(testitems.begin(), testitems.end(), random_engine());

        if (settings.method == WordStudyMethod::Gradual)
            state.reset(new WordStudyGradual(settings.gradual));
        else if (settings.method == WordStudyMethod::Single)
            state.reset(new WordStudySingle);
    }

    state->init(testSize());
    dictionary()->setToUserModified();
}

bool WordStudy::initNext()
{
    dictionary()->setToUserModified();
    return state->initNext(testSize());
}

void WordStudy::finish()
{
    state->finish(testSize());
    dictionary()->setToUserModified();
}

const WordStudySettings& WordStudy::studySettings() const
{
    if (settingsValid())
        return settings;
    else
        return ZKanji::defaultWordStudySettings();
}

Dictionary* WordStudy::dictionary() const
{
    return owner->dictionary();
}

int WordStudy::testSize() const
{
    return settings.usewordlimit ? std::min<int>(tosigned(testitems.size()), settings.wordlimit) : tosigned(testitems.size());
}

void WordStudy::setup(const WordStudySettings &newsettings, const std::vector<WordStudyItem> &newlist)
{
    settings = newsettings;
    list = newlist;

    testitems.clear();

    state.reset();

    dictionary()->setToUserModified();
}

void WordStudy::abort()
{
    applyScore();

    testitems.clear();

    state.reset();

    dictionary()->setToUserModified();
}

void WordStudy::excludeWord(int windex)
{
    auto it = std::find_if(list.begin(), list.end(), [windex](const WordStudyItem &item) { return item.windex == windex; });
    if (it == list.end() || it->excluded)
        return;

    dictionary()->setToUserModified();
    it->excluded = true;
}

void WordStudy::includeWord(int windex)
{
    auto it = std::find_if(list.begin(), list.end(), [windex](const WordStudyItem &item) { return item.windex == windex; });
    if (it == list.end() || !it->excluded)
        return;
    dictionary()->setToUserModified();
    it->excluded = false;
}

bool WordStudy::suspended() const
{
    return state != nullptr && !state->finished(testSize());
}

int WordStudy::nextIndex()
{
    if (state == nullptr)
        return -1;

    int p = state->index(testSize());
    if (p < 0)
        return -1;
    return list[testitems[p].pos].windex;
}

int WordStudy::nextQuestion()
{
    int p = state->index(testSize());
    if (p < 0)
        throw "Don't call when there's no word to test next.";

    WordStudyQuestion q = testitems[p].tested;
    if (q == WordStudyQuestion::KanjiKana)
        return (int)WordPartBits::Kanji | (int)WordPartBits::Kana;
    if (q == WordStudyQuestion::DefKanji)
        return (int)WordPartBits::Kanji | (int)WordPartBits::Definition;
    if (q == WordStudyQuestion::DefKana)
        return (int)WordPartBits::Kana | (int)WordPartBits::Definition;
    return (int)q;
}

void WordStudy::randomIndexes(int *arr, int cnt)
{
    int wix = nextIndex();
    if (wix == -1)
        throw "Don't call when there's no word to test next.";

    // Select a position in the array for the next word's index.
    int currpos = rnd(0, cnt - 1);
    arr[currpos] = wix;

    // Copy of list contains possible word testitems to fill in arr. Already added testitems
    // are removed one by one.
    std::vector<int> listcpy;
    listcpy.reserve(list.size());
    int testedpos = testitems[state->index(testSize())].pos;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        if (testedpos != ix && !list[ix].excluded)
            listcpy.push_back(list[ix].windex);

    while (cnt-- != 0)
    {
        if (currpos == cnt)
            continue;
        int pos = rnd(0, tosigned(listcpy.size()) - 1);
        arr[cnt] = listcpy[pos];

        listcpy.erase(listcpy.begin() + pos);
    }
}

void WordStudy::answer(bool correct)
{
    int pos = state->index(testSize());
#ifdef _DEBUG
    if (pos < 0)
        throw "Can't answer non existing items";
#endif
    if (correct)
        ++testitems[pos].correct;
    if (!correct)
        ++testitems[pos].incorrect;
    state->answer(correct, testSize());
    dictionary()->setToUserModified();
}

void WordStudy::changeAnswer(bool correct)
{
    int p = state->roundPosition() - 1;
    setTestedCorrect(p, correct);
    dictionary()->setToUserModified();
}

bool WordStudy::canUndo() const
{
    return state->roundPosition() > 0 && (state->index(testSize()) != state->indexAt(state->roundPosition() - 1, testSize()));
}

bool WordStudy::finished() const
{
    return state == nullptr || state->finished(testSize());
}

void WordStudy::applyScore()
{
    if (settings.method != WordStudyMethod::Single || state == nullptr)
        return;

    bool changed = false;
    for (int ix = 0, siz = state->roundSize(testSize()); ix != siz; ++ix)
    {
        int p = state->indexAt(ix, testSize());
        TestItem &titem = testitems[p];
        WordStudyItem &item = list[titem.pos];

        if (titem.correct == 0 && titem.incorrect == 0)
            continue;

        changed = true;

        if (titem.incorrect)
            ++item.incorrect;
        else
            ++item.correct;

        titem.correct = titem.incorrect = 0;
    }
    if (changed)
        dictionary()->setToUserModified();
}

int WordStudy::testedCount() const
{
    return state->roundSize(testSize());
}

const WordStudyItem& WordStudy::testedItems(int pos) const
{
    int p = state->indexAt(pos, testSize());
#ifdef _DEBUG
    if (p < 0)
        throw "Can't get index of non existing items";
#endif
    return list[testitems[p].pos];
}

int WordStudy::testedIndex(int pos) const
{
    int p = state->indexAt(pos, testSize());
#ifdef _DEBUG
    if (p < 0)
        throw "Can't get index of non existing items";
#endif
    return list[testitems[p].pos].windex;
}

bool WordStudy::testedCorrect(int pos) const
{
#ifdef _DEBUG
    int p = state->indexAt(pos, testSize());
    if (p < 0)
        throw "Can't get index of non existing items";
#endif

    return state->correctAt(pos, testSize());
}

void WordStudy::setTestedCorrect(int pos, bool correct)
{
    int p = state->indexAt(pos, testSize());
#ifdef _DEBUG
    if (p < 0)
        throw "Can't get index of non existing items";
#endif

    if (state->correctAt(pos, testSize()) == correct)
        return;

    state->setCorrectAt(pos, correct, testSize());
    if (correct)
    {
        ++testitems[p].correct;
        --testitems[p].incorrect;
    }
    else
    {
        --testitems[p].correct;
        ++testitems[p].incorrect;
    }
    dictionary()->setToUserModified();
}

bool WordStudy::previousCorrect() const
{
#ifdef _DEBUG
    if (state == nullptr || state->roundPosition() <= 0)
        throw "Accessing null state.";
#endif
    int p = state->roundPosition();

    return state->correctAt(p - 1, testSize());
}

int WordStudy::currentRound() const
{
    if (settings.method != WordStudyMethod::Gradual)
        return 0;
    return state == nullptr ? 0 : ((const WordStudyGradual*)state.get())->currentRound();
}

int WordStudy::newCount() const
{
    if (settings.method != WordStudyMethod::Gradual)
        return 0;
    dictionary()->setToUserModified();
    return ((const WordStudyGradual*)state.get())->newCount(testSize());
}

int WordStudy::dueCount() const
{
    return state->dueCount(testSize());
}

int WordStudy::correctCount() const
{
    return state->correctCount();
}

int WordStudy::wrongCount() const
{
    return state->wrongCount();
}

bool WordStudy::nextNew() const
{
    return state->nextNew();
}

bool WordStudy::nextFailed() const
{
    return state->nextFailed(testSize());
}

int WordStudy::createRandomTested(int lix, int choices)
{
    if (choices == 0)
        throw "Empty choices";

    // Count number of bits set in choices.
    int hi = 0;
    for (int ix = 0; ix != 32; ++ix)
        if ((choices & (1 << ix)) != 0)
            ++hi;

    if (hi <= 1)
        return choices;

    // We must exclude only kanji and only kana questions from the choices if the kana and
    // kanji forms of a word are the same.
    if ((choices & ((int)WordStudyQuestion::Kanji | (int)WordStudyQuestion::Kana)) != 0 &&
        hiraganize(dictionary()->wordEntry(list[lix].windex)->kanji) == hiraganize(dictionary()->wordEntry(list[lix].windex)->kana))
    {
        if ((choices & (int)WordStudyQuestion::Kanji) != 0)
            --hi;
        if ((choices & (int)WordStudyQuestion::Kana) != 0)
            --hi;
        choices &= ~((int)WordStudyQuestion::Kanji | (int)WordStudyQuestion::Kana);
#ifdef _DEBUG
        if (choices == 0)
            throw "No question type to select when word has same written and kana forms.";
#endif
    }

    //int r = -1;

    // Pick a random bit.
    hi = rnd(1, hi);

    int pos = -1;
    do
    {
        ++pos;
        if ((choices & (1 << pos)) != 0)
            --hi;
    } while (hi != 0);

    return (1 << pos);
}

bool WordStudy::settingsValid() const
{
    return settings.tested != 0;
}


//-------------------------------------------------------------


WordStudyGradual::WordStudyGradual(const WordStudyGradualSettings &settings) : settings(settings), round(-1), included(0), pos(-1), unanswered(false)
{

}

void WordStudyGradual::load(QDataStream &stream)
{
    qint32 i;
    qint16 s;
    qint8 c;
    stream >> i;
    round = i;
    stream >> i;
    included = i;
    stream >> i;
    pos = i;

    stream >> s;
    testitems.resize(s);
    for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
    {
        Item &t = testitems[ix];
        stream >> i;
        t.index = i;
        stream >> c;
        t.newitem = c;
        stream >> c;
        t.correct = c;
    }

}

void WordStudyGradual::save(QDataStream &stream) const
{
    stream << (qint32)round;
    stream << (qint32)included;
    stream << (qint32)(pos - (unanswered ? 1 : 0));

    stream << (qint16)testitems.size();
    for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
    {
        const Item &i = testitems[ix];
        stream << (qint32)i.index;
        stream << (qint8)i.newitem;
        stream << (qint8)i.correct;
    }
}

void WordStudyGradual::copy(WordStudyState *src)
{
    round = ((WordStudyGradual*)src)->round;
    repeats = ((WordStudyGradual*)src)->repeats;
    included = ((WordStudyGradual*)src)->included;
    pos = ((WordStudyGradual*)src)->pos;
    unanswered = ((WordStudyGradual*)src)->unanswered;
    testitems = ((WordStudyGradual*)src)->testitems;
}

void WordStudyGradual::init(int indexcnt)
{
    if (round == -1)
    {
        // New test.
        round = 0;
        repeats = 0;
        pos = -1;
        included = std::min(indexcnt, settings.initnum);

        for (int ix = 0; ix != included; ++ix)
            testitems.emplace_back(ix, true);

        if (settings.roundshuffle)
            std::shuffle(testitems.begin(), testitems.end(), random_engine());
        return;
    }

    // Nothing to do in the middle of a round.
    if (pos + 1 < tosigned(testitems.size()))
        return;

    pos = -1;

    // If any item was incorrectly answered, repeat the same test.
    for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
    {
        if (!testitems[ix].correct)
        {
            // Repeat round.

            for (; ix != siz; ++ix)
            {
                testitems[ix].newitem = false;
                testitems[ix].correct = true;
            }

            ++repeats;

            if (settings.roundshuffle)
                std::shuffle(testitems.begin(), testitems.end(), random_engine());

            return;
        }
    }

    // The items tested in the latest round are generated from the new items, which are always
    // included, and random items from the past round-size number of items * 2.
    ++round;
    repeats = 0;
    testitems.clear();
    if (included < indexcnt)
    {
        int lastinc = included;
        included = std::min(included + settings.incnum, indexcnt);

        // Number of items to study apart from the new ones.
        int num = std::min(settings.roundlimit, lastinc);

        // Selecting items to test from the past items by adding all possible items, and
        // randomly removing some of them.
        for (int ix = std::max(0, lastinc - settings.roundlimit * 2); ix < lastinc; ++ix)
            testitems.emplace_back(ix, false);

        int testsize = tosigned(testitems.size());
        while (testsize != num)
        {
            testitems.erase(testitems.begin() + rnd(0, testsize - 1));
            --testsize;
        }

        // Add the indexes of the new items.
        for (int ix = lastinc; ix < included; ++ix)
            testitems.emplace_back(ix, true);

        if (settings.roundshuffle)
            std::shuffle(testitems.begin(), testitems.end(), random_engine());
        return;
    }

    // Randomly selecting items to test from past items.
    for (int ix = std::max(0, included - settings.roundlimit * 2); ix != included; ++ix)
        testitems.emplace_back(ix, false);

    int testsize = tosigned(testitems.size());
    while (testsize > settings.roundlimit)
    {
        testitems.erase(testitems.begin() + rnd(0, testsize - 1));
        --testsize;
    }
    if (settings.roundshuffle)
        std::shuffle(testitems.begin(), testitems.end(), random_engine());
}

bool WordStudyGradual::initNext(int /*indexcnt*/)
{
    if (pos + 1 >= tosigned(testitems.size()))
        return false;

    ++pos;
    unanswered = true;
    return true;
}

void WordStudyGradual::finish(int /*indexcnt*/)
{
    if (unanswered)
        --pos;
    unanswered = false;
}

bool WordStudyGradual::itemRemoved(int index, int indexcnt)
{
    if (index >= included)
        return std::max(settings.initnum + settings.incnum, settings.roundlimit) <= indexcnt;
    --included;

    for (int ix = tosigned(testitems.size()) - 1; ix != -1; --ix)
    {
        if (testitems[ix].index > index)
            --testitems[ix].index;
        else if (testitems[ix].index == index)
        {
            if (pos > ix)
                --ix;
            testitems.erase(testitems.begin() + ix);
        }
    }

    // Fix the round in case too many words have been erased.

    // If we are at the end of the tested words, there's no way to decide whether the test can
    // continue or not.
    if (included == indexcnt)
        return false;

    round = (std::max(0, included - std::min(indexcnt, settings.initnum)) + settings.incnum - 1) / settings.incnum;

    return !testitems.empty() && std::max(settings.initnum + settings.incnum, settings.roundlimit) <= indexcnt;
}

int WordStudyGradual::index(int /*indexcnt*/) const
{
    if (pos >= tosigned(testitems.size()))
        return -1;
    return testitems[pos].index;
}

int WordStudyGradual::roundPosition() const
{
    return pos;
}

int WordStudyGradual::roundSize(int /*indexcnt*/) const
{
    return tosigned(testitems.size());
}

int WordStudyGradual::indexAt(int p, int /*indexcnt*/) const
{
    return testitems[p].index;
}

void WordStudyGradual::answer(bool correct, int /*indexcnt*/)
{
    if (!correct)
        testitems[pos].correct = false;
    testitems[pos].newitem = false;
    unanswered = false;
}

bool WordStudyGradual::finished(int indexcnt) const
{
    if (included != indexcnt)
        return false;
    int maxrounds = ((indexcnt - std::min(indexcnt, settings.initnum)) / settings.incnum) + 2;
    return round > maxrounds || (round == maxrounds && pos >= (int)testitems.size() - 1);
}

bool WordStudyGradual::correctAt(int p, int /*indexcnt*/) const
{
    return testitems[p].correct;
}

void WordStudyGradual::setCorrectAt(int p, bool correct, int /*indexcnt*/)
{
    testitems[p].correct = correct;
}

int WordStudyGradual::currentRound() const
{
    return round;
}

int WordStudyGradual::newCount(int /*indexcnt*/) const
{
    int cnt = 0;
    for (int ix = 0, siz = tosigned(testitems.size()); ix != siz; ++ix)
        cnt += testitems[ix].newitem ? 1 : 0;
    return cnt;
}

int WordStudyGradual::dueCount(int /*indexcnt*/) const
{
    return tosigned(testitems.size()) - pos;
}

int WordStudyGradual::correctCount() const
{
    int cnt = 0;
    for (int ix = 0; ix != pos; ++ix)
        cnt += testitems[ix].correct ? 1 : 0;
    return cnt;
}

int WordStudyGradual::wrongCount() const
{
    int cnt = 0;
    for (int ix = 0; ix != pos; ++ix)
        cnt += !testitems[ix].correct ? 1 : 0;
    return cnt;
}

bool WordStudyGradual::nextNew() const
{
    return testitems[pos].newitem;
}

bool WordStudyGradual::nextFailed(int /*indexcnt*/) const
{
    return false;
}


//-------------------------------------------------------------


WordStudySingle::WordStudySingle() : pos(-1), unanswered(false)
{

}

void WordStudySingle::load(QDataStream &stream)
{
    qint32 i;
    stream >> i;
    pos = i;
    stream >> make_zvec<qint32, quint8>(correctlist);
    stream >> make_zvec<qint32, qint32>(repeated);
}

void WordStudySingle::save(QDataStream &stream) const
{
    stream << (qint32)(pos - (unanswered ? 1 : 0));
    stream << make_zvec<qint32, quint8>(correctlist);
    stream << make_zvec<qint32, qint32>(repeated);
}

void WordStudySingle::copy(WordStudyState *src)
{
    pos = ((WordStudySingle*)src)->pos;
    unanswered = ((WordStudySingle*)src)->unanswered;
    correctlist = ((WordStudySingle*)src)->correctlist;
    repeated = ((WordStudySingle*)src)->repeated;
}

void WordStudySingle::init(int indexcnt)
{
    if (!unanswered)
        --indexcnt;
    if (pos != -1 && pos != indexcnt + tosigned(repeated.size()))
        return;
    pos = -1;
    unanswered = false;
    correctlist.clear();
    repeated.clear();
}

bool WordStudySingle::initNext(int indexcnt)
{
    if (pos + 1 >= indexcnt && pos - indexcnt + 1 >= tosigned(repeated.size()))
        return false;

    ++pos;
    unanswered = true;
    return true;
}

void WordStudySingle::finish(int /*indexcnt*/)
{
    if (unanswered)
        --pos;
    unanswered = false;
}

bool WordStudySingle::itemRemoved(int index, int indexcnt)
{
    if (pos > index)
    {
        if (pos < indexcnt)
        {
            --pos;
            correctlist.erase(correctlist.begin() + index);
        }
        else if (pos - indexcnt < tosigned(repeated.size()))
        {
            for (int ix = 0; ix != pos - indexcnt; ++ix)
                if (repeated[ix] == index)
                    --pos;
        }
    }

    removeIndexFromList(index, repeated);
    //for (int ix = repeated.size() - 1; ix != -1; --ix)
    //{
    //    if (repeated[ix] == index)
    //        repeated.erase(repeated.begin() + ix);
    //    else if (repeated[ix] > index)
    //        --repeated[ix];
    //}

    return indexcnt >= 10 && pos < tosigned(repeated.size()) + indexcnt;
}

int WordStudySingle::index(int indexcnt) const
{
    if (!unanswered)
        --indexcnt;
    if (pos < indexcnt)
        return pos;
    if (pos - indexcnt < tosigned(repeated.size()))
        return repeated[pos - indexcnt];
    return -1;
}

int WordStudySingle::roundPosition() const
{
    return pos;
}

int WordStudySingle::roundSize(int indexcnt) const
{
    return indexcnt;
}

int WordStudySingle::indexAt(int p, int indexcnt) const
{
    if (p < indexcnt)
        return p;
    if (p - indexcnt < tosigned(repeated.size()))
        return repeated[p - indexcnt];
    return -1;
}

void WordStudySingle::answer(bool correct, int indexcnt)
{
    if (pos < indexcnt)
        correctlist.push_back(correct ? 1 : 0);

    if (!correct)
    {
        int p = (pos < indexcnt ? pos : repeated[pos - indexcnt]);
        repeated.push_back(p);
    }

    unanswered = false;
}

bool WordStudySingle::finished(int indexcnt) const
{
    return index(indexcnt) == -1;
}

bool WordStudySingle::correctAt(int p, int indexcnt) const
{
    if (p < indexcnt)
        return correctlist[p] == 1;

    if (p >= pos)
        throw "Cannot access items at or after pos.";

    // An item is correct if it has not been added to the repeated list after the passed p
    // position. If the same index exists later, that answer was incorrect.
    p -= indexcnt;
    int i = repeated[p];
    while (++p != tosigned(repeated.size()))
        if (repeated[p] == i)
            return false;
    return true;
}

void WordStudySingle::setCorrectAt(int p, bool correct, int indexcnt)
{
    bool f = finished(indexcnt);
    if (!f && p != pos - 1)
        throw "Only the last answer can be changed during a test.";
    if (f && p >= indexcnt)
        throw "Cannot change answers beyond the index count.";

    if (p < indexcnt)
    {
        if (correctlist[p] == (uchar)correct)
            return;
        correctlist[p] = correct;
    }

    if (!f)
    {
        if (!correct)
            repeated.push_back(p);
        else if (repeated.back() == p)
            repeated.pop_back();
        else
            throw "The last wrong answer should be at the end of repeated.";
    }
}

int WordStudySingle::dueCount(int indexcnt) const
{
    if (!unanswered)
        --indexcnt;
    return indexcnt - pos + tosigned(repeated.size());
}

int WordStudySingle::correctCount() const
{
    int cnt = 0;
    return std::accumulate(correctlist.begin(), correctlist.end(), cnt);
}

int WordStudySingle::wrongCount() const
{
    int cnt = 0;
    return tosigned(correctlist.size()) - std::accumulate(correctlist.begin(), correctlist.end(), cnt);
}

bool WordStudySingle::nextNew() const
{
    return false;
}

bool WordStudySingle::nextFailed(int indexcnt) const
{
    return pos - indexcnt >= 0;
}


//-------------------------------------------------------------


namespace ZKanji
{
    static WordStudySettings defwset{
            (int)WordStudyQuestion::Definition, 0, false, false, WordStudyAnswering::CorrectWrong, WordStudyMethod::Gradual,
            false, 15, false, 15, false, 50, true, WordStudyGradualSettings{ 5, 3, 5, false }, WordStudySingleSettings{ true, false } };
    const WordStudySettings& defaultWordStudySettings()
    {
        return defwset;
    }

    void setDefaultWordStudySettings(const WordStudySettings &newsettings)
    {
        defwset = newsettings;
    }

}
