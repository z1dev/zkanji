/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QDataStream>
#include <QFile>
#include <QTextStream>
#include <QStringBuilder>
#include <QMessageBox>
#include <memory>

#include "zkanjimain.h"
#include "romajizer.h"
#include "kanji.h"
#include "kanjisettings.h"
#include "colorsettings.h"
#include "fontsettings.h"
#include "generalsettings.h"

#include "checked_cast.h"

//-------------------------------------------------------------

namespace ZKanji
{
    // Same as the radsymbols in ZRadicalGrid, without the added stroke count.
    QChar radsymbols[214] = {
        //1,
        0x4e00, 0x4e28, 0x4e36, 0x4e3f, 0x4e59, 0x4e85,
        //2,
        0x4e8c, 0x4ea0, 0x4eba, 0x513f, 0x5165, 0x516b, 0x5182, 0x5196,
        0x51ab, 0x51e0, 0x51f5, 0x5200, 0x529B, 0x52F9, 0x5315, 0x531A,
        0x5338, 0x5341, 0x535C, 0x5369, 0x5382, 0x53B6, 0x53C8,
        //3,
        0x53E3, 0x56D7, 0x571F, 0x58EB, 0x5902, 0x590A, 0x5915, 0x5927,
        0x5973, 0x5B50, 0x5B80, 0x5BF8, 0x5C0F, 0x5C22, 0x5C38, 0x5C6E,
        0x5C71, 0x5DDB, 0x5DE5, 0x5DF1, 0x5DFE, 0x5E72, 0x5E7A, 0x5E7F,
        0x5EF4, 0x5EFE, 0x5F0B, 0x5F13, 0x5F50, 0x5F61, 0x5F73,
        //4,
        0x5FC3, 0x6208, 0x6236, 0x624B, 0x652F, 0x6534, 0x6587, 0x6597,
        0x65A4, 0x65B9, 0x65E0, 0x65E5, 0x66F0, 0x6708, 0x6728, 0x6B20,
        0x6B62, 0x6B79, 0x6BB3, 0x6BCB, 0x6BD4, 0x6BDB, 0x6C0F, 0x6C14,
        0x6C34, 0x706B, 0x722A, 0x7236, 0x723B, 0x723F, 0x7247, 0x7259,
        0x725B, 0x72AC,
        //5,
        0x7384, 0x7389, 0x74DC, 0x74E6, 0x7518, 0x751F, 0x7528, 0x7530,
        0x758B, 0x7592, 0x7676, 0x767D, 0x76AE, 0x76BF, 0x76EE, 0x77DB,
        0x77E2, 0x77F3, 0x793A, 0x79B8, 0x79BE, 0x7A74, 0x7ACB,
        //6,
        0x7AF9, 0x7C73, 0x7CF8, 0x7F36, 0x7F51, 0x7F8A, 0x7FBD, 0x8001,
        0x800C, 0x8012, 0x8033, 0x807F, 0x8089, 0x81E3, 0x81EA, 0x81F3,
        0x81FC, 0x820C, 0x821B, 0x821F, 0x826E, 0x8272, 0x8278, 0x864D,
        0x866B, 0x8840, 0x884C, 0x8863, 0x897F,
        //7,
        0x898B, 0x89D2, 0x8A00, 0x8C37, 0x8C46, 0x8C55, 0x8C78, 0x8C9D,
        0x8D64, 0x8D70, 0x8DB3, 0x8EAB, 0x8ECA, 0x8F9B, 0x8FB0, 0x8FB5,
        0x9091, 0x9149, 0x91C6, 0x91CC,
        //8,
        0x91D1, 0x9577, 0x9580, 0x961C, 0x96B6, 0x96B9, 0x96E8, 0x9751,
        0x975E,
        //9,
        0x9762, 0x9769, 0x97CB, 0x97ED, 0x97F3, 0x9801, 0x98A8, 0x98DB,
        0x98DF, 0x9996, 0x9999,
        //10,
        0x99AC, 0x9AA8, 0x9AD8, 0x9ADF, 0x9B25, 0x9B2F, 0x9B32, 0x9B3C,
        //11,
        0x9B5A, 0x9CE5, 0x9E75, 0x9E7F, 0x9EA5, 0x9EBB,
        //12,
        0x9EC4, 0x9ECD, 0x9ED1, 0x9EF9,
        //13,
        0x9EFD, 0x9F0E, 0x9F13, 0x9F20,
        //14,
        0x9F3B, 0x9F4A,
        //15,
        0x9F52,
        //16,
        0x9F8D,
        //17,
        0x9F9C, 0x9FA0
    };


    KanjiRadicalList radlist;
    smartvector<KanjiEntry> kanjis;

    std::map<ushort, std::pair<int, int>> radkmap;
    std::vector<std::pair<ushort, fastarray<ushort>>> radklist;
    std::vector<uchar> radkcnt;
    std::map<ushort, std::pair<int, fastarray<ushort>>> similarkanji;

    void loadSimilarKanji(const QString &filename)
    {

        QFile f;
        f.setFileName(filename);
        if (!f.open(QIODevice::ReadOnly))
            return;

        QTextStream stream;

        stream.setDevice(&f);
        //stream.setVersion(QDataStream::Qt_5_5);
        //stream.setByteOrder(QDataStream::LittleEndian);

        QString line;
        while (stream.readLineInto(&line))
        {
            if (line.size() < 3 || kanjiIndex(line.at(0).unicode()) == -1)
                continue;

            int scpos = line.indexOf(QChar(';'), 1);
            if (scpos < 1)
                continue;

            //QChar kchar = line.at(0);
            int lsiz = tosigned(line.size());

            // Instead of error checking, count the number of (possibly) valid kanji in the
            // line before and after the semicolon.

            int before = 0, after = 0;

            for (int ix = 1; ix != scpos; ++ix)
                if (kanjiIndex(line.at(ix).unicode()) != -1)
                    ++before;
            for (int ix = scpos + 1; ix != lsiz; ++ix)
                if (kanjiIndex(line.at(ix).unicode()) != -1)
                    ++after;

            auto &pair = similarkanji[line.at(0).unicode()];
            pair.first = before;
            pair.second.setSize(before + after);

            int pos = 0;
            int kix;
            for (int ix = 1; ix != scpos; ++ix)
                if ((kix = kanjiIndex(line.at(ix).unicode())) != -1)
                {
                    pair.second[pos] = kix;
                    ++pos;
                }
            for (int ix = scpos + 1; ix != lsiz; ++ix)
                if ((kix = kanjiIndex(line.at(ix).unicode())) != -1)
                {
                    pair.second[pos] = kix;
                    ++pos;
                }
        }

        //QMessageBox::information(nullptr, "zkanji", QString("no kanji? %1").arg(ZKanji::similarkanji.size()));
    }

    KanjiEntry* addKanji(QChar ch, int kindex)
    {
        if (kindex != -1 && tosigned(kanjis.size()) > kindex && kanjis[kindex] != nullptr)
            return nullptr;

        KanjiEntry *kanji = new KanjiEntry;
        kanji->ch = ch;
        kanji->index = kindex;

        //int siz = kanjis.size();
        if (kindex == -1)
            kanjis.push_back(kanji);
        else
        {
            if (kindex >= tosigned(kanjis.size()))
                kanjis.resizeAddNull(kindex + 1);
            kanjis[kindex] = kanji;
        }

        return kanji;
    }

    std::map<QChar, int> kanjiindexmap;
    int kmapchecked = 0;
    int kanjiIndex(QChar kanjichar)
    {
        // Kanji are not in any particular order. To speed up looking for them, a mapping is
        // created on request.

        if (kanjis.empty())
            return -1;

        if (!kanjiindexmap.empty())
        {
            auto it = kanjiindexmap.find(kanjichar);
            if (it != kanjiindexmap.end())
                return it->second;
        }

        for (int ksiz = tosigned(kanjis.size()); kmapchecked != ksiz; ++kmapchecked)
        {
            QChar ch = kanjis[kmapchecked]->ch;
            kanjiindexmap[ch] = kmapchecked;
            if (ch == kanjichar)
            {
                ++kmapchecked;
                return kmapchecked - 1;
            }
        }
        return -1;
    }

    bool isKanjiMissing()
    {
        if (kanjis.size() == 0)
            return true;
        for (int ix = 0, siz = tosigned(kanjis.size()); ix != siz; ++ix)
            if (kanjis[ix] == nullptr)
                return true;
        return false;
    }

    int kanjiReadingCount(KanjiEntry *k, bool compact)
    {
        if (!compact)
            return 1 + tosigned(k->on.size() + k->kun.size());

        // Only ON reading and one for the possible non-standard readings.
        if (k->kun.empty())
            return tosigned(k->on.size()) + 1;

        // First reading is not matched to any other one so it always
        // counts as 1.
        int cnt = 1;

        // Save the first reading. The reading in w is updated when the
        // new reading found differs before the period character or str end.
        const QChar *w = k->kun[0].data();
        const QChar *p = qcharchr(w, '.');
        if (!p)
            p = qcharchr(w, 0);

        for (int ix = 1, siz = tosigned(k->kun.size()); ix != siz; ++ix)
        {
            const QChar *kun = k->kun[ix].data();
            const QChar *kp = qcharchr(kun, '.');
            if (!kp)
                kp = qcharchr(kun, 0);

            // Tries to match the saved reading and the ixth reading till the
            // period character.
            if (p - w != kp - kun || qcharncmp(w, kun, p - w))
            {
                p = kp;
                w = kun;
                cnt++;
            }
        }
        return 1 + tosigned(k->on.size()) + cnt;
    }

    int kanjiCompactToReal(KanjiEntry *k, int index)
    {
        // Irregular or on reading index is returned unchanged.
        if (index <= tosigned(k->on.size()))
            return index;

        // Position of index in compact kun readings.
        int pos = index - 1 - tosigned(k->on.size());

        // Save the first reading. The reading in w is updated when the
        // new reading found differs before the period character or str end.
        const QChar *w = k->kun[0].data();
        const QChar *p = qcharchr(w, '.');
        if (!p)
            p = qcharchr(w, 0);
        int ix = 1;

        while (pos)
        {
#ifdef _DEBUG
            if (ix >= tosigned(k->kun.size()))
                throw "Overindexing in kun reading count";
#endif

            const QChar *kun = k->kun[ix++].data();
            const QChar *kp = qcharchr(kun, '.');
            if (!kp)
                kp = qcharchr(kun, 0);

            // Found a reading which differs from the saved one.
            if (p - w != kp - kun || qcharncmp(w, kun, p - w))
            {
                p = kp;
                w = kun;
                --pos;
                continue;
            }
            ++index;
        }

        return index;
    }

    QString kanjiCompactReading(KanjiEntry *k, int readingindex)
    {
        if (readingindex == 0)
            return k->ch;

        if (readingindex <= tosigned(k->on.size()))
            return k->on[readingindex - 1].toQString();

        return k->kun[kanjiCompactToReal(k, readingindex) - k->on.size() - 1].toQStringRaw().section('.', 0, 0, QString::SectionSkipEmpty);
    }

    QString kanjiMeanings(Dictionary *d, int index);
    QString kanjiInfoText(Dictionary *d, int index)
    {
        if (d == nullptr)
            return QString();

        if (index < 0)
            return QString("<html><body><p>%1</p></body></html>").arg(qApp->translate("KanjiInfoForm", "The displayed diagram is not a kanji."));

        QString result;
        KanjiEntry *k = ZKanji::kanjis[index];

        QString on;
        QString kun;

        for (QCharString &chr : k->on)
        {
            int siz = tosigned(chr.size());
            QString str(siz * 2, QChar(' '));
            for (int ix = 0; ix != siz; ++ix)
            {
                str[ix * 2] = chr[ix];
                str[ix * 2 + 1] = 0x2060;
            }
            on += QString("<font face=\"%1\">%2</font>, ").arg(Settings::fonts.kana.toHtmlEscaped(), str);
        }
        if (!k->on.empty())
            on.resize(on.size() - 2);

        for (QCharString &chr : k->kun)
        {
            int siz = tosigned(chr.size());
            QString str(siz * 2, QChar(' '));
            bool hasoku = false;
            for (int ix = 0, spansiz = 0; ix != siz; ++ix)
            {
                str[ix * 2 + spansiz] = chr[ix];
                str[ix * 2 + 1 + spansiz] = 0x2060;
                if (!hasoku && Settings::colors.coloroku && chr[ix] == '.')
                {
                    QColor c = Settings::uiColor(ColorSettings::Oku);
                    hasoku = true;
                    // TODO: .arg() usage where user input is replaced, defend against inserting %1 etc. which can cause trouble with subsequent .arg() calls.
                    // HTML escape any user input string with toHtmlEscaped().
                    str.insert(ix * 2 + 2, QString("<span style='color: %1'>").arg(c.name()));
                    spansiz = 29;
                }
            }
            kun += QString("<font face=\"%1\">").arg(Settings::fonts.kana.toHtmlEscaped()) % str % (hasoku ? QString("</span>") : QString()) % QString("</font>") % ", ";
        }
        if (!k->kun.empty())
            kun.resize(kun.size() - 2);

        result += QString("<html><body>");

        result += QString("<p><b>%1:</b><br><span style=\"font-size:%2pt\">%3</span></p>").arg(qApp->translate("KanjiInfoForm", "Meanings").toHtmlEscaped()).arg(Settings::scaled(11)).arg(kanjiMeanings(d, index).toHtmlEscaped());
        result += QString("<p><b>%1:</b><br>%2<br>").arg(qApp->translate("KanjiInfoForm", "ON readings").toHtmlEscaped()).arg(on) %
            QString("<b>%1:</b><br>%2</p>").arg(qApp->translate("KanjiInfoForm", "Kun readings").toHtmlEscaped()).arg(kun);

        QString str;
        for (int ix = 0; ix != 4; ++ix)
        {
            int ref = 0;
            switch (ix)
            {
            case 0:
                ref = Settings::kanji.mainref1;
                break;
            case 1:
                ref = Settings::kanji.mainref2;
                break;
            case 2:
                ref = Settings::kanji.mainref3;
                break;
            case 3:
                ref = Settings::kanji.mainref4;
                break;
            }
            if (ref == 0)
                continue;

            str += QString("<b>%1:</b> %2<br>").arg(ZKanji::kanjiReferenceTitle(ref), ZKanji::kanjiReference(k, ref));
        }

        if (!str.isEmpty())
            result += QString("<p>%1</p>").arg(str.left(str.size() - 4));

        result += "<p><hr></p>";

        str = QString();
        for (int ix = 0, siz = ZKanji::kanjiReferenceCount(); ix != siz; ++ix)
        {
            if (Settings::kanji.showref[Settings::kanji.reforder[ix]] == false)
                continue;
            str += QString("<b>%1:</b> %2<br>").arg(ZKanji::kanjiReferenceTitle(Settings::kanji.reforder[ix]).toHtmlEscaped(), ZKanji::kanjiReference(k, Settings::kanji.reforder[ix]).toHtmlEscaped());
        }
        if (!str.isEmpty())
            result += QString("<p>%1</p>").arg(str.left(str.size() - 4));

        result += QString("</body></html>");

        return result;
    }

    int kanjiReferenceCount()
    {
        return 33;
    }

    QString kanjiReferenceTitle(int index, bool longname)
    {
        if (!longname)
        {
            switch (index)
            {
            case 0:
                return qApp->translate("kanji", "Strokes");
            case 1:
                return qApp->translate("kanji", "JIS X0208");
            case 2:
                return qApp->translate("kanji", "Unicode");
            case 3:
                return qApp->translate("kanji", "Jouyou");
            case 4:
                return qApp->translate("kanji", "JLPT");
            case 5:
                return qApp->translate("kanji", "Frequency");
            case 6:
                return qApp->translate("kanji", "SKIP code");
            case 7:
                return qApp->translate("kanji", "Kanji & Kana");
            case 8:
                return qApp->translate("kanji", "Nelson #");
            case 9:
                return qApp->translate("kanji", "Halpern #");
            case 10:
                return qApp->translate("kanji", "Heisig #");
            case 11:
                return qApp->translate("kanji", "Gakken #");
            case 12:
                return qApp->translate("kanji", "Henshall #");
            case 13:
                return qApp->translate("kanji", "S&H Dict.");
            case 14:
                return qApp->translate("kanji", "New Nelson #");
            case 15:
                return qApp->translate("kanji", "4BusyPeople");
            case 16:
                return qApp->translate("kanji", "Crowley #");
            case 17:
                return qApp->translate("kanji", "Flashcards");
            case 18:
                return qApp->translate("kanji", "Kodansha CKG");
            case 19:
                return qApp->translate("kanji", "Guide (3rd)");
            case 20:
                return qApp->translate("kanji", "K.In Context");
            case 21:
                return qApp->translate("kanji", "K.Learners (1999)");
            case 22:
                return qApp->translate("kanji", "K.Learners (2013)");
            case 23:
                return qApp->translate("kanji", "Heisig # (Fr)");
            case 24:
                return qApp->translate("kanji", "Heisig # (6th)");
            case 25:
                return qApp->translate("kanji", "O'Neill #");
            case 26:
                return qApp->translate("kanji", "Kodansha Dict.");
            case 27:
                return qApp->translate("kanji", "2001 Kanji");
            case 28:
                return qApp->translate("kanji", "Guide");
            case 29:
                return qApp->translate("kanji", "Tuttle Cards");
            case 30:
                return qApp->translate("kanji", "Kuten");
            case 31:
                return qApp->translate("kanji", "EUC-JP");
            case 32:
                return qApp->translate("kanji", "Shift-JIS");
            default:
                return QString();
            }
        }

        switch (index)
        {
        case 0:
            return qApp->translate("kanji", "Number of strokes");
        case 1:
            return qApp->translate("kanji", "JIS X0208");
        case 2:
            return qApp->translate("kanji", "Unicode");
        case 3:
            return qApp->translate("kanji", "Jouyou grade");
        case 4:
            return qApp->translate("kanji", "JLPT level");
        case 5:
            return qApp->translate("kanji", "Frequency");
        case 6:
            return qApp->translate("kanji", "SKIP code");
        case 7:
            return qApp->translate("kanji", "Kanji and Kana");
        case 8:
            return qApp->translate("kanji", "Modern Reader's Japanese-English Character Dictionary (Nelson)");
        case 9:
            return qApp->translate("kanji", "New Japanese-English Character Dictionary (Halpern)");
        case 10:
            return qApp->translate("kanji", "Remembering The Kanji (Heisig)");
        case 11:
            return qApp->translate("kanji", "A New Dictionary of Kanji Usage (Gakken)");
        case 12:
            return qApp->translate("kanji", "A Guide To Remembering Japanese Characters (Henshall)");
        case 13:
            return qApp->translate("kanji", "Spahn & Hadamitsky dictionary");
        case 14:
            return qApp->translate("kanji", "The New Nelson Japanese-English Character Dictionary");
        case 15:
            return qApp->translate("kanji", "Japanese For Busy People");
        case 16:
            return qApp->translate("kanji", "The Kanji Way to Japanese Language Power (Crowley)");
        case 17:
            return qApp->translate("kanji", "Japanese Kanji Flashcards");
        case 18:
            return qApp->translate("kanji", "Kodansha Compact Kanji Guide");
        case 19:
            return qApp->translate("kanji", "A Guide To Reading and Writing Japanese 3rd edition");
        case 20:
            return qApp->translate("kanji", "Kanji in Context");
        case 21:
            return qApp->translate("kanji", "Kanji Learners Dictionary (1999 edition)");
        case 22:
            return qApp->translate("kanji", "Kanji Learners Dictionary (2013)");
        case 23:
            return qApp->translate("kanji", "Remembering The Kanji (Heisig) - French version");
        case 24:
            return qApp->translate("kanji", "Remembering The Kanji, 6th Edition (Heisig)");
        case 25:
            return qApp->translate("kanji", "Essential Kanji (O'Neill)");
        case 26:
            return qApp->translate("kanji", "Kodansha Kanji Dictionary (2013)");
        case 27:
            return qApp->translate("kanji", "2001 Kanji");
        case 28:
            return qApp->translate("kanji", "A Guide To Reading and Writing Japanese");
        case 29:
            return qApp->translate("kanji", "Tuttle Kanji Cards");
        case 30:
            return qApp->translate("kanji", "Kuten code");
        case 31:
            return qApp->translate("kanji", "EUC-JP");
        case 32:
            return qApp->translate("kanji", "Shift-JIS");
        default:
            return QString();
        }
    }

    QString kanjiReference(KanjiEntry *k, int index)
    {
        switch (index)
        {
        case 0:
            return QString::number(k->strokes);
        case 1:
            return "0x" + QString::number(k->jis, 16);
        case 2:
            return "0x" + QString::number(k->ch.unicode(), 16);
        case 3:
            return k->jouyou == 0 ? QString("-") : QString::number(k->jouyou);
        case 4:
            return k->jlpt == 0 ? QString("-") : QString::number(k->jlpt);
        case 5:
            return k->frequency == 0 ? QString("-") : QString::number(k->frequency);
        case 6:
            return QString("%1-%2-%3").arg(k->skips[0]).arg(k->skips[1]).arg(k->skips[2]);
        case 7:
            return k->knk == 0 ? QString("-") : QString::number(k->knk);
        case 8:
            return k->nelson == 0 ? QString("-") : QString::number(k->nelson);
        case 9:
            return k->halpern == 0 ? QString("-") : QString::number(k->halpern);
        case 10:
            return k->heisig == 0 ? QString("-") : QString::number(k->heisig);
        case 11:
            return k->gakken == 0 ? QString("-") : QString::number(k->gakken);
        case 12:
            return k->henshall == 0 ? QString("-") : QString::number(k->henshall);
        case 13:
            if (memchr(k->snh, 0, 8) != nullptr)
                return QString::fromLatin1(k->snh);
            return QString::fromLatin1(k->snh, 8);
        case 14:
            return k->newnelson == 0 ? QString("-") : QString::number(k->newnelson);
        case 15:
            if (memchr(k->busy, 0, 4) != nullptr)
                return k->busy[0] == 0 ? QString("-") : QString::fromLatin1(k->busy);
            return QString::fromLatin1(k->busy, 4);
        case 16:
            return k->crowley == 0 ? QString("-") : QString::number(k->crowley);
        case 17:
            return k->flashc == 0 ? QString("-") : QString::number(k->flashc);
        case 18:
            return k->kguide == 0 ? QString("-") : QString::number(k->kguide);
        case 19:
            return k->henshallg == 0 ? QString("-") : QString::number(k->henshallg);
        case 20:
            return k->context == 0 ? QString("-") : QString::number(k->context);
        case 21:
            return k->halpernk == 0 ? QString("-") : QString::number(k->halpernk);
        case 22:
            return k->halpernl == 0 ? QString("-") : QString::number(k->halpernl);
        case 23:
            return k->heisigf == 0 ? QString("-") : QString::number(k->heisigf);
        case 24:
            return k->heisign == 0 ? QString("-") : QString::number(k->heisign);
        case 25:
            return k->oneil == 0 ? QString("-") : QString::number(k->oneil);
        case 26:
            return k->halpernn == 0 ? QString("-") : QString::number(k->halpernn);
        case 27:
            return k->deroo == 0 ? QString("-") : QString::number(k->deroo);
        case 28:
            return k->sakade == 0 ? QString("-") : QString::number(k->sakade);
        case 29:
            return k->tuttle == 0 ? QString("-") : QString::number(k->tuttle);
        case 30:
            return JIStoKuten(k->jis);
        case 31:
            return "0x" + QString::number(JIStoEUC(k->jis), 16);
        case 32:
            return "0x" + QString::number(JIStoShiftJIS(k->jis), 16);
        default:
            return QString();
        }
    }
}


//-------------------------------------------------------------


KanjiRadicalList::KanjiRadicalList() : base(/*false,*/)
{
}

KanjiRadicalList::~KanjiRadicalList()
{
}

void KanjiRadicalList::clear()
{
    base::clear();
    list.clear();
}

void KanjiRadicalList::addRadical(QChar ch, std::vector<ushort> &&kanji, uchar strokes, uchar radical, QCharStringList &&names)
{
    KanjiRadical *rad = new KanjiRadical;
    rad->ch = ch;
    std::swap(rad->kanji, kanji);
    rad->strokes = strokes;
    rad->radical = radical;
    rad->names = std::move(names);
    rad->romajinames.reserve(rad->names.size());
    for (int ix = 0, siz = tosigned(rad->names.size()); ix != siz; ++ix)
        rad->romajinames.add(romanize(rad->names[ix]).constData());
    list.push_back(rad);
}

KanjiRadicalList::size_type KanjiRadicalList::size() const
{
    return list.size();
}

bool KanjiRadicalList::empty()
{
    return list.empty();
}

bool KanjiRadicalList::isKana() const
{
    return true;
}

bool KanjiRadicalList::isReversed() const
{
    return false;
}

KanjiRadical* KanjiRadicalList::operator[](int ix)
{
    return list[ix];
}

KanjiRadical* KanjiRadicalList::items(int ix)
{
    return list[ix];
}

int KanjiRadicalList::find(QChar radchar) const
{
    int pos = std::find_if(list.begin(), list.end(), [radchar](const KanjiRadical *rad) {
        return rad->ch == radchar;
    }) - list.begin();
    if (pos == tosigned(list.size()))
        return -1;
    return pos;
}

//void KanjiRadicalList::reset()
//{
//    groups.clear();
//}

void KanjiRadicalList::doGetWord(int index, QStringList &texts) const
{
    uchar name = uchar((index & 0xff000000) >> 24);
    index = index & 0xffffff;

    const KanjiRadical *rad = list[index];

    texts << rad->romajinames[name].toQString();
            //romanize(rad->names[name].data());


    //int len = qcharlen(rad->names);
    //QChar *cc = nullptr;
    //int pos = 0;
    //try
    //{
    //    cc = new QChar[len + 1];
    //    memcpy(cc, rad->names, sizeof(QChar) * len);
    //    cc[len] = 0;

    //    int j = 0;
    //    for (int ix = 0; ix <= len; ++ix)
    //    {
    //        if (ix == len || cc[ix] == QChar(' '))
    //        {
    //            cc[ix] = 0;
    //            if (len - pos)
    //            {
    //                if (j == name)
    //                {
    //                    delete[] cc;
    //                    return romanize(cc + pos);
    //                }
    //                j++;
    //            }
    //            if (ix < len)
    //                cc[ix] = QChar(' ');
    //            pos = ix + 1;
    //        }
    //    }
    //}
    //catch (...)
    //{
    //    ;
    //}
    //delete[] cc;

    throw "We shouldn't be here...";
}

void KanjiRadicalList::load(QDataStream &stream)
{
    clear();

    quint16 cnt;
    stream >> cnt;

    list.reserve(cnt);
    int lsiz = 0;

    while (lsiz != cnt)
    {
        KanjiRadical *r = new KanjiRadical;
        quint16 u16;

        stream >> u16;
        r->ch = QChar(u16);

        stream >> make_zvec<quint16, quint16>(r->kanji);
        //r->kanji.resize(u16);
        for (int ix = 0, siz = tosigned(r->kanji.size()); ix != siz; ++ix)
            ZKanji::kanjis[r->kanji[ix]]->rads.push_back(lsiz);

        quint8 u8;
        stream >> u8;
        r->strokes = u8;
        stream >> u8;
        r->radical = u8;
        stream >> r->names;
        r->romajinames.reserve(r->names.size());
        for (int ix = 0, siz = tosigned(r->names.size()); ix != siz; ++ix)
            r->romajinames.add(romanize(r->names[ix]).constData());

        list.push_back(r);
        ++lsiz;
    }
}

void KanjiRadicalList::save(QDataStream &stream) const
{
    stream << (quint16)list.size();

    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        const KanjiRadical *r = list[ix];
        stream << (quint16)r->ch.unicode();
        stream << make_zvec<quint16, quint16>(r->kanji);
        //stream << (quint16)r->kanji.size();
        //for (int iy = 0; iy != r->kanji.size(); ++iy)
        //    stream << (quint16)r->kanji[iy];
        stream << (quint8)r->strokes;
        stream << (quint8)r->radical;
        stream << r->names;
    }
}


//-------------------------------------------------------------


KanjiEntry::KanjiEntry()
{
    index = 0; // Index of kanji in "kanjis" list.

    ch = 0; // The kanji itself.
    jis = 0; // jis lookup.
    rad = 0; // B - radical of the kanji.
    crad = 0; // C - classical radical of the kanji.
    jouyou = 0; // G - jouyou grade.
    jlpt = 0; // Assumed N kanji level of the JLPT from 2010.
    strokes = 0; // S - stroke number for the kanji.
    frequency = 0; // F - frequency number from 1 to 2501, lower the better.
    element = -1; // Element number in the kanji stroke order database.
    knk = 0; // IN - index in "Kanji and kana" for lookup.
    nelson = 0; // N - Modern Reader's Japanese-English Character Dictionary.
    halpern = 0; // H - New Japanese-English Character Dictionary.
    heisig = 0; // L - Remembering The Kanji.
    gakken = 0; // K - Gakken Kanji Dictionary - A New Dictionary of Kanji Usage.
    henshall = 0; // E - A Guide To Remembering Japanese Characters.
    newnelson = 0; // V - The New Nelson Japanese-English Character Dictionary
    crowley = 0; // DC -The Kanji Way to Japanese Language Power
    flashc = 0; // DF - Japanese Kanji Flashcards
    kguide = 0; // DG - Kodansha Compact Kanji Guide
    henshallg = 0;  // DH - 3rd edition of "A Guide To Reading and Writing Japanese"
    context = 0; // DJ - Kanji in Context
    halpernk = 0; // DK - Kanji Learners Dictionary (1999 edition)
    halpernl = 0; // DL - Kanji Learners Dictionary (2013 edition)
    heisigf = 0; // DM - French version of Remembering The Kanji
    heisign = 0; // DN - Remembering The Kanji, 6th Edition
    oneil = 0; // DO - Essential Kanji
    halpernn = 0; // DP - Kodansha Kanji Dictionary (2013)
    deroo = 0; // DR - 2001 Kanji
    sakade = 0; // DS - A Guide To Reading and Writing Japanese
    tuttle = 0; // DT - Tuttle Kanji Cards

    word_freq = 0; // Sum of the frequency of all words this kanji is in.


    memset(skips, 0, sizeof(uchar) * 3);
    memset(snh, 0, sizeof(char) * 8);
    memset(busy, 0, sizeof(char) * 4);
}


//-------------------------------------------------------------


ushort JIStoShiftJIS(ushort w)
{
    ushort res = (((w + 0x100) >> 1) & 0xff00) + 0x7000;
    if (res > 0x9f00)
        res += 0x4000;
    if (res & 0x100)
        res |= (w & 0xff) + 0x1f + ((w & 0xff) >= 0x60 ? 1 : 0);
    else
        res |= (w & 0xff) + 0x7e;
    return res;
}

ushort JIStoEUC(ushort w)
{
    return (w & 0x7f7f) + 0x8080;
}

QString JIStoKuten(ushort w)
{
    return QString::number(((w & 0xff00) >> 8) - 0x20) + QStringLiteral("-") + QString::number(((w & 0xff) - 0x20));
}


//-------------------------------------------------------------

