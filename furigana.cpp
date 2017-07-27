/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QTextStream>
#include <set>

#include "furigana.h"
#include "zkanjimain.h"
#include "romajizer.h"
#include "kanji.h"
#include "words.h"

const uchar consonantcolumn[84] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 101, 1, 101, 1, //14
    101, 1, 101, 1, 101, 2, 102, 2, 102, 2, 102, 2, 102, 2, 102, 3, //30
    103, 3, 103, 0, 3, 103, 3, 103, 3, 103, 4, 4, 4, 4, 4, 5, //46
    105, 205, 5, 105, 205, 5, 105, 205, 5, 105, 205, 5, 105, 205, 6, 6, //62
    6, 6, 6, 0, 7, 0, 7, 0, 7, 8, 8, 8, 8, 8, 9, 9, //78
    0, 0, 9, 4, 0
};


// Furigana search:

// Given a kanji and a kana form (which should match,) a list of FuriganaData items are
// produced. Each item marks a substring in kanji and kana (by their position and length).
// The kana substring can appear above the kanji as furigana. This data is also used for
// finding kanji readings in words (if the kanji substring contains only a single kanji.)
//
// Furigana is found by recursion. There's a current position saved in both kanji and kana.
// First furiganaStep() is called to skip any unwanted characters (kana, middle dot, etc.)
// that appear the same way both in kanji and kana at the current position. It in turn calls
// furiganaStep2() to fill the FuriganaData structures.
// There are several ways to do that. 1) the kanji readings are matched with the kana string
// after the current position. If a possible result is found, it calls furiganaStep() after
// advancing the positions in kanji and kana. If that recursive call returns with no result,
// the next reading is tested.
// 2) No reading was matched, but the kanji stands alone without other (unprocessed) kanji,
// surrounding it, the kana at the current position is assumed to be its furigana. First one
// kana by advancing the current position in kana by one. Then furiganaStep() is called to
// find results again. If it doesn't produce a result, we advance in kana again.
// 3) Checking all dictionaries for words matching the current kanji/kana position. Multiple
// substrings are checked like previously, by slowly advancing the current position in both
// kanji and kana. The kanji string must only contain kanji. If a match is found, the whole
// substring in kana is marked as the furigana for the kanji, and the following substring is
// matched with furiganaStep().
// 4) If the current characters in kanji are not kanji nor kana, but valid unicode characters
// that can appear in the dictionary, they might have a reading too. The longest substring in
// kanji that only contains such characters, is counted as one unit. Skipping kana position
// one by one, the rest of the strings are checked with furiganaStep().
// 5) When everything failed and the current character is a kanji with other kanji following,
// it's skipped by advancing the current kanji and kana position, calling findFurigana().
// While no result is found, kana is advanced until it's not possible. At the end, kanji is
// advanced by one again (now two kanji skipped) and the process starts over.
//
// Methods apart from the first one are basically hacks, because they don't matched kanji with
// real kanji readings. To avoid finding too many false positives, these steps increase a
// value called 'hacks' every time they are used. When one recursion returns, and the hacks
// value is not zero, the current method goes to its next step, and then the other methods are
// used as well. The current possible results are saved, and only overwritten if the next call
// to findFurigana() returns with less hacks.


bool furiganaStep2(const QCharString &kanji, const QCharString &kana, int &kanjistart, int &kanjiend, int &kanastart, int &kanaend, std::vector<FuriganaData> &dat, int &datpos, int &hacks);

// Determines the furigana positions inside a word defined by kanji and kana. Kanji/kana start/end are
// extents in the word searched in this step. Dat is updated with furigana positions, datpos is the
// number of usable values in dat.
bool furiganaStep(const QCharString &kanji, const QCharString &kana, int &kanjistart, int &kanjiend, int &kanastart, int &kanaend, std::vector<FuriganaData> &dat, int &datpos, int &hacks)
{
    //std::vector<int> l;
    //int ix;
    //int iy;
    //bool res = true;
    //wchar_t och, kch, tmp;

    if (kanjistart != 0 && kanjistart <= kanjiend && kanastart <= kanaend && KANJI(kanji[kanjistart].unicode()))
    {
        if (DASH(kana[kanastart].unicode()))
            return false;

        QChar ch = hiraganaCh(kana.data(), kanastart);

        if (ch == 0x3083 /* small ya */ || ch == 0x3085 /* small yu */ || ch == 0x3087 /* small yo */ ||
            ch == 0x3041 /* small a */ || ch == 0x3043 /* small i */ || ch == 0x3045 /* small u */ || ch == 0x3047 /* small e */ || ch == 0x3049 /* small o */)
            return false;
    }

    bool res = true;
    int ix = 0;

    bool changed = true;

    while (changed)
    {
        changed = false;
        // Exclude kana matching at the start of the checked parts.
        while (kanjistart <= kanjiend && kanastart <= kanaend && !KANJI(kanji[kanjistart].unicode()) &&
            !VALIDCODE(kanji[kanjistart].unicode()) && kanaMatch(kanji.data(), kanjistart, kana.data(), kanastart))
            kanjistart++, kanastart++, ix++, changed = true;

        // Word has a full width 0 character in front but no zero kana found. Skip the number.
        while (kanjistart <= kanjiend && kanastart <= kanaend && kanji[kanjistart] == 0xFF10 /* fullwidth zero */ &&
            (kanjistart == 0 || kanji[kanjistart - 1].unicode() < 0xFF10 || kanji[kanjistart - 1].unicode() > 0xFF19) &&
            (kanastart == kanaend || hiraganaCh(kana.data(), kanastart) != 0x305c /*hiragana ze*/ ||
            (kanastart < kanaend && hiraganaCh(kana.data(), kanastart + 1) != 0x308D /* hiragana ro */)))
            kanjistart++, ix++, changed = true;

        // Remove any dot punctuation in the middle of the word
        while (kanjistart <= kanjiend && kanastart <= kanaend &&
            (kanji[kanjistart] == 0x30fb /* kana mid-dot*/ ||
            kanji[kanjistart] == 0xff1d /* fullwidth equal sign*/ ||
            kanji[kanjistart] == 0xff0e /* fullwidth stop */ ||
            kanji[kanjistart] == 0xff0f /* fullwidth / */))
            kanjistart++, ix++, changed = true;
    }

    // The whole checked part is made up of a single kanji. The valid kana
    // string part is its furigana.
    if (kanjistart == 0 && kanjiend == 0 && kanastart <= kanaend && datpos == 0)
    {
        dat[datpos].kanji.pos = kanjistart;
        dat[datpos].kanji.len = 1;
        dat[datpos].kana.pos = kanastart;
        dat[datpos].kana.len = kanaend - kanastart + 1;
        kanjistart++;
        kanastart = kanaend + 1;
        datpos++;
        return true;
    }


    if (kanjistart <= kanjiend && kanastart <= kanaend)
    {
        res = furiganaStep2(kanji, kana, kanjistart, kanjiend, kanastart, kanaend, dat, datpos, hacks);
        if (!res && !datpos && ix == kanjistart + (kana.size() - 1 - kanaend) && kanjiend - kanjistart + 1 <= 3)
        {
            res = true;
            for (int i = kanjistart; i <= kanjiend && res; i++)
                if (!KANJI(kanji[i].unicode()) && !VALIDCODE(kanji[i].unicode()))
                    res = false;
            if (res)
            {
                dat[datpos].kanji.pos = kanjistart;
                dat[datpos].kanji.len = kanjiend - kanjistart + 1;
                dat[datpos].kana.pos = kanastart;
                dat[datpos].kana.len = kanaend - kanastart + 1;
                hacks += std::max(2, std::abs(dat[datpos].kana.len - dat[datpos].kanji.len)) + 1;
                kanastart = kanaend + 1;
                kanjistart = kanjiend + 1;
                datpos++;
            }
        }
    }
    else if (kanjistart <= kanjiend || kanastart <= kanaend)
        res = false;

    return res;
}

// Returns a list of readings for the passed character, to be used for
// furigana lookup.
//QCharStringList& validReadings(QChar vchar)
//{
//    switch (vchar.unicode())
//    {
//    case 0xFF10:
//        return zero0;
//    case 0xFF11:
//        return ichi1;
//    case 0xFF12:
//        return ni2;
//    case 0xFF13:
//        return san3;
//    case 0xFF14:
//        return yon4;
//    case 0xFF15:
//        return go5;
//    case 0xFF16:
//        return roku6;
//    case 0xFF17:
//        return nana7;
//    case 0xFF18:
//        return hachi8;
//    case 0xFF19:
//        return kyuu9;
//    case 0x3006:
//        return shimekun;
//    case 0xFF21:
//    case 0xFF41:
//        return letterA;
//    case 0xFF22:
//    case 0xFF42:
//        return letterB;
//    case 0xFF23:
//    case 0xFF43:
//        return letterC;
//    case 0xFF24:
//    case 0xFF44:
//        return letterD;
//    case 0xFF25:
//    case 0xFF45:
//        return letterE;
//    case 0xFF26:
//    case 0xFF46:
//        return letterF;
//    case 0xFF27:
//    case 0xFF47:
//        return letterG;
//    case 0xFF28:
//    case 0xFF48:
//        return letterH;
//    case 0xFF29:
//    case 0xFF49:
//        return letterI;
//    case 0xFF2A:
//    case 0xFF4A:
//        return letterJ;
//    case 0xFF2B:
//    case 0xFF4B:
//        return letterK;
//    case 0xFF2C:
//    case 0xFF4C:
//        return letterL;
//    case 0xFF2D:
//    case 0xFF4D:
//        return letterM;
//    case 0xFF2E:
//    case 0xFF4E:
//        return letterN;
//    case 0xFF2F:
//    case 0xFF4F:
//        return letterO;
//    case 0xFF30:
//    case 0xFF50:
//        return letterP;
//    case 0xFF31:
//    case 0xFF51:
//        return letterQ;
//    case 0xFF32:
//    case 0xFF52:
//        return letterR;
//    case 0xFF33:
//    case 0xFF53:
//        return letterS;
//    case 0xFF34:
//    case 0xFF54:
//        return letterT;
//    case 0xFF35:
//    case 0xFF55:
//        return letterU;
//    case 0xFF36:
//    case 0xFF56:
//        return letterV;
//    case 0xFF37:
//    case 0xFF57:
//        return letterW;
//    case 0xFF38:
//    case 0xFF58:
//        return letterX;
//    case 0xFF39:
//    case 0xFF59:
//        return letterY;
//    case 0xFF3A:
//    case 0xFF5A:
//        return letterZ;
//    case 0x3003:
//        return onajikun;
//    case 0x3007:
//        return marukun;
//    case 0xff06:
//        return andokun;
//    case 0xff20:
//        return attokun;
//    case 0xFF0B:
//        return purasukun;
//    default:
//        return noreadings;
//    }
//}

// Length of kanji kun reading without okurigana.
int kunLen(const QChar *kun)
{
    const QChar *c = kun;

    while (*c != 0)
    {
        if (*c == '.')
            break;
        ++c;
    }
    return c - kun;
}

// Length of kanji on reading without the leading dash.
int onLen(const QChar *on)
{
    return qcharlen(on) - (on[0] == '-' ? 1 : 0);
}

// Checks whether the passed reading might be found at kanastart in kana, ending at or before
// kanaend.
// Returns 0 if the kana does not match the passed reading, and a positive number if the
// reading might match. The larger the returned value, the bigger the difference between the
// reading and the actual kana string. (Differences are possible between them because the
// sounds can change in context.)
// Pass an integer to matchlen to receive the number of characters found in kana at kanastart,
// that match the characters at the front of reading.
int fuReading(const QChar *reading, int readinglen, const QChar *kana, int kanastart, int kanaend, int *mismatch = nullptr, bool recurse = false)
{
    //int readinglen = kunLen(reading);
    int val = 0;

    //if (readinglen > kanaend - kanastart + 1)
    //    return 0;

    bool changedlen = false;
    int siz = readinglen;

    if (readinglen > kanaend - kanastart + 1)
    {
        if (mismatch == nullptr)
            return 0;

        changedlen = true;
        siz = kanaend - kanastart + 1;
    }


    int ix;
    QChar kch2;

    for (ix = 0; ix != siz && kanastart + ix <= kanaend; ++ix)
    {
        if (kanaMatch(kana, kanastart + ix, reading, ix))
            continue;

        QChar och = hiraganaCh(reading, ix);
        QChar kch = hiraganaCh(kana, kanastart + ix);

        if (!changedlen && ix == readinglen - 1 && readinglen != 1)
        {
            if (kch == 0x3063 /* minitsu */)
            {
                if (och == 0x3061 /* chi */)
                {
                    val += 2;
                    continue;
                }
                if (och == 0x3064 /* tsu */)
                {
                    val += 1;
                    continue;
                }
            }
            if (och == 0x3064 /* tsu */ && kch == 0x3061 /* chi */)
            {
                val += 2;
                continue;
            }
            if (och == 0x3061 /* chi */ && kch == 0x3058 /* ji */)
            {
                val += 2;
                continue;
            }
            if (och == 0x305F /* ta */ && kch == 0x3060 /* da */)
            {
                val += 1;
                continue;
            }
        }

        if (ix == 0 && !recurse)
        {
            if ((och == 0x3061 /* chi */ && kch == 0x3058 /* ji */) || (och == 0x3064 /* tsu */ && kch == 0x305A /* zu */))
            {
                val += 2;
                continue;
            }
            if (kch.unicode() == och.unicode() + 1 && ((och.unicode() >= 0x304b && och.unicode() < 0x3062 && (och.unicode() % 2)) || (och.unicode() >= 0x3064 && och.unicode() < 0x3069 && !(och.unicode() % 2))))
            {
                ++val;
                continue;
            }
            if (och.unicode() >= 0x306f && och.unicode() < 0x307d && (kch.unicode() == och.unicode() + 1 || kch.unicode() == och.unicode() + 2) && !(och.unicode() % 3))
            {
                ++val;
                continue;
            }
        }

        if ((och == 0x30F6 /* miniKE */ && kch == 0x304B /* ka */) || (och == 0x30F5 /* miniKA */ && kch == 0x304B /* ka */) ||
            (och == 0x3088 /* yo */ && kch == 0x3087 /* miniyo */) || (och == 0x3086 /* yu */ && kch == 0x3085 /* miniyu */) || (och == 0x3084 /* ya */ && kch == 0x3083 /* miniya */) || //h row matches
            (och == 0x3042 /* a */ && kch == 0x308F /* wa */))
        {
            ++val;
            continue;
        }

        if (kanastart + ix + 1 <= kanaend && (!changedlen && ix == readinglen - 1 && readinglen != 1) && kch == 0x3063 /* minitsu */ && och.unicode() - 0x3041 >= 0 && och.unicode() - 0x3041 < 84)
        {
            kch2 = hiraganaCh(kana, kanastart + ix + 1);
            if (kch2.unicode() - 0x3041 >= 0 && kch2.unicode() - 0x3041 < 84 && consonantcolumn[och.unicode() - 0x3041] == consonantcolumn[kch2.unicode() - 0x3041] && consonantcolumn[och.unicode() - 0x3041] > 0)
                val += 2;
            continue;
        }
        break;
    }

    if (mismatch != nullptr)
    {
        if (ix == 0)
        {
            *mismatch = 999999;
            if (!recurse)
            {
                // In case there was no match at all, we try again by skipping the first characters in
                // both the reading and the current kana position.

                int retrymatch;

                if (kanastart != kanaend && fuReading(reading, readinglen, kana, kanastart + 1, kanaend, &retrymatch, true))
                    *mismatch = std::min(*mismatch, 2 + retrymatch + 1);

                if (readinglen > 1 && fuReading(reading + 1, readinglen - 1, kana, kanastart, kanaend, &retrymatch, true))
                    *mismatch = std::min(*mismatch, 2 + retrymatch + 1);

                if (readinglen > 1 && kanastart != kanaend && fuReading(reading + 1, readinglen - 1, kana, kanastart + 1, kanaend, &retrymatch, true))
                    *mismatch = std::min(*mismatch, 4 + retrymatch + 2);
            }

            *mismatch = std::min(*mismatch, std::max((kanaend - kanastart + 1), readinglen) + 1);
        }
        else
            *mismatch = readinglen - ix + (kanaend - kanastart + 1) - ix + val / 2 + std::max(0, (kanaend - kanastart + 1) - readinglen);

        if (readinglen != kanaend - kanastart + 1)
            return 0;
    }

    return (!changedlen && ix == readinglen) ? std::max(val, 1) : 0;
}

bool kanjiOrKurikaeshi(const QCharString &kanji, int pos)
{
    return (pos != 0 && kanji[pos].unicode() == KURIKAESHI && KANJI(kanji[pos - 1].unicode())) || KANJI(kanji[pos].unicode());
}

int furiReadingDiff(KanjiEntry *k, const QCharString &kana, int kanastart, int kanaend)
{
    int mindiff = 999999;

    int mismatch;

    for (int ix = 0; ix < k->on.size(); ++ix)
    {
        const QChar *on = k->on.items(ix).data();
        if (on[0] == '-')
            ++on;
        int onlen = qcharlen(on);
        if (!fuReading(on, onlen, kana.data(), kanastart, kanaend, &mismatch))
            mindiff = std::min(mindiff, mismatch);
        else
            return 0;
    }

    // Try to match kanji KUN-readings or irregular readings.

    for (int ix = 0; ix < k->kun.size(); ++ix)
    {
        int klen = kunLen(k->kun[ix].data());
        if (!fuReading(k->kun[ix].data(), klen, kana.data(), kanastart, kanaend, &mismatch))
            mindiff = std::min(mindiff, mismatch);
        else
            return 0;
    }

    return mindiff;
}

// Called during furigana lookup. Fills dat and datpos with furigana data corresponding to
// kanji/kana start/end. Checks kanji readings and tries to match them with the kana. In case
// it doesn't work, try other methods, like looking up shorter words that have the exact same
// form.
bool furiganaStep2(const QCharString &kanji, const QCharString &kana, int &kanjistart, int &kanjiend, int &kanastart, int &kanaend, std::vector<FuriganaData> &dat, int &datpos, int &hacks)
{
    // We rely on the fact that set holds its values in increasing order. Don't change it
    // unless sorting is added later.
    std::set<int> l;

    KanjiEntry *k = nullptr;
    int kindex = -1;
    bool res = false;

    if ((kanjistart == 0 && kanji[0].unicode() == KURIKAESHI) || ((!VALIDCODE(kanji[kanjistart].unicode()) || kanji[kanjistart].unicode() == KURIKAESHI) && (kindex = ZKanji::kanjiIndex(kanji[(kanji[kanjistart].unicode() == KURIKAESHI ? kanjistart - 1 : kanjistart)])) < 0))
        return false;

    if (kindex >= 0)
        k = ZKanji::kanjis[kindex];

    // Values saved after furiganaStep() returns with less hacks than before.

    int lastkanjistart = -1;
    int lastkanastart = -1;
    int lastdatpos = -1;

    // First set to the hacks value returned by furiganaStep(). Only updated (with the rest
    // of the 'last...' values) if the next furiganaStep() call returns with less hacks.
    int lasthacks = -1;
    // Hacks that will be added in this step. Only updated with the other 'last...' values.
    // It'll increase the value of 'hacks', but it is not used when comparing the hacks set by
    // subsequent calls to furiganaStep().
    int extrahacks = 0;


    // If validReadings are used again, remove the if below with curly braces, but leave
    // contents. Only the readings value and its use should be removed.
    //QCharStringList *readings = (k != nullptr ? k->on : validReadings(kanji[kanjistart]));

    if (k != nullptr)
    {
        QCharStringList *readings = nullptr;
        readings = &k->on;

        // Try to match kanji ON-readings **** REMOVED: or special character pronunciations.

        for (int ix = 0; ix < readings->size(); ++ix)
        {
            const QChar *on = (readings->items(ix).data() + (readings->items(ix)[0] == '-' ? 1 : 0));
            int onlen = qcharlen(on);
            if (l.count(onlen) == 0 && fuReading(on, onlen, kana.data(), kanastart, kanaend))
                l.insert(onlen);
        }

        // Try to match kanji KUN-readings or irregular readings.

        for (int ix = 0; ix < k->kun.size(); ++ix)
        {
            int kunlen = kunLen(k->kun[ix].data());
            if (l.count(kunlen) == 0 && fuReading(k->kun[ix].data(), kunlen, kana.data(), kanastart, kanaend))
                l.insert(kunlen);
        }

        // TO-NOT-DO: remove irreg unless using irregulars.txt again.
        //for (int ix = 0; ix < k->irreg.size(); ++ix)
        //    if (std::find(l.begin(), l.end(), k->irreg[ix].size()) == l.end() &&
        //        fuReading(k->irreg[ix].data(), kana.data(), kanastart, kanaend))
        //        l.push_back(k->irreg[ix].size());

        if (l.size() == 1)
        {
            // Only one reading is found which matches the current kanji/kana position.
            int len = abs(*l.begin());

            int furidatpos = datpos + 1;
            int furikanjistart = kanjistart + 1;
            int furikanastart = kanastart + len;
            int furihacks = 0;

            if ((furikanjistart == kanjiend + 1 && furikanastart == kanaend + 1) ||
                furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, dat, furidatpos, furihacks))
            {
                res = true;

                dat[datpos].kanji.pos = kanjistart;
                dat[datpos].kanji.len = 1;
                dat[datpos].kana.pos = kanastart;
                dat[datpos].kana.len = len;

                lastkanjistart = furikanjistart;
                lastkanastart = furikanastart;
                lastdatpos = furidatpos;
                lasthacks = furihacks;
                extrahacks = 0;
            }
        }
        else if (!l.empty())
        {
            // Multiple readings can match the current kanji/kana position. Try them from longest
            // to shortest, until one is found which allows the whole word part to be matched with
            // furigana.

            //std::sort(l.begin(), l.end(), [](int a, int b) { return a > b; });
            //l->Sort(KunLenSort);

            std::vector<FuriganaData> furi(kanjiend - kanjistart + 1);
            //for (int ix = 0; ix < l.size() && lasthacks != 0; ++ix)
            for (auto it = l.rbegin(); it != l.rend(); ++it)
            {
                int len = std::abs(*it);

                int furikanjistart = kanjistart + 1;
                int furikanastart = kanastart + len;
                int furidatpos = 0;
                int furihacks = 0;

                if ((furikanjistart == kanjiend + 1 && furikanastart == kanaend + 1) ||
                    furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, furi, furidatpos, furihacks))
                {
                    int hackinc = 0;
                    if (!res || furihacks + hackinc < lasthacks + extrahacks)
                    {
                        res = true;

                        dat[datpos].kanji.pos = kanjistart;
                        dat[datpos].kanji.len = 1;
                        dat[datpos].kana.pos = kanastart;

                        lastkanjistart = furikanjistart;
                        lastkanastart = furikanastart;
                        lastdatpos = furidatpos + datpos + 1;

                        lasthacks = furihacks;
                        extrahacks = hackinc;

                        dat[datpos].kana.len = len;

                        for (int iy = 0; iy != furidatpos; ++iy)
                            dat[datpos + 1 + iy] = furi[iy];
                    }
                }
            }
        }

        // Create readings formed by the -masu base of verbs which only have this one kanji
        // at the front. Doesn't actually take verbs, but tries to construct new ones from the
        // kana readings that have okurigana information.

        std::set<int> l2;

        for (int ix = 0; ix < k->kun.size(); ++ix)
        {
            const QChar *c = qcharchr(k->kun[ix].data(), QChar('.'));
            if (c == nullptr || c[1].unicode() == 0)
                continue;

            int dotpos = c - k->kun[ix].data();

            if (c[2].unicode() == 0)
            {
                // The kana reading has one extra furigana after the . character. It can be
                // converted into a fake masu base form of a verb.

                QChar ch = hiraganaCh(c + 1, 0);
                QChar newch = 0;

                QCharString masureading;
                if (ch == 0x3046 /* kana u */ || ch == 0x304f /* kana ku */ || ch == 0x3059 /* kana su */ || ch == 0x305a /* kana zu */)
                    newch = QChar(ch.unicode() - 2);
                if (ch == 0x3064 /* kana tsu */ || ch == 0x3065 /* kana dzu */ || ch == 0x3075 /* kana fu */ ||
                    ch == 0x3076 /* kana bu */ || ch == 0x3077 /* kana pu */)
                    newch = QChar(ch.unicode() - 3);
                if (ch == 0x306c /* kana nu */ || ch == 0x3080 /* kana mu */ || ch == 0x308b /* kana ru */)
                    newch = QChar(ch.unicode() - 1);

                if (newch.unicode() != 0)
                {
                    masureading.copy(k->kun[ix].data(), dotpos + 1);
                    masureading[dotpos] = newch;
                }

                if (!masureading.empty() && l.count(dotpos + 1) == 0 && l2.count(dotpos + 1) == 0 && fuReading(masureading.data(), dotpos + 1, kana.data(), kanastart, kanaend))
                    l2.insert(dotpos + 1);
            }
            else
            {
                // The reading has more characters after the dot. Try to convert the last kana
                // character into the masu base form.

                // Number of characters after the . character.
                int clen = qcharlen(c + 1);

                QChar ch = hiraganaCh(c + clen, 0);
                QChar newch = 0;
                QCharString masureading;

                if (ch == 0x3046 /* kana u */ || ch == 0x304f /* kana ku */ || ch == 0x3059 /* kana su */ || ch == 0x305a /* kana zu */)
                    newch = QChar(ch.unicode() - 2);
                if (ch == 0x3064 /* kana tsu */ || ch == 0x3065 /* kana dzu */ || ch == 0x3075 /* kana fu */ ||
                    ch == 0x3076 /* kana bu */ || ch == 0x3077 /* kana pu */)
                    newch = QChar(ch.unicode() - 3);
                if (ch == 0x306c /* kana nu */ || ch == 0x3080 /* kana mu */ || ch == 0x308b /* kana ru */)
                    newch = QChar(ch.unicode() - 1);

                if (newch.unicode() != 0)
                {
                    masureading.copy(k->kun[ix].data(), dotpos);
                    masureading.resize(dotpos + clen);
                    for (int iy = dotpos; iy != dotpos + clen - 1; ++iy)
                        masureading[iy] = k->kun[ix][iy + 1];
                    masureading[dotpos + clen - 1] = newch;

                    if (!masureading.empty() && l.count(dotpos + clen) == 0 && l2.count(dotpos + clen) == 0 && fuReading(masureading.data(), dotpos + clen, kana.data(), kanastart, kanaend))
                        l2.insert(dotpos + clen);

                    // Verbs ending in -ru can be converted to masu base by simply cutting off
                    // that trailing character.
                    if (ch == 0x308b /* kana ru */)
                    {
                        if (!masureading.empty() && l.count(dotpos + clen - 1) == 0 && l2.count(dotpos + clen - 1) == 0 && fuReading(masureading.data(), dotpos + clen - 1, kana.data(), kanastart, kanaend))
                            l2.insert(dotpos + clen - 1);
                    }
                }
            }
        }

        if (l2.size() == 1)
        {
            // Only one reading is found which matches the current kanji/kana position.
            int len = abs(*l2.begin());

            int furidatpos = 0;
            int furihacks = 0;
            int furikanjistart = kanjistart + 1;
            int furikanastart = kanastart + len;

            std::vector<FuriganaData> furi(kanjiend - kanjistart + 1);

            if ((furikanjistart == kanjiend + 1 && furikanastart == kanaend + 1) ||
                furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, furi, furidatpos, furihacks))
            {
                int hackinc = 1;
                if (!res || furihacks + hackinc < lasthacks + extrahacks)
                {
                    res = true;

                    dat[datpos].kanji.pos = kanjistart;
                    dat[datpos].kanji.len = 1;
                    dat[datpos].kana.pos = kanastart;
                    dat[datpos].kana.len = len;

                    lastkanjistart = furikanjistart;
                    lastkanastart = furikanastart;
                    lastdatpos = furidatpos + datpos + 1;
                    lasthacks = furihacks;
                    extrahacks = hackinc;

                    for (int iy = 0; iy != furidatpos; ++iy)
                        dat[datpos + 1 + iy] = furi[iy];
                }
            }
        }
        else if (!l2.empty())
        {
            // Multiple readings can match the current kanji/kana position. Try them from longest
            // to shortest, until one is found which allows the whole word part to be matched with
            // furigana.

            std::vector<FuriganaData> furi(kanjiend - kanjistart + 1);
            for (auto it = l2.rbegin(); it != l2.rend(); ++it)
            {
                int len = std::abs(*it);

                int furikanjistart = kanjistart + 1;
                int furikanastart = kanastart + len;
                int furidatpos = 0;
                int furihacks = 0;

                if (((furikanjistart == kanjiend + 1 && furikanastart == kanaend + 1) ||
                    furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, furi, furidatpos, furihacks)) && (!res || furihacks + 1 < lasthacks + extrahacks))
                {
                    res = true;

                    dat[datpos].kanji.pos = kanjistart;
                    dat[datpos].kanji.len = 1;
                    dat[datpos].kana.pos = kanastart;

                    lastkanjistart = furikanjistart;
                    lastkanastart = furikanastart;
                    lastdatpos = furidatpos + datpos + 1;

                    lasthacks = furihacks;
                    extrahacks = 1;

                    dat[datpos].kana.len = len;

                    for (int iy = 0; iy != furidatpos; ++iy)
                        dat[datpos + 1 + iy] = furi[iy];
                }
            }
        }

    }

    if ((!res || lasthacks != 0) && kanjistart != kanjiend && kanaend - kanastart > 0 && KANJI(kanji[kanjistart].unicode()) && (kanji[kanjistart + 1] == kanji[kanjistart] || kanji[kanjistart + 1].unicode() == KURIKAESHI))
    {
        // Kanji is repeated. It's possible the reading is repeated too. Check at most 3
        // characters. If match is found, nothing else is checked.

        for (int kanalen = 1; kanalen != 4 && kanastart + kanalen * 2 - 1 <= kanaend; ++kanalen)
        {
            if (std::find(l.begin(), l.end(), kanalen) != l.end())
                continue;

            int kanacateg = 0;
            QChar ch = hiraganaCh(kana.data(), kanastart + kanalen);

            // The leading kana of the current part might have changed sound due to its
            // position. Depending on the kana, a neighboring unicode position contains
            // the original kana.
            if ((ch > 0x304b && ch <= 0x3062 && (ch.unicode() % 2) == 0) || (ch > 0x3064 && ch <= 0x3069 && (ch.unicode() % 2) != 0))
                kanacateg = 1;
            else if (ch > 0x306f && ch <= 0x307d && (ch.unicode() % 3) != 0)
                kanacateg = 2;

            QChar kch = hiraganaCh(kana.data(), kanastart);
            // Check the first characters for match separately. The latter ones don't
            // change sound.
            if (ch.unicode() != kch.unicode() && (kanacateg != 1 || ch.unicode() - 1 != kch.unicode()) &&
                (kanacateg != 2 && ch.unicode() - (ch.unicode() % 3) != kch.unicode()))
                continue;

            bool match = true;

            for (int ix = 1; ix != kanalen && match; ++ix)
                match = hiraganaCh(kana.data(), kanastart + ix) == hiraganaCh(kana.data(), kanastart + kanalen + ix);

            if (match)
            {
                std::vector<FuriganaData> furi(kanjiend - kanjistart + 1);

                int furikanjistart = kanjistart + 2;
                int furikanastart = kanastart + kanalen * 2;
                int furidatpos = 0;
                int furihacks = 0;

                if (((furikanjistart == kanjiend + 1 && furikanastart == kanaend + 1) ||
                    furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, furi, furidatpos, furihacks)) && (!res || furihacks < lasthacks))
                {
                    dat[datpos].kanji.pos = kanjistart;
                    dat[datpos].kanji.len = 2;
                    dat[datpos].kana.pos = kanastart;
                    dat[datpos].kana.len = kanalen * 2;

                    lastkanjistart = furikanjistart;
                    lastkanastart = furikanastart;
                    lasthacks = furihacks;
                    lastdatpos = furidatpos + datpos + 1;
                    extrahacks = 0;

                    for (int iy = 0; iy != furidatpos; ++iy)
                        dat[datpos + 1 + iy] = furi[iy];

                    res = true;
                }

                break;
            }
        }
    }


    // As a last step when all else failed, kanji are skipped one by one (see below.) When
    // that happens, some other hacks shouldn't be done. Check this value in those cases.
    bool kanjiskipping = datpos == 1 && (short)dat[0].kanji.len < 0 && dat[0].kanji.pos - (short)dat[0].kanji.len == kanjistart;


    if ((!res || lasthacks != 0) && k && !kanjiskipping)
    {
        // Kanji readings might not match. If the kanji is standalone, and a non-kanji
        // follows, try to match that to the following kana string.

        if (kanjistart == kanjiend && kanastart <= kanaend)
        {
            // Standalone kanji at the back of the word. Let's predict that the remaining kana
            // matches it. Because this can produce invalid results, increase hacks
            // accordingly.

            int hackinc = furiReadingDiff(k, kana, kanastart, kanaend);

            if (!res || extrahacks > hackinc /*|| lasthacks > std::abs(kanaend - kanastart) + 3*/)
            {
                dat[datpos].kanji.pos = kanjistart;
                dat[datpos].kanji.len = 1;
                dat[datpos].kana.pos = kanastart;
                dat[datpos].kana.len = kanaend - kanastart + 1;

                lasthacks = std::max(0, lasthacks);
                extrahacks = hackinc; //std::max(1, std::abs((kanaend - kanastart) - (kanjiend - kanjistart)));
                lastdatpos = datpos + 1;
                lastkanjistart = kanjiend + 1;
                lastkanastart = kanaend + 1;
            }

            res = true;
        }
        else if (kanjistart != kanjiend && !kanjiOrKurikaeshi(kanji, kanjistart + 1))
        {
            int furikanjistart = kanjistart + 1;
            int furikanastart = kanastart;

            std::vector<FuriganaData> furi(kanjiend - kanjistart + 1);

            QChar ch = hiraganaCh(kanji.data(), furikanjistart);

            while (furikanastart != kanaend && lasthacks != 0)
            {
                ++furikanastart;
                if (hiraganaCh(kana.data(), furikanastart).unicode() != ch.unicode())
                    continue;

                // Kana start position is saved because furiganaStep() can update it.
                int kanapos = furikanastart;

                int furidatpos = 0;
                int furihacks = 0;


                if (furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, furi, furidatpos, furihacks))
                {
                    int hackinc = furiReadingDiff(k, kana, kanastart, kanapos - 1);
                    if (!res || furihacks + hackinc < lasthacks + extrahacks)
                    {
                        dat[datpos].kanji.pos = kanjistart;
                        dat[datpos].kanji.len = 1;
                        dat[datpos].kana.pos = kanastart;

                        lastkanjistart = furikanjistart;
                        lastkanastart = furikanastart;
                        lastdatpos = furidatpos + datpos + 1;
                        lasthacks = furihacks;

                        extrahacks = hackinc; //std::max(1, (kanapos - kanastart) - 1);

                        dat[datpos].kana.len = kanapos - dat[datpos].kana.pos;

                        for (int iy = 0; iy != furidatpos; ++iy)
                            dat[datpos + 1 + iy] = furi[iy];

                        res = true;
                    }
                }

                furikanastart = kanapos;
                furikanjistart = kanjistart + 1;
            }
        }
    }

    if ((!res /*|| lasthacks + extrahacks != 0*/) && kanji[kanjistart].unicode() != KURIKAESHI && VALIDCODE(kanji[kanjistart].unicode()))
    {
        // Nothing is found because the current character is not a kanji or kana. The
        // consecutive validcode characters are skipped until something is found.

        int lastvalidpos = kanjistart;
        while (lastvalidpos != kanjiend && kanji[lastvalidpos + 1].unicode() != 0x30fb /* kana mid-dot*/ &&
            kanji[lastvalidpos + 1].unicode() != 0xff1d /* fullwidth equal sign*/ && kanji[lastvalidpos + 1].unicode() != 0xff0e /* fullwidth stop */ &&
            kanji[lastvalidpos + 1].unicode() != 0xff0f /* fullwidth / */ && VALIDCODE(kanji[lastvalidpos + 1].unicode()))
            ++lastvalidpos;

        // Number of skipped valid characters.
        int skipvalid = lastvalidpos - kanjistart + 1;

        if (lastvalidpos == kanjiend)
        {
            int hackinc = std::max(1, std::abs(kanaend - kanastart - lastvalidpos + kanjistart) / 2);
            if (!res || extrahacks > hackinc)
            {
                // Nothing comes after the valid code characters. Pretend that they match the
                // rest of the kana, but increase the value of hacks accordingly.

                dat[datpos].kanji.pos = kanjistart;
                dat[datpos].kana.pos = kanastart;
                dat[datpos].kanji.len = lastvalidpos - kanjistart + 1;

                dat[datpos].kana.len = kanaend - kanastart + 1;

                lasthacks = std::max(0, lasthacks);
                extrahacks = hackinc;

                lastdatpos = datpos + 1;

                lastkanjistart = kanjiend + 1;
                lastkanastart = kanaend + 1;

                res = true;
            }
        }
        else
        {
            // Skip as many kana characters as it takes to find something.
            std::vector<FuriganaData> furi(kanjiend - kanjistart + 1);

            for (int furikanastart = kanastart + std::max(1, skipvalid / 3); furikanastart < kanaend && lasthacks != 0; ++furikanastart)
            {
                int furidatpos = 0;
                int furihacks = 0;

                int furikanjistart = lastvalidpos + 1;
                int kanapos = furikanastart;

                if (furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, furi, furidatpos, furihacks))
                {
                    int hackinc = std::max(1, ((kanapos - kanastart) - (lastvalidpos - kanjistart + 1)) / 2);
                    if (!res || furihacks + hackinc < lasthacks + extrahacks)
                    {
                        dat[datpos].kanji.pos = kanjistart;
                        dat[datpos].kana.pos = kanastart;
                        dat[datpos].kanji.len = lastvalidpos - kanjistart + 1;

                        lastkanjistart = furikanjistart;
                        lastkanastart = furikanastart;
                        lastdatpos = furidatpos + datpos + 1;
                        lasthacks = furihacks;
                        extrahacks = hackinc;

                        dat[datpos].kana.len = kanapos - kanastart;

                        for (int iy = 0; iy != furidatpos; ++iy)
                            dat[datpos + 1 + iy] = furi[iy];

                        res = true;
                    }
                }


                furikanastart = kanapos;
            }

            //if (furires)
            //{
            //    // Increasing hacks when skipping characters without any checks until match.
            //    //lasthacks += std::abs(dat[datpos].kana.len - dat[datpos].kanji.len) + 1 + 2;

            //    //datpos = lastdatpos + datpos + 1;
            //    //kanjistart = lastkanjistart;
            //    //kanastart = lastkanastart;

            //    //hacks += lasthacks;

            //}
        }
    }

    if ((!res || lasthacks != 0) && !kanjiskipping && kanjistart < kanjiend &&
        kanjiOrKurikaeshi(kanji, kanjistart) && kanjiOrKurikaeshi(kanji, kanjistart + 1))
    {
        // One last chance if what we are at is a kanji character. Let's pretend it never
        // existed, and also skip kana while reasonable. If one kanji was not enough, skip
        // the next one etc. until we run out of kanji.

        // Find the first character that wouldn't fit.
        int endpos = kanjistart + 1;

        while (endpos != kanjiend && kanjiOrKurikaeshi(kanji, endpos + 1))
            ++endpos;

        // Last kana character that can be skipped while checking the remaining string.
        int endkana = std::max(kanastart, kanaend - std::max(0, kanjiend - endpos - 1));
        std::vector<FuriganaData> furi(kanjiend - kanjistart + 2);

        furi[0].kanji.pos = kanjistart;
        furi[0].kana.pos = kanastart;

        // The position at endpos is the last kanji character that we can skip. By going
        // till endpos + 1 (!= endpos + 2), we can skip the last kanji as well, and match
        // anything coming after.
        for (int furikanjistart = kanjistart + 1; furikanjistart != std::min(kanjiend + 1, endpos + 2) && lasthacks != 0; ++furikanjistart)
        {
            // Number of skipped kanji.
            int skipkanji = furikanjistart - kanjistart;
            // The kana characters should be skipped one by one while possible.
            for (int furikanastart = kanastart + skipkanji; furikanastart < std::min(endkana + 1, kanastart + skipkanji * 3 + 1) && lasthacks != 0; ++furikanastart)
            {
                // Kana start position is saved because furiganaStep() can update it.
                int kanjipos = furikanjistart;
                int kanapos = furikanastart;

                // Negative marks that we are doing hacks. This is checked in the
                // following recursions, and in some branches (where hacks occur with
                // kanji,) the function seeing this can return false.
                furi[0].kanji.len = -(furikanjistart - kanjistart);

                // The first datapos is skipped to allow next recursions to see what is set
                // above.
                int furidatpos = 1;
                int furihacks = 0;

                if (furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, furi, furidatpos, furihacks))
                {
                    int hackinc = -1;
                    if (kanjipos - kanjistart == 1)
                        hackinc = furiReadingDiff(k, kana, kanastart, kanapos - 1);
                    else
                        hackinc = std::max(1, std::abs((kanjipos - kanjistart + 1) + (kanapos - kanastart + 1)));

                    if (!res || furihacks + hackinc < lasthacks + extrahacks)
                    {
                        dat[datpos].kanji.pos = kanjistart;
                        dat[datpos].kana.pos = kanastart;

                        lastkanjistart = furikanjistart;
                        lastkanastart = furikanastart;
                        lastdatpos = furidatpos - 1 + datpos + 1;
                        lasthacks = furihacks;

                        extrahacks = hackinc;

                        dat[datpos].kanji.len = kanjipos - kanjistart;
                        dat[datpos].kana.len = kanapos - kanastart;

                        for (int iy = 0; iy != furidatpos - 1; ++iy)
                            dat[datpos + 1 + iy] = furi[iy + 1];

                        res = true;
                    }
                }

                furikanastart = kanapos;
                furikanjistart = kanjipos;
            }
        }

        if (endpos == kanjiend)
        {
            int hackinc = -1;
            if (kanjistart == kanjiend)
                hackinc = furiReadingDiff(k, kana, kanastart, kanaend);
            else
                hackinc = std::max(1, std::abs((kanjiend - kanjistart + 1) + (kanaend - kanastart + 1)));
            if (!res || extrahacks > hackinc)
            {
                // Couldn't skip a kanji substring and match characters after them. If this kanji
                // substring is at the end of the whole kanji string, it's paired with the rest of
                // the kana.

                dat[datpos].kanji.pos = kanjistart;
                dat[datpos].kana.pos = kanastart;

                dat[datpos].kanji.len = kanjiend - kanjistart + 1;
                dat[datpos].kana.len = kanaend - kanastart + 1;

                lastkanjistart = kanjiend + 1;
                lastkanastart = kanaend + 1;
                lastdatpos = datpos + 1;
                lasthacks = std::max(0, lasthacks);

                extrahacks = hackinc;

                res = true;
            }
        }
    }

    //// Try to find words with same kanji and kana as the current checked part as whole words
    //// in every dictionary.
    //if ((!res /*|| lasthacks != 0*/) && kanjistart < kanjiend + 1 && (kanjiOrKurikaeshi(kanji, kanjistart)))
    //{
    //    // Last position after consecutive kanji and kurikaeshi symbol characters in kanji
    //    // from kanjistart till kanjiend.
    //    int endpos = kanjistart + 1;

    //    //for (; endpos <= kanjiend; ++endpos)
    //    //    if (!KANJI(kanji[endpos].unicode()) && kanji[endpos].unicode() != KURIKAESHI)
    //    //        break;
    //    if (endpos <= kanjiend && kanjiOrKurikaeshi(kanji, endpos))
    //        ++endpos;

    //    QCharString c;

    //    c.copy(kana.data() + kanastart, kanaend - kanastart + 1);

    //    QChar och = hiraganaCh(c.data(), 0);
    //    QChar kch = c[0];

    //    int kanacateg = 0;
    //    // The leading kana of the current part might have changed sound due to its position.
    //    // Depending on the kana, a neighboring unicode position contains the original kana.
    //    if ((och > 0x304b && och <= 0x3062 && (och.unicode() % 2) == 0) || (och > 0x3064 && och <= 0x3069 && (och.unicode() % 2) != 0))
    //        kanacateg = 1;
    //    else if (och > 0x306f && och <= 0x307d && (och.unicode() % 3) != 0)
    //        kanacateg = 2;

    //    std::vector<FuriganaData> furi(kanjiend - kanjistart + 1);

    //    for (int ix = 0; ix < std::min(kanaend - kanastart + 1, (endpos - kanjistart) * 3 + 1) && lasthacks != 0; ++ix)
    //    {
    //        std::vector<ZKanji::WordEntriesResult> wlist;
    //        ZKanji::findEntriesByKana(wlist, c.toQString(0, ix + 1));

    //        if (kanacateg == 1)
    //        {
    //            c[0] = kch.unicode() - 1;

    //            ZKanji::findEntriesByKana(wlist, c.toQString(0, ix + 1));
    //        }
    //        else if (kanacateg == 2)
    //        {
    //            c[0] = kch.unicode() - (och.unicode() % 3);

    //            ZKanji::findEntriesByKana(wlist, c.toQString(0, ix + 1));
    //        }

    //        // Restore the original string.
    //        c[0] = kch;

    //        // Sort list by length of word kanji. Shorter first.
    //        std::sort(wlist.begin(), wlist.end(), [](const ZKanji::WordEntriesResult &a, const ZKanji::WordEntriesResult &b) {
    //            return a.dict->wordEntry(a.index)->kanji.size() < b.dict->wordEntry(b.index)->kanji.size();
    //        });

    //        // Because there's a possible kurikaeshi symbol at the current kanji location, the
    //        // first characters are matched via firstch.
    //        QChar firstch = kanji[kanjistart];
    //        if (kanjistart != 0 && kanji[kanjistart].unicode() == KURIKAESHI && KANJI(kanji[kanjistart - 1].unicode()))
    //            firstch = kanji[kanjistart - 1];

    //        for (int j = 0; j != wlist.size() && lasthacks != 0; ++j)
    //        {
    //            QChar *ilkanji = wlist[j].dict->wordEntry(wlist[j].index)->kanji.data();
    //            int illen = qcharlen(ilkanji);

    //            if (illen == 0 || ilkanji[0] != firstch || qcharncmp(ilkanji + 1, kanji.data() + 1 + kanjistart, std::min(illen - 1, kanjiend - kanjistart)))
    //                continue;

    //            int pos;
    //            for (pos = endpos - kanjistart; pos < std::min(illen, kanjiend - kanjistart + 1); ++pos)
    //                if (KANJI(ilkanji[pos].unicode()) || (pos != 0 && ilkanji[pos].unicode() == KURIKAESHI && KANJI(ilkanji[pos - 1].unicode())))
    //                    break;
    //            if (pos < std::min(illen, kanjiend - kanjistart + 1))
    //                continue;

    //            int kanjilen = std::min(illen, endpos - kanjistart);
    //            int kanalen = ix + 1 - (illen - kanjilen);

    //            if (kanjilen != 0 && kanalen != 0)
    //            {
    //                int furikanjistart = kanjistart + kanjilen;
    //                int furikanastart = kanastart + kanalen;
    //                int furidatpos = 0;
    //                int furihacks = 0;

    //                if (furiganaStep(kanji, kana, furikanjistart, kanjiend, furikanastart, kanaend, furi, furidatpos, furihacks) &&
    //                    (!res || /*furihacks < lasthacks ||*/ furihacks + 1 < lasthacks + extrahacks))
    //                {
    //                    dat[datpos].kanji.pos = kanjistart;
    //                    dat[datpos].kana.pos = kanastart;

    //                    lastkanjistart = furikanjistart;
    //                    lastkanastart = furikanastart;
    //                    lastdatpos = furidatpos + datpos + 1;
    //                    lasthacks = furihacks;
    //                    extrahacks = 1;

    //                    dat[datpos].kanji.len = kanjilen;
    //                    dat[datpos].kana.len = kanalen;

    //                    for (int iy = 0; iy != furidatpos; ++iy)
    //                        dat[datpos + 1 + iy] = furi[iy];

    //                    res = true;
    //                }
    //            }
    //        }
    //    }

    //    //if (furires)
    //    //{
    //    //    //++lasthacks;

    //    //    //datpos = lastdatpos + datpos + 1;
    //    //    //kanjistart = lastkanjistart;
    //    //    //kanastart = lastkanastart;

    //    //    //hacks += lasthacks;

    //    //    //// Increasing hacks when basing matching on dictionary search not readings match.
    //    //    //++hacks;
    //    //}
    //}

    if (res)
    {
        kanjistart = lastkanjistart;
        kanastart = lastkanastart;
        datpos = lastdatpos;
        hacks += lasthacks + extrahacks;
    }

    return res;
}


void findFurigana(const QCharString &kanji, const QCharString &kana, std::vector<FuriganaData> &furigana)
{
    int klen = kanji.size();
    int index;
    for (index = 0; index < klen; ++index)
        if (KANJI(kanji[index].unicode()))
            break;

    int datpos = 0;
    if (index == klen) // No kanji was found, so no furigana can be shown.
    {
        furigana.clear();
        return;
    }

    bool ok;

    int kanjistart = 0;
    int kanastart = 0;

    int kanjiend = klen - 1;
    int kanaend = kana.size() - 1;

    int hacks = 0;

    furigana.resize(kanjiend + 1);

    ok = furiganaStep(kanji, kana, kanjistart, kanjiend, kanastart, kanaend, furigana, datpos, hacks);
    // TODO (or not): Originally furigana.txt was used for words that were not correctly determinable with this algorithm.
    // Those words are not likely to be studied or added to vocabulary lists so it's possible this won't be implemented again.
    //if (!ok && FileExists(paths.installdatapath + L"furigana.txt"))
    //    ok = import_one_furigana((paths.installdatapath + L"furigana.txt").c_str(), kanji.c_str(), kana.c_str(), dat, datpos);

    if (kanjiend >= kanjistart || !ok)
        furigana.clear();
    else
        furigana.resize(datpos);

    //return datpos;
}

int findKanjiReading(const QCharString &kanji, const QCharString &kana, int kanjipos, KanjiEntry *k, std::vector<FuriganaData> *furigana)
{
    std::vector<FuriganaData> fdat;
    if (furigana == nullptr)
        findFurigana(kanji, kana, fdat);
    else
        fdat = *furigana;

    if (fdat.empty())
        return -1;

    //int r = 0;

    // Find the kanji at kanjipos in the furigana data.
    int pos;
    for (pos = 0; pos != fdat.size() && kanjipos >= fdat[pos].kanji.pos + fdat[pos].kanji.len; ++pos)
        ;

    // Kanji not found.
    if (pos == fdat.size() || fdat[pos].kanji.pos + fdat[pos].kanji.len <= kanjipos)
        return -1;

    if (pos < 0)
        return 0;

    if (k == nullptr)
        k = ZKanji::kanjis[ZKanji::kanjiIndex(kanji[kanjipos].unicode())];

    int ccnt = ZKanji::kanjiReadingCount(k, true);

    int val;

    // Exact match.
    bool fullmatch = false;
    // Reading that matched last.
    int lastmatch = -1;
    // Difference between the kanji reading and the kana. Smaller the better.
    int matchval = 9999;

    int lastkana = -1;

    int kanaval = 9999;

    const QChar *c;
    QString kanahira = hiraganize(kana);
    //const wchar_t *kana = kanatmp.c_str();

    if (fdat[pos].kanji.len == 1)
    {
        // Furigana corresponding to the one kanji was found.

        for (int ix = 0; ix < k->on.size(); ++ix)
        {
            c = k->on[ix].data();
            if (c[0] == '-')
                c++;

            int clen = qcharlen(c);

            if (clen != fdat[pos].kana.len)
                continue;

            // The kana corresponding for the kanji in the word exactly matches the kanji
            // reading.
            if (!qcharncmp(kanahira.constData() + fdat[pos].kana.pos, hiraganize(c).constData(), fdat[pos].kana.len))
            {
                fullmatch = true;
                lastmatch = ix;
                matchval = 1;
                break;
            }

            // Reading at lastmatch matches. It is not exact but no other matches can get
            // closer, unless it's an exact match.
            if (lastmatch >= 0 && matchval == 1)
                continue;

            // Match ON reading with hiraganized word-kana at the kanji's furigana position.
            // Only save it if it matches and it's closer than a previous match.
            val = fuReading(c, clen, kanahira.constData(), fdat[pos].kana.pos, fdat[pos].kana.pos + fdat[pos].kana.len - (pos < fdat.size() - 1 ? 0 : 1));
            if (val != 0 && val < matchval)
            {
                lastmatch = ix;
                matchval = val;
            }
        }

        // ON reading does not match or the reading part is followed by possible okurigana.
        // It's possible the reading is from the KUN readings.
        bool afterkana = lastmatch < 0 || (kanji.size() > fdat[pos].kanji.pos + fdat[pos].kanji.len && KANA(kanji[fdat[pos].kanji.pos + fdat[pos].kanji.len]));

        if (fullmatch && !afterkana)
            return lastmatch + 1; // Found the on-reading. no need to go further.
        else if (fullmatch)
        {
            // Found on-reading but it could be kun too.
            QString on = hiraganize(c);
            for (int ix = 1 + k->on.size(); ix < ccnt; ++ix)
            {
                int kk = ZKanji::kanjiCompactToReal(k, ix) - 1 - k->on.size();

                // A KUN reading matches, so we assume it's the winner.
                if (!qcharncmp(k->kun[kk].data(), on.constData(), kunLen(k->kun[kk].data())))
                    return ix;
            }

            // Stay with the ON reading.
            return lastmatch + 1;
        }
        else
        {
            // No perfect match or no match at all.
            for (int ix = 1 + k->on.size(); ix < ccnt; ++ix)
            {
                int kk = ZKanji::kanjiCompactToReal(k, ix) - 1 - k->on.size();

                int klen = kunLen(k->kun[kk].data());
                if (klen != fdat[pos].kana.len)
                    continue;

                // Found a perfectly matching KUN reading.
                if (!qcharncmp(kanahira.constData() + fdat[pos].kana.pos, k->kun[kk].data(), fdat[pos].kana.len))
                    return ix;

                // There's a possible ON reading match with very little difference, or a
                // similar kana match.
                if ((!afterkana && lastmatch >= 0 && matchval == 1) || (lastkana >= 0 && kanaval == 1))
                    continue;

                val = fuReading(k->kun[kk].data(), klen, kanahira.constData(), fdat[pos].kana.pos, fdat[pos].kana.pos + fdat[pos].kana.len - (pos < fdat.size() - 1 ? 0 : 1));
                if (val != 0 && val < kanaval)
                {
                    lastkana = ix;
                    kanaval = val;
                }
            }

            // Either KUN reading matched (lastkana) or ON matched.
            return (lastkana >= 0 ? lastkana : lastmatch + 1);
        }
    }
    else
    {
        // Furigana corresponding to multiple kanji was found, and the kanji we are looking
        // for is in there somewhere. Try to figure out where it is and whether a reading
        // might be in there too.

        enum KanjiWordPos { wpStart, wpEnd, wpMid };
        KanjiWordPos wp;

        if (fdat[pos].kanji.pos == kanjipos)
            wp = wpStart;
        else if (fdat[pos].kanji.pos + fdat[pos].kanji.len - 1 == kanjipos)
            wp = wpEnd;
        else
            wp = wpMid;

        // There's kana after the last kanji in the current furigana item.
        bool afterkana = wp == wpEnd && fdat[pos].kanji.pos + fdat[pos].kanji.len < kanji.size() && KANA(kanji[fdat[pos].kanji.pos + fdat[pos].kanji.len].unicode());

        for (int i = 0; i < ccnt - 1; i++)
        {
            int kk;
            if (i >= k->on.size())
                kk = ZKanji::kanjiCompactToReal(k, i + 1);
            c = i < k->on.size() ? k->on[i].data() : k->kun[kk - 1 - k->on.size()].data();
            if (c[0] == '-')
                c++;

            int len = kunLen(c);

            if (len > fdat[pos].kana.len - 1 - (wp == wpMid ? 1 : 0))
                continue;

            if (wp != wpMid)
            {
                if (i < k->on.size() && !fullmatch && !qcharncmp(kanahira.constData() + fdat[pos].kana.pos + (wp == wpStart ? 0 : fdat[pos].kana.len - len), hiraganize(c).constData(), len))
                {
                    fullmatch = true;
                    lastmatch = i;
                    matchval = 1;
                    if (!afterkana)
                        break;

                    // Found exact match, skip other on readings.
                    i = k->on.size() - 1;
                    continue;
                }

                if (i >= k->on.size() && (lastmatch < 0 || afterkana) && !qcharncmp(kanahira.constData() + fdat[pos].kana.pos + (wp == wpStart ? 0 : fdat[pos].kana.len - len), c, len))
                {
                    lastkana = i + 1;
                    break;
                }

                if (i < k->on.size() && lastmatch >= 0 && matchval == 1 || i >= k->on.size() && kanaval == 1)
                    continue;

                int s = fdat[pos].kana.pos + (wp == wpStart ? 0 : fdat[pos].kana.len - len);
                if ((val = fuReading(c, len, kanahira.constData(), s, s + len - 1)) != 0)
                {
                    if (i < k->on.size() && val < matchval)
                    {
                        lastmatch = i;
                        matchval = val;
                    }
                    else if (i >= k->on.size() && val < kanaval)
                    {
                        lastkana = i + 1;
                        kanaval = val;
                    }
                }

            }
            else
            {
                int start = fdat[pos].kana.pos + (kanjipos - fdat[pos].kanji.pos);
                int end = fdat[pos].kana.pos + fdat[pos].kana.len - len - (fdat[pos].kanji.pos + fdat[pos].kanji.len - 1 - kanjipos);

                if (i >= k->on.size() || !fullmatch)
                {
                    for (int pp = start; pp <= end; pp++)
                    {
                        if (!qcharncmp(kanahira.constData() + pp, hiraganize(c).constData(), len))
                        {
                            if (i < k->on.size())
                            {
                                lastmatch = i;
                                matchval = 1;
                                fullmatch = true;
                                break;
                            }
                            else
                            {
                                lastkana = i + 1;
                                kanaval = 1;
                                fullmatch = true;
                                break;
                            }
                        }

                        if ((i < k->on.size() && lastmatch >= 0) || (i >= k->on.size() && lastkana >= 0))
                            continue;

                        if ((val = fuReading(c, len, kanahira.constData(), start, start + len - 1)) != 0)
                            if (i < k->on.size() && val < matchval)
                            {
                                lastmatch = i;
                                matchval = val;
                            }
                            else if (i >= k->on.size() && val < kanaval)
                            {
                                lastkana = i + 1;
                                kanaval = val;
                            }
                    }
                    if (fullmatch)
                        break;
                }
            }

        }

        return lastkana >= 0 ? lastkana : lastmatch + 1;
    }
}


// TODO: check after dictionary + data import, that the example words marked for kanji really fit the kanji reading. (Is reading even kept?)
bool matchKanjiReading(const QCharString &kanji, const QCharString &kana, KanjiEntry *k, int reading)
{
    std::vector<FuriganaData> furi;
    findFurigana(kanji, kana, furi);

    int siz = kanji.size();
    for (int ix = 0; ix != siz; ++ix)
        if (kanji[ix] == k->ch && findKanjiReading(kanji, kana, ix, k, &furi) == reading)
            return true;
    return false;
}

//#define CHECKED_WORD_INDEX  21522         206067

void testFuriganaReadingTest()
{
#if 1 && !defined(CHECKED_WORD_INDEX)
    QFile f("d:/allfurigana.txt");
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&f);
    stream.setCodec("UTF-8");

    QFile f2("d:/nofurigana.txt");
    f2.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream2(&f2);
    stream2.setCodec("UTF-8");
#endif

    Dictionary *d;

    for (int dx = 0; dx != ZKanji::dictionaryCount(); ++dx)
    {
        d = ZKanji::dictionary(dx);

        int s = d->entryCount();
        for (int ix = 0; ix != s; ++ix)
        {
#ifdef  CHECKED_WORD_INDEX
            ix = CHECKED_WORD_INDEX;
#endif
            WordEntry *e = d->wordEntry(ix);

            std::vector<FuriganaData> furi;
            findFurigana(e->kanji, e->kana, furi);
#ifdef  CHECKED_WORD_INDEX
            return;
#endif

#if 1 && !defined(CHECKED_WORD_INDEX)
            int siz = e->kanji.size();

            for (int iy = 0; iy != siz; ++iy)
                if (KANJI(e->kanji[iy].unicode()))
                {
                    if (!furi.empty())
                    {
                        stream << QString("%1 %2 %3 ").arg(ix).arg(e->kanji.toQString()).arg(e->kana.toQString());
                        for (int j = 0; j != furi.size(); ++j)
                            stream << QString("{%1, %2}, {%3, %4} ").arg(furi[j].kanji.pos).arg(furi[j].kanji.len).arg(furi[j].kana.pos).arg(furi[j].kana.len);
                        stream << endl;
                    }
                    else
                    {
                        stream2 << QString("%1 %2 %3").arg(ix).arg(e->kanji.toQString()).arg(e->kana.toQString());
                        stream2 << endl;
                    }
                    break;
                }
#endif

            //int siz = e->kanji.size();
            //for (int iy = 0; iy != siz; ++iy)
            //    if (KANJI(e->kanji[iy].unicode()))
            //        findKanjiReading(e->kanji, e->kana, iy, nullptr, &furi);
        }
    }
}

