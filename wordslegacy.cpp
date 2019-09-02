/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QFileInfo>
#include <set>


#include "words.h"
#include "grammar.h"
#include "grammar_enums.h"
#include "zkanjimain.h"
#include "kanji.h"
#include "romajizer.h"
#include "groups.h"
#include "studydecks.h"
#include "worddeck.h"

namespace ZKanji
{
    // Temporary till first release:
    ushort tmppartsymbols[253] = {
        0x4e00, 0xff5c, 0x4e36, 0x30ce, 0x4e59, 0x4e85,

        0x4e8c, 0x4ea0, 0x4eba, 0x4ebb, 0x4e2a, 0x513f, 0x5165, 0x30cf,
        0x4e37, 0x5182, 0x5196, 0x51ab, 0x51e0, 0x51f5, 0x5200, 0x5202,
        0x529b, 0x52f9, 0x5315, 0x531a, 0x5341, 0x535c, 0x5369, 0x5382,
        0x53b6, 0x53c8, 0x30de, 0x4e5d, 0x30e6, 0x4e43, 0x0000,

        0x8fb6, 0x53e3, 0x56d7, 0x571f, 0x58eb, 0x5902, 0x5915, 0x5927,
        0x5973, 0x5b50, 0x5b80, 0x5bf8, 0x5c0f, 0x5c1a, 0x5c22, 0x5c38,
        0x5c6e, 0x5c71, 0x5ddd, 0x5ddb, 0x5de5, 0x5df2, 0x5dfe, 0x5e72,
        0x5e7a, 0x5e7f, 0x5ef4, 0x5efe, 0x5f0b, 0x5f13, 0x30e8, 0x5f51,
        0x5f61, 0x5f73, 0x5fc4, 0x624c, 0x6c35, 0x72ad, 0x8279, 0x9092,
        0x961d, 0x4e5f, 0x4ea1, 0x53ca, 0x4e45,

        0x8002, 0x5fc3, 0x6208, 0x6238, 0x624b, 0x652f, 0x6535, 0x6587,
        0x6597, 0x65a4, 0x65b9, 0x65e0, 0x65e5, 0x66f0, 0x6708, 0x6728,
        0x6b20, 0x6b62, 0x6b79, 0x6bb3, 0x6bd4, 0x6bdb, 0x6c0f, 0x6c14,
        0x6c34, 0x706b, 0x706c, 0x722a, 0x7236, 0x723b, 0x723f, 0x7247,
        0x725b, 0x72ac, 0x793b, 0x738b, 0x5143, 0x4e95, 0x52ff, 0x5c24,
        0x4e94, 0x5c6f, 0x5df4, 0x6bcb,

        0x7384, 0x74e6, 0x7518, 0x751f, 0x7528, 0x7530, 0x758b, 0x7592,
        0x7676, 0x767d, 0x76ae, 0x76bf, 0x76ee, 0x77db, 0x77e2, 0x77f3,
        0x793a, 0x79b9, 0x79be, 0x7a74, 0x7acb, 0x8864, 0x4e16, 0x5de8,
        0x518a, 0x6bcd, 0x7f52, 0x7259,

        0x74dc, 0x7af9, 0x7c73, 0x7cf8, 0x7f36, 0x7f8a, 0x7fbd, 0x800c,
        0x8012, 0x8033, 0x807f, 0x8089, 0x81ea, 0x81f3, 0x81fc, 0x820c,
        0x821f, 0x826e, 0x8272, 0x864d, 0x866b, 0x8840, 0x884c, 0x8863,
        0x897f,

        0x81e3, 0x898b, 0x89d2, 0x8a00, 0x8c37, 0x8c46, 0x8c55, 0x8c78,
        0x8c9d, 0x8d64, 0x8d70, 0x8db3, 0x8eab, 0x8eca, 0x8f9b, 0x8fb0,
        0x9149, 0x91c6, 0x91cc, 0x821b, 0x9ea6,

        0x91d1, 0x9577, 0x9580, 0x96b6, 0x96b9, 0x96e8, 0x9752, 0x975e,
        0x5944, 0x5ca1, 0x514d, 0x6589,

        0x9762, 0x9769, 0x97ed, 0x97f3, 0x9801, 0x98a8, 0x98db, 0x98df,
        0x9996, 0x9999, 0x54c1,

        0x99ac, 0x9aa8, 0x9ad8, 0x9adf, 0x9b25, 0x9b2f, 0x9b32, 0x9b3c,
        0x7adc, 0x97cb,

        0x9b5a, 0x9ce5, 0x9e75, 0x9e7f, 0x9ebb, 0x4e80, 0x6ef4, 0x9ec4,
        0x9ed2,

        0x9ecd, 0x9ef9, 0x7121, 0x6b6f,

        0x9efd, 0x9f0e, 0x9f13, 0x9f20,
        0x9f3b, 0x9f4a,

        0x9fa0
    };
}

void fixDefTypes(const QChar *kanjiform, fastarray<WordDefinition> &defs)
{
    int auxverbadj = 0;
    int auxverb = 0;
    int auxadj = 0;
    for (int ix = 0; ix != (int)WordTypes::Count; ++ix)
    {
        switch (ix)
        {
        case (int)WordTypes::TakesSuru:
        case (int)WordTypes::GodanVerb:
        case (int)WordTypes::IchidanVerb:
        case (int)WordTypes::SuruVerb:
        case (int)WordTypes::IkuVerb:
        case (int)WordTypes::KuruVerb:
        case (int)WordTypes::AruVerb:
        case (int)WordTypes::ZuruVerb:
            auxverbadj |= (1 << ix);
            auxverb |= (1 << ix);
            break;
        case (int)WordTypes::NaAdj:
        case (int)WordTypes::TrueAdj:
            auxverbadj |= (1 << ix);
            auxadj |= (1 << ix);
            break;
        }
    }

    int verbadj = 0;
    bool isverb = false;
    bool hadaux = false;

    int lasttype = 0;
    int lastnote = 0;
    // Fixing old format error where the word types and notes are not used
    // in latter definitions. Also when Aux is used, it will be marked
    // with the correct adjective or verb tag too.
    for (uchar ix = 0; ix != defs.size(); ++ix)
    {
        // Saving the first verb or adjective settings encountered for fixing
        // Aux.
        if (!hadaux && (defs[ix].attrib.types & auxverbadj) != 0)
            verbadj = (defs[ix].attrib.types & auxverbadj);

        // Types are not filled in for every sense in case a previous one
        // had them.
        if (defs[ix].attrib.types == 0)
            defs[ix].attrib.types = lasttype;
        if (defs[ix].attrib.notes == 0)
            defs[ix].attrib.notes = lastnote;

        // Fixing Aux in case a previous sense contained the correct type.
        if ((defs[ix].attrib.types & (1 << (int)WordTypes::Aux)) != 0)
        {
            if ((lasttype & auxverbadj) != 0)
            {
                defs[ix].attrib.types |= (lasttype & auxverbadj);
                verbadj = (defs[ix].attrib.types & auxverbadj);
            }
            else
                hadaux = true;
        }

        lasttype = defs[ix].attrib.types;
        lastnote = defs[ix].attrib.notes;

        isverb = isverb || (lasttype & auxverb) != 0;
    }

    // No Aux or all of them were fixed.
    if (!hadaux)
        return;

    // It's possible that Aux is in the types but no adjective or verb
    // form was set in that sense because it was not in JMdict. Only -i
    // adjectives can be guessed. For verbs, we can hope a latter sense
    // had it.

    // Couldn't find the verb or adjective type before the Aux. Let's hope
    // it's an -i adjective.
    if (!isverb && verbadj == 0 && kanjiform[qcharlen(kanjiform) - 1] == QChar(0x3044) /* i */)
        verbadj = (1 << (int)WordTypes::TrueAdj);

    if (!hadaux || verbadj == 0)
        return;

    // Add the verb or adjective type to everything with Aux only
    for (uchar ix = 0; ix != defs.size(); ++ix)
    {
        // Aux coming later can inherit the type before it.
        if ((defs[ix].attrib.types & auxverbadj) != 0)
            verbadj = (defs[ix].attrib.types & auxverbadj);

        // Aux found but no verb or adjective type.
        if ((defs[ix].attrib.types & (1 << (int)WordTypes::Aux)) != 0 && (defs[ix].attrib.types & auxverbadj) == 0)
            defs[ix].attrib.types |= verbadj;
    }
}

void OriginalWordsList::loadLegacy(QDataStream &stream, int /*version*/)
{
    clear();

    qint32 c, ch;
    quint8 b;

    qint32 i;
    quint8 f;

    stream >> c;

    list.reserve(c);

    int cnt = 0;

    while (cnt != c)
    {
        OriginalWord *w = new OriginalWord;
        w->freq = 0;
        w->inf = 0;

        list.push_back(w);
        ++cnt;

        stream >> ch;
        //fread(&c, sizeof(int), 1, f);
        w->change = OriginalWord::ChangeType(ch); //(TOriginalWordChange)c;
        stream >> make_zstr(w->kanji, ZStrFormat::Byte/*AddNull*/);
        stream >> make_zstr(w->kana, ZStrFormat::Byte/*AddNull*/);
        //read(w->kanji, f, sltByteAddNULL);
        //read(w->kana, f, sltByteAddNULL);

        stream >> b;

        //stream >> w->defcnt;
        //w->defs = new WordDefinition[w->defcnt];

        w->defs.resize(b);

        for (uchar ix = 0; ix != b; ++ix)
        {
            WordDefinition *def = w->defs.data() + ix;

            stream >> make_zstr(def->def, ZStrFormat::Word/*AddNull*/);
            //read(w->data[ix].meaning, f, sltWordAddNULL);
            stream >> i;
            def->attrib.types = convertOldTypes(i);
            //fread(&w->data[ix].types, sizeof(int), 1, f);
            stream >> i;
            def->attrib.notes = convertOldNotes(i);
            //fread(&w->data[ix].notes, sizeof(int), 1, f);
            stream >> f;
            def->attrib.fields = convertOldFields(f);
            //fread(&w->data[ix].fields, sizeof(byte), 1, f);
            stream.skipRawData(sizeof(qint16));
            //fread(&w->data[ix].nametag, sizeof(word), 1, f);
        }

        fixDefTypes(w->kanji.data(), w->defs);
    }
}

void OriginalWordsList::updateItemLegacy(int index, int windex, ushort wfreq)
{
    OriginalWord *o = list[index];
    o->index = windex;
    if (windex != -1 && wfreq != (ushort)-1)
        o->freq = wfreq;
}

void WordCommonsTree::loadLegacy(QDataStream &stream, int /*version*/)
{
    quint32 cnt;

    stream >> cnt;

    list.reserve(cnt);

    while (list.size() != cnt)
    {
        WordCommons *wc;

        wc = new WordCommons;

        stream >> make_zstr(wc->kanji, ZStrFormat::Byte);
        stream >> make_zstr(wc->kana, ZStrFormat::Byte);

        quint8 b;
        stream >> b;
        wc->jlptn = b;

        list.push_back(wc);
    }

    rebuild(false);
}

void Dictionary::loadBaseLegacy(QDataStream &stream, int version)
{
    char tmp[9];
    tmp[8] = 0;

    stream.readRawData(tmp, 8);
    bool good = !strncmp("zwrds", tmp, 5);
    int dversion = strtol(tmp + 5, 0, 10);

    if (!good || dversion < 10)
        throw ZException("Invalid legacy file format or file too old.");

    Temporary::SYSTEMTIME st;
    stream >> st;

    basedate = QDateTime(QDate(st.wYear, st.wMonth, st.wDay), QTime(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds));
    basedate.setTimeSpec(Qt::UTC);

    //double dbversion;
    stream.skipRawData(/*(char*)&dbversion,*/ sizeof(double));

    prgversion = QStringLiteral("pre2015");

    ZKanji::commons.loadLegacy(stream, version);

    //bool kreload = !ZKanji::kanjis.empty();

    KanjiEntry *k;
    uchar b;

    quint8 rcnt = 0; // Count of readings for kanji.
    QChar *rtmp = nullptr; // Temporary array for reading in old format kanji readings.

    for (int ix = 0; ix < 6355; ++ix)
    {
        QChar ch;
        stream >> ch;
        k = ZKanji::addKanji(ch);

        quint16 u16;
        quint8 u8;

        stream >> u16;
        k->jis = u16;

        stream.skipRawData(2); // k->uni;

        stream >> u8;
        k->rad = u8;
        k->crad = 255;

        stream >> u8;
        k->jouyou = u8;

        // Nobody cares for the old jlpt level anymore.
        stream.skipRawData(sizeof(uchar)); //stream >> k->JLPT;

        stream >> u8;
        k->jlpt = u8;
        stream >> u8;
        k->strokes = u8;

        stream >> u16;
        k->frequency = u16;

        stream >> u16;
        k->knk = u16;

        stream >> u8;
        k->skips[0] = u8;
        stream >> u8;
        k->skips[1] = u8;
        stream >> u8;
        k->skips[2] = u8;

        stream >> u16;
        k->nelson = u16;
        stream >> u16;
        k->halpern = u16;
        stream >> u16;
        k->heisig = u16;
        stream >> u16;
        k->gakken = u16;
        stream >> u16;
        k->henshall = u16;

        stream.readRawData(k->snh, 8);

        // 4Corner skip
        stream.skipRawData(5);
        stream.skipRawData(1);

        qint32 i32;
        stream >> i32;
        k->word_freq = i32;

        stream >> rcnt;
        k->on.reserve(rcnt);
        for (int j = 0; j < rcnt; ++j)
        {
            stream >> b;
            rtmp = new QChar[b];
            for (uchar iy = 0; iy < b; ++iy)
                stream.readRawData((char*)(rtmp + iy), sizeof(QChar));

            k->on.add(rtmp, b);
            delete[] rtmp;
        }

        stream >> rcnt;
        k->kun.reserve(rcnt);
        for (int j = 0; j < rcnt; ++j)
        {
            stream >> b;
            rtmp = new QChar[b];
            for (uchar iy = 0; iy < b; ++iy)
                stream.readRawData((char*)(rtmp + iy), sizeof(QChar));

            k->kun.add(rtmp, b);
            delete[] rtmp;
        }

        QString meaning;
        stream >> make_zstr(meaning, ZStrFormat::Word);
        k->meanings.copy(meaning.split(", ", QString::SkipEmptyParts));
    }

    ZKanji::generateValidKanji();

    quint16 cc;
    ZKanji::radkcnt.clear();
    ZKanji::radkcnt.resize(18, 0);
    ZKanji::radkcnt[1] = 6;
    ZKanji::radkcnt[2] = 37; // 31;
    ZKanji::radkcnt[3] = 82; // 45;
    ZKanji::radkcnt[4] = 126; // 44;
    ZKanji::radkcnt[5] = 154; // 28;
    ZKanji::radkcnt[6] = 179; // 25;
    ZKanji::radkcnt[7] = 200; // 21;
    ZKanji::radkcnt[8] = 212; // 12;
    ZKanji::radkcnt[9] = 223; // 11;
    ZKanji::radkcnt[10] = 233; // 10;
    ZKanji::radkcnt[11] = 242; // 9;
    ZKanji::radkcnt[12] = 246; // 4;
    ZKanji::radkcnt[13] = 249; // 3;
    ZKanji::radkcnt[14] = 252; // 3;
    ZKanji::radkcnt[15] = 252; // 0;
    ZKanji::radkcnt[16] = 252; // 0;
    ZKanji::radkcnt[17] = 253; // 1;

    ZKanji::radklist.clear();
    ZKanji::radklist.resize(253);

    // Read radical parts.
    for (int ix = 0; ix < 253; ++ix)
    {
        // In old format the 37th (with index 36) was missing.
        if (ix == 36)
            continue;

        stream >> cc;

        ZKanji::radklist[ix].first = ZKanji::tmppartsymbols[ix];
        ZKanji::radklist[ix].second.setSize(cc);

        for (int j = 0, sizj = tosigned(ZKanji::radklist[ix].second.size()); j != sizj; ++j)
        {
            stream >> cc;
            ZKanji::radklist[ix].second[j] = cc;
            ZKanji::kanjis[cc]->radks.push_back(ix);
        }
        std::sort(ZKanji::radklist[ix].second.begin(), ZKanji::radklist[ix].second.end());
    }


    ZKanji::radlist.loadLegacy(stream, version);
}

void Dictionary::loadLegacy(QDataStream &stream, int version, bool maindict, bool skiporiginals)
{
    // Usually prgversion is loaded with the base dictionary, but if we use legacy code here,
    // that means the base dictionary is for the new version. In that case loadBaseLegacy is
    // never seen.
    prgversion = QStringLiteral("pre2015");

    // Main dictionary write date not used here.
    stream.skipRawData(sizeof(Temporary::SYSTEMTIME));

    Temporary::SYSTEMTIME st;
    stream >> st;
    writedate = QDateTime(QDate(st.wYear, st.wMonth, st.wDay), QTime(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds));
    writedate.setTimeSpec(Qt::UTC);

    stream >> make_zstr(info);

    dtree.loadLegacy(stream, 1);
    ktree.loadLegacy(stream, 1);
    btree.loadLegacy(stream, 1);

    // Read additional kanji data for the given dictionary.
    kanjidata.clear();
    kanjidata.reserve(ZKanji::kanjicount);

    quint16 cc;

    while (kanjidata.size() != 6355)
    {
        stream >> cc;

        while (cc--)
            kanjidata.push_back(new KanjiDictData());

        if (kanjidata.size() == 6355)
            break;

        stream >> cc;

        QString meaning;
        while (cc--)
        {
            kanjidata.push_back(new KanjiDictData());
            stream >> make_zstr(meaning, ZStrFormat::Word);
            kanjidata.back()->meanings.copy(meaning.split(", ", QString::SkipEmptyParts));
        }
    }

    cc = 0;
    quint32 cnt;

    auto pos = kanjidata.begin();
    while (pos - kanjidata.begin() != 6355)
    {
        stream >> cc;
        std::advance(pos, cc); // cnt += cc;

        // Skip zeroes.
        if (pos - kanjidata.begin() == 6355)
            break;

        stream >> cc;

        while (cc--)
        {
            stream >> cnt;

            std::vector<int> &kw = (*pos)->words;
            kw.reserve(cnt);

            while (cnt--)
            {
                int i;
                stream >> i;
                kw.push_back(i);
            }

            ++pos;
        }
    }

    // Read the dictionary words.
    stream >> cnt;

    words.reserve(cnt);

    //quint8 u8;
    //quint16 u16;
    //quint32 u32;

    qint32 i;
    quint8 f;

    while (cnt--)
    {
        WordEntry *w = new WordEntry;

        qint8 b;
        QChar *ktmp;

        w->inf = 0;
        stream >> w->freq;

        stream >> b;
        ktmp = new QChar[b];
        for (int ix = 0; ix < b; ++ix)
            stream.readRawData((char*)(ktmp + ix), sizeof(QChar));
        w->kanji.copy(ktmp, b);
        delete[] ktmp;

        stream >> b;
        ktmp = new QChar[b];

        for (int ix = 0; ix < b; ++ix)
            stream.readRawData((char*)(ktmp + ix), sizeof(QChar));
        w->kana.copy(ktmp, b);
        delete[] ktmp;

        stream >> make_zstr(w->romaji, ZStrFormat::Byte);
        if (w->romaji != romanize(w->kana))
            throw ZException("Invalid romanized form for kana in legacy dictionary file.");

        stream >> b;
        w->defs.resize(b);

        for (int ix = 0; ix < b; ++ix)
        {
            WordDefinition &def = w->defs[ix];
            stream >> make_zstr(def.def, ZStrFormat::Word);

            stream >> i;
            def.attrib.types = convertOldTypes(i);
            stream >> i;
            def.attrib.notes = convertOldNotes(i);
            stream >> f;
            def.attrib.fields = convertOldFields(f);
            def.attrib.dialects = 0;

            stream.skipRawData(sizeof(qint16));
        }

        fixDefTypes(w->kanji.data(), w->defs);

        words.push_back(w);
    }

    if (stream.atEnd())
        throw ZException("Unexpected end of legacy dictionary file.");

    // Skip originally saved symbol-to-word data. Even the count was different.
    for (int ix = 0; ix != (0x30ff - 0x3000) + (0xffe0 - 0xff00); ++ix)
    {
        stream >> cnt;
        stream.skipRawData(cnt * sizeof(qint32));
    }

    for (int ix = 0, siz = tosigned(words.size()); ix != siz; ++ix)
    {
        QCharString &k = words[ix]->kanji;
        std::set<ushort> codes;
        for (int j = k.size() - 1; j != -1; --j)
        {
            //incode = false;

            ushort ch = k[j].unicode();

            if (KANA(ch) || !UNICODE_J(ch))
                continue;

            //if (ch >= 0x3000 && ch <= 0x30ff)
            //{
            //    if (!ZKanji::validcode[ch - 0x3000])
            //        continue;
            //    //codemin = 0x3000;
            //    incode = true;
            //}
            //else if (k[j].unicode() >= 0xff01 && k[j].unicode() <= 0xffee)
            //{
            //    if (!ZKanji::validcode[ch - 0xff01 + ZKanji::validcodelowerhalf + 1])
            //        continue;
            //    //codemin = 0xff01 - ZKanji::validcodelowerhalf - 1;
            //    incode = true;
            //}

            //if (incode)
            //{
            if (codes.count(ch) != 0)
                continue;

            codes.insert(ch);
            symdata[ch].push_back(ix);
            //}
        }
    }

#ifdef _DEBUG
    // Creation of kanadata takes a lot of time both in the debugger and in release code.
    // If the data does not exist, or its write date is different from the dictionary write
    // date, we recompute it in a slow way, but save it to tempkanadata.temp next to the
    // debug executable.
    // If it exists and has the correct date we read it from there, hopefully making
    // startup of the debug code faster.

    // This is only done for the base dictionary.
    bool kanadata_exists = false;

    QFileInfo fi;
    QFile f2;

    if (maindict)
        f2.setFileName(qApp->applicationDirPath() + "/tempkanadata.temp");
    if (maindict && fi.exists(qApp->applicationDirPath() + "/tempkanadata.temp"))
    {
        kanadata_exists = true;
        f2.open(QIODevice::ReadOnly);
        QDataStream stream2(&f2);

        stream2.setVersion(QDataStream::Qt_5_5);
        stream2.setByteOrder(QDataStream::LittleEndian);

        QDateTime kanadate;
        stream2 >> kanadate;

        if (kanadate != writedate)
        {
            kanadata_exists = false;
            f2.close();
        }
        else
        {
            //for (const auto &it : kanadata)
            //{
            //    stream2 << (qint16)it.first;
            //    stream2 << (quint32)it.second.size();
            //    for (int j = 0; j != it.second.size(); ++j)
            //        stream << (qint32)it.second.at(j);
            //}

            qint16 ch;
            quint32 datcnt;
            qint32 wix;
            while (!stream2.atEnd())
            {
                stream2 >> ch;
                stream2 >> datcnt;
                for (int j = 0, sizj = tosigned(datcnt); j != sizj; ++j)
                {
                    stream2 >> wix;
                    kanadata[ch].push_back(wix);
                }
            }
        }
    }


    if (!maindict || !kanadata_exists)
    {
#endif
        for (auto ix = 0, siz = tosigned(words.size()); ix != siz; ++ix)
        {
            QCharString &r = words[ix]->romaji;
            int rlen = r.size();
            std::set<ushort> codes;

            for (int j = 0; j != rlen; ++j)
            {
                ushort ch;


                int dummy;
                if (!kanavowelize(ch, dummy, r.data() + j, rlen - j) || codes.count(ch) != 0)
                    continue;

                codes.insert(ch);
                kanadata[ch].push_back(ix);
            }
        }
#ifdef _DEBUG

        if (maindict)
        {
            f2.open(QIODevice::WriteOnly);

            QDataStream stream2(&f2);
            stream2.setVersion(QDataStream::Qt_5_5);
            stream2.setByteOrder(QDataStream::LittleEndian);

            stream2 << writedate;

            for (const auto &it : kanadata)
            {
                stream2 << (qint16)it.first;
                stream2 << (quint32)it.second.size();
                for (int j = 0, sizj = tosigned(it.second.size()); j != sizj; ++j)
                    stream2 << (qint32)it.second[j];
            }
        }
    }
#endif

    if (stream.atEnd())
        throw ZException("Unexpected end of legacy dictionary file.");

    abcde.resize(words.size());
    aiueo.resize(words.size());

    //qint32 i32;

    // Can't trust alphabetic ordering of old version, which might have
    // ordered the words differently.

    stream.skipRawData(tosigned(sizeof(qint32) * 2 * words.size()));

    for (int ix = 0, siz = tosigned(words.size()); ix != siz; ++ix)
    {
        abcde[ix] = ix;
        aiueo[ix] = ix;
    }

    std::map<QCharString, QString> hira;
    std::sort(abcde.begin(), abcde.end(), [this, &hira](int a, int b) {
        int val = qcharcmp(words[a]->romaji.data(), words[b]->romaji.data());
        if (val != 0)
            return val < 0;

        QString ah;
        QString bh;
        auto it = hira.find(words[a]->kana);
        if (it != hira.end())
            ah = it->second;
        else
            hira[words[a]->kana] = ah = hiraganize(words[a]->kana);
        it = hira.find(words[b]->kana);
        if (it != hira.end())
            bh = it->second;
        else
            hira[words[b]->kana] = bh = hiraganize(words[b]->kana);
        val = qcharcmp(ah.constData(), bh.constData());
        if (val != 0)
            return val < 0;

        val = qcharcmp(words[a]->kana.data(), words[b]->kana.data());
        if (val != 0)
            return val < 0;

        return qcharcmp(words[a]->kanji.data(), words[b]->kanji.data()) < 0;
    });


    std::sort(aiueo.begin(), aiueo.end(), [this, &hira](int a, int b) {
        QString ah;
        QString bh;
        auto it = hira.find(words[a]->kana);
        if (it != hira.end())
            ah = it->second;
        else
            hira[words[a]->kana] = ah = hiraganize(words[a]->kana);
        it = hira.find(words[b]->kana);
        if (it != hira.end())
            bh = it->second;
        else
            hira[words[b]->kana] = bh = hiraganize(words[b]->kana);
        int val = qcharcmp(ah.constData(), bh.constData());
        if (val != 0)
            return val < 0;

        val = qcharcmp(words[a]->kana.data(), words[b]->kana.data());
        if (val != 0)
            return val < 0;

        return qcharcmp(words[a]->kanji.data(), words[b]->kanji.data()) < 0;
    });

    if (stream.atEnd())
        throw ZException("Unexpected end of legacy dictionary file.");

    if (maindict)
    {
        if (!skiporiginals)
        {
            ZKanji::originals.loadLegacy(stream, version);
            for (int ix = 0; ix != tosigned(ZKanji::originals.size()); ++ix)
            {
                //const OriginalWord *o = ZKanji::originals.items(ix);
                int oindex = findKanjiKanaWord(ZKanji::originals.items(ix)->kanji, ZKanji::originals.items(ix)->kana);
                ushort ofreq = (ushort)-1;
                if (oindex != -1)
                    ofreq = words[oindex]->freq;
                ZKanji::originals.updateItemLegacy(ix, oindex, ofreq);
            }
        }
        else if (!ZKanji::originals.skip(stream))
        {
            //MsgBox(L"Error occurred while loading dictionary. Please make sure the base dictionary is valid.", L"Error", MB_OK);
            //THROW(L"Corrupt file. Originals list must be empty.");
            throw ZException("Legacy dictionary file corrupt. Original words data missing or invalid.");
        }
    }
}

void Dictionary::loadUserDataLegacy(QDataStream &stream, int version)
{
    Temporary::SYSTEMTIME st;
    stream >> st;

    QDateTime dictwritedate = QDateTime(QDate(st.wYear, st.wMonth, st.wDay), QTime(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds));
    dictwritedate.setTimeSpec(Qt::UTC);

    if (dictwritedate != lastWriteDate())
    {
        //QString wt = writedate.toString();
        //QString lwt = dict->lastWriteDate().toString();
        //QString dgt = dict->baseGenerationDate().toString();
        throw ZException("Dictionary and user data date mismatch in legacy format.");
    }

    groups->loadWordsLegacy(stream, version);

    studydecks->loadDecksLegacy(stream, -1);
    decks->add(tr("Deck %1").arg(1));
    decks->items(0)->loadLegacy(stream);
    //worddecks.back()->loadLegacy(stream);
    studydecks->fixStats();

    int cnt = 0;
    qint16 w;
    qint32 val;
    quint8 b;
    while (cnt < 6355)
    {
        stream >> w;
        cnt += w;
        if (cnt >= 6355)
            break;

        //int d = 0;
        stream >> w;
        for (w += cnt; cnt < w; ++cnt)
        {
            qint32 excnt; // Number of word examples marked for a kanji.

            stream >> excnt;

            kanjidata[cnt]->ex.reserve(excnt);
            for (int ix = 0; ix != excnt; ++ix)
            {
                stream >> val; // Index of word.
                stream >> b; // Index of reading in kanji.
                //KanjiExample ex;
                //ex.windex = val;
                //ex.reading = b;
                kanjidata[cnt]->ex.push_back(val);
            }

            // Skip reading the bits of checked kanji readings.
            stream >> b;
            stream.skipRawData((b + 7) / 8);
        }
    }

    std::map<int, QCharString> dummy;
    Temporary::LoadGapList(stream, dummy);
    dummy.clear();
    Temporary::LoadGapList(stream, dummy);

    groups->loadKanjiLegacy(stream, version);
}
