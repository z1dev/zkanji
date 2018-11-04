/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QDataStream>
#include <memory>

#include "kanji.h"
#include "zkanjimain.h"

#include "checked_cast.h"


void KanjiRadicalList::loadLegacy(QDataStream &stream, int version)
{
    clear();

    quint16 cnt;
    stream >> cnt;

    list.reserve(cnt);

    while (list.size() != cnt)
    {
        KanjiRadical *r = new KanjiRadical;
        quint16 u16;

        stream >> r->ch;
        stream >> u16;

        std::unique_ptr<ushort> kanji(new ushort[u16 + 1]);
        kanji.get()[u16] = 0;
        stream.readRawData((char*)kanji.get(), u16 * sizeof(ushort));
        for (int ix = 0; ix != u16; ++ix)
            r->kanji.push_back(ZKanji::kanjiIndex(kanji.get()[ix]));
        std::sort(r->kanji.begin(), r->kanji.end());
        kanji.reset();

        assert(list.size() <= std::numeric_limits<ushort>::max());

        for (int ix = 0, siz = tosigned(r->kanji.size()); ix != siz; ++ix)
            ZKanji::kanjis[r->kanji[ix]]->rads.push_back((ushort)list.size());

        stream >> r->strokes;
        stream >> r->radical;

        stream >> u16;

        std::unique_ptr<QChar[]> names(new QChar[u16 + 1]);
        stream.readRawData((char*)names.get(), u16 * sizeof(QChar));
        names.get()[u16] = 0;

        // Count names.
        int namecnt = 1;
        for (int ix = 1; u16 != 1 && ix != u16 - 1; ++ix)
            if (names.get()[ix] == QChar(' '))
                ++namecnt;

        r->names.reserve(namecnt);

        QCharTokenizer tok(names.get(), u16);
        while (tok.next())
            r->names.add(tok.token(), tok.tokenSize());



        list.push_back(r);
    }

    base::loadLegacy(stream, version);
}
