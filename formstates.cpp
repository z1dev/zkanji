/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMainWindow>
#include <QSplitter>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QStringBuilder>
#include <QDesktopWidget>
#include <QScreen>

#include "formstates.h"
#include "settings.h"
#include "words.h"
#include "zexamplestrip.h"
#include "kanjistrokes.h"
#include "kanji.h"
#include "zstudylistmodel.h"
#include "kanapracticesettingsform.h"
#include "wordstudylistform.h"
#include "zui.h"
#include "globalui.h"
#include "generalsettings.h"
#include "dialogs.h"

#include "checked_cast.h"

//-------------------------------------------------------------


namespace FormStates
{
    const QString True = QStringLiteral("1");
    const QString False = QStringLiteral("0");

    // Saved state of dialog windows with splitter.
    std::map<QString, SplitterFormData> splitters;

    // Saved size of dialog windows.
    std::map<QString, QSize> sizes;

    // Saved size and maximized state of dialog windows.
    std::map<QString, FormSizeStateData> maxsizes;

    // Saved state of the CollectWordsForm window.
    CollectFormData collectform;
    KanjiInfoData kanjiinfo;
    WordStudyListFormData wordstudylist;
    RecognizerFormData recognizer;

    bool emptyState(const SplitterFormData &data)
    {
        return /*data.pos.isNull() && data.screenpos.isNull() &&*/ !data.siz.isValid();
    }

    bool emptyState(const CollectFormData &data)
    {
        return data.kanjinum == "3" && data.kanalen == 8 && data.minfreq == 1000 && data.strict && data.limit && !data.siz.isValid() && emptyState(data.dict);
    }

    bool emptyState(const DictionaryWidgetData &data)
    {
        return data.mode == (SearchMode)1 && !data.multi && !data.filter && data.showex && data.exmode == (ExampleDisplay)0 &&
                !data.frombefore && data.fromafter && !data.fromstrict && !data.frominfl && data.toafter && !data.tostrict &&
                data.conditionex == (Inclusion)0 && data.conditiongroup == (Inclusion)0 && data.conditions.empty() &&
                (!data.savecolumndata || (data.sortcolumn == -1 && emptyState(data.tabledata)));
    }

    bool emptyState(const ListStateData &data)
    {
        return data.columns.empty();
    }

    bool emptyState(const KanjiInfoData &data)
    {
        return !data.siz.isValid() && data.grid == true && data.sod == true && data.words == false && data.similar == false &&
            data.parts == false && data.partof == false && data.loop == false && data.shadow == false && data.numbers == false &&
            data.dir == false && data.speed == 3 && data.toph == -1 && data.dicth == -1 && data.refdict == false && emptyState(data.dict);
    }

    bool emptyState(const WordStudyListFormDataItems &data)
    {
        return data.showkanji == true && data.showkana == true && data.showdef == true && data.mode == (DeckItemViewModes)0 &&
            emptyState(data.dict) && data.queuesort.column == -1 && data.studysort.column == -1 && data.testedsort.column == -1 &&
            data.queuesizes.empty() && data.studysizes.empty() && data.testedsizes.empty() && data.queuecols.empty() && data.studycols.empty() && data.testedcols.empty();
    }

    bool emptyState(const WordStudyListFormDataStats &data)
    {
        return data.page == (DeckStatPages)0 && data.itemsinterval == (DeckStatIntervals)0 && data.forecastinterval == (DeckStatIntervals)1;
    }

    bool emptyState(const WordStudyListFormData &data)
    {
        return !data.siz.isValid() && emptyState(data.items) && emptyState(data.stats);
    }

    bool emptyState(const PopupDictData &data)
    {
        return data.floating == false && !data.floatrect.isValid() && !data.normalsize.isValid() && emptyState(data.dict);
    }

    bool emptyState(const RecognizerFormData &data)
    {
        return data.showgrid == true && data.allresults == true && !data.rect.isValid();
    }

    void saveXMLSettings(const SplitterFormData &data, QXmlStreamWriter &writer)
    {
        //writer.writeAttribute("x", QString::number(data.pos.x()));
        //writer.writeAttribute("y", QString::number(data.pos.y()));
        //writer.writeAttribute("screenx", QString::number(data.screenpos.x()));
        //writer.writeAttribute("screeny", QString::number(data.screenpos.y()));
        writer.writeAttribute("width", QString::number(data.siz.width()));
        writer.writeAttribute("height", QString::number(data.siz.height()));

        writer.writeAttribute("size1", QString::number(data.wsizes[0]));
        writer.writeAttribute("size2", QString::number(data.wsizes[1]));
    }

    void loadXMLSettings(SplitterFormData &data, QXmlStreamReader &reader)
    {
        bool ok = false;
        //int x;
        //int y;
        //int sx;
        //int sy;
        int w;
        int h;
        //x = reader.attributes().value("x").toInt(&ok);
        //if (ok)
        //    y = reader.attributes().value("y").toInt(&ok);
        //if (ok)
        //    sx = reader.attributes().value("screenx").toInt(&ok);
        //if (ok)
        //    sy = reader.attributes().value("screeny").toInt(&ok);
        //if (ok)

        //if (Settings::general.savewinpos)
        w = reader.attributes().value("width").toInt(&ok);

        if (ok)
        {
            h = reader.attributes().value("height").toInt(&ok);

            //data.pos = QPoint(x, y);
            //data.screenpos = QPoint(sx, sy);

            data.siz = QSize(w, h);
        }
        else
        {
            //data.pos = QPoint();
            //data.screenpos = QPoint();
            data.siz = QSize();
        }

        int s = reader.attributes().value("size1").toInt(&ok);
        if (ok)
            data.wsizes[0] = s;
        s = reader.attributes().value("size2").toInt(&ok);
        if (ok)
            data.wsizes[1] = s;

        reader.skipCurrentElement();
    }

    void saveXMLSettings(const CollectFormData &data, QXmlStreamWriter &writer)
    {
        if (emptyState(data))
            return;

        if (data.kanjinum != "3")
            writer.writeAttribute("kanjistring", data.kanjinum);
        if (data.kanalen != 8)
            writer.writeAttribute("kanalength", QString::number(data.kanalen));
        if (data.minfreq != 1000)
            writer.writeAttribute("minfrequency", QString::number(data.minfreq));

        writer.writeAttribute("strict", data.strict ? True : False);
        writer.writeAttribute("limited", data.limit ? True : False);

        if (!data.siz.isEmpty())
        {
            writer.writeAttribute("width", QString::number(data.siz.width()));
            writer.writeAttribute("height", QString::number(data.siz.height()));
        }

        writer.writeStartElement("Dictionary");
        saveXMLSettings(data.dict, writer);
        writer.writeEndElement(); /* Dictionary */
    }

    void loadXMLSettings(CollectFormData &data, QXmlStreamReader &reader)
    {
        if (reader.attributes().hasAttribute("kanjistring"))
            data.kanjinum = reader.attributes().value("kanjistring").toString();
        else
            data.kanjinum = "3";

        bool ok = false;
        int val = 0;
        if (reader.attributes().hasAttribute("kanalength"))
        {
            val = reader.attributes().value("kanalength").toInt(&ok);
            if (val < 1 || val > 99)
                ok = false;
        }
        data.kanalen = ok ? val : 8;

        ok = false;
        if (reader.attributes().hasAttribute("minfrequency"))
        {
            val = reader.attributes().value("minfrequency").toInt(&ok);
            if (val < 0 || val > 9999)
                ok = false;
        }
        data.minfreq = ok ? val : 1000;

        data.strict = reader.attributes().value("strict") == True;
        data.limit = reader.attributes().value("limited") == True;

        ok = false;
        if (reader.attributes().hasAttribute("width"))
        {
            val = reader.attributes().value("width").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.siz.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("height"))
        {
            val = reader.attributes().value("height").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.siz.setHeight(val);
        }
        if (!ok)
            data.siz = QSize();

        while (reader.readNextStartElement())
        {
            if (reader.name() == "Dictionary")
                loadXMLSettings(data.dict, reader);
            else
                reader.skipCurrentElement();
        }
    }

    void saveXMLSettings(const DictionaryWidgetData &data, QXmlStreamWriter &writer)
    {
        writer.writeAttribute("mode", data.mode == SearchMode::Browse ? "browse" : data.mode == SearchMode::Japanese ? "from" : "to");
        writer.writeAttribute("multiline", data.multi ? True : False);
        writer.writeAttribute("filtering", data.filter ? True : False);

        if (!data.showex || data.exmode != (ExampleDisplay)0)
        {
            writer.writeEmptyElement("Examples");
            writer.writeAttribute("show", data.showex ? True : False);
            switch (data.exmode)
            {
            case ExampleDisplay::Both:
                writer.writeAttribute("mode", "both");
                break;
            case ExampleDisplay::Japanese:
                writer.writeAttribute("mode", "from");
                break;
            case ExampleDisplay::Translated:
                writer.writeAttribute("mode", "to");
                break;
            }
        }

        writer.writeEmptyElement("From");

        writer.writeAttribute("before", data.frombefore ? True : False);
        writer.writeAttribute("after", data.fromafter ? True : False);
        writer.writeAttribute("strict", data.fromstrict ? True : False);
        if (data.frominfl)
            writer.writeAttribute("deinflect", True);

        writer.writeEmptyElement("To");
        writer.writeAttribute("after", data.toafter ? True : False);
        writer.writeAttribute("strict", data.tostrict ? True : False);

        if (data.conditionex != (Inclusion)0 || data.conditiongroup != (Inclusion)0 || !data.conditions.empty())
        {
            writer.writeStartElement("Filters");
            const QString Include = QStringLiteral("include");
            const QString Exclude = QStringLiteral("exclude");
            const QString Ignore = QStringLiteral("0");

            if (data.conditionex != Inclusion::Ignore)
                writer.writeAttribute("inexample", data.conditionex == Inclusion::Include ? Include : data.conditionex == Inclusion::Exclude ? Exclude : Ignore);
            if (data.conditiongroup != Inclusion::Ignore)
                writer.writeAttribute("ingroup", data.conditiongroup == Inclusion::Include ? Include : data.conditiongroup == Inclusion::Exclude ? Exclude : Ignore);

            for (int ix = 0, siz = tosigned(data.conditions.size()); ix != siz; ++ix)
            {
                if (data.conditions[ix].second == Inclusion::Ignore)
                    continue;
                if (data.conditions[ix].second == Inclusion::Include)
                    writer.writeEmptyElement("Include");
                else
                    writer.writeEmptyElement("Exclude");
                writer.writeAttribute("name", data.conditions[ix].first);
            }
            writer.writeEndElement(); /* Filters */
        }

        if (!data.savecolumndata)
            return;

        if (data.sortcolumn != -1)
        {
            writer.writeEmptyElement("Sorting");
            writer.writeAttribute("column", QString::number(data.sortcolumn));
            writer.writeAttribute("order", data.sortorder == Qt::AscendingOrder ? QStringLiteral("ascending") : QStringLiteral("descending"));
        }

        if (!emptyState(data.tabledata))
        {
            writer.writeStartElement("Table");
            saveXMLSettings(data.tabledata, writer);
            writer.writeEndElement(); /* Table */
        }
    }

    void loadXMLSettings(DictionaryWidgetData &data, QXmlStreamReader &reader)
    {
        QStringRef modestr = reader.attributes().value("mode");
        if (modestr == "browse")
            data.mode = SearchMode::Browse;
        else if (modestr == "from")
            data.mode = SearchMode::Japanese;
        else if (modestr == "to")
            data.mode = SearchMode::Definition;

        data.multi = reader.attributes().value("multiline") == True;

        //bool usefilters = false;
        data.filter = reader.attributes().value("filtering") == True;

        while (reader.readNextStartElement())
        {
            if (reader.name() == "Examples")
            {
                data.showex = reader.attributes().value("show") == True;
                QStringRef ref = reader.attributes().value("mode");
                if (ref == "both")
                    data.exmode = ExampleDisplay::Both;
                else if (ref == "to")
                    data.exmode = ExampleDisplay::Translated;
                else
                    data.exmode = ExampleDisplay::Japanese;

                reader.skipCurrentElement();
            }
            else if (reader.name() == "From")
            {
                data.frombefore = reader.attributes().value("before") == True;
                data.fromafter = reader.attributes().value("after") == True;
                data.fromstrict = reader.attributes().value("strict") == True;
                data.frominfl = reader.attributes().value("deinflect") == True;
                // Leaving "From"
                reader.skipCurrentElement();
            }
            else if (reader.name() == "To")
            {
                data.toafter = reader.attributes().value("after") == True;
                data.tostrict = reader.attributes().value("strict") == True;
                // Leaving "To"
                reader.skipCurrentElement();
            }
            else if (reader.name() == "Filters")
            {
                const QString Include = QStringLiteral("include");
                const QString Exclude = QStringLiteral("exclude");
                const QString Ignore = QStringLiteral("0");

                QStringRef ref = reader.attributes().value("inexample");
                if (ref == Include)
                    data.conditionex = Inclusion::Include;
                else if (ref == Exclude)
                    data.conditionex = Inclusion::Exclude;
                else
                    data.conditionex = Inclusion::Ignore;

                ref = reader.attributes().value("ingroup");
                if (ref == Include)
                    data.conditiongroup = Inclusion::Include;
                else if (ref == Exclude)
                    data.conditiongroup = Inclusion::Exclude;
                else
                    data.conditiongroup = Inclusion::Ignore;

                data.conditions.clear();
                while (reader.readNextStartElement())
                {
                    if (reader.name() == "Include" || reader.name() == "Exclude")
                    {
                        QStringRef namestr = reader.attributes().value("name");
                        int ix = ZKanji::wordfilters().itemIndex(namestr);
                        if (ix != -1)
                            data.conditions.push_back(std::make_pair(namestr.toString(), reader.name() == "Include" ? Inclusion::Include : Inclusion::Exclude));
                        // Leaving "Include" or "Exclude"
                        reader.skipCurrentElement();
                    }
                    else
                        reader.skipCurrentElement();
                }
            }
            else if (reader.name() == "Sorting")
            {
                bool ok;
                int val = reader.attributes().value("column").toInt(&ok);
                if (ok)
                    data.sortcolumn = val;
                data.sortorder = reader.attributes().value("order") == QStringLiteral("descending") ? Qt::DescendingOrder : Qt::AscendingOrder;
            }
            else if (reader.name() == "Table")
            {
                loadXMLSettings(data.tabledata, reader);
            }
            else
                reader.skipCurrentElement();
        }
    }

    void saveXMLSettings(const ListStateData &data, QXmlStreamWriter &writer)
    {
        QString colstr;
        QString sizstr;

        for (auto &p : data.columns)
        {
            if (colstr.isEmpty())
            {
                colstr = QString::number(p.first);
                sizstr = QString::number(p.second);
            }
            else
            {
                colstr = colstr % "," % QString::number(p.first);
                sizstr = sizstr % "," % QString::number(p.second);
            }
        }

        writer.writeAttribute("columns", colstr);
        writer.writeAttribute("widths", sizstr);
    }

    void loadXMLSettings(ListStateData &data, QXmlStreamReader &reader)
    {
        data.columns.clear();

        QVector<QStringRef> cols = reader.attributes().value("columns").split(",");
        QVector<QStringRef> wids = reader.attributes().value("widths").split(",");

        if (cols.isEmpty() || cols.size() != wids.size())
            return;

        bool ok = false;
        for (int ix = 0, siz = cols.size(); ix != siz; ++ix)
        {
            int col = cols.at(ix).trimmed().toInt(&ok);
            int wid = ok ? wids.at(ix).trimmed().toInt(&ok) : -1;

            if (ok && col >= 0 && col <= 99 && wid >= 0 && wid <= 999999)
                data.columns.push_back(std::make_pair(col, wid));
        }
        reader.skipCurrentElement();
    }

    void saveXMLSettings(const KanjiInfoData &data, QXmlStreamWriter &writer)
    {
        if (emptyState(data))
            return;

        if (data.siz.isValid())
        {
            writer.writeAttribute("width", QString::number(data.siz.width()));
            writer.writeAttribute("height", QString::number(data.siz.height()));
        }
        if (!data.pos.isNull())
        {
            writer.writeAttribute("x", QString::number(data.pos.x()));
            writer.writeAttribute("y", QString::number(data.pos.y()));
        }
        if (!data.screen.isEmpty())
        {
            writer.writeAttribute("screenx", QString::number(data.screen.left()));
            writer.writeAttribute("screeny", QString::number(data.screen.top()));
            writer.writeAttribute("screenw", QString::number(data.screen.width()));
            writer.writeAttribute("screenh", QString::number(data.screen.height()));
        }

        writer.writeAttribute("grid", data.grid ? True : False);
        writer.writeAttribute("sod", data.sod ? True : False);
        writer.writeAttribute("words", data.words ? True : False);
        writer.writeAttribute("similar", data.similar ? True : False);
        writer.writeAttribute("parts", data.parts ? True : False);
        writer.writeAttribute("partof", data.partof ? True : False);
        writer.writeAttribute("loop", data.loop ? True : False);
        writer.writeAttribute("shadow", data.shadow ? True : False);
        writer.writeAttribute("numbers", data.numbers ? True : False);
        writer.writeAttribute("direction", data.dir ? True : False);

        if (data.speed != 3)
            writer.writeAttribute("speed", QString::number(data.speed));

        if (data.toph != -1)
            writer.writeAttribute("topheight", QString::number(data.toph));
        if (data.dicth != -1)
            writer.writeAttribute("bottomheight", QString::number(data.dicth));

        writer.writeAttribute("kanjiexamples", data.refdict ? True : False);

        writer.writeStartElement("Dictionary");
        saveXMLSettings(data.dict, writer);
        writer.writeEndElement(); /* Dictionary */
    }

    void loadXMLSettings(KanjiInfoData &data, QXmlStreamReader &reader)
    {
        bool ok = false;
        int val;
        if (reader.attributes().hasAttribute("width"))
        {
            val = reader.attributes().value("width").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.siz.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("height"))
        {
            val = reader.attributes().value("height").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.siz.setHeight(val);
        }
        if (!ok)
            data.siz = QSize();

        ok = false;
        if (reader.attributes().hasAttribute("x"))
        {
            val = reader.attributes().value("x").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.pos.setX(val);
        }
        if (ok && reader.attributes().hasAttribute("y"))
        {
            val = reader.attributes().value("y").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.pos.setY(val);
        }
        if (!ok)
            data.pos = QPoint();

        if (reader.attributes().hasAttribute("screenx"))
        {
            val = reader.attributes().value("screenx").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.screen.setLeft(val);
        }
        if (ok && reader.attributes().hasAttribute("screeny"))
        {
            val = reader.attributes().value("screeny").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.screen.setTop(val);
        }
        if (reader.attributes().hasAttribute("screenw"))
        {
            val = reader.attributes().value("screenw").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.screen.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("screenh"))
        {
            val = reader.attributes().value("screenh").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.screen.setHeight(val);
        }
        if (!ok)
            data.screen = QRect();

        data.grid = reader.attributes().value("grid") == True;
        data.sod = reader.attributes().value("sod") == True;
        data.words = reader.attributes().value("words") == True;
        data.similar = reader.attributes().value("similar") == True;
        data.parts = reader.attributes().value("parts") == True;
        data.partof = reader.attributes().value("partof") == True;
        data.shadow = reader.attributes().value("shadow") == True;
        data.loop = reader.attributes().value("loop") == True;
        data.numbers = reader.attributes().value("numbers") == True;
        data.dir = reader.attributes().value("direction") == True;

        ok = false;
        if (reader.attributes().hasAttribute("speed"))
        {
            val = reader.attributes().value("speed").toInt(&ok);
            if (val < 1 || val > 4)
                ok = false;
            if (ok)
                data.speed = val;
        }
        if (!ok)
            data.speed = 3;

        ok = false;
        if (reader.attributes().hasAttribute("topheight"))
        {
            val = reader.attributes().value("topheight").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.toph = val;
        }
        if (!ok)
            data.toph = -1;

        ok = false;
        if (reader.attributes().hasAttribute("bottomheight"))
        {
            val = reader.attributes().value("bottomheight").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.dicth = val;
        }
        if (!ok)
            data.dicth = -1;

        data.refdict = reader.attributes().value("kanjiexamples") == True;

        while (reader.readNextStartElement())
        {
            if (reader.name() == "Dictionary")
                loadXMLSettings(data.dict, reader);
            else
                reader.skipCurrentElement();
        }
    }

    void saveXMLSettings(const KanjiInfoFormData &data, QXmlStreamWriter &writer)
    {
        writer.writeAttribute("kanjiindex", QString::number(data.kindex));
        writer.writeAttribute("dictionary", data.dictname);
        if (data.locked)
            writer.writeAttribute("locked", QStringLiteral("1"));

        saveXMLSettings(data.data, writer);
    }

    void loadXMLSettings(KanjiInfoFormData &data, QXmlStreamReader &reader)
    {
        bool ok = false;
        int val;
        if (reader.attributes().hasAttribute("kanjiindex"))
        {
            val = reader.attributes().value("kanjiindex").toInt(&ok);
            if (val < -1 - tosigned(ZKanji::elements()->size()) || val >= tosigned(ZKanji::kanjis.size()))
                ok = false;
            if (ok)
                data.kindex = val;
        }
        if (!ok)
            data.kindex = 0;

        if (reader.attributes().hasAttribute("dictionary"))
            data.dictname = reader.attributes().value("dictionary").toString();
        else
            data.dictname = QString();
        data.locked = reader.attributes().value("locked") == QStringLiteral("1");

        loadXMLSettings(data.data, reader);
    }

    void saveXMLSettings(const WordStudyListFormDataItems &data, QXmlStreamWriter &writer)
    {
        if (emptyState(data))
            return;

        writer.writeAttribute("kanji", data.showkanji ? True : False);
        writer.writeAttribute("kana", data.showkana ? True : False);
        writer.writeAttribute("definition", data.showdef ? True : False);

        writer.writeAttribute("mode", data.mode == DeckItemViewModes::Queued ? "queue" : data.mode == DeckItemViewModes::Studied ? "study" : "tested");

        writer.writeStartElement("Dictionary");
        saveXMLSettings(data.dict, writer);
        writer.writeEndElement(); /* Dictionary */

        writer.writeStartElement("Sorting");
        if (data.queuesort.column != -1)
        {
            writer.writeEmptyElement("Queue");
            writer.writeAttribute("column", QString::number(data.queuesort.column));
            writer.writeAttribute("order", data.queuesort.order == Qt::AscendingOrder ? "ascending" : "descending");
        }
        if (data.studysort.column != -1)
        {
            writer.writeEmptyElement("Study");
            writer.writeAttribute("column", QString::number(data.studysort.column));
            writer.writeAttribute("order", data.studysort.order == Qt::AscendingOrder ? "ascending" : "descending");
        }
        if (data.testedsort.column != -1)
        {
            writer.writeEmptyElement("LastTest");
            writer.writeAttribute("column", QString::number(data.testedsort.column));
            writer.writeAttribute("order", data.testedsort.order == Qt::AscendingOrder ? "ascending" : "descending");
        }
        writer.writeEndElement(); /* Sorting */

        writer.writeStartElement("Columns");
        if (!data.queuesizes.empty())
        {
            writer.writeEmptyElement("Queue");
            QString str;
            for (int ix = 0, siz = tosigned(data.queuesizes.size()); ix != siz; ++ix)
                str.append(QString::number(data.queuesizes[ix]) % ",");
            writer.writeAttribute("sizes", str.left(str.size() - 1));
            str = QString();
            for (int ix = 0, siz = tosigned(data.queuecols.size()); ix != siz; ++ix)
                str.append(QString::number(data.queuecols[ix]) % ",");
            writer.writeAttribute("shown", str.left(str.size() - 1));
        }

        if (!data.studysizes.empty())
        {
            writer.writeEmptyElement("Study");
            QString str;
            for (int ix = 0, siz = tosigned(data.studysizes.size()); ix != siz; ++ix)
                str.append(QString::number(data.studysizes[ix]) % ",");
            writer.writeAttribute("sizes", str.left(str.size() - 1));
            str = QString();
            for (int ix = 0, siz = tosigned(data.studycols.size()); ix != siz; ++ix)
                str.append(QString::number(data.studycols[ix]) % ",");
            writer.writeAttribute("shown", str.left(str.size() - 1));
        }

        if (!data.testedsizes.empty())
        {
            writer.writeEmptyElement("LastTest");
            QString str;
            for (int ix = 0, siz = tosigned(data.testedsizes.size()); ix != siz; ++ix)
                str.append(QString::number(data.testedsizes[ix]) % ",");
            writer.writeAttribute("sizes", str.left(str.size() - 1));
            str = QString();
            for (int ix = 0, siz = tosigned(data.testedcols.size()); ix != siz; ++ix)
                str.append(QString::number(data.testedcols[ix]) % ",");
            writer.writeAttribute("shown", str.left(str.size() - 1));
        }

        writer.writeEndElement(); /* Columns */
    }

    void loadXMLSettings(WordStudyListFormDataItems &data, QXmlStreamReader &reader)
    {
        bool ok = false;
        int val;

        data.showkanji = reader.attributes().value("kanji") != False;
        data.showkana = reader.attributes().value("kana") != False;
        data.showdef = reader.attributes().value("definition") != False;

        QStringRef modestr = reader.attributes().value("mode");
        data.mode = modestr == "study" ? DeckItemViewModes::Studied : modestr == "tested" ? DeckItemViewModes::Tested : DeckItemViewModes::Queued;

        while (reader.readNextStartElement())
        {
            if (reader.name() == "Dictionary")
                loadXMLSettings(data.dict, reader);
            else if (reader.name() == "Sorting")
            {
                while (reader.readNextStartElement())
                {
                    QStringRef elemname = reader.name();
                    if (elemname == "Queue" || elemname == "Study" || elemname == "LastTest")
                    {
                        WordStudyListFormDataItems::SortData &sort = elemname == "Queue" ? data.queuesort : elemname == "Study" ? data.studysort : data.testedsort;
                        val = reader.attributes().value("column").toInt(&ok);
                        if (ok)
                            sort.column = val;
                        else
                            sort.column = -1;
                        sort.order = reader.attributes().value("order") == "descending" ? Qt::DescendingOrder : Qt::AscendingOrder;

                        reader.skipCurrentElement();
                    }
                    else
                        reader.skipCurrentElement();
                }
            }
            else if (reader.name() == "Columns")
            {
                while (reader.readNextStartElement())
                {
                    QStringRef elemname = reader.name();
                    if (elemname == "Queue" || elemname == "Study" || elemname == "LastTest")
                    {
                        std::vector<int> &colsizes = elemname == "Queue" ? data.queuesizes : elemname == "Study" ? data.studysizes : data.testedsizes;
                        std::vector<char> &cols = elemname == "Queue" ? data.queuecols : elemname == "Study" ? data.studycols : data.testedcols;

                        QVector<QStringRef> refs = reader.attributes().value("sizes").split(',');
                        colsizes.clear();
                        colsizes.resize((elemname == "Queue" ? StudyListModel::queueColCount() : elemname == "Study" ? StudyListModel::studiedColCount() : StudyListModel::testedColCount()) - 1, -1);
                        for (int ix = 0, siz = std::min<int>(refs.size(), tosigned(colsizes.size())); ix != siz; ++ix)
                        {
                            val = refs.at(ix).toInt(&ok);
                            if (ok && val >= 0 && val < 9999)
                                colsizes[ix] = val;
                        }

                        refs = reader.attributes().value("shown").split(',');
                        cols.clear();
                        cols.resize((elemname == "Queue" ? StudyListModel::queueColCount() : elemname == "Study" ? StudyListModel::studiedColCount() : StudyListModel::testedColCount()) - 3, 1);
                        for (int ix = 0, siz = std::min<int>(refs.size(), tosigned(cols.size())); ix != siz; ++ix)
                        {
                            val = refs.at(ix).toInt(&ok);
                            if (ok && val == 0)
                                cols[ix] = 0;
                        }

                        reader.skipCurrentElement();
                    }
                    else
                        reader.skipCurrentElement();
                }
            }
            else
                reader.skipCurrentElement();
        }
    }

    void saveXMLSettings(const WordStudyListFormDataStats &data, QXmlStreamWriter &writer)
    {
        if (emptyState(data))
            return;

        writer.writeAttribute("page", data.page == DeckStatPages::Items ? "items" : data.page == DeckStatPages::Forecast ? "forecast" : data.page == DeckStatPages::Levels ? "levels" : "tests");
        writer.writeAttribute("itemsinterval", data.itemsinterval == DeckStatIntervals::All ? "all" : data.itemsinterval == DeckStatIntervals::Year ? "year" : data.itemsinterval == DeckStatIntervals::HalfYear ? "halfyear" : "month");
        writer.writeAttribute("forecastinterval", data.forecastinterval == DeckStatIntervals::All ? "all" : data.forecastinterval == DeckStatIntervals::Year ? "year" : data.forecastinterval == DeckStatIntervals::HalfYear ? "halfyear" : "month");
    }

    void loadXMLSettings(WordStudyListFormDataStats &data, QXmlStreamReader &reader)
    {
        QStringRef str = reader.attributes().value("page");
        data.page = str == "items" ? DeckStatPages::Items : str == "forecast" ? DeckStatPages::Forecast : str == "levels" ? DeckStatPages::Levels : DeckStatPages::Tests;
        str = reader.attributes().value("itemsinterval");
        data.itemsinterval = str == "year" ? DeckStatIntervals::Year : str == "halfyear" ? DeckStatIntervals::HalfYear : str == "month" ? DeckStatIntervals::Month : DeckStatIntervals::All;
        str = reader.attributes().value("forecastinterval");
        data.forecastinterval = str == "all" ? DeckStatIntervals::All : str == "year" ? DeckStatIntervals::Year : str == "halfyear" ? DeckStatIntervals::HalfYear : DeckStatIntervals::Month;

        reader.skipCurrentElement();
    }

    void saveXMLSettings(const WordStudyListFormData &data, QXmlStreamWriter &writer)
    {
        if (emptyState(data))
            return;

        if (!data.siz.isEmpty())
        {
            writer.writeAttribute("width", QString::number(data.siz.width()));
            writer.writeAttribute("height", QString::number(data.siz.height()));
        }

        writer.writeStartElement("Items");
        saveXMLSettings(data.items, writer);
        writer.writeEndElement(); /* Items */

        writer.writeStartElement("Stats");
        saveXMLSettings(data.stats, writer);
        writer.writeEndElement(); /* Stats */
    }

    void loadXMLSettings(WordStudyListFormData &data, QXmlStreamReader &reader)
    {
        bool ok = false;
        int val;

        if (reader.attributes().hasAttribute("width"))
        {
            val = reader.attributes().value("width").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.siz.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("height"))
        {
            val = reader.attributes().value("height").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.siz.setHeight(val);
        }
        if (!ok)
            data.siz = QSize();

        while (reader.readNextStartElement())
        {
            if (reader.name() == "Items")
                loadXMLSettings(data.items, reader);
            else if (reader.name() == "Stats")
                loadXMLSettings(data.stats, reader);
            else
                reader.skipCurrentElement();
        }
    }

    void saveXMLSettings(const PopupDictData &data, QXmlStreamWriter &writer)
    {
        writer.writeAttribute("floating", data.floating ? True : False);

        if (!data.floatrect.isEmpty())
        {
            writer.writeAttribute("floatleft", QString::number(data.floatrect.left()));
            writer.writeAttribute("floattop", QString::number(data.floatrect.top()));
            writer.writeAttribute("floatwidth", QString::number(data.floatrect.width()));
            writer.writeAttribute("floatheight", QString::number(data.floatrect.height()));
        }

        if (!data.normalsize.isEmpty())
        {
            writer.writeAttribute("normalwidth", QString::number(data.normalsize.width()));
            writer.writeAttribute("normalheight", QString::number(data.normalsize.height()));
        }

        writer.writeStartElement("Dictionary");
        saveXMLSettings(data.dict, writer);
        writer.writeEndElement(); /* Dictionary */
    }

    void loadXMLSettings(PopupDictData &data, QXmlStreamReader &reader)
    {
        data.floating = !reader.attributes().hasAttribute("floating") || reader.attributes().value("floating").toString() != False;

        bool ok = false;
        int val;

        data.floatrect = QRect(0, 0, 0, 0);
        if (reader.attributes().hasAttribute("floatleft"))
        {
            val = reader.attributes().value("floatleft").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.floatrect.setLeft(val);
        }
        if (ok && reader.attributes().hasAttribute("floattop"))
        {
            val = reader.attributes().value("floattop").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.floatrect.setTop(val);
        }
        if (reader.attributes().hasAttribute("floatwidth"))
        {
            val = reader.attributes().value("floatwidth").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.floatrect.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("floatheight"))
        {
            val = reader.attributes().value("floatheight").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.floatrect.setHeight(val);
        }
        if (!ok)
            data.floatrect = QRect();

        if (reader.attributes().hasAttribute("normalwidth"))
        {
            val = reader.attributes().value("normalwidth").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.normalsize.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("normalheight"))
        {
            val = reader.attributes().value("normalheight").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.normalsize.setHeight(val);
        }
        if (!ok)
            data.normalsize = QSize();

        while (reader.readNextStartElement())
        {
            if (reader.name() == "Dictionary")
                loadXMLSettings(data.dict, reader);
            else
                reader.skipCurrentElement();
        }
    }

    void saveXMLSettings(const KanarPracticeData &data, QXmlStreamWriter &writer)
    {
        writer.writeStartElement("Hiragana");
        for (int ix = 0, siz = tosigned(data.hirause.size()); ix != siz; ++ix)
        {
            if (data.hirause[ix] == 1 && (ix == 0 || data.hirause[ix - 1] != 1))
            {
                for (int iy = ix + 1; iy != siz + 1; ++iy)
                {
                    if (iy == siz || data.hirause[iy] == 0)
                    {
                        writer.writeEmptyElement("On");
                        writer.writeAttribute("v", QString::number(ix));
                        writer.writeAttribute("c", QString::number(iy - ix));
                        ix = iy - 1;
                        break;
                    }
                }
            }
        }
        writer.writeEndElement(); /* Hiragana */

        writer.writeStartElement("Katakana");
        for (int ix = 0, siz = tosigned(data.katause.size()); ix != siz; ++ix)
        {
            if (data.katause[ix] == 1 && (ix == 0 || data.katause[ix - 1] != 1))
            {
                for (int iy = ix + 1; iy != siz + 1; ++iy)
                {
                    if (iy == siz || data.katause[iy] == 0)
                    {
                        writer.writeEmptyElement("On");
                        writer.writeAttribute("v", QString::number(ix));
                        writer.writeAttribute("c", QString::number(iy - ix));
                        ix = iy - 1;
                        break;
                    }
                }
            }
        }
        writer.writeEndElement(); /* Katakana */
    }

    void loadXMLSettings(KanarPracticeData &data, QXmlStreamReader &reader)
    {
        data.hirause.resize((int)KanaSounds::Count);
        data.katause.resize((int)KanaSounds::Count);

        bool ok;

        while (reader.readNextStartElement())
        {
            if (reader.name() == "Hiragana" || reader.name() == "Katakana")
            {
                std::vector<uchar> &vec = reader.name() == "Hiragana" ? data.hirause : data.katause;

                while (reader.readNextStartElement())
                {
                    if (reader.name() == "On")
                    {
                        int c = 0;
                        int v = reader.attributes().value("v").toInt(&ok);
                        if (ok)
                            c = reader.attributes().value("c").toInt(&ok);
                        ok = ok && (v >= 0 && v < (int)KanaSounds::Count && c > 0 && v + c <= (int)KanaSounds::Count);
                        if (ok)
                        {
                            for (int ix = 0; ix != c; ++ix)
                                vec[ix + v] = 1;
                        }
                        reader.skipCurrentElement();
                    }
                    else
                        reader.skipCurrentElement();
                }
            }
            else
                reader.skipCurrentElement();
        }
    }

    void saveXMLSettings(const RecognizerFormData &data, QXmlStreamWriter &writer)
    {
        writer.writeAttribute("grid", data.showgrid ? True : False);
        writer.writeAttribute("allresults", data.allresults ? True : False);

        writer.writeAttribute("x", QString::number(data.rect.x()));
        writer.writeAttribute("y", QString::number(data.rect.y()));
        writer.writeAttribute("width", QString::number(data.rect.width()));
        writer.writeAttribute("height", QString::number(data.rect.height()));
        writer.writeAttribute("screenx", QString::number(data.screen.x()));
        writer.writeAttribute("screeny", QString::number(data.screen.y()));
        writer.writeAttribute("screenw", QString::number(data.screen.width()));
        writer.writeAttribute("screenh", QString::number(data.screen.height()));
    }

    void loadXMLSettings(RecognizerFormData &data, QXmlStreamReader &reader)
    {
        data.showgrid = !reader.attributes().hasAttribute("grid") || reader.attributes().value("grid").toString() != False;
        data.allresults = !reader.attributes().hasAttribute("allresults") || reader.attributes().value("allresults").toString() != False;

        bool ok = false;
        int val;
        if (reader.attributes().hasAttribute("x"))
        {
            val = reader.attributes().value("x").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.rect.setLeft(val);
        }
        if (ok && reader.attributes().hasAttribute("y"))
        {
            val = reader.attributes().value("y").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.rect.setTop(val);
        }
        if (ok && reader.attributes().hasAttribute("width"))
        {
            val = reader.attributes().value("width").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.rect.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("height"))
        {
            val = reader.attributes().value("height").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.rect.setHeight(val);
        }
        if (ok && reader.attributes().hasAttribute("screenx"))
        {
            val = reader.attributes().value("screenx").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.screen.setX(val);
        }
        if (ok && reader.attributes().hasAttribute("screeny"))
        {
            val = reader.attributes().value("screeny").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.screen.setY(val);
        }
        if (ok && reader.attributes().hasAttribute("screenw"))
        {
            val = reader.attributes().value("screenw").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.screen.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("screenh"))
        {
            val = reader.attributes().value("screenh").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.screen.setHeight(val);
        }

        if (!ok)
        {
            data.rect = QRect();
            data.screen = QRect();
        }

        reader.skipCurrentElement();
    }

    void saveDialogSplitterState(QString statename, const QSize &siz, int size1, int size2)
    {
        SplitterFormData &data = splitters[statename];
        data.siz = siz;
        data.wsizes[0] = size1;
        data.wsizes[1] = size2;
    }

    void restoreDialogSplitterState(QString statename, QSize &siz, int &size1, int &size2)
    {
        auto it = splitters.find(statename);
        if (it == splitters.end())
            return;

        SplitterFormData &data = it->second;
        if (!data.siz.isValid())
            return;

        siz = data.siz;
        size1 = data.wsizes[0];
        size2 = data.wsizes[1];
    }

    void saveDialogSplitterState(QString statename, QMainWindow *window, QSplitter *splitter, const int *size2)
    {
        bool nodata = splitters.find(statename) == splitters.end();
        SplitterFormData &data = splitters[statename];
        if (nodata)
            data.wsizes[1] = -1;

        //data.pos = window->pos();

        //QRect sg = qApp->desktop()->screenGeometry(window);
        //data.screenpos = sg.topLeft();

        data.siz = window->isMaximized() ? window->normalGeometry().size() : window->size();

        // Saving the size of widgets in the splitter.
        data.wsizes[0] = splitter->sizes().at(0);

        if (splitter->widget(1)->isVisibleTo(window))
        {
            // If the second widget is visible, its value is saved normally, but it is removed.
            // from the saved geometry. This makes sure the geometry is always correctly restored
            // independent of whether the second widget is visible in the splitter or not.

            data.wsizes[1] = splitter->sizes().at(1);

            if (splitter->orientation() == Qt::Horizontal)
                data.siz.setWidth(data.siz.width() - splitter->handleWidth() - data.wsizes[1]);
            else
                data.siz.setHeight(data.siz.height() - splitter->handleWidth() - data.wsizes[1]);
        }
        else  if (size2 != nullptr)
            data.wsizes[1] = *size2;
    }

    void restoreDialogSplitterState(QString statename, QMainWindow *window, QSplitter *splitter, int *size2)
    {
        auto it = splitters.find(statename);
        if (it == splitters.end())
            return;

        SplitterFormData &data = it->second;
        if (!data.siz.isValid())
        {
            //// Move window to same screen as the current visible main window.
            //int screennum = qApp->desktop()->screenNumber((QWidget*)gUI->mainForm());
            //int windownum = qApp->desktop()->screenNumber(window);
            //if (screennum != windownum)
            //{
            //    QRect sg = qApp->screens().at(screennum)->geometry();
            //    if (windownum != -1)
            //        window->move((window->geometry().topLeft() - qApp->screens().at(windownum)->geometry().topLeft()) + sg.topLeft());
            //    else
            //        window->move(window->pos() + (sg.center() - window->geometry().center()) );
            //}

            return;
        }

        //QRect r = data.geom;
        //QPoint pos = data.pos;
        //QPoint spos = data.screenpos;
        QSize siz = data.siz;

        if (splitter->widget(1)->isVisibleTo(window))
        {
            if (data.wsizes[1] == -1)
            {
                // The second widget doesn't have a saved size, nor a default.

                if (splitter->orientation() == Qt::Horizontal)
                    data.wsizes[1] = splitter->widget(1)->sizeHint().width();
                else
                    data.wsizes[1] = splitter->widget(1)->sizeHint().height();
            }

            if (splitter->orientation() == Qt::Horizontal)
                siz.setWidth(siz.width() + splitter->handleWidth() + data.wsizes[1]);
            else
                siz.setHeight(siz.height() + splitter->handleWidth() + data.wsizes[1]);

        }

        window->resize(siz);

        QRect sg;

        //int screennum = screenNumber(QRect(pos, siz));
        //if (screennum == -1)
        //{
        //    screennum = qApp->desktop()->screenNumber((QWidget*)gUI->mainForm());
        //    sg = qApp->screens().at(screennum)->geometry();
        //    pos = (data.pos - data.screenpos) + sg.topLeft();
        //}
        //else
        //    sg = qApp->screens().at(screennum)->geometry();

        // Geometry might be out of bounds. Move window within bounds.

        //if (pos.x() < sg.left())
        //    pos.setX(sg.left());
        //else if (pos.x() + siz.width() > sg.left() + sg.width())
        //    pos.setX(sg.left() + sg.width() - siz.width());
        //if (pos.y() < sg.top())
        //    pos.setY(sg.top());
        //else if (pos.y() + siz.height() > sg.top() + sg.height())
        //    pos.setY(sg.top() + sg.height() - siz.height());

        //window->move(pos);

        if (splitter->widget(1)->isVisibleTo(window))
            splitter->setSizes({ data.wsizes[0], data.wsizes[1] });
        else
        {
            if (size2 != nullptr)
                *size2 = data.wsizes[1];
            splitter->setSizes({ data.wsizes[0] });
        }
    }

    void loadXMLDialogSplitterState(QXmlStreamReader &reader)
    {
        QString statename = reader.name().toString();
        if (statename.isEmpty())
            return;

        bool nodata = splitters.find(statename) == splitters.end();
        SplitterFormData &data = splitters[statename];
        if (nodata)
            data.wsizes[1] = -1;

        loadXMLSettings(data, reader);
    }

    namespace
    {
        RestoreDialogHelperPrivate restorehelper;
    }

    void saveDialogSize(QString sizename, QMainWindow *window)
    {
        FormStates::sizes[sizename] = window->isMaximized() ? window->normalGeometry().size() : window->size();
    }

    void restoreDialogSize(QString sizename, QMainWindow *window, bool addfilter)
    {
        if (sizes.find(sizename) != sizes.end())
            window->resize(sizes[sizename]);
        if (addfilter)
            restorehelper.installFor(sizename, window);
    }

    void saveDialogMaximizedAndSize(QString sizename, QMainWindow *window)
    {
        FormSizeStateData &dat = FormStates::maxsizes[sizename];
        dat.siz = window->isMaximized() ? window->normalGeometry().size() : window->size();
        dat.maximized = window->isMaximized();
    }

    void restoreDialogMaximizedAndSize(QString sizename, QMainWindow *window, bool addfilter)
    {
        auto it = maxsizes.find(sizename);
        if (it != maxsizes.end())
        {
            window->resize(it->second.siz);
            if (it->second.maximized)
                window->setWindowState(Qt::WindowMaximized);
        }

        if (addfilter)
            restorehelper.installFor(sizename, window, true);
    }

    void saveXMLDialogSize(const QSize size, QXmlStreamWriter &writer)
    {
        writer.writeAttribute("width", QString::number(size.width()));
        writer.writeAttribute("height", QString::number(size.height()));
    }

    void loadXMLDialogSize(QXmlStreamReader &reader)
    {
        QString sizename = reader.name().toString();
        if (sizename.isEmpty())
            return;

        //bool data = FormStates::sizes.find(sizename) != FormStates::sizes.end();
        QSize &size = FormStates::sizes[sizename];

        bool ok = false;
        //if (Settings::general.savewinpos)
        //{

        int val;
        if (reader.attributes().hasAttribute("width"))
        {
            val = reader.attributes().value("width").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                size.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("height"))
        {
            val = reader.attributes().value("height").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                size.setHeight(val);
        }

        //}
        if (!ok)
            size = QSize();

        reader.skipCurrentElement();
    }

    void saveXMLDialogMaximizedAndSize(const FormSizeStateData dat, QXmlStreamWriter &writer)
    {
        writer.writeAttribute("width", QString::number(dat.siz.width()));
        writer.writeAttribute("height", QString::number(dat.siz.height()));
        writer.writeAttribute("maximized", dat.maximized ? True : False);
    }

    void loadXMLDialogMaximizedAndSize(QXmlStreamReader &reader)
    {
        QString sizename = reader.name().toString();
        if (sizename.isEmpty())
            return;

        FormSizeStateData &dat = FormStates::maxsizes[sizename];

        bool ok = false;
        int val;
        if (reader.attributes().hasAttribute("width"))
        {
            val = reader.attributes().value("width").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                dat.siz.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("height"))
        {
            val = reader.attributes().value("height").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                dat.siz.setHeight(val);
        }
        if (ok && reader.attributes().hasAttribute("maximized"))
            dat.maximized = reader.attributes().value("maximized") != False;

        if (!ok)
        {
            dat.siz = QSize();
            dat.maximized = false;
        }

        reader.skipCurrentElement();
    }


    //-------------------------------------------------------------


    void RestoreDialogHelperPrivate::installFor(QString sizename, QMainWindow *window, bool maximized)
    {
        auto it = filtered.find(window);
        if (it != filtered.end())
            return;

        window->installEventFilter(this);
        filtered[window].first = sizename;
        filtered[window].second = maximized;
        connect(window, &QObject::destroyed, this, &RestoreDialogHelperPrivate::windowDestroyed);
    }

    bool RestoreDialogHelperPrivate::eventFilter(QObject *o, QEvent *e)
    {
        if (e->type() == QEvent::Close)
        {
            const auto &dat = filtered[(QMainWindow*)o];
            QString sizename = dat.first;
            if (!dat.second)
                saveDialogSize(sizename, (QMainWindow*)o);
            else
                saveDialogMaximizedAndSize(sizename, (QMainWindow*)o);
        }
        return base::eventFilter(o, e);
    }

    void RestoreDialogHelperPrivate::windowDestroyed(QObject *o)
    {
        auto it = filtered.find((QMainWindow*)o);
        if (it != filtered.end())
            filtered.erase(it);
    }


    //-------------------------------------------------------------

}


//-------------------------------------------------------------


