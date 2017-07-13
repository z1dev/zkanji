#include <QFile>
#include <QDataStream>
#include <memory>

#include "kanji.h"
#include "zkanjimain.h"


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

        for (int ix = 0; ix != r->kanji.size(); ++ix)
            ZKanji::kanjis[r->kanji[ix]]->rads.push_back(list.size());

        stream >> r->strokes;
        stream >> r->radical;

        stream >> u16;

        std::unique_ptr<QChar[]> names(new QChar[u16 + 1]);
        stream.readRawData((char*)names.get(), u16 * sizeof(QChar));
        names.get()[u16] = 0;

        // Count names.
        int cnt = 1;
        for (int ix = 1; u16 != 1 && ix != u16 - 1; ++ix)
            if (names.get()[ix] == QChar(' '))
                ++cnt;

        r->names.reserve(cnt);

        QCharTokenizer tok(names.get(), u16);
        while (tok.next())
            r->names.add(tok.token(), tok.tokenSize());



        list.push_back(r);
    }

    base::loadLegacy(stream, version);
}
