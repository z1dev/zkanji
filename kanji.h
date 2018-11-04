/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJI_H
#define KANJI_H

#include <QChar>

#include "searchtree.h"
#include "fastarray.h"

//struct KanjiExample
//{
//    // Index of word containing the kanji.
//    int windex;
//
//    // The reading used when marked as an example for the kanji.
//    //uchar reading;
//};

struct KanjiRadical
{
    QChar ch; // Unicode character representing the radical.
    //QCharString kanji; // List of kanji containing this radical.
    std::vector<ushort> kanji; // Indexes of kanji in the main kanjilist that contain this radical.
    uchar strokes; // Number of strokes in radical.
    uchar radical; // Number of radical.
    QCharStringList names; // List for names of radical. (Many radicals can have the same name.)
    QCharStringList romajinames; // List for names of radical in romaji form. (Many radicals can have the same name.)
};

class KanjiRadicalList : public TextSearchTreeBase
{
public:
    typedef size_t  size_type;

    KanjiRadicalList();
    virtual ~KanjiRadicalList();

    void loadLegacy(QDataStream &stream, int version);
    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    virtual void clear() override;
    //void reset();

    void addRadical(QChar ch, std::vector<ushort> &&kanji, uchar strokes, uchar radical, QCharStringList &&names);

    virtual size_type size() const override;
    bool empty();

    virtual bool isKana() const override;
    virtual bool isReversed() const override;

    KanjiRadical* operator[](int ix);
    KanjiRadical* items(int ix);

    int find(QChar radchar) const;
protected:
    virtual void doGetWord(int index, QStringList &texts) const override;
private:
    typedef TextSearchTreeBase  base;

    // TODO: replace with a continuous array or vector. There's no need to dynamically
    // allocate each radical.
    smartvector<KanjiRadical> list;
};

struct KanjiEntry
{
    KanjiEntry();

    ushort index; // Index of kanji in "kanjis" list.

    // "numeric" data.
    QChar ch; // The kanji itself.
    ushort jis; // jis lookup.
    //ushort uni; // U - unicode lookup.
    uchar rad; // B - radical of the kanji.
    uchar crad; // C - classical radical of the kanji.
    uchar jouyou; // G - jouyou grade.
    //uchar JLPT; // JLPT level.

    uchar jlpt; // Assumed N kanji level of the JLPT from 2010.

    uchar strokes; // S - stroke number for the kanji.
    ushort frequency; // F - frequency number from 1 to 2501, lower the better.
    int element; // Element number in the kanji stroke order database.

    uchar skips[3]; // Pn-n-n - 3 skip codes

    ushort knk; // IN - index in "Kanji and kana" for lookup.
    ushort nelson; // N - Modern Reader's Japanese-English Character Dictionary.
    ushort halpern; // H - New Japanese-English Character Dictionary.
    ushort heisig; // L - Remembering The Kanji.
    ushort gakken; // K - Gakken Kanji Dictionary - A New Dictionary of Kanji Usage.
    ushort henshall; // E - A Guide To Remembering Japanese Characters.
    char snh[8]; // I - Spahn & Hadamitzky dictionary.

    /* Added info */
    ushort newnelson; // V - The New Nelson Japanese-English Character Dictionary
    char busy[4]; // DB - Japanese For Busy People - In the form n.nn (volume + chapter) or 1.A.
    ushort crowley; // DC -The Kanji Way to Japanese Language Power
    ushort flashc; // DF - Japanese Kanji Flashcards
    ushort kguide; // DG - Kodansha Compact Kanji Guide
    ushort henshallg;  // DH - 3rd edition of "A Guide To Reading and Writing Japanese"
    ushort context; // DJ - Kanji in Context
    ushort halpernk; // DK - Kanji Learners Dictionary (1999 edition)
    ushort halpernl; // DL - Kanji Learners Dictionary (2013 edition)
    ushort heisigf; // DM - French version of Remembering The Kanji
    ushort heisign; // DN - Remembering The Kanji, 6th Edition
    ushort oneil; // DO - Essential Kanji
    ushort halpernn; // DP - Kodansha Kanji Dictionary (2013)
    ushort deroo; // DR - 2001 Kanji
    ushort sakade; // DS - A Guide To Reading and Writing Japanese
    ushort tuttle; // DT - Tuttle Kanji Cards

    /**/

    //char c4[5]; // Q - Four-Corner code.
    //qint8 c4b;

    int word_freq; // Sum of the frequency of all words this kanji is in.

    //QChar *similar; // Length of similar is sim1+sim2.
    //int sim1;
    //int sim2;

    // Character arrays
    QCharStringList on; // List of ON readings.
    QCharStringList kun; // List of KUN readings.
    //QCharStringList irreg; // List of irregular readings from irregulars.txt.
    QCharStringList nam; // List of readings only found in names.

    QCharStringList meanings; // "Meaning" of the kanji.

    // Indexes or parts in radklist that contain this kanji.
    std::vector<ushort> radks;

    // Indexes of radicals in radlist that contain this kanji.
    std::vector<ushort> rads;
};

class Dictionary;
namespace ZKanji
{
    extern KanjiRadicalList radlist;

    // TODO: replace with a continuous array or vector. There's no need to dynamically
    // allocate each kanji.
    extern smartvector<KanjiEntry> kanjis;

    // Maps the radical symbols (from radkfile) with stroke-order element/variant pairs.
    extern std::map<ushort, std::pair<int, int>> radkmap;
    // The radical symbols (from radkfile) in order, and the kanji listed below them.
    extern std::vector<std::pair<ushort, fastarray<ushort>>> radklist;
    // Number of radicals with less than or equal stroke count to the vector index.
    // There's a padding 0 at index 0 so the vector indexes match up with the stroke count.
    extern std::vector<uchar> radkcnt;
    // Maps a kanji character to its similar kanji data. Similar kanji are classified in two
    // groups. The number in the data pair is the count of kanji in the array that belong to
    // the first classification group. The second part is an array of indexes to kanji.
    // WARNING: the key is a unicode character, while the array contains indexes to kanjis.
    extern std::map<ushort, std::pair<int, fastarray<ushort>>> similarkanji;

    // Must be called after the main dictionary has been loaded and the kanjis list has been
    // filled. Reads in similar.txt for similar kanji. If the file is not found it does nothing.
    void loadSimilarKanji(const QString &filename);

    KanjiEntry* addKanji(QChar ch, int kindex = -1);
    int kanjiIndex(QChar kanjichar);

    // Returns true if the ZKanji::kanjis list has nullptr items or the list is empty.
    bool isKanjiMissing();

    // Returns the sum of kanji readings in a given kanji. If compact is true, KUN readings
    // that only differ in okurigana are counted as a single reading.
    int kanjiReadingCount(KanjiEntry *k, bool compact);
    // Converts index of a reading in a kanji. The compact reading index is the index in a
    // list of readings, that only include a single KUN reading from every group of KUN
    // readings that only differ in okurigana.
    int kanjiCompactToReal(KanjiEntry *k, int readingindex);
    // Returns the reading of the k kanji at the compact reading index. An index of 0 returns
    // the kanji character itself for irregular readings, then the on and finally the kun
    // readings.
    QString kanjiCompactReading(KanjiEntry *k, int readingindex);

    // Returns the HTML formatted text to be displayed as the kanji information.
    QString kanjiInfoText(Dictionary *d, int index);

    // Returns the count of available kanji reference numbers in KanjiEntry objects.
    int kanjiReferenceCount();
    // Returns the readable title of available kanji reference numbers in KanjiEntry objects.
    QString kanjiReferenceTitle(int index, bool longname = false);
    // Returns the readable value of available kanji reference numbers in KanjiEntry objects.
    QString kanjiReference(KanjiEntry *k, int index);

    // Returns a widget to be displayed inside a ZToolTip for kanji.
    QWidget* kanjiTooltipWidget(Dictionary *d, int kindex);
}

// Converts JISX0208 to Shift-JIS.
ushort JIStoShiftJIS(ushort w);
// Converts JISX0208 to EUC-JP.
ushort JIStoEUC(ushort w);
// Converts JISX0208 to Kuten code.
QString JIStoKuten(ushort w);

#endif
