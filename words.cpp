/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QStringBuilder>
#include <QDir>

#define TIMED_LOAD 0
#if TIMED_LOAD == 1
#include <QElapsedTimer>
#endif

#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include <algorithm>
#include <set>

#include "smartvector.h"
#include "zkanjimain.h"
#include "words.h"
#include "romajizer.h"
#include "groups.h"
#include "kanji.h"
#include "studydecks.h"
#include "worddeck.h"
#include "grammar.h"
#include "grammar_enums.h"
#include "ranges.h"
#include "dictionarysettings.h"
#include "globalui.h"
#include "zstrings.h"
#include "datasettings.h"

// WARNING: none of these strings should be longer than 255 bytes.

char ZKANJI_PROGRAM_VERSION[] = "v0.0.2-alpha";

static char ZKANJI_BASE_FILE_VERSION[] = "002";
static char ZKANJI_DICTIONARY_FILE_VERSION[] = "001";

static char ZKANJI_GROUP_FILE_VERSION[] = "001";

const QChar GLOSS_SEP_CHAR = QChar(0x0082);


namespace ZKanji
{
    WordCommonsTree commons;
    OriginalWordsList originals;

    const int popularFreqLimit = 2500;
    const int mediumFreqLimit = 500;

    static smartvector<Dictionary> dictionaries;

    static std::vector<quint8> dictionaryorder;

    static WordAttributeFilterList *wordfiltersinst = nullptr;
    WordAttributeFilterList& wordfilters()
    {
        if (wordfiltersinst == nullptr)
            wordfiltersinst = new WordAttributeFilterList(qApp);
        return *wordfiltersinst;
    }

    void findEntriesByKana(std::vector<WordEntriesResult> &result, const QString &kana)
    {
        int oldsiz = result.size();

        std::vector<int> l;

        // Add unsorted and unfiltered results from every dictionary first.
        for (int ix = 0; ix != ZKanji::dictionaryCount(); ++ix)
        {
            Dictionary *dict = ZKanji::dictionary(ix);

            dict->findKanaWords(l, kana, 0, true, nullptr, nullptr);
            result.reserve(result.size() + l.size());

            for (int i : l)
                result.emplace_back(i, dict);

            l.clear();
        }

        // Sort the result list, ignoring the old contents to be able to remove duplicates.
        std::sort(result.begin() + oldsiz, result.end(), [](const WordEntriesResult &a, const WordEntriesResult &b) {
            int dif = qcharcmp(a.dict->wordEntry(a.index)->kana.data(), b.dict->wordEntry(b.index)->kana.data());
            if (dif != 0)
                return dif < 0;
            return qcharcmp(a.dict->wordEntry(a.index)->kanji.data(), b.dict->wordEntry(b.index)->kanji.data()) < 0;
        });

        auto itend = std::unique(result.begin() + oldsiz, result.end(), [](const WordEntriesResult &a, const WordEntriesResult &b) {
            int dif = qcharcmp(a.dict->wordEntry(a.index)->kana.data(), b.dict->wordEntry(b.index)->kana.data());
            if (dif != 0)
                return false;
            return qcharcmp(a.dict->wordEntry(a.index)->kanji.data(), b.dict->wordEntry(b.index)->kanji.data()) == 0;
        });

        result.resize(itend - result.begin());
    }

    //void addImportDictionary(Dictionary *dict)
    //{
    //    if (!dictionaries.empty())
    //        throw "Dictionary list must be empty before import add.";
    //    dictionaries.push_back(dict);
    //}

    //void removeImportDictionary(Dictionary *dict)
    //{
    //    if (dictionaries.size() != 1 || dictionaries[0] != dict)
    //        throw "Dictionary list must only hold passed dictionary when import remove.";

    //    // Making sure the dictionary is not destroyed.
    //    dictionaries[0] = nullptr;

    //    dictionaries.clear();
    //}

    int dictionaryCount()
    {
        return dictionaries.size();
    }

    void addDictionary(Dictionary *dict)
    {
        dictionaries.push_back(dict);
        dictionaryorder.push_back(dictionaryorder.size());

        gUI->signalDictionaryAdded();
    }

    Dictionary* addDictionary()
    {
        dictionaries.push_back(new Dictionary());
        dictionaryorder.push_back(dictionaryorder.size());

        gUI->signalDictionaryAdded();
        return dictionaries.back();
    }

    //void replaceDictionary(int index, Dictionary *dict)
    //{
    //    if (index < 0 || index >= dictionaries.size())
    //        throw "out of range.";
    //    if (dictionaries[index] == dict)
    //        return;
    //    delete dictionaries[index];
    //    dictionaries[index] = dict;
    //}

    bool dictionaryNameTaken(const QString &name)
    {
        QString lname = name.toLower();
        for (const Dictionary *d : dictionaries)
            if (d->name().toLower() == lname)
                return true;
        return false;
    }

    bool dictionaryNameValid(const QString &name, bool allowused)
    {
        if (name.isEmpty() || (!allowused && dictionaryNameTaken(name)))
            return false;

        QString invalid = "\\/*.?:|\"<>\t";
        QString lname = name.toLower();

        for (int ix = 0, siz = lname.size(); ix != siz; ++ix)
        {
            QChar ch = lname.at(ix);
            if (invalid.contains(ch))
                return false;
        }

        QFile f(ZKanji::userFolder() % "/" % lname % ".zkdict");
        if (f.exists())
            return allowused;

        if (!f.open(QIODevice::ReadWrite))
            return false;

        if (!f.remove())
        {
            f.close();
            f.remove();
            if (f.exists())
                return false;
        }

        return true;
    }

    void deleteDictionary(int index)
    {
        int orderpos = std::find(dictionaryorder.begin(), dictionaryorder.end(), index) - dictionaryorder.begin();
        Dictionary *d = dictionaries[index];
        gUI->signalDictionaryToBeRemoved(index, orderpos, d);
        dictionaries.erase(dictionaries.begin() + index);
        removeIndexFromList(index, dictionaryorder);
        gUI->signalDictionaryRemoved(index, orderpos, d);
    }

    Dictionary* dictionary(int index)
    {
#ifdef _DEBUG
        if (index < 0 || index >= dictionaries.size())
            throw "?";
#endif
        return dictionaries[index];
    }

    int dictionaryIndex(Dictionary *dict)
    {
        int pos = std::find(dictionaries.begin(), dictionaries.end(), dict) - dictionaries.begin();
        if (pos == dictionaries.size())
            return -1;
        return pos;
    }

    int dictionaryIndex(QString dictname)
    {
        dictname = dictname.toLower();
        int pos = std::find_if(dictionaries.begin(), dictionaries.end(), [&dictname](const Dictionary *d) { return d->name().toLower() == dictname; }) - dictionaries.begin();
        if (pos == dictionaries.size())
            return -1;
        return pos;
    }

    int dictionaryOrderIndex(Dictionary *dict)
    {
        return dictionaryOrder(dictionaryIndex(dict));
    }

    int dictionaryOrderIndex(const QString &dictname)
    {
        int pos = dictionaryIndex(dictname);
        if (pos == -1)
            return -1;
        return dictionaryOrder(pos);
    }

    int dictionaryPosition(int index)
    {
#ifdef _DEBUG
        if (index < 0 || index >= dictionaryorder.size())
            throw "?";
#endif
        return dictionaryorder[index];
    }

    int dictionaryOrder(int index)
    {
        return std::find(dictionaryorder.begin(), dictionaryorder.end(), index) - dictionaryorder.begin();
    }

    void moveDictionaryOrder(int from, int to)
    {
        if (from == to || from + 1 == to)
            return;

        int oldto = to;
        if (to > from)
            --to;
        int ix = dictionaryorder[from];
        dictionaryorder.erase(dictionaryorder.begin() + from);
        dictionaryorder.insert(dictionaryorder.begin() + to, ix);

        gUI->signalDictionaryMoved(from, oldto);
    }

    void renameDictionary(int index, const QString &str)
    {
        if (dictionaries[index]->name() == str)
            return;
        dictionaries[index]->setName(str);

        gUI->signalDictionaryRenamed(index, dictionaryOrder(index));
    }

    void changeDictionaryOrder(const std::list<quint8> &order)
    {
        if (dictionaryorder.size() != order.size())
            return;

        dictionaryorder.clear();
        auto it = order.begin();
        while (it != order.end())
        {
            dictionaryorder.push_back(*it);
            ++it;
        }
    }

    void saveUserData(bool forced)
    {
        if (forced || ZKanji::profile().isModified())
            ZKanji::profile().save(userFolder() + "/data/student.zkp");


        for (Dictionary *d : dictionaries)
        {
            if (d == dictionaries[0])
            {
                // TODO: (later) fix in case different base dictionary is implemented.
                if (d->name().toLower() != QStringLiteral("english"))
                {
                    QMessageBox::information(nullptr, "zkanji", qApp->translate(0, "Only the English dictionary can be used as the main dictionary at the moment."), QMessageBox::Ok);
                    return;
                }

                // TO-DO: get rid of the zkd copy of the main dictionary. The installer should install some other file name
                // and replace the base dictionary after the user data update.

                // The main dictionary data should be an exact copy of the file in the program folder.
                bool exists = QFileInfo::exists(userFolder() + "/data/English.zkdict");
                QDateTime basedate = Dictionary::fileWriteDate(ZKanji::appFolder() + "/data/English.zkj");
                QDateTime writtendate = exists ? Dictionary::fileWriteDate(userFolder() + "/data/English.zkdict") : QDateTime();
                if (!exists || basedate != writtendate)
                {
                    if (exists)
                    {
                        if (!QFile::remove(userFolder() + "/data/English.zkdict"))
                        {
                            QMessageBox::warning(nullptr, "zkanji", qApp->translate(0, "Couldn't save base dictionary in user folder. Make sure the folder exists and is not read-only, and the old file is not write protected."), QMessageBox::Ok);
                            continue;
                        }
                    }

                    if (!QFile::copy(appFolder() + "/data/English.zkj", userFolder() + "/data/English.zkdict"))
                    {
                        QMessageBox::warning(nullptr, "zkanji", qApp->translate(0, "Couldn't copy base dictionary to user folder. Make sure the folder exists and is not read-only, and the old file is not write protected."), QMessageBox::Ok);
                        continue;
                    }
                    d->setToUserModified();
                }
            }
            else if (forced || d->isModified())
            {
                // Making sure user data is saved if dictionary changed.
                forced = true;

                d->save(userFolder() + QString("/data/%1.zkdict").arg(d->name()));
            }

            if (forced || d->isUserModified())
                d->saveUserData(userFolder() + QString("/data/%1.zkuser").arg(d->name()));
        }
    }

    void backupUserData()
    {
        if (!Settings::data.backup)
            return;

        NTFSPermissionGuard permissionguard;

        QString path;
        if (!Settings::data.location.isEmpty())
            path = Settings::data.location;
        else
            path = ZKanji::userFolder() + "/data";
        QDir dir(path);
        if (!dir.exists())
        {
            QMessageBox::warning(gUI->activeMainForm(), "zkanji", gUI->tr("The backup folder specified in the settings does not exist. Please update your settings and restart zkanji, to create a safety backup of the current data files."));
            return;
        }

        dir.mkdir("backup");

        if (!dir.cd("backup"))
        {
            QMessageBox::warning(gUI->activeMainForm(), "zkanji", gUI->tr("Couldn't create or access the \"backup\" folder at the data backup location specified in the settings. Please update your settings and restart zkanji, to create a safety backup of the current data files."));
            return;
        }

        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

        QStringList dirs = dir.entryList();
        dirs.sort();
        std::vector<QDateTime> dates;
        QDateTime now = QDateTime::currentDateTimeUtc();
        now.setTime(QTime(0, 0, 0));
        for (int ix = dirs.size() - 1; ix != -1; --ix)
        {
            bool invalid = false;
            QString str = dirs.at(ix);
            if (str.size() != 8)
                invalid = true;
            for (int iy = 0; !invalid && iy != 8; ++iy)
                if (str.at(iy) < QChar('0') || str.at(iy) > QChar('9'))
                    invalid = true;

            QDateTime dt = QDateTime(QDate(str.left(4).toInt(), str.mid(4, 2).toInt(), str.right(2).toInt()), QTime(0, 0, 0));
            if (!dt.isValid())
                invalid = true;

            dt.setTimeSpec(Qt::UTC);

            if (invalid || dt.daysTo(now) < 0)
            {
                dirs.removeAt(ix);
                continue;
            }

            dates.insert(dates.begin(), dt);
        }

        if (!dates.empty() && dates.back().daysTo(now) < Settings::data.backupskip)
            return;

        while (dates.size() >= Settings::data.backupcnt)
        {
            dir.cd(dirs.at(0));

            if (!dir.removeRecursively())
            {
                QMessageBox::warning(gUI->activeMainForm(), "zkanji", gUI->tr("Couldn't delete files and folder of an old backup at the user data backup location. Please fix file access to the backup folder or update your settings and restart zkanji, to create a safety backup of the current data files."));
                return;
            }
            dir.cdUp();
            dates.erase(dates.begin());
            dirs.removeAt(0);
        }

        QString newdir = QString("%1%2%3").arg(now.date().year(), 4, 10, QChar('0')).arg(now.date().month(), 2, 10, QChar('0')).arg(now.date().day(), 2, 10, QChar('0'));
        if (!dir.mkdir(newdir))
        {
            QMessageBox::warning(gUI->activeMainForm(), "zkanji", gUI->tr("Couldn't create folder at the user data backup location. Please fix file access to the backup folder or update your settings and restart zkanji, to create a safety backup of the current data files."));
            return;
        }
        if (!dir.cd(newdir))
        {
            QMessageBox::warning(gUI->activeMainForm(), "zkanji", gUI->tr("Couldn't access folder at the user data backup location. Please fix file access to the backup folder or update your settings and restart zkanji, to create a safety backup of the current data files."));
            return;
        }

        bool fail = false;
        fail = QFile::exists(ZKanji::userFolder() + "/data/English.zkuser") && !QFile::copy(ZKanji::userFolder() + "/data/English.zkuser", dir.absolutePath() + "/Engilsh.zkuser");
        fail = (QFile::exists(ZKanji::userFolder() + "/data/student.zkp") && !QFile::copy(ZKanji::userFolder() + "/data/student.zkp", dir.absolutePath() + "/student.zkp")) || fail;

        for (int ix = 1, siz = ZKanji::dictionaryCount(); !fail && ix != siz; ++ix)
        {
            QString n = ZKanji::dictionary(ix)->name();
            fail = (QFile::exists(ZKanji::userFolder() + QString("/data/%1.zkdict").arg(n)) && !QFile::copy(ZKanji::userFolder() + QString("/data/%1.zkdict").arg(n), dir.absolutePath() + QString("/%1.zkdict").arg(n))) || fail;
            fail = (QFile::exists(ZKanji::userFolder() + QString("/data/%1.zkuser").arg(n)) && !QFile::copy(ZKanji::userFolder() + QString("/data/%1.zkuser").arg(n), dir.absolutePath() + QString("/%1.zkuser").arg(n))) || fail;
        }

        if (fail)
        {
            QMessageBox::warning(gUI->activeMainForm(), "zkanji", gUI->tr("Couldn't create backup of the user data files at the backup location specified in the settings. Please fix file access to the backup folder or update your settings and restart zkanji, to create a safety backup of the current data files."));
            return;
        }
    }

    void cloneWordData(WordEntry *dest, WordEntry *src, bool copykanjikana)
    {
        if (copykanjikana)
        {
            dest->kanji = src->kanji;
            dest->kana = src->kana;
            dest->romaji = src->romaji;
        }

        dest->freq = src->freq;
        //uint infmask = 0xffffffff << (int)WordInfo::Count;

        dest->inf = src->inf; //(src->inf & ~infmask) | (dest->inf & infmask);

        dest->defs.setSize(src->defs.size());
        for (int ix = 0; ix != dest->defs.size(); ++ix)
            dest->defs[ix] = src->defs[ix];
    }

    bool sameWord(WordEntry *a, WordEntry *b)
    {
        if (a->kanji != b->kanji || a->kana != b->kana || a->freq != b->freq || a->inf != b->inf || a->defs.size() != b->defs.size())
            return false;

        //uint infmask = 0xffffffff << (int)WordInfo::Count;
        //if ((a->inf & ~infmask) != (b->inf & ~infmask))
        //    return false;

        for (int ix = 0; ix != a->defs.size(); ++ix)
        {
            if (a->defs[ix] != b->defs[ix])
                return false;
        }
        return true;
    }

    QString kanjiMeanings(Dictionary *d, int index)
    {
        return d->kanjiMeaning(index);
    }
}


//-------------------------------------------------------------


bool operator!=(const WordDefAttrib &a, const WordDefAttrib &b)
{
    return a.dialects != b.dialects || a.fields != b.fields || a.notes != b.notes || a.types != b.types;
}

bool operator==(const WordDefAttrib &a, const WordDefAttrib &b)
{
    return a.dialects == b.dialects && a.fields == b.fields && a.notes == b.notes && a.types == b.types;
}

bool operator==(const WordFilterConditions &a, const WordFilterConditions &b)
{
    return a.examples == b.examples && a.groups == b.groups && a.inclusions == b.inclusions;
}

bool operator!=(const WordFilterConditions &a, const WordFilterConditions &b)
{
    return a.examples != b.examples || a.groups != b.groups || a.inclusions != b.inclusions;
}

bool operator!(const WordFilterConditions &a)
{
    if (a.examples != Inclusion::Ignore || a.groups != Inclusion::Ignore)
        return false;

    if (a.inclusions.empty())
        return true;

    for (int ix = 0; ix != a.inclusions.size(); ++ix)
        if (a.inclusions[ix] != Inclusion::Ignore)
            return false;

    return true;
}


//-------------------------------------------------------------


WordAttributeFilterList::WordAttributeFilterList(QObject *parent) : base(parent)
{
}

WordAttributeFilterList::~WordAttributeFilterList()
{
}

void WordAttributeFilterList::saveXMLSettings(QXmlStreamWriter &writer) const
{
    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
    {
        const WordAttributeFilter &f = list[ix];

        writer.writeEmptyElement("Filter");
        writer.writeAttribute("name", f.name);

        writer.writeAttribute("types", QString::number(f.attrib.types));
        writer.writeAttribute("notes", QString::number(f.attrib.notes));
        writer.writeAttribute("fields", QString::number(f.attrib.fields));
        writer.writeAttribute("dialects", QString::number(f.attrib.dialects));

        writer.writeAttribute("inf", QString::number(f.inf));
        writer.writeAttribute("JLPT", QString::number(f.jlpt));

        writer.writeAttribute("type", f.matchtype == FilterMatchType::AnyCanMatch ? "any" : "all");
    }
}

void WordAttributeFilterList::loadXMLSettings(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement())
    {
        if (reader.name() != "Filter")
        {
            reader.skipCurrentElement();
            continue;
        }

        bool ok = true;
        WordAttributeFilter f;
        f.name = reader.attributes().value("name").toString();
        if (itemIndex(f.name) != -1)
        {
            reader.skipCurrentElement();
            continue;
        }

        f.attrib.types = reader.attributes().value("types").toInt(&ok) & (0xffffffff >> (32 - (int)WordTypes::Count));
        if (ok)
            f.attrib.notes = reader.attributes().value("notes").toInt(&ok) & (0xffffffff >> (32 - (int)WordNotes::Count));
        if (ok)
            f.attrib.fields = reader.attributes().value("fields").toInt(&ok) & (0xffffffff >> (32 - (int)WordFields::Count));
        if (ok)
            f.attrib.dialects = reader.attributes().value("dialects").toInt(&ok) & (0xffff >> (16 - (int)WordDialects::Count));
        if (ok)
            f.inf = reader.attributes().value("inf").toInt(&ok) & (0xff >> (8 - (int)WordInfo::Count));
        if (ok)
            f.jlpt = reader.attributes().value("JLPT").toInt(&ok) & (0xff >> (8 - 5));
        if (!ok)
        {
            reader.skipCurrentElement();
            continue;
        }

        QStringRef typeref = reader.attributes().value("type");
        if (typeref.isEmpty() || typeref == "any")
            f.matchtype = FilterMatchType::AnyCanMatch;
        else
            f.matchtype = FilterMatchType::AllMustMatch;

        list.push_back(f);

        // Leaving "Filter"
        reader.skipCurrentElement();
    }
}

int WordAttributeFilterList::size() const
{
    return list.size();
}

void WordAttributeFilterList::clear()
{
    while (!list.empty())
        erase(0);
}

bool WordAttributeFilterList::empty() const
{
    return list.empty();
}

const WordAttributeFilter& WordAttributeFilterList::items(int index) const
{
    return list[index];
}

int WordAttributeFilterList::itemIndex(const QString &name)
{
    auto it = std::find_if(list.begin(), list.end(), [&name](const WordAttributeFilter &f) { return f.name == name; });
    if (it == list.end())
        return -1;
    return it - list.begin();
}

int WordAttributeFilterList::itemIndex(const QStringRef &name)
{
    auto it = std::find_if(list.begin(), list.end(), [&name](const WordAttributeFilter &f) { return f.name == name; });
    if (it == list.end())
        return -1;
    return it - list.begin();
}

void WordAttributeFilterList::erase(int index)
{
    list.erase(list.begin() + index);
    emit filterErased(index);
}

void WordAttributeFilterList::move(int index, int to)
{
    if (to < 0 || to > list.size() || to == index || to == index + 1)
        return;
    WordAttributeFilter f = list[index];
    list.erase(list.begin() + index);
    list.insert(list.begin() + (to - (to > index ? 1 : 0)), f);
    emit filterMoved(index, to);
}

void WordAttributeFilterList::rename(int index, const QString &newname)
{
    list[index].name = newname;
    emit filterRenamed(index);
}

void WordAttributeFilterList::update(int index, const WordDefAttrib &attrib, uchar info, uchar jlpt, FilterMatchType matchtype)
{
    WordAttributeFilter &f = list[index];
    f.attrib = attrib;
    f.inf = info;
    f.jlpt = jlpt;
    f.matchtype = matchtype;

    emit filterChanged(index);
}

void WordAttributeFilterList::add(const QString &name, const WordDefAttrib &attrib, uchar info, uchar jlpt, FilterMatchType matchtype)
{
    if (list.size() == 255)
        return;

    list.push_back(WordAttributeFilter());
    WordAttributeFilter &f = list.back();
    f.name = name;
    f.attrib = attrib;
    f.inf = info;
    f.jlpt = jlpt;
    f.matchtype = matchtype;

    emit filterCreated();
}

bool WordAttributeFilterList::match(const WordEntry *w, const WordFilterConditions *conditions) const
{
    if (conditions->groups != Inclusion::Ignore)
    {
        if ((conditions->groups == Inclusion::Include) == ((w->dat & (1 << (int)WordRuntimeData::InGroup)) == 0) /*((w->inf & (1 << (int)WordInfo::InGroup)) == 0)*/)
            return false;
    }

    const WordCommons *commons = nullptr;

    if (conditions->examples != Inclusion::Ignore)
        commons = ZKanji::commons.findWord(w->kanji.data(), w->kana.data(), w->romaji.data());

    if (conditions->examples != Inclusion::Ignore && (conditions->examples == Inclusion::Include) == (commons == nullptr || commons->examples.empty()))
        return false;

    for (int ix = 0; commons == nullptr && ix != conditions->inclusions.size(); ++ix)
    {
        if (list[ix].jlpt != 0)
        {
            commons = ZKanji::commons.findWord(w->kanji.data(), w->kana.data(), w->romaji.data());
            break;
        }
    }

    for (int ix = 0; ix != conditions->inclusions.size(); ++ix)
    {
        if (conditions->inclusions[ix] != Inclusion::Ignore && domatch(w, commons, ix) != (conditions->inclusions[ix] == Inclusion::Include))
            return false;
    }
    return true;
}

bool WordAttributeFilterList::domatch(const WordEntry *w, const WordCommons *commons,  int index) const
{
    const WordAttributeFilter &f = list[index];

    if (f.matchtype == FilterMatchType::AllMustMatch)
    {
        if (f.inf != 0)
        {
            if ((f.inf & w->inf) != f.inf)
                return false;
        }

        WordDefAttrib attrib;
        for (int ix = 0; ix != w->defs.size(); ++ix)
        {
            attrib.types |= w->defs[ix].attrib.types;
            attrib.notes |= w->defs[ix].attrib.notes;
            attrib.fields |= w->defs[ix].attrib.fields;
            attrib.dialects |= w->defs[ix].attrib.dialects;
        }

        if ((f.attrib.types & attrib.types) != f.attrib.types ||
            (f.attrib.notes & attrib.notes) != f.attrib.notes ||
            (f.attrib.fields & attrib.fields) != f.attrib.fields ||
            (f.attrib.dialects & attrib.dialects) != f.attrib.dialects)
            return false;

        if (f.jlpt != 0)
        {
            //if (!commonsset)
            //{
            //    commons = ZKanji::commons.findWord(w->kanji.data(), w->kana.data(), w->romaji.data());
            //    commonsset = true;
            //}

            if (commons == nullptr || (1 << (5 - commons->jlptn)) != f.jlpt)
                return false;
        }
        return true;
    }

    // Any match counts.

    if (f.inf != 0)
    {
        if ((f.inf & w->inf) != 0)
            return true;
    }

    for (int ix = 0; ix != w->defs.size(); ++ix)
    {
        if ((f.attrib.types & w->defs[ix].attrib.types) != 0 ||
            (f.attrib.notes & w->defs[ix].attrib.notes) != 0 ||
            (f.attrib.fields & w->defs[ix].attrib.fields) != 0 ||
            (f.attrib.dialects & w->defs[ix].attrib.dialects))
            return true;
    }
    if (f.jlpt != 0)
    {
        //if (!commonsset)
        //{
        //    commons = ZKanji::commons.findWord(w->kanji.data(), w->kana.data(), w->romaji.data());
        //    commonsset = true;
        //}

        if (commons != nullptr && ((1 << (5 - commons->jlptn)) & f.jlpt) != 0)
            return true;
    }

    return false;
}

//-------------------------------------------------------------


WordDefinition::WordDefinition()
{

}

WordDefinition::WordDefinition(WordDefinition &&src)
{
    *this = std::move(src);
}

WordDefinition& WordDefinition::operator=(WordDefinition &&src)
{
    std::swap(def, src.def);
    std::swap(attrib, src.attrib);

    return *this;
}

bool operator!=(const WordDefinition &a, const WordDefinition &b)
{
    return a.attrib != b.attrib || a.def != b.def;
}

bool operator==(const WordDefinition &a, const WordDefinition &b)
{
    return a.attrib == b.attrib && a.def == b.def;
}

//QDataStream& operator<<(QDataStream &stream, const WordDefAttrib &a)
//{
//    stream << (quint32)a.types;
//    stream << (quint32)a.notes;
//    stream << (quint32)a.fields;
//    stream << (quint16)a.dialects;
//    return stream;
//}
//
//QDataStream& operator>>(QDataStream &stream, WordDefAttrib &a)
//{
//    quint32 ui;
//    quint16 us;
//
//    stream >> ui;
//    a.types = ui;
//    stream >> ui;
//    a.notes = ui;
//    stream >> ui;
//    a.fields = ui;
//    stream >> us;
//    a.dialects = us;
//
//    return stream;
//}


//-------------------------------------------------------------


OriginalWordsList::~OriginalWordsList()
{
    clear();
}

void OriginalWordsList::swap(OriginalWordsList &other)
{
    std::swap(list, other.list);
}

bool OriginalWordsList::empty() const
{
    return list.empty();
}

void OriginalWordsList::clear()
{
    list.clear();
}

bool OriginalWordsList::skip(QDataStream &stream)
{
    qint32 c, ch;
    qint8 b;
    qint16 w;

    stream >> c;
    int pos = 0;
    while (pos++ != c)
    {
        stream >> ch;

        stream >> b;
        stream.skipRawData(b);
        //stream >> make_zstr(w->kanji, ZStrFormat::ByteAddNull);
        stream >> b;
        stream.skipRawData(b);
        //stream >> make_zstr(w->kana, ZStrFormat::ByteAddNull);

        stream >> b;
        //stream >> w->defcnt;

        //w->defs = new WordDefinition[w->defcnt];

        for (uchar ix = 0; ix != b; ++ix)
        {
            stream >> w;
            stream.skipRawData(w + sizeof(qint32) + sizeof(qint32) + sizeof(qint8) + sizeof(qint16));
            //stream >> make_zstr(w->defs[ix].def, ZStrFormat::WordAddNull);
            //stream >> w->defs[ix].types;
            //stream >> w->defs[ix].notes;
            //stream >> w->defs[ix].fields;
            //stream.skipRawData(sizeof(qint16));
        }
    }

    return true;
}

int OriginalWordsList::size() const
{
    return list.size();
}

const OriginalWord* OriginalWordsList::items(int index) const
{
    return list[index];
}

int OriginalWordsList::wordIndex(int windex) const
{
    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
        if (list[ix]->index == windex)
            return ix;
    return -1;
}


const OriginalWord* OriginalWordsList::find(int windex)const
{
    for (const OriginalWord *w : list)
        if (w->index == windex)
            return w;
    return nullptr;
}

//void OriginalWordsList::remove(int index)
//{
//    list.erase(list.begin() + index);
//}

bool OriginalWordsList::createAdded(int windex, const QChar *kanji, const QChar *kana, int kanjilen, int kanalen)
{
    OriginalWord *o = new OriginalWord;
    o->change = OriginalWord::Added;

    o->kanji.copy(kanji, kanjilen);
    o->kana.copy(kana, kanalen);
    o->index = windex;
    o->freq = 0;
    o->inf = 0;

    list.push_back(o);

    return true;
}

bool OriginalWordsList::createModified(int windex, WordEntry *w)
{
    OriginalWord *o = new OriginalWord;
    o->change = OriginalWord::Modified;

    o->kanji = w->kanji;
    o->kana = w->kana;
    o->index = windex;
    o->freq = w->freq;
    o->inf = w->inf; // (w->inf & ~(1 << (int)WordInfo::InGroup));

    o->defs.setSize(w->defs.size());
    for (int ix = 0, siz = w->defs.size(); ix != siz; ++ix)
        o->defs[ix] = w->defs[ix];

    list.push_back(o);

    return true;
}

bool OriginalWordsList::revertModified(int windex, WordEntry *w)
{
    int ix = wordIndex(windex);
    if (ix == -1)
        return false;

    OriginalWord *o = list[ix];

    if (o->change != OriginalWord::Modified)
        return false;

#ifdef _DEBUG
    if (windex != o->index || w->kanji != o->kanji || w->kana != o->kana)
        throw "Different word passed to revert.";
#endif

    w->freq = o->freq;
    w->inf = o->inf;
    w->defs.setSize(o->defs.size());
    for (int ix = 0, siz = o->defs.size(); ix != siz; ++ix)
        w->defs[ix] = o->defs[ix];

    list.erase(list.begin() + ix);

    return true;
}

bool OriginalWordsList::processRemovedWord(int windex)
{
    int indexindex = -1;
    bool changed = false;
    for (int ix = 0; ix != list.size(); ++ix)
    {
        OriginalWord *w = list[ix];
        if (w->index == windex)
        {
#ifdef _DEBUG
            if (w->change != OriginalWord::Added)
                throw "Only added words should be removed by users.";
#endif
            indexindex = ix;
        }
        else if (w->index > windex)
        {
            --w->index;
            changed = true;
        }
    }
    if (indexindex != -1)
    {
        list.erase(list.begin() + indexindex);
        changed = true;
    }

    return changed;
}


//-------------------------------------------------------------


bool definitionsMatch(WordEntry *e1, WordEntry *e2)
{
    if (e1->defs.size() != e2->defs.size())
        return false;
    for (int ix = 0; ix != e1->defs.size(); ++ix)
        if (e1->defs[ix] != e2->defs[ix])
            return false;
    return true;
}

//QDataStream& operator<<(QDataStream &stream, const WordEntry &w)
//{
//    stream << make_zstr(w.kanji, ZStrFormat::Byte);
//    stream << make_zstr(w.kana, ZStrFormat::Byte);
//    stream << make_zstr(w.romaji, ZStrFormat::Byte);
//
//    stream << (quint16)w.freq;
//    stream << (quint32)(w.inf & ~(1 << (int)WordInfo::InGroup));
//
//    stream << (quint8)w.defs.size();
//    for (int iy = 0; iy != w.defs.size(); ++iy)
//    {
//        const WordDefinition &d = w.defs[iy];
//        stream << make_zstr(d.def, ZStrFormat::Word);
//        stream << d.attrib;
//    }
//
//    return stream;
//}
//
//QDataStream& operator>>(QDataStream &stream, WordEntry &w)
//{
//    stream >> make_zstr(w.kanji, ZStrFormat::Byte);
//    stream >> make_zstr(w.kana, ZStrFormat::Byte);
//    stream >> make_zstr(w.romaji, ZStrFormat::Byte);
//
//    quint16 us;
//    quint32 ui;
//
//    stream >> us;
//    w.freq = us;
//    stream >> ui;
//    w.inf = ui;
//
//    quint8 b;
//    stream >> b;
//    w.defs.resize(b);
//    for (int iy = 0; iy != w.defs.size(); ++iy)
//    {
//        WordDefinition &d = w.defs[iy];
//        stream >> make_zstr(d.def, ZStrFormat::Word);
//        stream >> d.attrib;
//    }
//
//    return stream;
//}


//-------------------------------------------------------------


//WordResultList::WordResultList() : dict(nullptr)
//{
//
//}

WordResultList::WordResultList(Dictionary *dict) : dict(dict)
{

}

WordResultList::WordResultList(WordResultList &&src) : dict(nullptr)
{
    *this = std::forward<WordResultList>(src);
}

WordResultList& WordResultList::operator=(WordResultList &&src)
{
    std::swap(dict, src.dict);
    std::swap(indexes, src.indexes);
    std::swap(infs, src.infs);

    return *this;
}

Dictionary* WordResultList::dictionary()
{
    return dict;
}

const Dictionary* WordResultList::dictionary() const
{
    return dict;
}

void WordResultList::setDictionary(Dictionary *newdict)
{
    if (dict == newdict)
        return;
    dict = newdict;
    clear();
}

void WordResultList::set(const std::vector<int> &wordindexes)
{
    indexes = wordindexes;
    infs.clear();
}

void WordResultList::set(std::vector<int> &&wordindexes)
{
    std::swap(indexes, wordindexes);
    infs.clear();
}

inline WordEntry* WordResultList::items(int ix)
{
#ifdef _DEBUG
    if (dict == nullptr)
        throw "Dictionary shouldn't be a null pointer here.";
#endif
    return dict->wordEntry(indexes[ix]);
}

inline const WordEntry* WordResultList::items(int ix) const
{
#ifdef _DEBUG
    if (dict == nullptr)
        throw "Dictionary shouldn't be a null pointer here.";
#endif
    return dict->wordEntry(indexes[ix]);
}

WordEntry* WordResultList::operator[](int ix)
{
    return items(ix);
}

const WordEntry* WordResultList::operator[](int ix) const
{
    return items(ix);
}

void WordResultList::clear()
{
    indexes.clear();
    infs.clear();
}

bool WordResultList::empty() const
{
    return indexes.empty();
}

int WordResultList::size() const
{
    return indexes.size();
}

std::vector<int>& WordResultList::getIndexes()
{
    return indexes;
}

const std::vector<int>& WordResultList::getIndexes() const
{
    return indexes;
}

//int WordResultList::indexAt(int ix) const
//{
//#ifdef _DEBUG
//    if (dict == nullptr)
//        throw "Dictionary shouldn't be a null pointer here.";
//#endif
//    return indexes[ix];
//}

smartvector<std::vector<InfTypes>>& WordResultList::getInflections()
{
    return infs;
}

const smartvector<std::vector<InfTypes>>& WordResultList::getInflections() const
{
    return infs;
}

void WordResultList::sortByList(const std::vector<int> &list)
{
    std::vector<int> indextmp;
    std::vector<std::vector<InfTypes>*> inftmp;
    indexes.swap(indextmp);
    infs.swap(inftmp);
    indexes.reserve(list.size());
    for (int ix = 0; ix != list.size(); ++ix)
    {
        indexes.push_back(indextmp[list[ix]]);
        if (inftmp.size() > list[ix])
        {
            while (infs.size() != indexes.size() - 1)
                infs.push_back(nullptr);
            infs.push_back(inftmp[list[ix]]);
        }
    }
}

void WordResultList::sortByIndex()
{
    std::vector<std::pair<int, int>> ordered;
    int siz = indexes.size();
    ordered.reserve(siz);
    for (int ix = 0; ix != siz; ++ix)
        ordered.push_back(std::make_pair(ix, indexes[ix]));

    std::sort(ordered.begin(), ordered.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
        return a.second < b.second;
    });

    std::vector<std::vector<InfTypes>*> inftmp;
    infs.swap(inftmp);
    std::vector<int>().swap(indexes);
    indexes.reserve(siz);

    int infsiz = inftmp.size();
    for (int ix = 0; ix != siz; ++ix)
    {
        auto &p = ordered[ix];
        indexes.push_back(p.second);
        if (infsiz > p.first && inftmp[p.first] != nullptr)
        {
            infs.reserve(ix + 1);
            while (infs.size() <= ix)
                infs.push_back(nullptr);
            infs[ix] = inftmp[p.first];
        }
    }
}

void WordResultList::jpSort()
{
    // When changing, also change jpInsertPos().

    std::vector<int> list;
    std::vector<std::pair<WordEntry*, std::vector<InfTypes>*>> pairlist;
    pairlist.reserve(indexes.size());
    list.reserve(indexes.size());

    for (int ix = 0, siz = indexes.size(); ix != siz; ++ix)
    {
        list.push_back(ix);
        pairlist.push_back(std::make_pair(dict->wordEntry(indexes[ix]), infs.size() > ix ? infs[ix] : nullptr));
    }

    std::sort(list.begin(), list.end(), [this, &pairlist](int aix, int bix) {
        return Dictionary::jpSortFunc(pairlist[aix], pairlist[bix]);
    });

    //[this](int aix, int bix)
    //{
    //    const WordEntry *aw = dict->wordEntry(indexes[aix]);
    //    const WordEntry *bw = dict->wordEntry(indexes[bix]);

    //    int afreq = aw->freq;
    //    int bfreq = bw->freq;

    //    if (abs(afreq - bfreq) > 1500 || (abs(afreq - bfreq) > 500 && afreq + bfreq < 2400))
    //        return afreq > bfreq;

    //    int alen = aw->kana.size();
    //    int blen = bw->kana.size();

    //    bool ako = (aw->defs[0].attrib.notes & (1 << (int)WordNotes::KanaOnly)) != 0;
    //    bool bko = (bw->defs[0].attrib.notes & (1 << (int)WordNotes::KanaOnly)) != 0;

    //    if ((ako && !bko && alen < blen) || (bko && !ako && blen < alen))
    //        return alen < blen;

    //    int akanjicnt = 0;
    //    int bkanjicnt = 0;

    //    int aklen = aw->kanji.size();
    //    int bklen = bw->kanji.size();
    //    for (int ix = 0; ix < aklen; ++ix)
    //        if (KANJI(aw->kanji[ix].unicode()))
    //            ++akanjicnt;
    //    for (int ix = 0; ix < bklen; ++ix)
    //        if (KANJI(bw->kanji[ix].unicode()))
    //            ++bkanjicnt;

    //    if (akanjicnt < bkanjicnt && alen - 1 <= blen)
    //        return true;
    //    if (bkanjicnt < akanjicnt && blen - 1 <= alen)
    //        return false;

    //    if (alen == blen)
    //    {
    //        if ((infs.size() <= aix || infs.null(aix) || infs[aix]->empty()) != (infs.size() <= bix || infs.null(bix) || infs[bix]->empty()))
    //            return (infs.size() <= aix || infs.null(aix) || infs[aix]->empty());
    //    }

    //    return alen < blen;
    //});

    sortByList(list);
}

int WordResultList::jpInsertPos(int windex, const std::vector<InfTypes> &winfs, int *oldpos)
{
    std::vector<std::pair<WordEntry*, const std::vector<InfTypes>*>> list;

    int wpos = -1;
    for (int ix = 0, siz = indexes.size(); ix != siz && wpos == -1; ++ix)
    {
        if (windex == indexes[ix])
        {
            wpos = ix;
            break;
        }
    }

    if (oldpos != nullptr)
        *oldpos = wpos;

    // Every comparison is done on list, which is a copy of indexes and infs. The word with
    // the dictionary index of windex is not included.

    list.resize(indexes.size() - (wpos == -1 ? 0 : 1));
    std::pair<WordEntry*, const std::vector<InfTypes>*> *data = list.data();

    for (int ix = 0, ix2 = 0, siz = list.size(); ix != siz; ++ix, ++ix2)
    {
        if (ix == wpos)
            ++ix2;
        data[ix] = std::make_pair(dict->wordEntry(indexes[ix2]), infs.size() > (ix2) ? infs[ix2] : nullptr);
    }

    WordEntry *w = dict->wordEntry(windex);

    // Finding the insert position for windex.
    auto it = std::lower_bound(list.begin(), list.end(), std::make_pair(w, &winfs), &Dictionary::jpSortFunc);

    int pos = it == list.end() ? list.size() : it - list.begin();

    return pos;
}

void WordResultList::defSort(QString searchstr)
{
    // When changing, also change defInsertPos().

    // Before the words can be sorted by definition, some data must be collected
    // and cached till the end of the sort to speed the sort up.
    searchstr = searchstr.toLower();

    std::vector<std::pair<const WordEntry*, Dictionary::WordResultSortData>> sortlist;
    sortlist.reserve(indexes.size());

    while (sortlist.size() != indexes.size())
    {
        WordEntry *w = dict->wordEntry(indexes[sortlist.size()]);
        sortlist.push_back(std::make_pair(w, Dictionary::defSortDataGen(searchstr, w)));
    }

    std::vector<int> list;
    list.resize(indexes.size());

    int *data = list.data();
    for (int ix = list.size() - 1; ix != -1; --ix)
        data[ix] = ix;

    //std::sort(list.begin(), list.end(), [this, &sortlist](int aix, int bix /*const WordResultSortData &a, const WordResultSortData &b*/)
    //{
    //    int afreq = dict->wordEntry(indexes[aix])->freq;
    //    int bfreq = dict->wordEntry(indexes[bix])->freq;

    //    if ((std::min(afreq, bfreq) < 3000 && abs(afreq - bfreq) > 1500) || 
    //        (std::min(afreq, bfreq) >= 3000 && abs(afreq - bfreq) > 2400) ||
    //        (abs(afreq - bfreq) > 500 && afreq + bfreq < 2400))
    //        return afreq > bfreq;
    //    //if (bfreq <= 1400 && afreq > 1700)
    //    //    return true;
    //    //if (bfreq > 1700 && afreq <= 1400)
    //    //    return false;
    //    //if (abs(bfreq - afreq) > 2400)
    //    //    return afreq < bfreq;

    //    return (bfreq - afreq + (sortlist[aix].defpos - sortlist[bix].defpos) * 500 + ((sortlist[aix].len + sortlist[aix].pos) - (sortlist[bix].len + sortlist[bix].pos)) * 500 + (sortlist[aix].pos - sortlist[bix].pos) * 50) < 0;
    //});

    std::sort(list.begin(), list.end(), [this, &sortlist](int ax, int bx) {
        return Dictionary::defSortFunc(sortlist[ax], sortlist[bx]);
    });

    sortByList(list);
}

int WordResultList::defInsertPos(QString searchstr, int windex, int *oldpos)
{
    int wpos = -1;
    for (int ix = 0; ix != indexes.size() && wpos == -1; ++ix)
    {
        if (windex == indexes[ix])
        {
            wpos = ix;
            break;
        }
    }

    if (oldpos != nullptr)
        *oldpos = wpos;

    int siz = indexes.size() - (wpos == -1 ? 0 : 1);

    // Before the words can be sorted by definition, some data must be collected
    // and cached till the end of the sort to speed the sort up.
    searchstr = searchstr.toLower();

    std::vector<std::pair<const WordEntry*, Dictionary::WordResultSortData>> sortlist;
    sortlist.reserve(siz);

    for (int ix = 0; ix != indexes.size(); ++ix)
    {
        if (ix == wpos)
        {
            ++ix;
            if (ix == indexes.size())
                break;
        }
        WordEntry *w = dict->wordEntry(indexes[ix]);

        sortlist.push_back(std::make_pair(w, Dictionary::defSortDataGen(searchstr, w)));
    }

    const WordEntry *w = dict->wordEntry(windex);
    Dictionary::WordResultSortData wdata = Dictionary::defSortDataGen(searchstr, dict->wordEntry(windex));

    auto it = std::lower_bound(sortlist.begin(), sortlist.end(), std::make_pair(w, wdata), &Dictionary::defSortFunc);

    int pos = it == sortlist.end() ? sortlist.size() : it - sortlist.begin();

    return pos;
}

void WordResultList::removeAt(int ix)
{
    indexes.erase(indexes.begin() + ix);
    if (infs.size() > ix)
        infs.erase(infs.begin() + ix);
}

void WordResultList::insert(int pos, int wordindex)
{
    indexes.insert(indexes.begin() + pos, wordindex);
    if (infs.size() > pos)
        infs.insert(infs.begin() + pos, nullptr);
}

void WordResultList::insert(int pos, int wordindex, const std::vector<InfTypes> &inf)
{
    if (inf.empty())
    {
        insert(pos, wordindex);
        return;
    }

    indexes.insert(indexes.begin() + pos, wordindex);

    if (infs.size() < pos)
        infs.insert(infs.end(), pos - infs.size(), nullptr);
    infs.insert(infs.begin() + pos, inf);
}

void WordResultList::reserve(int newreserve, bool infs_too)
{
    indexes.reserve(newreserve);
    if (infs_too)
        infs.reserve(newreserve);
}

void WordResultList::add(int wordindex)
{
    indexes.push_back(wordindex);
}

void WordResultList::add(int wordindex, const std::vector<InfTypes> &inf)
{
    if (inf.empty())
    {
        add(wordindex);
        return;
    }

    // Items in infs should line up with items in indexes. Infs are not added
    // unnecessarily but when one is added, we have to pad the infs vector
    // with null up to the new inflection.
    if (infs.size() != indexes.size())
        infs.resizeAddNull(indexes.size());

    indexes.push_back(wordindex);
    if (inf.empty())
        infs.push_back(nullptr);
    else
        infs.push_back(inf);
}

//void WordResultList::setWordInflections(int ix, QVector<InfTypes> inflist)
//{
//    list[ix].inf = inflist;
//}


//-------------------------------------------------------------


TextSearchTree::TextSearchTree(Dictionary *dict, bool kana, bool reversed) : base(/*true,*/), dict(dict), kana(kana), reversed(reversed)
{
    //if (kana && reversed)
    //    nodes.addNode(&QChar('\''), 1, true);
}

TextSearchTree::TextSearchTree(Dictionary *dict, TextSearchTree &&src) : base(), dict(dict), kana(src.kana), reversed(src.reversed)
{
    base::swap(src);
}

TextSearchTree::~TextSearchTree()
{
}

void TextSearchTree::swap(TextSearchTree &src, Dictionary *d)
{
    swap(src);
    dict = d;
}

void TextSearchTree::swap(TextSearchTree &src)
{
    base::swap(src);
}

void TextSearchTree::copy(TextSearchTree *src)
{
    if (src == this)
        return;

    kana = src->kana;
    reversed = src->reversed;
    base::copy(src);
}

//void TextSearchTree::setDictionary(Dictionary *newdict)
//{
//    dict = newdict;
//}

                               
void TextSearchTree::findWords(std::vector<int> &result, QString search, bool exact, bool sameform, const std::vector<int> *wordpool, const WordFilterConditions *conditions, uint infsize) 
{
    // When changing this: update wordMatches() as well.

    // Warning: the result list is not erased since conditions were added. If any error occurs
    // because of this, you'll know where to look.

    // If the wordpool is not null, only words also found in it can be returned in result.
    // Because we go backwards in the result list, we have to go backwards in wordpool too
    // as both are sorted in increasing order.

    uint filterpos = -2;
    const int *wordpooldata = nullptr;
    if (wordpool != nullptr)
    {
        filterpos = wordpool->size() - 1;
        wordpooldata = wordpool->data();
    }

    if (!kana)
    {
        QString str = search.toLower();

        // The search string might contain delimiters or other characters that are not stored.
        // All other parts are considered to be "words", which are recognized and stored in
        // the tree.

        // First we find the node for the word in the search string, that contains the least
        // number of possible results.
        QCharTokenizer tokens(str.constData(), str.size());

        QString match;

        TextNode *selected = nullptr;
        TextNode *n = nullptr;

        int contained = std::numeric_limits<int>::max();
        while (tokens.next())
        {
            findContainer(tokens.token(), tokens.tokenSize(), n);

            if (n && (!selected || n->sum < selected->sum))
            {
                selected = n;
                match = QString(tokens.token(), tokens.tokenSize());
            }
        }

        if (selected == nullptr)
            return;

        if (!sameform)
            search = str;

        // Every meaning of the possible results are checked for a match with the search string.

        std::vector<int> lines = selected->lines;

        if (!exact)
            selected->nodes.collectLines(lines, match.constData(), match.size());

        // Lines now contains lots of words which can be duplicates too. Those must be removed.
        std::sort(lines.begin(), lines.end(), [this](int a, int b) { return wordForLine(a) < wordForLine(b); });

        auto uit = std::unique(lines.begin(), lines.end());

        for (int ix = uit - lines.begin() - 1; ix != -1; --ix)
        {
            if (wordpool != nullptr)
            {
                // Skip words not in the word filter.

                int val = wordForLine(lines[ix]);
                while (filterpos != -1 && wordpooldata[filterpos] > val)
                    --filterpos;
                if (filterpos == -1)
                {
                    // There are no more possible matches in tmp to the ix-th word, when
                    // we run out of wordpool.
                    break;
                }

                if (val > wordpooldata[filterpos])
                {
                    while (ix != -1 && wordForLine(lines[ix]) > wordpooldata[filterpos])
                        --ix;

                    // The loop also decrements this.
                    ++ix;

                    continue;
                }
            }

            int line = lines[ix];
            int windex = wordForLine(lines[ix]);

            if (conditions != nullptr)
            {
                const WordEntry *w = dict->wordEntry(windex);
                if (!ZKanji::wordfilters().match(w, conditions))
                    continue;
            }

            bool found = false;

            for (int j = 0, siz = lineDefinitionCount(line) /* w->defs.size() */; !found && j != siz; ++j)
            {
                QString def = lineDefinition(line, j); //w->defs[j].def.toQString();
                if (!sameform)
                    def = def.toLower();

                int pos = -1;
                do
                {
                    pos = def.indexOf(search, pos + 1);
                    if (pos != -1 && (pos == 0 || qcharisdelim(def.at(pos - 1)) == QCharKind::Delimiter) &&
                        (!exact || pos + search.size() == def.size() || qcharisdelim(def.at(pos + search.size())) == QCharKind::Delimiter))
                        found = true;
                } while (pos != -1 && !found);
            }

            if (found)
                result.push_back(windex);
        }

        return;
    }

    // Kana search, not definition.

    QString romaji = romanize(search);

    if (romaji.isEmpty())
        return;

    if (reversed)
        std::reverse(romaji.begin(), romaji.end());

    TextNode *node;
    findContainer(romaji.constData(), romaji.size(), node);

    if (!node)
        return;

    std::vector<int> lines = node->lines;

    if (!exact)
        node->nodes.collectLines(lines, romaji.constData(), romaji.size());

    if (reversed)
        std::reverse(romaji.begin(), romaji.end());

    // Lines now contains lots of words which can be duplicates too.
    std::sort(lines.begin(), lines.end());

    QChar *word;
    int wlen;

    if (!sameform)
        search = romaji;
    else if (infsize != 0)
        search = search.left(search.size() - infsize) + hiraganize(search.right(infsize));

    // Remove duplicates and invalid matches.
    auto uit = std::unique(lines.begin(), lines.end());
    //result.erase(uit, result.end());

    for (int ix = uit - lines.begin() - 1; ix != -1; --ix)
    {
        if (wordpool != nullptr)
        {
            int line = lines[ix];
            while (filterpos != -1 && wordpooldata[filterpos] > line)
                --filterpos;
            if (filterpos == -1)
            {
                // There are no more possible matches in result to the ix-th word, when
                // we run out of wordpool.
                //result.erase(result.begin(), result.begin() + (ix + 1));
                break;
            }
            if (lines[ix] > wordpooldata[filterpos])
            {
                while (ix != -1 && lines[ix] > wordpooldata[filterpos])
                {
                    //result.erase(result.begin() + ix);
                    --ix;
                }
                // The loop also decrements this.
                ++ix;

                continue;
            }
        }

        int windex = lines[ix];

        WordEntry *w = dict->wordEntry(windex);
        if (conditions != nullptr)
        {
            if (!ZKanji::wordfilters().match(w, conditions))
                continue;
        }

        int klen;

        word = sameform ? w->kana.data() : w->romaji.data();
        wlen = qcharlen(word);

        if (wlen < search.size() || (exact && wlen != search.size()))
            ;// result.erase(result.begin() + ix);
        else if ((!sameform || infsize == 0 || search.size() <= infsize) &&
            ((exact && qcharcmp(word, search.constData())) ||
            (!reversed && qcharncmp(search.constData(), word, search.size())) ||
            (reversed && qcharncmp(search.constData(), word + wlen - search.size(), search.size()))))
            ;// result.erase(result.begin() + ix);
        else if ((sameform && infsize != 0 && search.size() > infsize) &&
            ((exact && (qcharncmp(word, search.constData(), wlen - infsize) || hiraganize(word + wlen - infsize) != search.rightRef(infsize))) ||
            (!reversed && !exact /* in theory reversed is always true, because only word endings are checked when deinflecting. */) ||
            ((reversed || exact) && (qcharncmp(search.constData(), word + wlen - search.size(), search.size() - infsize) ||
            search.rightRef(infsize) != hiraganize(word + wlen - infsize)))))
            ;// result.erase(result.begin() + ix);
        else
            result.push_back(windex);
    }
}

bool TextSearchTree::wordMatches(int windex, QString search, bool exact, bool sameform, uint infsize, bool *f)
{
    if (!kana)
    {
        QString str = search.toLower();

        if (!sameform)
            search = str;

        int line = lineForWord(windex);
        if (line == -1)
        {
            if (f != nullptr)
                *f = false;
            return false;
        }
        if (f != nullptr)
            *f = true;

        //const WordEntry *w = dict->wordEntry(windex);

        bool found = false;

        for (int j = 0, siz = lineDefinitionCount(line) /* w->defs.size() */; !found && j != siz; ++j)
        {
            QString def = lineDefinition(line, j); //w->defs[j].def.toQString();
            if (!sameform)
                def = def.toLower();

            int pos = -1;
            do
            {
                pos = def.indexOf(search, pos + 1);
                if (pos != -1 && (pos == 0 || qcharisdelim(def.at(pos - 1)) == QCharKind::Delimiter) &&
                    (!exact || pos + search.size() == def.size() || qcharisdelim(def.at(pos + search.size())) == QCharKind::Delimiter))
                    found = true;
            } while (pos != -1 && !found);
        }

        if (found)
            return true;

        return false;
    }

    // Kana search, not definition.

    QString romaji = romanize(search);

    if (!sameform)
        search = romaji;
    else if (infsize != 0)
        search = search.left(search.size() - infsize) + hiraganize(search.right(infsize));

    WordEntry *w = dict->wordEntry(windex);

    QChar *word = sameform ? w->kana.data() : w->romaji.data();
    int wlen = qcharlen(word);

    if (wlen < search.size() || (exact && wlen != search.size()))
        return false;// result.erase(result.begin() + ix);
    else if ((!sameform || infsize == 0 || search.size() <= infsize) &&
        ((exact && qcharcmp(word, search.constData())) ||
        (!reversed && qcharncmp(search.constData(), word, search.size())) ||
        (reversed && qcharncmp(search.constData(), word + wlen - search.size(), search.size()))))
        return false;// result.erase(result.begin() + ix);
    else if ((sameform && infsize != 0 && search.size() > infsize) &&
        ((exact && (qcharncmp(word, search.constData(), wlen - infsize) || hiraganize(word + wlen - infsize) != search.rightRef(infsize))) ||
        (!reversed && !exact /* in theory reversed is always true, because only word endings are checked when deinflecting. */) ||
        ((reversed || exact) && (qcharncmp(search.constData(), word + wlen - search.size(), search.size() - infsize) ||
        search.rightRef(infsize) != hiraganize(word + wlen - infsize)))))
        return false;// result.erase(result.begin() + ix);
    //else

    return true;// result.push_back(tmp[ix]);

    //return false;
}

bool TextSearchTree::isKana() const
{
    return kana;
}

bool TextSearchTree::isReversed() const
{
    return reversed;
}

Dictionary* TextSearchTree::dictionary() const
{
    return dict;
}

void TextSearchTree::expandWith(int windex, bool inserted)
{
    doExpand(windex, inserted);
}

int TextSearchTree::wordForLine(int line) const
{
    return line;
}

int TextSearchTree::lineForWord(int windex) const
{
    return windex;
}

int TextSearchTree::lineDefinitionCount(int line) const
{
    return dict->wordEntry(line)->defs.size();
}

QString TextSearchTree::lineDefinition(int line, int def) const
{
    return dict->wordEntry(line)->defs[def].def.toQString();
}

void TextSearchTree::doGetWord(int index, QStringList &texts) const
{
    const WordEntry* w = dict->wordEntry(index);
    if (kana)
    {
        QString s = romanize(w->kana.data()); //romaji;
        if (reversed)
            std::reverse(s.begin(), s.end());
        texts << s;
        return;
    }

    QCharString k;
    for (int ix = 0; ix < w->defs.size(); ++ix)
    {
        k.copy(w->defs[ix].def.toLower().constData()); // AnsiStrLower
        QCharTokenizer tokens(k.data());

        while (tokens.next())
            texts << QString(tokens.token(), tokens.tokenSize());
    }
}

int TextSearchTree::size() const
{
    return dict->entryCount();
}

//int TextSearchTree::doMoveFromFullNode(TextNode *node, int index)
//{
//    if (kana)
//        return base::doMoveFromFullNode(node, index);
//
//    int result = 0;
//    //QChar *k;
//    int slen = node->label.size();
//    QChar *l;
//    const QChar *s;
//    TextNode *n;
//
//    const WordEntry* w = dict->wordEntry(node->lines[index]);
//
//    // Same as above, but we have to separate the meanings and test for every single part.
//
//    // Add every word to this first.
//    QStringList ms;
//
//    bool fullmove = true;
//    QCharString k;
//    for (int ix = 0; ix < w->defs.size(); ++ix)
//    {
//        k.copy(w->defs[ix].def.toLower().constData()); // AnsiStrLower
//        QCharTokenizer tokens(k.data());
//
//        while (tokens.next())
//        {
//            if (tokens.tokenSize() >= slen && !qcharncmp(node->label.data(), tokens.token(), slen))
//            {
//                if (tokens.tokenSize() == slen) // Fits this node well
//                    fullmove = false;
//                else if (!blacklisted(tokens.token(), tokens.tokenSize()))
//                    ms << QString(tokens.token(), tokens.tokenSize());
//            }
//        }
//    }
//
//    std::sort(ms.begin(), ms.end(), [](const QString &a, const QString &b) { return a < b; });
//
//    for (int ix = 0; ix < ms.size(); ++ix)
//    {
//        // Find length of new node to create
//        int len = slen + 1;
//        while (len <= ms.at(ix).size() && blacklisted(ms.at(ix).constData(), len))
//            len++;
//
//        
//        if (len > ms.at(ix).size())
//        {
//            // If blacklisted, delete.
//            ms.removeAt(ix);
//        }
//        else
//        { 
//            // Delete everything that would go to the same node.
//            while (ms.size() > ix + 1 && !qcharncmp(ms.at(ix).constData(), ms.at(ix + 1).constData(), len))
//                ms.removeAt(ix + 1);
//
//            s = ms.at(ix).constData();
//            n = node->nodes.searchContainer(s, len);
//            if (!n || qcharncmp(n->label.data(), s, len))
//            {
//                n = node->nodes.addNode(s, len, true);
//                //node->nodes.sort();
//            }
//            n->lines.push_back(node->lines[index]);
//            ++result;
//            ++n->sum;
//        }
//    }
//
//    // Delete line from original node.
//    if (fullmove)
//    {
//        node->lines.erase(node->lines.begin() + index);
//        --result;
//    }
//
//    return result;
//}


//-------------------------------------------------------------


StudyDefinitionTree::StudyDefinitionTree(Dictionary *dict) : base(dict, false, false)
{

}

void StudyDefinitionTree::load(QDataStream &stream)
{
    base::load(stream);

    qint32 i;

    stream >> i;
    int cnt = i;

    list.clear();
    list.reserve(i);

    for (int ix = 0; ix != cnt; ++ix)
    {
        stream >> i;
        list.emplace_back(i, QCharString());
        stream >> make_zstr(list.back().second, ZStrFormat::Word);

    }
}

void StudyDefinitionTree::save(QDataStream &stream) const
{
    base::save(stream);

    stream << (qint32)list.size();
    for (auto &p : list)
    {
        stream << (qint32)p.first;
        stream << make_zstr(p.second, ZStrFormat::Word);
    }
}

void StudyDefinitionTree::swap(StudyDefinitionTree &src, Dictionary *dict)
{
   base::swap(src, dict);
   std::swap(list, src.list);
}

void StudyDefinitionTree::copy(StudyDefinitionTree *src)
{
    if (src == this)
        return;

    base::copy(src);
    list = src->list;
}

void StudyDefinitionTree::applyChanges(std::map<int, int> &changes)
{
    std::set<int> found;
    int delcnt = 0;
    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
    {
        int newval = changes[list[ix].first];

        if (newval == -1 || found.count(newval) != 0)
        {
            list[ix].first = -1;
            ++delcnt;
        }
        else
        {
            list[ix].first = newval;
            found.insert(newval);
        }
    }

    std::sort(list.begin(), list.end(), [](const std::pair<int, QCharString> &a, const std::pair<int, QCharString> &b) { 
        if (a.first == -1)
            return false;
        if (b.first == -1)
            return true;
        return a.first < b.first;
    });
    if (delcnt != 0)
        list.erase(list.end() - delcnt, list.end());

    rebuild();
}

void StudyDefinitionTree::processRemovedWord(int windex)
{
    auto it = wordIt(windex);
    int pos = it - list.begin();

    // The list is ordered by word indexes. Any item starting at pos will be equal or greater
    // than windex.
    for (int ix = pos, siz = list.size(); ix != siz; ++ix)
        --list[ix].first;

    // The item's value at it was decremented above, so matching windex - 1 instead of windex.
    if (it == list.end() || it->first != windex - 1)
        return;

    removeLine(pos, true);
    list.erase(it);
}

int StudyDefinitionTree::size() const
{
    return list.size();
}

const QCharString* StudyDefinitionTree::itemDef(int windex) const
{
    auto it = wordIt(windex);
    if (it == list.end() || it->first != windex)
        return nullptr;
    return &it->second;
}

bool StudyDefinitionTree::setDefinition(int windex, QString def)
{
    if (def.indexOf('\t') != -1)
        def.replace('\t', ' ');
    if (def.size() > 32767)
        def.resize(32767);

    auto it = wordIt(windex);

    int pos = it - list.begin();
    if (it != list.end() && it->first == windex)
    {
        if (!def.isEmpty() && it->second == def)
            return false;

        removeLine(pos, def.isEmpty());
        if (def.isEmpty())
        {
            list.erase(it);
            return true;
        }
        it->second.copy(def.constData());
        doExpand(pos, false);
        return true;
    }
    else
    {
        if (def.isEmpty())
            return false;
        list.emplace(it, windex, QCharString())->second.copy(def.constData());
        doExpand(pos, true);
        return true;
    }
}

void StudyDefinitionTree::listWordIndexes(std::vector<int> &result) const
{
    for (int ix = 0, siz = list.size(); ix != siz; ++ix)
        result.push_back(list[ix].first);
}

int StudyDefinitionTree::wordForLine(int line) const
{
    return list[line].first;
}

int StudyDefinitionTree::lineForWord(int windex) const
{
    auto it = wordIt(windex);
    if (it == list.end() || it->first != windex)
        return -1;
    return it - list.begin();
}

int StudyDefinitionTree::lineDefinitionCount(int line) const
{
#ifdef _DEBUG
    if (line < 0 || line >= list.size())
        throw "index out of bounds";
#endif
    return 1;
}

QString StudyDefinitionTree::lineDefinition(int line, int def) const
{
#ifdef _DEBUG
    if (line < 0 || line >= list.size())
        throw "index out of bounds";
#endif
    return list[line].second.toQString();
}

void StudyDefinitionTree::doGetWord(int index, QStringList &texts) const
{
    //auto it = defs.find(indexes[index]);
    //if (it == defs.end())
    //    return;

    QCharString k;
    k.copy(list[index].second.toLower().constData());
    QCharTokenizer tokens(k.data());

    while (tokens.next())
        texts << QString(tokens.token(), tokens.tokenSize());
}

std::vector<std::pair<int, QCharString>>::iterator StudyDefinitionTree::wordIt(int windex)
{
    return std::lower_bound(list.begin(), list.end(), windex, [](const std::pair<int, QCharString> &item, int windex) {
        return item.first < windex;
    });
}

std::vector<std::pair<int, QCharString>>::const_iterator StudyDefinitionTree::wordIt(int windex) const
{
    return std::lower_bound(list.begin(), list.end(), windex, [](const std::pair<int, QCharString> &item, int windex) {
        return item.first < windex;
    });
}


//-------------------------------------------------------------


WordCommonsTree::WordCommonsTree() : base(/*false,*/)
{
}

WordCommonsTree::~WordCommonsTree()
{
}

void WordCommonsTree::clear()
{
    list.clear();
    base::clear();
}

void WordCommonsTree::load(QDataStream &stream)
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

    base::load(stream);
}

void WordCommonsTree::save(QDataStream &stream) const
{
    stream << (quint32)list.size();

    for (int ix = 0; ix != list.size(); ++ix)
    {
        stream << make_zstr(list[ix]->kanji, ZStrFormat::Byte);
        stream << make_zstr(list[ix]->kana, ZStrFormat::Byte);

        stream << (quint8)list[ix]->jlptn;
    }

    base::save(stream);
}

void WordCommonsTree::clearJLPTData()
{
    int cnt = list.size();
    bool erased = false;
    for (int ix = cnt - 1; ix >= 0; --ix)
    {
        WordCommons *w = list[ix];
        if (w->jlptn == 0)
            continue;
        if (w->examples.empty())
        {
            erased = true;
            list.erase(list.begin() + ix);
        }
        else
            w->jlptn = 0;
    }
    if (erased)
        rebuild(false);
}

void WordCommonsTree::clearExamplesData()
{
    int cnt = list.size();
    bool erased = false;
    for (int ix = cnt - 1; ix >= 0; --ix)
    {
        WordCommons *w = list[ix];
        if (w->examples.empty())
            continue;
        if (w->jlptn == 0)
        {
            erased = true;
            list.erase(list.begin() + ix);
        }
        else
            w->examples.clear();
    }
    if (erased)
        rebuild(false);
}

int WordCommonsTree::addJLPTN(const QChar *kanji, const QChar *kana, int jlptN, bool insertsorted)
{
    int ix = list.size();
    
    WordCommons *wc = nullptr;
    if (insertsorted)
    {
        // When a word must be added in the sort order, we assume the tree is valid. Otherwise
        // the word order in the tree is invalid.

        int ind;
        if (!insertIndex(kanji, kana, ind))
            wc = list[ind];
        ix = ind;
    }

    if (wc == nullptr)
    {
        wc = new WordCommons;
        wc->kanji.copy(kanji);
        wc->kana.copy(kana);

        list.insert(list.begin() + ix, wc);
        doExpand(ix, true);
    }

    wc->jlptn = jlptN;

    return ix;
}

bool WordCommonsTree::removeJLPTN(int commonsindex)
{
#ifdef _DEBUG
    if (commonsindex < 0 || commonsindex >= list.size())
        throw "Index out of bounds.";
#endif

    WordCommons *wc = list[commonsindex];
    wc->jlptn = 0;
    if (!wc->examples.empty())
        return false;

    list.erase(list.begin() + commonsindex);
    removeLine(commonsindex, true);

    return true;
}

int WordCommonsTree::addExample(const QChar *kanji, const QChar *kana, const WordCommonsExample &data)
{
    int ix = -1;

    if (!insertIndex(kanji, kana, ix))
    {
        // The examples array has at most ushort::max positions.
        if (list[ix]->examples.size() >= std::numeric_limits<ushort>::max())
            return -1;
        list[ix]->examples.resize(list[ix]->examples.size() + 1);
        list[ix]->examples[list[ix]->examples.size() - 1] = data;
        return ix;
    }

    WordCommons *wc = new WordCommons;
    wc->kanji.copy(kanji);
    wc->kana.copy(kana);
    wc->jlptn = 0;
    wc->examples.resize(1);
    wc->examples[0] = data;

    list.insert(list.begin() + ix, wc);

    return ix;
}

void WordCommonsTree::rebuild(bool checkandsort, const std::function<bool()> &callback)
{
    if (checkandsort && list.size() > 1)
    {

        if (interruptSort(list.begin(), list.end(), [&callback](const WordCommons *a, const WordCommons *b, bool stop) {
            stop = false;
            if (callback)
                stop = !callback();
            if (stop)
                return false;

            return wordcompare(a->kanji.data(), a->kana.data(), b->kanji.data(), b->kana.data()) < 0;
        }))
            return;

        // list.size() > 1 is used here to make sure there's a next item after
        // list.begin()

        auto it = list.begin();
        auto nextit = std::next(it);

        while (nextit != list.end())
        {
            if (callback && !callback())
                return;

            WordCommons *a = *it;
            WordCommons *b = *nextit;
            if (wordcompare(a->kanji.data(), a->kana.data(), b->kanji.data(), b->kana.data()) == 0)
            {
                // Word at it and nextit must be merged.
                if (a->jlptn == 0)
                    a->jlptn = b->jlptn;
                a->examples.append(b->examples);
            }
            else
            {
                it = std::next(it);
                if (it != nextit)
                    std::swap(*it, *nextit);
            }
            nextit = std::next(nextit);
        }

        list.shrink(it - list.begin() + 1);
    }

    base::rebuild(callback);
}


bool WordCommonsTree::isKana() const
{
    return true;
}

bool WordCommonsTree::isReversed() const
{
    return false;
}

WordCommons* WordCommonsTree::findWord(const QChar *kanji, const QChar *kana, const QChar *romaji)
{
    if (kanji == nullptr || kana == nullptr)
        return nullptr;

    QString r;
    if (romaji == nullptr)
        r = romanize(kana);
    else
        r = QString(romaji);
    TextNode *n;
    findContainer(r.constData(), r.size(), n);

    if (n == nullptr)
        return nullptr;

    for (int ix = 0; ix != n->lines.size(); ++ix)
    {
        WordCommons *dat = list[n->lines[ix]];
        if (dat->kanji != kanji || dat->kana != kana)
            continue;
        return dat;
    }

    return nullptr;
}

WordCommons* WordCommonsTree::addWord(const QChar *kanji, const QChar *kana)
{
    WordCommons *dat = new WordCommons;
    dat->kanji.copy(kanji);
    dat->kana.copy(kana);
    dat->jlptn = 0;
    list.push_back(dat);

    return dat;
}

const smartvector<WordCommons>& WordCommonsTree::getItems()
{
    return list;
}

void WordCommonsTree::doGetWord(int index, QStringList &texts) const
{
    texts << romanize(list[index]->kana.data());
}

int WordCommonsTree::size() const
{
    return list.size();
}

bool WordCommonsTree::insertIndex(const QChar *kanji, const QChar *kana, int &index) const
{
    auto it = std::lower_bound(list.begin(), list.end(), nullptr, [kanji, kana](const WordCommons *c, void *) {
        return wordcompare(c->kanji.data(), c->kana.data(), kanji, kana) < 0;
    });

    if (it == list.end())
    {
        index = list.size();
        return true;
    }
    index = it - list.begin();
    if (wordcompare(kanji, kana, list[index]->kanji.data(), list[index]->kana.data()) == 0)
        return false;
    return true;
}


//-------------------------------------------------------------


Dictionary::Dictionary() : mod(false), usermod(false), dtree(this, false, false), ktree(this, true, false), btree(this, true, true), wordstudydefs(this), studydecks(new StudyDeckList)
{
    groups = new Groups(this);

    decks = new WordDeckList(this);

    kanjidata.resize(ZKanji::kanjis.size(), KanjiDictData());

    //decks->rename(0, tr("Deck %1").arg(1));

    //dtree.setDictionary(this);
    //ktree.setDictionary(this);
    //btree.setDictionary(this);
}

Dictionary::Dictionary(smartvector<WordEntry> &&words, TextSearchTree &&dtree, TextSearchTree &&ktree, TextSearchTree &&btree,
    smartvector<KanjiDictData> &&kanjidata, std::map<ushort, std::vector<int>> &&symdata, std::map<ushort, std::vector<int>> &&kanadata,
    std::vector<int> &&abcde, std::vector<int> &&aiueo) : words(std::move(words)), dtree(this, std::move(dtree)), ktree(this, std::move(ktree)), btree(this, std::move(btree)),
    kanjidata(std::move(kanjidata)), symdata(std::move(symdata)), kanadata(std::move(kanadata)), abcde(std::move(abcde)), aiueo(std::move(aiueo)), wordstudydefs(this), studydecks(new StudyDeckList)
{
    groups = new Groups(this);
    decks = new WordDeckList(this);
}

Dictionary::~Dictionary()
{
    delete groups;
    delete decks;
    studydecks.release();
    
#ifdef _DEBUG
    words.clear();
    //worddecks.clear();
    dtree.clear();
    ktree.clear();
    btree.clear();
    kanjidata.clear();
    symdata.clear();
    kanadata.clear();
    //worddefs.clear();
    //kanjidefs.clear();
    abcde.clear();
    aiueo.clear();
    wordstudydefs.clear();
#endif
}

//QDateTime Dictionary::baseGenerationDate() const
//{
//    return basedate;
//}

void Dictionary::loadBaseFile(const QString &filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly))
        return;

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    //auto bo = stream.byteOrder();

    char tmp[9];
    tmp[8] = 0;

    stream.readRawData(tmp, 8);
    bool good = !strncmp("zdict", tmp, 5);
    int version = strtol(tmp + 5, 0, 10);

    if (!good || version < 1)
        throw ZException("Invalid base file version.");

    int dversion;
    if (version == 1)
        loadBaseLegacy(stream, version);
    else
        loadBase(stream);

    qint32 fs = 0;
    stream >> fs;
    if (fs != f.size())
        throw ZException("Base file is corrupted. Size error.");
}

void Dictionary::loadBase(QDataStream &stream)
{
    stream >> make_zdate(basedate);
    stream >> make_zstr(prgversion, ZStrFormat::Byte);

    ZKanji::commons.load(stream);

    //bool kreload = !ZKanji::kanjis.empty();

    KanjiEntry *k;
    uchar b;

    quint8 rcnt = 0; // Count of readings for kanji.
    QChar *rtmp = nullptr; // Temporary array for reading in old format kanji readings.

    for (int ix = 0; ix < ZKanji::kanjicount; ++ix)
    {
        quint16 ch;
        stream >> ch;
        k = ZKanji::addKanji(QChar(ch));

        quint16 u16;
        quint8 u8;

        stream >> u16;
        k->jis = u16;

        stream >> u8;
        k->rad = u8;

        stream >> u8;
        k->crad = u8;

        stream >> u8;
        k->jouyou = u8;

        stream >> u8;
        k->jlpt = u8;
        stream >> u8;
        k->strokes = u8;

        stream >> u16;
        k->frequency = u16;

        stream >> u8;
        k->skips[0] = u8;
        stream >> u8;
        k->skips[1] = u8;
        stream >> u8;
        k->skips[2] = u8;

        stream >> u16;
        k->knk = u16;

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

        stream >> u16;
        k->newnelson = u16;
        stream >> u16;
        k->crowley = u16;
        stream >> u16;
        k->flashc = u16;
        stream >> u16;
        k->kguide = u16;
        stream >> u16;
        k->henshallg = u16;
        stream >> u16;
        k->context = u16;
        stream >> u16;
        k->halpernk = u16;
        stream >> u16;
        k->halpernl = u16;
        stream >> u16;
        k->heisigf = u16;
        stream >> u16;
        k->heisign = u16;
        stream >> u16;
        k->oneil = u16;
        stream >> u16;
        k->halpernn = u16;
        stream >> u16;
        k->deroo = u16;
        stream >> u16;
        k->sakade = u16;
        stream >> u16;
        k->tuttle = u16;

        stream.readRawData(k->snh, 8);

        stream.readRawData(k->busy, 4);

        qint32 i32;
        stream >> i32;
        k->word_freq = i32;

        stream >> k->on;
        stream >> k->kun;
        //stream >> k->irreg;
        stream >> k->nam;
        stream >> k->meanings;
    }

    ZKanji::generateValidKanji();

    quint16 u16;
    stream >> u16;
    ZKanji::radklist.resize(u16);
    for (int ix = 0; ix != ZKanji::radklist.size(); ++ix)
    {
        stream >> u16;
        ZKanji::radklist[ix].first = u16;
        stream >> u16;
        ZKanji::radklist[ix].second.setSize(u16);
        for (int iy = 0; iy != ZKanji::radklist[ix].second.size(); ++iy)
        {
            stream >> u16;
            ZKanji::radklist[ix].second[iy] = u16;
            ZKanji::kanjis[u16]->radks.push_back(ix);
        }
    }

    quint8 u8;
    stream >> u8;
    ZKanji::radkcnt.resize(u8);
    for (int ix = 0; ix != ZKanji::radkcnt.size(); ++ix)
    {
        stream >> u8;
        ZKanji::radkcnt[ix] = u8;
    }

    ZKanji::radlist.load(stream);
}

void Dictionary::loadFile(const QString &filename, bool basedict, bool skiporiginals)
{
    QFile f(filename);

    setName(QFileInfo(filename).baseName());

    if (!f.open(QIODevice::ReadOnly))
        throw ZException("Couldn't open dictionary file");

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    char tmp[9];
    tmp[8] = 0;
    stream.readRawData(tmp, 8);

    bool oldver;
    int version;
    bool good = !strncmp("zwrds", tmp, 5);
    oldver = good;
    if (!good)
        good = !strncmp("zkdat", tmp, 5);

    if (good)
        version = strtol(tmp + 5, nullptr, 10);

    if (!good || (oldver && version < 10))
        throw ZException("Invalid or corrupted dictionary file version.");

    if (oldver)
        loadLegacy(stream, version, basedict, skiporiginals);
    else
        load(stream);

    quint32 u32;
    stream >> u32;
    if (u32 != f.pos())
        throw ZException("Incorrect dictionary file size.");

    mod = false;
    emit dictionaryModified(false);
}

void Dictionary::load(QDataStream &stream)
{
    char tmp[4];
    tmp[3] = 0;

    stream.readRawData(tmp, 3);

    int version = strtol(tmp, nullptr, 10);
    if (version == 0)
        throw ZException("Invalid or corrupted dictionary file version number.");

    stream >> make_zdate(writedate);
    stream >> make_zstr(prgversion, ZStrFormat::Byte);

    stream >> make_zstr(info);

    quint32 cnt;

    // Read the dictionary words.
    stream >> cnt;

    words.reserve(cnt);

    quint8 u8;
    quint16 u16;
    quint32 u32;

#if TIMED_LOAD == 1
    QElapsedTimer t;
    t.start();
#endif

    while (cnt--)
    {
        WordEntry *w = new WordEntry;

        stream >> make_zstr(w->kanji, ZStrFormat::Byte);
        stream >> make_zstr(w->kana, ZStrFormat::Byte);
        stream >> make_zstr(w->romaji, ZStrFormat::Byte);

        stream >> u16;
        w->freq = u16;
        stream >> u8;
        w->inf = u8;

        stream >> u8;
        w->defs.resize(u8);
        for (int iy = 0; iy != w->defs.size(); ++iy)
        {
            WordDefinition &d = w->defs[iy];
            stream >> make_zstr(d.def, ZStrFormat::Word);

            stream >> u8;
            quint8 which = u8;

            if ((which & 1) != 0)
            {
                stream >> u32;
                d.attrib.types = u32;
            }
            if ((which & 2) != 0)
            {
                stream >> u32;
                d.attrib.notes = u32;
            }
            if ((which & 4) != 0)
            {
                stream >> u32;
                d.attrib.fields = u32;
            }
            if ((which & 8) != 0)
            {
                stream >> u16;
                d.attrib.dialects = u16;
            }
        }

        words.push_back(w);
    }

#if TIMED_LOAD == 1
    qint64 t1 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    // Compress read the rest of the data.

    QByteArray data;
    stream >> u32;
    data.resize(u32);
    stream.readRawData(data.data(), u32);

    data = qUncompress(data);

    QDataStream dstream(data);

    dtree.load(dstream);
    ktree.load(dstream);
    btree.load(dstream);

#if TIMED_LOAD == 1
    qint64 t2 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    quint16 kfirst;
    quint16 kcnt;
    KanjiDictData *kd;

    kanjidata.clear();
    kanjidata.resize(ZKanji::kanjis.size(), KanjiDictData());

    dstream >> kfirst;
    while (kfirst != ZKanji::kanjis.size())
    {
        dstream >> kcnt;
        for (int ix = 0; ix != kcnt; ++ix)
        {
            KanjiDictData *kd = kanjidata[ix + kfirst];

            dstream >> make_zvec<qint32, qint32>(kd->words);

            //dstream >> u32;
            //kd->words.resize(u32);
            //for (int iy = 0; iy != kd->words.size(); ++iy)
            //{
            //    dstream >> u32;
            //    kd->words[iy] = u32;
            //}
            //dstream >> kd->meanings;
        }
        dstream >> kfirst;
    }

#if TIMED_LOAD == 1
    qint64 t3 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    // Symbol word data
    quint16 cnt16;
    dstream >> cnt16;

    for (int ix = 0; ix != cnt16; ++ix)
    {
        dstream >> u16;
        dstream >> u32;
        std::vector<int> &data = symdata[u16];
        data.resize(u32);
        for (int iy = 0; iy != data.size(); ++iy)
        {
            dstream >> u32;
            data[iy] = u32;
        }
    }

    // Kana word data
    quint8 cnt8;
    dstream >> cnt8;

    for (int ix = 0; ix != cnt8; ++ix)
    {
        dstream >> u16;
        dstream >> u32;
        std::vector<int> &data = kanadata[u16];
        data.resize(u32);
        for (int iy = 0; iy != data.size(); ++iy)
        {
            dstream >> u32;
            data[iy] = u32;
        }
    }

#if TIMED_LOAD == 1
    qint64 t4 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    abcde.resize(words.size());
    aiueo.resize(words.size());

    qint32 i32;

    // Read word alphabet.
    for (int ix = 0; ix != words.size(); ++ix)
    {
        dstream >> i32;
        abcde[ix] = i32;
        dstream >> i32;
        aiueo[ix] = i32;
    }

#if TIMED_LOAD == 1
    qint64 t5 = t.nsecsElapsed();
    t.invalidate();

    QMessageBox::information(nullptr, "zkanji", QString("Data: %1\n%2\n%3\n%4\n%5").arg(t1).arg(t2).arg(t3).arg(t4).arg(t5), QMessageBox::Ok);
#endif
}

void Dictionary::loadUserDataFile(const QString &filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly))
        return;

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    char tmp[7];
    stream.readRawData(tmp, 6);
    tmp[6] = 0;

    bool good = !strncmp("zgf", tmp, 3);
    bool oldversion = good;

    if (!oldversion)
        good = !strncmp("zud", tmp, 3);

    int version = strtol(tmp + 3, 0, 10);

    if (!good || (oldversion && version < 20))
        throw ZException("Invalid or corrupted user file version.");

    if (oldversion)
    {
        // The meanings for kanji was specified in the dictionary in old data files, but it
        // was moved to the user data in newer versions (after 2015). The clear would remove
        // the meanings loaded already from the legacy format, so those must be saved before
        // the clear.

        std::map<int, QCharStringList> meanings;
        for (int ix = 0, siz = kanjidata.size(); ix != siz; ++ix)
            if (!kanjidata[ix]->meanings.empty())
                meanings[ix] = std::move(kanjidata[ix]->meanings);
        clearUserData();
        for (auto it = meanings.begin(); it != meanings.end(); ++it)
            kanjidata[it->first]->meanings = std::move(it->second);

        loadUserDataLegacy(stream, version);
    }
    else
    {
        clearUserData();
        loadUserData(stream);
    }

    usermod = false;
    emit userDataModified(false);
    emit dictionaryReset();
}

void Dictionary::loadUserData(QDataStream &stream)
{
    QDateTime dictdate;
    stream >> make_zdate(dictdate);

    if (dictdate != lastWriteDate())
        throw ZException("User date does not match dictionary date.");

    quint8 b;
    qint8 c;
    qint32 i;

#if TIMED_LOAD == 1
    QElapsedTimer t;
    t.start();
#endif

    // The value written was 1 for the base dictionary and 0 for other dictionaries.
    stream >> b;
    if (b == 1)
    {
        // Originals loading. Instead of loading the originals list, loads the updated words
        // and modifies the dictionary with them. That in turn fills the originals list. When
        // not loading the base dictionary, the words are updated, but the originals list is
        // not changed.

        stream >> i;
        int cnt = i;

        uint32_t u32;
        uint16_t u16;
        uint8_t u8;
        for (int ix = 0; ix != cnt; ++ix)
        {
            OriginalWord::ChangeType change;
            int index;
            stream >> b;
            change = (OriginalWord::ChangeType)b;
            stream >> i;
            index = i;

            WordEntry w;
            //stream >> w;
            stream >> make_zstr(w.kanji, ZStrFormat::Byte);
            stream >> make_zstr(w.kana, ZStrFormat::Byte);
            stream >> make_zstr(w.romaji, ZStrFormat::Byte);

            stream >> u16;
            w.freq = u16;
            stream >> u8;
            w.inf = u8;

            stream >> u8;
            w.defs.resize(u8);
            for (int iy = 0; iy != w.defs.size(); ++iy)
            {
                WordDefinition &d = w.defs[iy];
                stream >> make_zstr(d.def, ZStrFormat::Word);

                //stream >> d.attrib;

                stream >> u32;
                d.attrib.types = u32;
                stream >> u32;
                d.attrib.notes = u32;
                stream >> u32;
                d.attrib.fields = u32;
                stream >> u16;
                d.attrib.dialects = u16;
            }


            if (change == OriginalWord::Added)
                addWordCopy(&w, true);
            else
                cloneWordData(index, &w, true, false);
        }
    }

#if TIMED_LOAD == 1
    qint64 t1 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    groups->load(stream);

#if TIMED_LOAD == 1
    qint64 t2 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    decks->clear();
    studydecks->load(stream);

#if TIMED_LOAD == 1
    qint64 t3 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    decks->load(stream);

#if TIMED_LOAD == 1
    qint64 t4 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    wordstudydefs.load(stream);

#if TIMED_LOAD == 1
    qint64 t5 = t.nsecsElapsed();
    t.invalidate();
    t.start();
#endif

    quint16 us = 1;
    while (us != 0)
    {
        stream >> us;
        quint16 ix = us;
        stream >> us;
        quint16 endix = ix + us;
        while (ix != endix)
        {
            stream >> kanjidata[ix]->meanings;
            stream >> make_zvec<qint32, qint32>(kanjidata[ix]->ex);
            ++ix;
        }
    }

#if TIMED_LOAD == 1
    qint64 t6 = t.nsecsElapsed();
    t.invalidate();

    QMessageBox::information(nullptr, "zkanji", QString("User: %1\n%2\n%3\n%4\n%5\n%6").arg(t1).arg(t2).arg(t3).arg(t4).arg(t5).arg(t6), QMessageBox::Ok);
#endif
}

void Dictionary::clearUserData()
{
    if (this == ZKanji::dictionary(0))
    {
        // Restoring dictionary from originals.
        while (!ZKanji::originals.empty())
        {
            const OriginalWord *o = ZKanji::originals.items(0);
            if (o->change == OriginalWord::Modified)
                revertEntry(o->index);
            else
                removeEntry(o->index);
        }
    }

    groups->clear();
    decks->clear();
    studydecks->clear();
    wordstudydefs.clear();

    for (int ix = 0, siz = kanjidata.size(); ix != siz; ++ix)
    {
        kanjidata[ix]->ex.clear();
        kanjidata[ix]->meanings.clear();
    }
}

QDateTime Dictionary::fileWriteDate(const QString &filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly))
        return QDateTime();

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    char tmp[9];
    tmp[8] = 0;
    stream.readRawData(tmp, 8);

    bool oldver;
    bool good = !strncmp("zwrds", tmp, 5);
    oldver = good;
    if (!good)
        good = !strncmp("zkdat", tmp, 5);

    int version;
    if (good)
        version = strtol(tmp + 5, nullptr, 10);

    if (!good || (oldver && version < 10))
        return QDateTime();

    if (oldver)
    {
        // Main dictionary write date not used here.
        stream.skipRawData(sizeof(Temporary::SYSTEMTIME));

        Temporary::SYSTEMTIME st;
        stream >> st;
        QDateTime r(QDate(st.wYear, st.wMonth, st.wDay), QTime(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds));
        r.setTimeSpec(Qt::UTC);
        return r;
    }

    stream.readRawData(tmp, 3);
    tmp[3] = 0;
    version = strtol(tmp, nullptr, 10);
    if (version == 0)
        return QDateTime();

    QDateTime dt;
    stream >> make_zdate(dt);
    return dt;
}

bool Dictionary::pre2015() const
{
    return prgversion == QStringLiteral("pre2015");
}

//void Dictionary::fixStudyData()
//{
//    studydecks->fixAnswerCounts();
//}

Error Dictionary::saveBase(const QString &filename)
{
    basedate = QDateTime::currentDateTimeUtc();

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly))
        return Error(Error::Access);

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    prgversion = QString::fromLatin1(ZKANJI_PROGRAM_VERSION);

    int errorcode = 1;
    try
    {
        stream.writeRawData("zdict", 5);
        stream.writeRawData(ZKANJI_BASE_FILE_VERSION, 3);

        errorcode = 2;

        stream << make_zdate(basedate);
        stream << make_zstr(prgversion, ZStrFormat::Byte);

        errorcode = 3;

        ZKanji::commons.save(stream);

        errorcode = 4;

        // Kanji data
        for (int ix = 0; ix != ZKanji::kanjis.size(); ++ix)
        {
            KanjiEntry *k = ZKanji::kanjis[ix];
            stream << (quint16)k->ch.unicode()
                << (quint16)k->jis
                << (quint8)k->rad
                << (quint8)k->crad
                << (quint8)k->jouyou
                << (quint8)k->jlpt
                << (quint8)k->strokes
                << (quint16)k->frequency
                << (quint8)k->skips[0]
                << (quint8)k->skips[1]
                << (quint8)k->skips[2]
                << (quint16)k->knk
                << (quint16)k->nelson
                << (quint16)k->halpern
                << (quint16)k->heisig
                << (quint16)k->gakken
                << (quint16)k->henshall

                << (quint16)k->newnelson
                << (quint16)k->crowley
                << (quint16)k->flashc
                << (quint16)k->kguide
                << (quint16)k->henshallg
                << (quint16)k->context
                << (quint16)k->halpernk
                << (quint16)k->halpernl
                << (quint16)k->heisigf
                << (quint16)k->heisign
                << (quint16)k->oneil
                << (quint16)k->halpernn
                << (quint16)k->deroo
                << (quint16)k->sakade
                << (quint16)k->tuttle;

            stream.writeRawData(k->snh, 8);
            stream.writeRawData(k->busy, 4);

            stream << (qint32)k->word_freq;
            stream << k->on;
            stream << k->kun;
            //stream << k->irreg;
            stream << k->nam;
            stream << k->meanings;
        }

        errorcode = 5;

        stream << (quint16)ZKanji::radklist.size();
        for (int ix = 0; ix != ZKanji::radklist.size(); ++ix)
        {
            stream << (quint16)ZKanji::radklist[ix].first;
            stream << (quint16)ZKanji::radklist[ix].second.size();
            for (int iy = 0; iy != ZKanji::radklist[ix].second.size(); ++iy)
                stream << (quint16)ZKanji::radklist[ix].second[iy];
        }

        errorcode = 6;

        stream << (quint8)ZKanji::radkcnt.size();
        for (int ix = 0; ix != ZKanji::radkcnt.size(); ++ix)
            stream << (quint8)ZKanji::radkcnt[ix];

        errorcode = 7;

        ZKanji::radlist.save(stream);

        errorcode = 8;

        stream << (quint32)(f.pos() + 4);
    }
    catch (...)
    {
        return Error(Error::Write, errorcode);
    }

    return Error();
}

Error Dictionary::save(const QString &filename)
{
    // To avoid compatibility problems later, Qt stream is only used for the simplest data
    // types.

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly))
        return Error(Error::Access);

    int errorcode = 1;

    writedate = QDateTime::currentDateTimeUtc();

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    try
    {
        stream.writeRawData("zkdat", 5);
        stream.writeRawData(ZKANJI_BASE_FILE_VERSION, 3);
        stream.writeRawData(ZKANJI_DICTIONARY_FILE_VERSION, 3);
        //stream << ZDateTimeStr(basedate);

        errorcode = 2;

        stream << make_zdate(writedate);
        prgversion = QString::fromLatin1(ZKANJI_PROGRAM_VERSION);
        stream << make_zstr(prgversion, ZStrFormat::Byte);

        stream << make_zstr(info);

        errorcode = 3;

        stream << (quint32)words.size();

        // Writing the words.
        for (int ix = 0; ix != words.size(); ++ix)
        {
            WordEntry *w = words[ix];

            stream << make_zstr(w->kanji, ZStrFormat::Byte);
            stream << make_zstr(w->kana, ZStrFormat::Byte);
            stream << make_zstr(w->romaji, ZStrFormat::Byte);

            stream << (quint16)w->freq;
            stream << (quint8)(w->inf & 0xff);
            quint8 cnt = w->defs.size();
            stream << cnt;
            for (int iy = 0; iy != cnt; ++iy)
            {
                const WordDefinition &d = w->defs[iy];
                stream << make_zstr(d.def, ZStrFormat::Word);

                quint8 which = 0;
                if (d.attrib.types != 0)
                    which |= 1;
                if (d.attrib.notes != 0)
                    which |= 2;
                if (d.attrib.fields != 0)
                    which |= 4;
                if (d.attrib.dialects != 0)
                    which |= 8;

                stream << which;

                if (d.attrib.types != 0)
                    stream << (quint32)d.attrib.types;
                if (d.attrib.notes != 0)
                    stream << (quint32)d.attrib.notes;
                if (d.attrib.fields != 0)
                    stream << (quint32)d.attrib.fields;
                if (d.attrib.dialects != 0)
                    stream << (quint16)d.attrib.dialects;

            }
        }

        errorcode = 4;

        // Compress write the rest of the data.

        QByteArray data;
        QDataStream dstream(&data, QIODevice::WriteOnly);

        dtree.save(dstream);
        ktree.save(dstream);
        btree.save(dstream);

        errorcode = 5;

        // Writing kanji data. Words using kanji and user defined kanji meanings. Kanji data is
        // written in blocks. Each block starts with the index of the first kanji in the block
        // and the number of kanji in the block.
        int kfirst = -1;
        int kend = -1;
        KanjiDictData *kd = nullptr;
        for (int ix = 0; ix != ZKanji::kanjis.size() + 1; ++ix)
        {
            if (ix != ZKanji::kanjis.size())
            {
                kd = kanjidata[ix];

                // Until a kanji is found with data to save, we skip everything.
                if (kd->words.empty() /*&& kd->meanings.empty()*/)
                {
                    if (kfirst == -1)
                        continue;

                    kend = ix;
                }
                else if (kfirst != -1)
                    continue;
            }
            else
                kend = ix;

            if (kfirst == -1)
            {
                // First kanji in block found.
                kfirst = ix;
                continue;
            }

            dstream << (quint16)kfirst;
            dstream << (quint16)(kend - kfirst);

            // Saving block between [kfirst, kend)
            for (ix = kfirst; ix != kend; ++ix)
            {
                kd = kanjidata[ix];
                dstream << make_zvec<qint32, qint32>(kd->words);
                //dstream << (quint32)kd->words.size();
                //for (int iy = 0; iy != kd->words.size(); ++iy)
                //    dstream << (quint32)kd->words[iy];

                //dstream << kd->meanings;
            }
            kfirst = -1;
            kend = -1;
        }

        errorcode = 6;

        // To mark the end of the kanadata blocks, the number of kanji is written.
        // No block can start at that index.
        dstream << (quint16)ZKanji::kanjis.size();

        errorcode = 7;

        dstream << (quint16)symdata.size();
        // Writing symdata and kanadata.
        for (auto syms : symdata)
        {
            dstream << (quint16)syms.first;
            dstream << (quint32)syms.second.size();
            for (int ix = 0; ix != syms.second.size(); ++ix)
                dstream << (qint32)syms.second[ix];
        }

        errorcode = 8;

        dstream << (quint8)kanadata.size();
        for (auto kds : kanadata)
        {
            dstream << (quint16)kds.first;
            dstream << (quint32)kds.second.size();
            for (int ix = 0; ix != kds.second.size(); ++ix)
                dstream << (qint32)kds.second[ix];
        }

        errorcode = 9;

        // Writing alphabetic and aiueo ordering. They have the same size as words.
        for (int ix = 0; ix != words.size(); ++ix)
        {
            dstream << (qint32)abcde[ix];
            dstream << (qint32)aiueo[ix];
        }

        errorcode = 10;

        data = qCompress(data);

        errorcode = 11;

        stream << (quint32)data.size();
        stream.writeRawData(data.data(), data.size());

        errorcode = 12;

        stream << (quint32)(f.pos() + 4);

        // Update modified status.
        mod = false;
        emit dictionaryModified(false);
    }
    catch (...)
    {
        return Error(Error::Write, errorcode);
    }

    return true;
}

Error Dictionary::saveUserData(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly))
        return Error::Access;

    int errorcode = 1;

    QDataStream stream(&f);
    stream.setVersion(QDataStream::Qt_5_5);
    stream.setByteOrder(QDataStream::LittleEndian);

    try
    {
        stream.writeRawData("zud", 3);
        stream.writeRawData(ZKANJI_GROUP_FILE_VERSION, 3);

        errorcode = 2;

        QDateTime lastwrite = lastWriteDate();
        stream << make_zdate(lastwrite);

        errorcode = 3;

        if (this == ZKanji::dictionary(0))
        {
            stream << (quint8)1;
            stream << (qint32)ZKanji::originals.size();
            for (int ix = 0; ix != ZKanji::originals.size(); ++ix)
            {
                stream << (quint8)ZKanji::originals.items(ix)->change;
                stream << (qint32)ZKanji::originals.items(ix)->index;

                WordEntry *w = words[ZKanji::originals.items(ix)->index];
                //stream << *words[ZKanji::originals.items(ix)->index];

                stream << make_zstr(w->kanji, ZStrFormat::Byte);
                stream << make_zstr(w->kana, ZStrFormat::Byte);
                stream << make_zstr(w->romaji, ZStrFormat::Byte);

                stream << (uint16_t)w->freq;
                stream << (uint8_t)(w->inf & 0xff);

                uint8_t cnt = w->defs.size();
                stream << cnt;
                for (int iy = 0; iy != cnt; ++iy)
                {
                    const WordDefinition &d = w->defs[iy];
                    stream << make_zstr(d.def, ZStrFormat::Word);
                    stream << (uint32_t)d.attrib.types;
                    stream << (uint32_t)d.attrib.notes;
                    stream << (uint32_t)d.attrib.fields;
                    stream << (uint16_t)d.attrib.dialects;
                }
            }
        }
        else
            stream << (quint8)0;

        errorcode = 4;

        groups->save(stream);

        errorcode = 5;

        // Student data must be saved before anything else which uses the spaced
        // repetition system.
        studydecks->save(stream);

        errorcode = 6;

        decks->save(stream);

        errorcode = 7;

        wordstudydefs.save(stream);

        errorcode = 8;

        // Save a sparse list of kanji data. Only those are saved which have examples or a custom
        // meaning. The data is saved in blocks. Each block starts with the kanji index (16bit
        // ushort) and number of kanji in the block (16bit ushort). Writes a 0 length block after
        // the last one. (The kanji index is not important.)
        for (int ix = 0, siz = kanjidata.size(); ix != siz; ++ix)
        {
            if (kanjidata[ix]->ex.empty() && kanjidata[ix]->meanings.empty())
                continue;

            // Count kanji with data to fit in the next block, where ix is the
            // index of the first kanji in the block.
            int end = ix + 1;
            while (end != kanjidata.size() && (!kanjidata[end]->ex.empty() || !kanjidata[end]->meanings.empty()))
                ++end;

            stream << (quint16)ix;
            stream << (quint16)(end - ix);

            while (ix != end)
            {
                stream << kanjidata[ix]->meanings;
                stream << make_zvec<qint32, qint32>(kanjidata[ix]->ex);
                ++ix;
            }
            if (ix == siz)
                break;
        }

        errorcode = 9;

        stream << (qint32)0;
    }
    catch (...)
    {
        return Error(Error::Write, errorcode);
    }

    // Update modified status.
    usermod = false;
    emit userDataModified(false);

    return true;
}

void Dictionary::exportUserData(const QString &filename, std::vector<KanjiGroup*> &kgroups, bool kexamples, std::vector<WordGroup*> &wgroups, bool usermeanings)
{
    // TODO: error reporting to users, generally in every file operation.

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream stream(&f);
    stream.setCodec("UTF-8");

    // File-format description in English. This won't get translated as the export file is not
    // meant for human reading anyway, but can help others if they wish to use the file.
    QString desc =
        "# zkanji user data export file. version 0\n"
        "#\n"
        "# This file was computer generated. While readable, this file was not meant to\n"
        "# be edited by hand. Do it at your own risk!\n"
        "#\n"
        "# Most data items are separated by space characters. If space is a valid\n"
        "# character inside a simple item, it's separated by TAB characters instead.\n"
        "# Names are separated by the TAB character and are encoded by escaping the /,\n"
        "# the [ and ^ characters with ^ in names, and using / as separator.\n"
        "#\n"
        "# The file consists of sections with headers between [] characters. Each section\n"
        "# has its own format described below.\n"
        "#\n"
        "# The [StudyDefinitions] section lists user defined word meanings for every exported\n"
        "# word, which has one. Each line in the section has the format:\n"
        "#\n"
        "# written_form(kana_reading)(TAB)user definition\n"
        "#\n"
        "# The parentheses around the kana_reading are included in the format, but (TAB)\n"
        "# marks the TAB character.\n"
        "#\n"
        "# The [KanjiWordExamples] section lists the kanji in the exported kanji groups\n"
        "# and the words selected for that kanji as example words. Data of a single\n"
        "# kanji can span multiple lines, but each line must start with the kanji:\n"
        "#\n"
        "# kanji_character(SPACE)words_list\n"
        "#\n"
        "# The words_list separates each word with spaces. The format of a single word:\n"
        "# written_form(kana_reading)\n"
        "#\n"
        "# The parentheses around the kana_reading are included in the format.\n"
        "#\n"
        "# The [KanjiGroups] section lists the exported kanji groups and the kanji inside\n"
        "# them. Data of a single group can span several lines, but every line has the\n"
        "# same format. The group name must be repeated for every line referring to that\n"
        "# group:\n"
        "#\n"
        "# G:(SPACE)group_name(TAB)kanji_list\n"
        "#\n"
        "# The kanji_list consists of kanji characters in UTF-8 without spaces.\n"
        "#\n"
        "# The [WordGroups] similarly to KanjiGroups lists the groups of words, and the\n"
        "# words inside them. The format is:\n"
        "#\n"
        "# G:(SPACE)group_name(TAB)words_list\n"
        "#\n"
        "# The words_list separates each word with spaces. The format of a single word:\n"
        "# written_form(kana_reading)\n"
        "#\n"
        "# The parentheses around the kana_reading are included in the format.\n"
        "#\n";
    stream << desc;

    // Word indexes paired with their user definition string. Each item is
    // unique.
    std::vector<std::pair<int, const QCharString&>> defs;
    // Indexes of words already checked for user definition. If the index is
    // in found, it shouldn't be added to defs.
    std::set<int> found;

    // Word examples selected for kanji. The numbers in the pair are:
    // <kanji_index, word_index_list>
    std::vector<std::pair<int, const std::vector<int>&>> kwords;
    // Indexes of kanji already checked for example words.
    std::set<int> kfound;

    // Check if there are words as kanji examples, and whether there are user definitions for
    // them (unless usermeanings is false).
    for (int ix = 0; (kexamples || usermeanings) && ix != kgroups.size(); ++ix)
    {
        KanjiGroup *g = kgroups[ix];
        for (int iy = 0; (kexamples || usermeanings) && iy != g->size(); ++iy)
        {
            int kix = g->items(iy)->index;
            const std::vector<int> &ex = kanjidata[kix]->ex;

            if (kexamples && !kfound.count(kix))
            {
                kfound.insert(kix);
                kwords.push_back(std::make_pair(kix, ex));
            }

            for (int iz = 0; usermeanings && iz != ex.size(); ++iz)
            {
                int wix = ex[iz];
                if (found.count(wix) != 0)
                    continue;
                found.insert(wix);
                const QCharString *str = wordstudydefs.itemDef(wix);
                if (str == nullptr)
                    continue;
                defs.push_back(std::pair<int, const QCharString&>(wix, *str));
            }
        }
    }

    // Check for user definitions for words in words list.
    for (int ix = 0; usermeanings && ix != wgroups.size(); ++ix)
    {
        WordGroup *g = wgroups[ix];
        const std::vector<int> &indexes = g->getIndexes();
        for (int iy = 0; iy != indexes.size(); ++iy)
        {
            int wix = indexes[iy];
            if (found.count(wix))
                continue;
            found.insert(wix);
            const QCharString *str = wordstudydefs.itemDef(wix);
            if (str == nullptr)
                continue;
            defs.push_back(std::pair<int, const QCharString&>(wix, *str));
        }
    }

    if (!defs.empty())
    {
        // Writing user definitions section.
        stream << "[StudyDefinitions]\n";
        for (int ix = 0; ix != defs.size(); ++ix)
        {
            WordEntry *w = words[defs[ix].first];
            stream << w->kanji.toQStringRaw() << "(" << w->kana.toQStringRaw() << ")" << "\t" << defs[ix].second.toQStringRaw() << "\n";
        }
        stream << "\n";
    }

    if (!kwords.empty())
    {
        // Writing kanji example words section. Only write at most 4 example
        // words on each line to avoid too long lines.
        stream << "[KanjiWordExamples]";
        for (int ix = 0; ix != kwords.size(); ++ix)
        {
            const std::vector<int> &ex = kwords[ix].second;
            for (int iy = 0; iy != ex.size(); ++iy)
            {
                bool newline = (iy % 4) == 0;
                if (newline)
                    stream << "\n" << ZKanji::kanjis[kwords[ix].first]->ch;
                WordEntry *w = words[ex[iy]];
                stream << " " << w->kanji.toQStringRaw() << "(" << w->kana.toQStringRaw() << ")";
            }
        }
        stream << "\n\n";
    }

    if (!kgroups.empty())
    {
        // Writing kanji groups and kanji inside them. At most 32 kanji is
        // written on a single line to avoid long lines.
        stream << "[KanjiGroups]";
        for (int ix = 0; ix != kgroups.size(); ++ix)
        {
            KanjiGroup *g = kgroups[ix];
            if (g->isEmpty())
            {
                // Write the name of the empty group too, because it was
                // exported for a reason.
                stream << "\nG: " << g->fullEncodedName();
                continue;
            }

            const std::vector<ushort> &indexes = g->getIndexes();
            for (int iy = 0; iy != g->size(); ++iy)
            {
                bool newline = (iy % 32) == 0;
                if (newline)
                    stream << "\nG: " << g->fullEncodedName() << "\t";
                stream << ZKanji::kanjis[indexes[iy]]->ch;
            }
        }
        stream << "\n\n";
    }

    if (!wgroups.empty())
    {
        // Writing word groups and words inside them. Only write at most 4
        // words on each line to avoid too long lines.
        stream << "[WordGroups]";
        for (int ix = 0; ix != wgroups.size(); ++ix)
        {
            WordGroup *g = wgroups[ix];
            if (g->isEmpty())
            {
                // Write the name of the empty group too, because it was
                // exported for a reason.
                stream << "\nG: " << g->fullEncodedName();
                continue;
            }

            const std::vector<int> &indexes = g->getIndexes();
            for (int iy = 0; iy != g->size(); ++iy)
            {
                bool newline = (iy % 4) == 0;
                if (newline)
                    stream << "\nG: " << g->fullEncodedName();

                WordEntry *w = words[indexes[iy]];
                stream << (newline ? "\t" : " ") << w->kanji.toQStringRaw() << "(" << w->kana.toQStringRaw() << ")";
            }
        }
        stream << "\n\n";
    }
}

void Dictionary::exportDictionary(const QString &filename, bool limit, const std::vector<ushort> &kanjilimit, const std::vector<int> wordlimit)
{
    // TODO: error reporting to users, generally in every file operation.

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream stream(&f);
    stream.setCodec("UTF-8");

    // File-format description in English. This won't get translated as the export file is not
    // meant for human reading anyway, but can help others if they wish to use the file.
    QString desc =
        "# zkanji dictionary export file. version 0\n"
        "#\n"
        "# This file was computer generated. While readable, this file was not meant to\n"
        "# be edited by hand. Do it at your own risk!\n"
        "#\n"
        "# The file consists of sections with headers between [] characters and is case\n"
        "# sensitive. Each section has its own format described below.\n"
        "#\n"
        "# The actual data starts at the first section, anything above it is comment.\n"
        "# To mark a line as comment below, start the line with the # character. Every\n"
        "# other character not matching the file format is an error and will cause the\n"
        "# importer to fail. Empty lines are ignored even in the middle of sections.\n"
        "#\n"
        "# The [About] section contains the information typed in by the dictionary author\n"
        "# as the information text for the dictionary. This section consists of the raw\n"
        "# text on several lines. Lines are broken at either 255 characters or at a\n"
        "# newline character. Each line either starts with * or -, depending whether the\n"
        "# line started at the start of a newline (*) or in the middle of a line (-).\n"
        "# Even if the text holds HTML or other tags, they will be shown as plain text\n"
        "# after the import.\n"
        "# Export file generated from the base dictionary starts with a section marked by\n"
        "# [Base], instead of [About]. The following lines hold the legal text for the\n"
        "# base dictionary. This text is ignored during an import.\n"
        "#\n"
        "# The [Words] section lists the dictionary words in random order. Every word is\n"
        "# in its own paragraph and every paragraph in the section has the format:\n"
        "#\n"
        "# written_form(kana_reading)(SPACE)frequency_number(SPACE)information_field\n"
        "# meaning_lines\n"
        "#\n"
        "# The format of each meaning line:\n"
        "# D:(SPACE)types_notes_fields_dialects(TAB)definition_text\n"
        "#\n"
        "# Both the information_field for the word and the types_notes_fields_dialects\n"
        "# for the meaning lines are semicolon separated lists of keywords. The\n"
        "# definition_text might include many spaces, but those are considered part of\n"
        "# the definition. Each definition_text can hold several definitions for a single\n"
        "# meaning, separated by the break-permitted-here unicode character.\n"
        "#\n"
        "# *The parentheses around the kana_reading are included in the format, but not\n"
        "# around the SPACE or TAB characters.\n"
        "#\n"
        "# The [KanjiDefinitions] section lists the user defined definition for each\n"
        "# kanji. Kanji without user definition are not listed\n"
        "#\n"
        "# kanji_character(TAB)kanji_definition\n"
        "#\n"
        "# The kanji_definition can hold several meanings, each of which are separted by\n"
        "# a break-permitted-here unicode space character.\n"
        "\n";
    stream << desc;

    if (this == ZKanji::dictionary(0))
    {
        stream << "[Base]\n";
        stream << QString("This file holds a compilation of the JMdict (http://www.edrdg.org/jmdict/j_jmdict.html)\n"
            "dictionary file which is the property of The Electronic Dictionary Research and Development\n"
            "Group, Monash University.\n"
            "The file is made available under a Creative Commons Attribution-ShareAlike Licence (V3.0).\n"
            "The group can be found at: http://www.edrdg.org/") << "\n\n";
    }
    else if (!info.isEmpty())
    {
        stream << "[About]\n";
        QStringList infs = info.split("\n");

        for (int ix = 0, siz = infs.size(); ix != siz; ++ix)
        {
            QString str = infs.at(ix);
            stream << "*";
            do
            {
                stream << str.left(255) << "\n";
                str = str.mid(255);
                if (!str.isEmpty())
                    stream << "-";
            } while (!str.isEmpty());
        }
    }


    if (!limit || !wordlimit.empty())
        stream << "[Words]\n";

    for (int ix = 0, siz = !limit ? words.size() : wordlimit.size(); ix != siz; ++ix)
    {
        WordEntry *e = words[!limit ? ix : wordlimit[ix]];
        stream << QString("%1(%2) %3 %4\n").arg(e->kanji.toQStringRaw()).arg(e->kana.toQStringRaw()).arg(e->freq).arg(Strings::wordInfoTags(e->inf));

        for (int iy = 0, siy = e->defs.size(); iy != siy; ++iy)
        {
            auto &def = e->defs[iy];
            stream << QString("D: %1%2%3%4\t%5\n").arg(Strings::wordTypeTags(def.attrib.types)).arg(Strings::wordNoteTags(def.attrib.notes)).arg(Strings::wordFieldTags(def.attrib.fields)).arg(Strings::wordDialectTags(def.attrib.dialects)).arg(def.def.toQStringRaw());
        }
    }

    if (!limit || !kanjilimit.empty())
    {
        stream << "\n";
        stream << "[KanjiDefinitions]\n";
    }
    for (int ix = 0, siz = !limit ? kanjidata.size() : kanjilimit.size(); ix != siz; ++ix)
    {
        int kix = !limit ? ix : kanjilimit[ix];
        if (kanjidata[kix]->meanings.empty())
            continue;
        QChar ch = ZKanji::kanjis[kix]->ch;
        KanjiDictData *dat = kanjidata[kix];
        stream << ch << "\t";
        for (int iy = 0, siy = dat->meanings.size(); iy != siy; ++iy)
        {
            if (iy != 0)
                stream << GLOSS_SEP_CHAR;
            stream << dat->meanings[iy].toQStringRaw();
        }
        stream << "\n";
    }
}

//void Dictionary::importUserData(const QString &filename, KanjiGroupCategory *kanjiroot, WordGroupCategory *wordsroot)
//{
//    QFile f(filename);
//    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
//        return;
//
//    QTextStream stream(&f);
//    stream.setCodec("UTF-8");
//
//    enum ReadCategory { NoCateg, UserDefs, KanjiEx, KanjiGroups, WordGroups };
//    ReadCategory categ;
//
//    QString kanji;
//    QString kana;
//
//    while (!stream.atEnd())
//    {
//        QString line = stream.readLine();
//        if (line.size() == 0 || line.at(0) == QChar('#'))
//            continue;
//
//        if (line.left(10) == QStringLiteral("[UserDefs]"))
//        {
//            categ = UserDefs;
//            continue;
//        }
//        else if (line.left(19) == QStringLiteral("[KanjiWordExamples]"))
//        {
//            categ = KanjiEx;
//            continue;
//        }
//        else if (line.left(13) == QStringLiteral("[KanjiGroups]"))
//        {
//            categ = KanjiGroups;
//            continue;
//        }
//        else if (line.left(12) == QStringLiteral("[WordGroups]"))
//        {
//            categ = WordGroups;
//            continue;
//        }
//        //else if (line.at(0) == QChar('[')) - TO-DO: Kanji form of word might start with [, check that later
//        //    categ = NoCateg;
//
//        if (categ == NoCateg)
//            continue;
//
//        if (categ == UserDefs)
//        {
//            // kanji(kana)TAB[userdef]
//            int tabpos = line.indexOf('\t');
//
//            // No tab, invalid line.
//            if (tabpos < 3)
//                continue;
//
//            if (!importedWordStrings(line, 0, tabpos, kanji, kana))
//                continue;
//
//            int windex = findKanjiKanaWord(kanji, kana);
//            if (windex == -1)
//                continue;
//
//            QString def = line.right(line.size() - tabpos - 1);
//
//            setWordStudyDefinition(windex, def);
//
//            continue;
//        }
//
//        if (categ == KanjiEx)
//        {
//            if (line.size() < 6 || line.at(1) != QChar(' '))
//                continue;
//
//            int kindex = ZKanji::kanjiIndex(line.at(0));
//
//            if (kindex == -1)
//                continue;
//
//            int pos = 3;
//
//            int windex;
//            do
//            {
//                int endpos = line.indexOf(QChar(' '), pos);
//                if (endpos == -1)
//                    endpos = line.size();
//
//                windex = importedWordStrings(line, pos, endpos - pos, kanji, kana);
//                if (windex == -1)
//                    break;
//
//                if (wordEntry(windex)->kanji.find(line.at(0)) != -1)
//                    addKanjiExample(kindex, windex);
//
//                pos = endpos + 1;
//
//            } while (pos < line.size());
//
//            continue;
//        }
//
//        if (categ == KanjiGroups)
//        {
//            // group_name(TAB)kanji_list
//
//            int tabpos = line.indexOf('\t');
//
//            // No tab, invalid line.
//            if (tabpos < 1)
//                continue;
//
//            KanjiGroup *g = groups->kanjiGroups().groupFromEncodedName(line, 0, tabpos, true);
//            if (g == nullptr)
//                continue;
//        }
//
//        if (categ == WordGroups)
//        {
//            // group_name(TAB)words_list
//
//            int tabpos = line.indexOf('\t');
//
//            // No tab, invalid line.
//            if (tabpos < 1)
//                continue;
//
//            WordGroup *g = groups->wordGroups().groupFromEncodedName(line, 0, tabpos, true);
//            if (g == nullptr)
//                continue;
//        }
//    }
//}

//void Dictionary::clear()
//{
//    dtree.clear();
//    ktree.clear();
//    btree.clear();
//}

void Dictionary::saveImport(const QString &path)
{
    saveBase(path + "/zdict.zkj");
    save(path + QString("/%1.zkj").arg(dictname));
}

void Dictionary::setName(const QString &newname)
{
    dictname = newname;
}

QString Dictionary::name() const
{
    return dictname;
}

QDateTime Dictionary::baseDate() const
{
    return basedate;
}

QDateTime Dictionary::lastWriteDate() const
{
    return writedate;
}

void Dictionary::setProgramVersion()
{
    prgversion = QString::fromLatin1(ZKANJI_PROGRAM_VERSION);
}

QString Dictionary::programVersion() const
{
    return prgversion;
}

QString Dictionary::infoText() const
{
    return info;
}

void Dictionary::setInfoText(QString text)
{
    info = text;
}

void Dictionary::swapDictionaries(Dictionary *src, std::map<int, int> &changes)
{
    //basedate.swap(src->basedate);

    writedate.swap(src->writedate);
    prgversion.swap(src->prgversion);
    dictname.swap(src->dictname);
    info.swap(src->info);
    std::swap(words, src->words);
    dtree.swap(src->dtree);
    ktree.swap(src->ktree);
    btree.swap(src->btree);
    std::swap(kanjidata, src->kanjidata);
    std::swap(symdata, src->symdata);
    std::swap(kanadata, src->kanadata);
    std::swap(abcde, src->abcde);
    std::swap(aiueo, src->aiueo);
    // Saving user data in the source dictionary, to be able to restore them on an error.
    src->wordstudydefs.copy(&wordstudydefs);
    src->groups->copy(groups);
    //src->studydecks->copy(studydecks);
    src->studydecks.reset(new StudyDeckList);
    src->decks->copy(decks);

    for (int ix = 0, siz = src->kanjidata.size(); ix != siz; ++ix)
    {
        if (src->kanjidata[ix]->ex.empty())
            continue;

        std::set<int> found;
        kanjidata[ix]->ex.clear();
        kanjidata[ix]->ex.reserve(src->kanjidata[ix]->ex.size());
        for (int iy = 0, siy = src->kanjidata[ix]->ex.size(); iy != siy; ++iy)
        {
            int val = changes[src->kanjidata[ix]->ex[iy]];

            if (val == -1 || found.count(val) != 0)
                continue;

            kanjidata[ix]->ex.push_back(val);
            found.insert(val);
        }
    }

    wordstudydefs.applyChanges(changes);
    groups->applyChanges(changes);
    decks->applyChanges(src, changes);

    emit dictionaryReset();
}

void Dictionary::restoreChanges(Dictionary *src)
{
    writedate.swap(src->writedate);
    prgversion.swap(src->prgversion);
    dictname.swap(src->dictname);
    info.swap(src->info);
    std::swap(words, src->words);
    dtree.swap(src->dtree);
    ktree.swap(src->ktree);
    btree.swap(src->btree);
    std::swap(kanjidata, src->kanjidata);
    std::swap(symdata, src->symdata);
    std::swap(kanadata, src->kanadata);
    std::swap(abcde, src->abcde);
    std::swap(aiueo, src->aiueo);
    // Saving user data in the source dictionary, to be able to restore them on an error.
    wordstudydefs.copy(&src->wordstudydefs);
    groups->copy(src->groups);

    studydecks.reset(new StudyDeckList);
    decks->copy(src->decks);

    emit dictionaryReset();
}

bool Dictionary::isModified() const
{
    return mod;
}

void Dictionary::setToModified()
{
    if (mod == false)
        return;
    mod = true;
    emit dictionaryModified(true);
}

bool Dictionary::isUserModified() const
{
    return usermod;
}

void Dictionary::setToUserModified()
{
    if (usermod)
        return;

    usermod = true;
    emit userDataModified(true);
}

//void Dictionary::applyChangesInOriginals()
//{
//    std::map<int, int> m(mapOriginalWords());
//
//    for (int ix = 0; ix != ZKanji::originals.size(); ++ix)
//    {
//        OriginalWord *o = ZKanji::originals.items(ix);
//        int newindex = m[o->index];
//        if (newindex == -1)
//            ZKanji::originals.remove(ix--);
//        else
//            o->index = newindex;
//    }
//}

WordGroups& Dictionary::wordGroups()
{
    return groups->wordGroups();
}

KanjiGroups& Dictionary::kanjiGroups()
{
    return groups->kanjiGroups();
}

const WordGroups& Dictionary::wordGroups() const
{
    return groups->wordGroups();
}

const KanjiGroups& Dictionary::kanjiGroups() const
{
    return groups->kanjiGroups();
}

int Dictionary::entryCount() const
{
    return words.size();
}

WordEntry* Dictionary::wordEntry(int ix)
{
    return words[ix];
}

const WordEntry* Dictionary::wordEntry(int ix) const
{
    return words[ix];
}

//int Dictionary::createEntry(const QString &kanji, const QString &kana, ushort freq, uint inf/*, QString defstr, const WordDefAttrib &attrib*/)
//{
//
//    int ix = findKanjiKanaWord(kanji.constData(), kana.constData());
//    if (ix != -1)
//        return -1;
//
//    WordEntry *e = new WordEntry;
//    e->kanji.copy(kanji.constData());
//    e->kana.copy(kana.constData());
//    e->romaji.copy(romanize(e->kana).constData());
//    e->freq = freq;
//    e->inf = inf;
//    //e->defs.resize(1);
//    //WordDefinition &def = e->defs[1];
//    //def.def.copy(defstr.constData());
//    //def.attrib = attrib;
//
//    words.push_back(e);
//    addWordData();
//
//    if (this == ZKanji::dictionary(0))
//        ZKanji::originals.createAdded(words.size() - 1, e->kanji.data(), e->kana.data());
//
//    return words.size() - 1;
//}

void Dictionary::removeEntry(int windex)
{
    //emit entryAboutToBeRemoved(windex);

    if (this == ZKanji::dictionary(0))
    {
        if (ZKanji::originals.processRemovedWord(windex))
            setToUserModified();
    }

    int aiueoix;
    int abcdeix;
    removeWordData(windex, abcdeix, aiueoix);

    groups->processRemovedWord(windex);
    wordstudydefs.processRemovedWord(windex);
    decks->processRemovedWord(windex);

    words.erase(words.begin() + windex);

    emit entryRemoved(windex, abcdeix, aiueoix);

    if (this != ZKanji::dictionary(0))
        setToModified();
}

bool Dictionary::isUserEntry(int windex) const
{
    if (windex == -1 || this != ZKanji::dictionary(0))
        return false;

    const OriginalWord *o = ZKanji::originals.find(windex);
    if (o == nullptr)
        return false;
    return o->change == OriginalWord::Added;
}

bool Dictionary::isModifiedEntry(int windex) const
{
    if (windex == -1 || this != ZKanji::dictionary(0))
        return false;

    const OriginalWord *o = ZKanji::originals.find(windex);
    if (o == nullptr)
        return false;
    return o->change == OriginalWord::Modified;
}

//void Dictionary::changeWordAttrib(int windex, int wfreq, int winf)
//{
//    WordEntry *e = words[windex];
//    if (this == ZKanji::dictionary(0) && ZKanji::originals.find(windex) == nullptr)
//        ZKanji::originals.createModified(windex, e);
//
//    e->freq = wfreq;
//    e->inf = winf;
//
//    emit entryChanged(windex);
//}

//void Dictionary::addWordDefinition(int windex, QString defstr, const WordDefAttrib &attrib)
//{
//    WordEntry *e = words[windex];
//    if (this == ZKanji::dictionary(0) && ZKanji::originals.find(windex) == nullptr)
//        ZKanji::originals.createModified(windex, e);
//
//    dtree.removeLine(windex, false);
//
//    e->defs.resize(e->defs.size() + 1);
//    WordDefinition &def = e->defs[e->defs.size() - 1];
//    def.def.copy(defstr.constData());
//    def.attrib = attrib;
//
//    dtree.expandWith(windex);
//
//    emit entryChanged(windex);
//}

//void Dictionary::changeWordDefinition(int windex, int dindex, QString defstr, const WordDefAttrib &attrib)
//{
//    WordEntry *e = words[windex];
//    if (this == ZKanji::dictionary(0) && ZKanji::originals.find(windex) == nullptr)
//        ZKanji::originals.createModified(windex, e);
//
//    dtree.removeLine(windex, false);
//
//    WordDefinition &def = e->defs[dindex];
//    def.def.copy(defstr.constData());
//    def.attrib = attrib;
//
//    dtree.expandWith(windex);
//
//    emit entryChanged(windex, dindex);
//}

//void Dictionary::removeWordDefinition(int windex, int dindex)
//{
//    WordEntry *e = words[windex];
//
//#ifdef _DEBUG
//    if (e->defs.size() == 1)
//        throw "Call removeEntry() when removing the last definition of an entry.";
//#endif
//
//    if (this == ZKanji::dictionary(0) && ZKanji::originals.find(windex) == nullptr)
//        ZKanji::originals.createModified(windex, e);
//
//    dtree.removeLine(windex, false);
//
//    e->defs.erase(dindex);
//
//    dtree.expandWith(windex);
//
//    emit entryDefinitionRemoved(windex, dindex);
//}

const std::vector<int>& Dictionary::wordOrdering(BrowseOrder order) const
{
    if (order == BrowseOrder::ABCDE)
        return abcde;
    return aiueo;
}

WordDeckList* Dictionary::wordDecks()
{
    return decks;
}

StudyDeckList* Dictionary::studyDecks()
{
    return studydecks.get();
}

//int Dictionary::wordDeckCount() const
//{
//    return worddecks.size();
//}
//
//WordDeck* Dictionary::wordDeck(int index)
//{
//    return worddecks[index];
//}
//
//bool Dictionary::renameWordDeck(int index, const QString &name)
//{
//    QString val = name.trimmed().left(255);
//
//    for (int ix = 0, siz = worddecks.size(); ix != siz; ++ix)
//        if (worddecks[ix]->getName() == name)
//            return false;
//    worddecks[index]->setName(val);
//    return true;
//}
//
//void Dictionary::moveDecks(const smartvector<Range> &ranges, int pos)
//{
//    if (ranges.empty())
//        return;
//
//    if (pos == -1)
//        pos = worddecks.size();
//
//    _moveRanges(ranges, pos, worddecks /*[this](const Range &r, int pos) { _moveRange(worddecks, r, pos); }*/);
//
//    setToUserModified();
//
//    emit decksMoved(ranges, pos);
//}

QString Dictionary::wordDefinitionString(int index, bool numbers) const
{
    auto &defs = words[index]->defs;
    QString result;

    bool drawnumbers = numbers && defs.size() != 1;

    QString separator = tr(", ");

    for (int ix = 0, siz = defs.size(); ix != siz; ++ix)
    {
        if (drawnumbers)
            result += QString::number(ix + 1) + ") ";
        QString str = defs[ix].def.toQString();
        str.replace(GLOSS_SEP_CHAR, separator);
        result += str;

        if (drawnumbers && ix != siz - 1)
            result += " ";
        else if (!drawnumbers && ix != siz - 1)
            //: Word definition separator when numbers or word tags are not provided.
            result += tr("; ");
    }

    return result;
}

QString Dictionary::displayedStudyDefinition(int index) const
{
    const QCharString *chr = wordstudydefs.itemDef(index);
    if (chr != nullptr)
        return chr->toQStringRaw();
    return wordDefinitionString(index, false);
}

const QCharString* Dictionary::studyDefinition(int index) const
{
    return wordstudydefs.itemDef(index);
}

void Dictionary::setWordStudyDefinition(int index, QString def)
{
    if (def == wordDefinitionString(index, false))
        def.clear();
    if (wordstudydefs.setDefinition(index, def))
    {
        setToUserModified();
        emit entryChanged(index, true);
    }
}

//void Dictionary::clearWordStudyDefinition(int index)
//{
//    auto it = wordstudydefs.find(index);
//    if (it != wordstudydefs.end())
//        wordstudydefs.erase(it);
//}

//const KanjiDictData* Dictionary::kanjiData(int kindex) const
//{
//    return kanjidata[kindex];
//}

void Dictionary::getKanjiWords(short kindex, std::vector<int> &dest) const
{
    dest = kanjidata[kindex]->words;
}

void Dictionary::getKanjiWords(const std::vector<ushort> &kanji, std::vector<int> &dest) const
{
    for (int ix = 0, siz = kanji.size(); ix != siz; ++ix)
    {
        const std::vector<int> &w = kanjidata[kanji[ix]]->words;
        dest.insert(dest.end(), w.begin(), w.end());
    }
    std::sort(dest.begin(), dest.end());
    dest.resize(std::unique(dest.begin(), dest.end()) - dest.begin());
}

int Dictionary::kanjiWordCount(short kindex) const
{
    return kanjidata[kindex]->words.size();
}

int Dictionary::kanjiWordAt(short kindex, int ix) const
{
    return kanjidata[kindex]->words[ix];
}

void Dictionary::addKanjiExample(short kindex, int windex)
{
    std::vector<int> &ex = kanjidata[kindex]->ex;
    if (std::find(ex.begin(), ex.end(), windex) != ex.end())
        return;

    ex.push_back(windex);
    setToUserModified();
    emit kanjiExampleAdded(kindex, windex);
}

void Dictionary::removeKanjiExample(short kindex, int windex)
{
    std::vector<int> &ex = kanjidata[kindex]->ex;
    auto it = std::find(ex.begin(), ex.end(), windex);
    if (it == ex.end())
        return;

    //emit kanjiExampleAboutToBeRemoved(kindex, windex);
    ex.erase(it);
    setToUserModified();
    emit kanjiExampleRemoved(kindex, windex);
}

bool Dictionary::isKanjiExample(short kindex, int windex) const
{
    const std::vector<int> &ex = kanjidata[kindex]->ex;
    return std::find(ex.begin(), ex.end(), windex) != ex.end();
}

bool Dictionary::hasKanjiMeaning(short ix) const
{
    return !kanjidata[ix]->meanings.empty();
}

QString Dictionary::kanjiMeaning(short ix, QString joinstr) const
{
    if (kanjidata[ix]->meanings.empty())
        return ZKanji::kanjis[ix]->meanings.join(joinstr);
    return kanjidata[ix]->meanings.join(joinstr);
}

void Dictionary::setKanjiMeaning(short ix, QString str)
{
    kanjidata[ix]->meanings.clear();

    QStringList meanings = str.split('\n', QString::SkipEmptyParts);
    for (const QString &meaning : meanings)
    {
        QString trimmed = meaning.trimmed();
        kanjidata[ix]->meanings.add(trimmed.constData(), trimmed.size());
    }

    if (ZKanji::kanjis[ix]->meanings == kanjidata[ix]->meanings)
        kanjidata[ix]->meanings.clear();

    emit kanjiMeaningChanged(ix);
    setToUserModified();
}

void Dictionary::setKanjiMeaning(short ix, QStringList &list)
{
    kanjidata[ix]->meanings.clear();
    for (const QString &meaning : list)
    {
        QString trimmed = meaning.trimmed();
        kanjidata[ix]->meanings.add(trimmed.constData(), trimmed.size());
    }

    if (ZKanji::kanjis[ix]->meanings == kanjidata[ix]->meanings)
        kanjidata[ix]->meanings.clear();

    emit kanjiMeaningChanged(ix);
    setToUserModified();
}

//WordResultList&& Dictionary::browseWords(WordResultList &&result, BrowseOrder order, const WordFilterConditions *conditions) const
//{
//    std::vector<int> list = order == BrowseOrder::ABCDE ? abcde : aiueo;
//    if (conditions == nullptr)
//        result.set(list);
//    else
//    {
//        result.clear();
//        for (int ix = 0; ix != list.size(); ++ix)
//        {
//            if (ZKanji::wordfilters().match(wordEntry(list[ix]), conditions))
//                result.add(list[ix]);
//        }
//    }
//
//    return std::move(result);
//}

WordResultList&& Dictionary::findWords(WordResultList &&result, SearchMode searchmode, QString search, SearchWildcards wildcards, bool sameform, bool inflections, bool studydefs, const std::vector<int> *wordpool, const WordFilterConditions *conditions, bool sort)
{
#ifdef _DEBUG
    if (searchmode == SearchMode::Browse)
        throw "Call browseWords with the wanted browse order instead.";
#endif

    if (search.isEmpty())
        return std::move(result);

    std::vector<int> wpool;
    if (wordpool != nullptr)
    {
        wpool = *wordpool;
        std::sort(wpool.begin(), wpool.end());
        wpool.resize(std::unique(wpool.begin(), wpool.end()) - wpool.begin());
    }

    if (searchmode == SearchMode::Japanese)
    {
        // Remove any romaji character from the search string because we want to
        // find results even while in the middle of typing in the search box.
        for (int ix = search.size() - 1; ix != -1; --ix)
            if (!JAPAN(search.at(ix).unicode()))
                search.remove(ix, 1);

        if (search.isEmpty())
            return std::move(result);

        bool kanjisearch = false;
        // Look for kanji or other non-kana character in the search string.
        for (int ix = 0; !kanjisearch && ix < search.size(); ++ix)
        {
            ushort ch = search.at(ix).unicode();
            if (VALIDCODE(ch) || KANJI(ch))
                kanjisearch = true;
        }


        std::vector<int> lines;
        if (kanjisearch)
            findKanjiWords(lines, search, wildcards, sameform, wordpool != nullptr ? &wpool : nullptr, conditions);
        else
            findKanaWords(lines, search, wildcards, sameform, wordpool != nullptr ? &wpool : nullptr, conditions);

        // Searching for deinflected results must end with the deinflected form.
        wildcards &= ~(int)SearchWildcard::AnyAfter;

        smartvector<InflectionForm> deinfs;
        if (inflections)
            deinflect(search, deinfs);

        result.set(lines);
        if (deinfs.empty())
        {
            if (sort)
                result.jpSort();
            return std::move(result);
        }

        for (int ix = 0; ix != deinfs.size(); ++ix)
        {
            std::vector<int> tmp;
            std::vector<std::vector<InfTypes>*> inftmp;
            int foundcnt = 0;

            if (kanjisearch)
                findKanjiWords(tmp, deinfs[ix]->form, wildcards, sameform, wordpool != nullptr ? &wpool : nullptr, conditions, deinfs[ix]->infsize);
            else
                findKanaWords(tmp, deinfs[ix]->form, wildcards, sameform, wordpool != nullptr ? &wpool : nullptr, conditions, deinfs[ix]->infsize);
            inftmp.resize(tmp.size());

            for (int j = 0; j != tmp.size(); ++j)
            {
                bool found = false;
                for (int k = 0; !found && k != result.dictionary()->wordEntry(tmp[j])->defs.size(); ++k)
                    if ((result.dictionary()->wordEntry(tmp[j])->defs[k].attrib.types & (1 << (int)deinfs[ix]->type)) != 0/* ||
                        ((result.dictionary()->wordEntry(tmp[j])->defs[k].types & (1 << (int)WordTypes::AuxAdj)) != 0
                        && deinfs[ix]->type == WordTypes::TrueAdj)*/)
                        found = true;
                if (!found)
                    tmp[j] = -1; // No match, disable this index. We check for -1 just below when adding to result. It's not used elsewhere.
                else
                {
                    inftmp[j] = &deinfs[ix]->inf;
                    ++foundcnt;
                }
            }

            result.reserve(result.size() + foundcnt, true);

            for (int j = 0; j != tmp.size(); ++j)
            {
                if (tmp[j] == -1)
                    continue;
                result.add(tmp[j], *inftmp[j]);
            }
        }

        if (sort)
            result.jpSort();
        return std::move(result);
    }

    if (searchmode == SearchMode::Definition)
    {
        std::vector<int> lines;

        std::vector<int> studyexclude;
        if (studydefs)
        {
            wordstudydefs.findWords(lines, search, (wildcards & SearchWildcard::AnyAfter) == 0, sameform, wordpool != nullptr ? &wpool : nullptr, conditions);
            wordstudydefs.listWordIndexes(studyexclude);

            if (wordpool != nullptr)
            {
                // Both wpool and studyexclude are ordered. After this wpool shouldn't include
                // anything found in studyexclude.
                int spos = studyexclude.size() - 1;
                int wpos = wpool.size() - 1;
                while (spos != -1 && wpos != -1)
                {
                    if (wpool[wpos] > studyexclude[spos])
                    {
                        --wpos;
                        continue;
                    }
                    if (wpool[wpos] < studyexclude[spos])
                    {
                        --spos;
                        continue;
                    }
                    wpool[wpos] = -1;
                    --wpos;
                    --spos;
                }
                wpool.resize(std::remove(wpool.begin(), wpool.end(), -1) - wpool.begin());
            }
        }

        dtree.findWords(lines, search, (wildcards & SearchWildcard::AnyAfter) == 0, sameform, wordpool != nullptr ? &wpool : nullptr, conditions);
        if (studydefs && wordpool == nullptr)
        {
            // Remove anything from lines found in wordstudydefs.
            std::sort(lines.begin(), lines.end());

            int spos = studyexclude.size() - 1;
            int lpos = lines.size() - 1;
            while (spos != -1 && lpos != -1)
            {
                if (lines[lpos] > studyexclude[spos])
                {
                    --lpos;
                    continue;
                }
                if (lines[lpos] < studyexclude[spos])
                {
                    --spos;
                    continue;
                }
                lines[lpos] = -1;
                --lpos;
                --spos;
            }
            lines.resize(std::remove(lines.begin(), lines.end(), -1) - lines.begin());
        }

        result.set(lines);
        if (sort)
            result.defSort(search);
        return std::move(result);
    }

    return std::move(result);
}

bool Dictionary::wordMatches(int windex, SearchMode searchmode, QString search, SearchWildcards wildcards, bool sameform, bool inflections, bool studydefs, const WordFilterConditions *conditions, std::vector<InfTypes> *inftypes)
{
#ifdef _DEBUG
    if (searchmode == SearchMode::Browse)
        throw "Browse mode doesn't support normal search with wildcards etc.";
#endif

    if (search.isEmpty())
        return false;

    if (conditions != nullptr && !ZKanji::wordfilters().match(words[windex], conditions))
        return false;

    if (searchmode == SearchMode::Japanese)
    {
        // Remove any romaji character from the search string because we want to
        // find results even while in the middle of typing in the search box.
        for (int ix = search.size() - 1; ix != -1; --ix)
            if (!JAPAN(search.at(ix).unicode()))
                search.remove(ix, 1);

        if (search.isEmpty())
            return false;

        bool kanjisearch = false;
        // Look for kanji or other non-kana character in the search string.
        for (int ix = 0; !kanjisearch && ix < search.size(); ++ix)
        {
            ushort ch = search.at(ix).unicode();
            if (VALIDCODE(ch) || KANJI(ch))
                kanjisearch = true;
        }


        std::vector<int> lines;
        if (kanjisearch)
        {
            if (wordMatchesKanjiSearch(windex, search, wildcards, sameform))
                return true;
        }
        else
        {
            if (wordMatchesKanaSearch(windex, search, wildcards, sameform))
                return true;
        }

        // Searching for deinflected results must end with the deinflected form.
        wildcards &= ~(int)SearchWildcard::AnyAfter;

        smartvector<InflectionForm> deinfs;
        if (inflections)
            deinflect(search, deinfs);

        if (deinfs.empty())
            return false;

        for (int ix = 0; ix != deinfs.size(); ++ix)
        {
            //std::vector<int> tmp;
            //std::vector<std::vector<InfTypes>*> inftmp;
            int foundcnt = 0;

            if (kanjisearch)
            {
                if (!wordMatchesKanjiSearch(windex, deinfs[ix]->form, wildcards, sameform, deinfs[ix]->infsize))
                    continue;
            }
            else
            {
                if (!wordMatchesKanaSearch(windex, deinfs[ix]->form, wildcards, sameform, deinfs[ix]->infsize))
                    continue;
            }

            //inftmp.resize(tmp.size());

            //for (int j = 0; j != tmp.size(); ++j)
            //{
                bool found = false;
                for (int k = 0; !found && k != /*result.dictionary()->*/wordEntry(windex)->defs.size(); ++k)
                    if ((/*result.dictionary()->*/wordEntry(windex)->defs[k].attrib.types & (1 << (int)deinfs[ix]->type)) != 0/* ||
                                                                                                                          ((result.dictionary()->wordEntry(tmp[j])->defs[k].types & (1 << (int)WordTypes::AuxAdj)) != 0
                                                                                                                          && deinfs[ix]->type == WordTypes::TrueAdj)*/)
                                                                                                                          found = true;
                if (!found)
                    //tmp[j] = -1; // No match, disable this index. We check for -1 just below when adding to result. It's not used elsewhere.
                    continue;
                else
                {
                    if (inflections && inftypes != nullptr)
                        *inftypes = deinfs[ix]->inf;
                    //inftmp[j] = &deinfs[ix]->inf;
                    //++foundcnt;
                    return true;
                }
            //}

            //result.reserve(result.size() + foundcnt, true);

            //for (int j = 0; j != tmp.size(); ++j)
            //{
            //    if (tmp[j] == -1)
            //        continue;
            //    result.add(tmp[j], *inftmp[j]);
            //}
        }

        //result.jpSort();
    }

    if (searchmode == SearchMode::Definition)
    {
        if (studydefs)
        {
            bool found = false;
            bool result = wordstudydefs.wordMatches(windex, search, (wildcards & SearchWildcard::AnyAfter) == 0, sameform, 0, &found);
            if (found)
                return result;
        }

        //std::vector<int> lines;
        return dtree.wordMatches(windex, search, (wildcards & SearchWildcard::AnyAfter) == 0, sameform);
        //result.set(lines);
        //result.defSort(search);
    }

    return false;
}

void Dictionary::findKanjiWords(std::vector<int> &result, QString search, SearchWildcards wildcards, bool sameform, const std::vector<int> *wordpool, const WordFilterConditions *conditions, uint infsize) const
{
    // When changing this, also update wordMatchesKanjiSearch().

    // List of words for the top 3 kanji or symbol with the least number of words.
    const std::vector<int> *symwords[3];

    // Number of valid lists in symwords.
    int symfound = 0;

    // Holds already processed unicode characters so they can be skipped the second time.
    std::set<ushort> found;

    // Find the top 3 kanji or symbol in the search string that have the least number of related words.
    for (int ix = 0; ix < search.size(); ++ix)
    {
        ushort ch = search.at(ix).unicode();
        if (VALIDKANA(ch) || found.count(ch) != 0)
            continue;
        found.insert(ch);

        const std::vector<int> *wordlist = nullptr;
        if (KANJI(ch))
            wordlist = &kanjidata[ZKanji::kanjiIndex(QChar(ch))]->words;
        else
        {
            auto it = symdata.find(ch);
            if (it != symdata.end())
                wordlist = &it->second;
        }

        if (wordlist == nullptr)
            continue;

        if (symfound != 3)
        {
            symwords[symfound] = wordlist;
            ++symfound;
        }
        else
        {
            for (int ix = 0; ix != 3; ++ix)
            {
                if (symwords[ix]->size() > wordlist->size())
                {
                    for (int j = ix; j != 2; ++j)
                        symwords[j + 1] = symwords[j];
                    symwords[ix] = wordlist;
                    break;
                }
            }
        }
    }

    // Make a list of indexes to every word containing the characters, and try to find those that contain all 3.

    std::vector<int> wordlist;
    for (int ix = 0; ix != symfound; ++ix)
    {
        wordlist.insert(wordlist.end(), (*symwords[ix]).begin(), (*symwords[ix]).end());

        // A list of 200 words or less is small enough to look through. Don't bother
        // checking more kanji.
        if (wordlist.size() < 200)
        {
            symfound = ix + 1;
            break;
        }
    }

    if (wordpool != nullptr)
    {
        wordlist.insert(wordlist.end(), (*wordpool).begin(), (*wordpool).end());;
        ++symfound;
    }

    // Create a new list which only holds words found in all kanji and the wordpool. Check
    // the filter conditions too.
    if (symfound > 1)
    {
        std::sort(wordlist.begin(), wordlist.end(), [](int a, int b) { return a < b; });
        std::vector<int> temp;
        temp.swap(wordlist);

        int last = -1;
        int foundsym = 0;
        for (int ix = 0; ix != temp.size(); ++ix)
        {
            if (temp[ix] == last)
                ++foundsym;
            else
            {
                foundsym = 1;
                last = temp[ix];
            }
            if (foundsym == symfound && (conditions == nullptr || ZKanji::wordfilters().match(words[temp[ix]], conditions)))
                wordlist.push_back(temp[ix]);
        }
    }
    else if (conditions != nullptr)
    {
        std::vector<int> temp;
        temp.swap(wordlist);
        for (int ix = 0; ix != temp.size(); ++ix)
            if (ZKanji::wordfilters().match(words[temp[ix]], conditions))
                wordlist.push_back(temp[ix]);
    }

    // Words list now only contains unique items. Look for the search string the classic way.
    if (!sameform)
        search = hiraganize(search);
    else if (infsize != 0)
        search = search.left(search.size() - infsize) + hiraganize(search.right(infsize));

    for (int ix = 0; ix != wordlist.size(); ++ix)
    {
        const WordEntry *e = words[wordlist[ix]];

        int klen;

        klen = e->kanji.size();
        if (klen < search.size())
            continue;



        QString k = !sameform ? hiraganize(e->kanji.data()) : (infsize == 0 || infsize >= search.size()) ? e->kanji.toQString() : e->kanji.toQString(0, klen - infsize) + hiraganize(e->kanji.data() + klen - infsize, infsize);
        if (wildcards == 0) // Kanji must exactly match search string.
        {
            if (k == search)
                result.push_back(wordlist[ix]);
        }
        else if (wildcards == (int)SearchWildcard::AnyAfter) // Kanji must start with search string.
        {
            if (k.leftRef(search.size()) == search)
                result.push_back(wordlist[ix]);
        }
        else if (wildcards == (int)SearchWildcard::AnyBefore) // Kanji must end with search string.
        {
            if (k.rightRef(search.size()) == search)
                result.push_back(wordlist[ix]);
        }
        else // Kanji must contain the search string somewhere in the middle.
        {
            if (k.indexOf(search, 0) != -1)
                result.push_back(wordlist[ix]);
        }
    }
}

bool Dictionary::wordMatchesKanjiSearch(int windex, QString search, SearchWildcards wildcards, bool sameform, uint infsize) const
{
    // The word index can be found in at least the most important kanji. Look for the search string the classic way.
    if (!sameform)
        search = hiraganize(search);
    else if (infsize != 0)
        search = search.left(search.size() - infsize) + hiraganize(search.right(infsize));

    const WordEntry *e = words[windex];

    int klen = e->kanji.size();
    if (klen < search.size())
        return false;

    QString k = !sameform ? hiraganize(e->kanji.data()) : (infsize == 0 || infsize >= search.size()) ? e->kanji.toQString() : e->kanji.toQString(0, klen - infsize) + hiraganize(e->kanji.data() + klen - infsize, infsize);
    if (wildcards == 0) // Kanji must exactly match search string.
    {
        if (k == search)
            return true;
    }
    else if (wildcards == (int)SearchWildcard::AnyAfter) // Kanji must start with search string.
    {
        if (k.leftRef(search.size()) == search)
            return true;
    }
    else if (wildcards == (int)SearchWildcard::AnyBefore) // Kanji must end with search string.
    {
        if (k.rightRef(search.size()) == search)
            return true;
    }
    else
    // Kanji must contain the search string somewhere in the middle.
    {
        if (k.indexOf(search, 0) != -1)
            return true;
    }

    return false;
}

void Dictionary::findKanaWords(std::vector<int> &result, QString search, SearchWildcards wildcards, bool sameform, const std::vector<int> *wordpool, const WordFilterConditions *conditions, uint infsize)
{
    // When changing this, also update wordMatchesKanaSearch().


    if (wildcards == (int)SearchWildcard::AnyBefore)
        return btree.findWords(result, search, false, sameform, wordpool, conditions, infsize);
    if (wildcards == (int)SearchWildcard::AnyAfter)
        return ktree.findWords(result, search, false, sameform, wordpool, conditions, infsize);
    if (wildcards == 0)
        return ktree.findWords(result, search, true, sameform, wordpool, conditions, infsize);

    // Search for kana in the middle of the word.

    // We want to make a list that holds indexes to words that contain some kana of the search string.
    // Not all kana are used here, because it would mean more words to check later. Once the list
    // of indexes is done, the words that were found in all of the searched kana are checked for
    // the original search string.
    std::vector<int> list;

    // The result is built up in multiple passes from words already in the list, and words containing
    // a given kana. Only those words are kept that are found in both. On each pass the words of
    // the next kana are added to the list. If we put the wordpool words as the initial contents of
    // the list, we can limit the result even more.
    if (wordpool != nullptr)
        list = *wordpool;

    // Create a string that only contains unique hiragana of the search word. The hiragana in front
    // of the string are in less words than the latter ones.
    QString hiragana = hiraganize(search);

    // Holds the number of characters that are used in the search. At most 3 characters are used.
    int hlen = hiragana.size();

    std::sort(hiragana.begin(), hiragana.end(), [](const QChar &a, const QChar &b) { return a.unicode() < b.unicode();  });
    auto it = std::unique(hiragana.begin(), hiragana.end());
    if (it != hiragana.end())
        hlen = it - hiragana.begin();

    std::sort(hiragana.begin(), it, [this](const QChar &a, const QChar &b) { 
        return kanadata.at(a.unicode()).size() < kanadata.at(b.unicode()).size();
    });
    hlen = std::min(hlen, 3);
    
    for (int ix = 0; ix != hlen; ++ix)
    {
        if (!kanadata.count(hiragana.at(ix).unicode()))
            continue;

        if (list.empty())
            list = kanadata.at(hiragana.at(ix).unicode());
        else
        {
            std::vector<int> llist; // Llist receives the sorted items of both klist and list.
            std::vector<int> klist = kanadata.at(hiragana.at(ix).unicode());
            llist.reserve(list.size() + klist.size());

            int lpos = 0;
            int kpos = 0;

            while (lpos != list.size() && kpos != klist.size())
            {
                int lval = list[lpos];
                int kval = klist[kpos];
                if (lval == kval)
                {
                    llist.push_back(lval);
                    llist.push_back(kval);
                    ++lpos;
                    ++kpos;
                }
                else if (lval < kval)
                {
                    llist.push_back(lval);
                    ++lpos;
                }
                else
                {
                    llist.push_back(kval);
                    ++kpos;
                }
            }


            while (lpos != list.size())
                llist.push_back(list[lpos++]);
            while (kpos != klist.size())
                llist.push_back(klist[kpos++]);

            list.swap(llist);
        }

        if (list.size() > 250000)
            hlen = ix + 1;
    }

    // Let's pretend there's another hiragana in the search string, to match the number of times
    // the words must be in list.
    if (wordpool != nullptr)
        ++hlen;

    // Remove duplicates and find real matches that also fit the conditions.

    QString romaji;
    if (!sameform)
        romaji = romanize(search);

    int found = 1;

    for (int ix = 0; ix != list.size(); ++ix)
    {
        if (ix == 0 || list[ix - 1] != list[ix])
            found = 1;
        else
            ++found;

        if (found == hlen &&
            (conditions == nullptr || ZKanji::wordfilters().match(words[list[ix]], conditions)) &&
            ((!sameform && words[list[ix]]->romaji.find(romaji.constData()) != -1) ||
            (sameform && words[list[ix]]->kana.find(search.constData()) != -1)))
            result.push_back(list[ix]);
    }

    //return WordResultList(this, result);
}

bool Dictionary::wordMatchesKanaSearch(int windex, QString search, SearchWildcards wildcards, bool sameform, const uint infsize)
{
    if (wildcards == (int)SearchWildcard::AnyBefore)
        return btree.wordMatches(windex, search, false, sameform, infsize);
    if (wildcards == (int)SearchWildcard::AnyAfter)
        return ktree.wordMatches(windex, search, false, sameform, infsize);
    if (wildcards == 0)
        return ktree.wordMatches(windex, search, true, sameform, infsize);

    // Search for kana in the middle of the word.

    if (!sameform)
    {
        QString romaji = romanize(search);
        if (words[windex]->romaji.find(romaji.constData()) != -1)
            return true;
    }
    else if (words[windex]->kana.find(search.constData()) != -1)
        return true;

    return false;
}

int Dictionary::findKanjiKanaWord(const QChar *kanji, const QChar *kana, const QChar *romaji, int kanjilen, int kanalen, int romajilen)
{
    QCharString tmp;
    if (romaji == nullptr)
    {
        tmp.copy(romanize(kana, kanalen).constData());
        romaji = tmp.data();
        romajilen = tmp.size();
    }

    if (kanjilen == -1)
        kanjilen = qcharlen(kanji);
    if (kanalen == -1)
        kanalen = qcharlen(kana);
    if (romajilen == -1)
        romajilen = qcharlen(romaji);

    // l will contain words that have a romanized kana starting with romaji.
    std::vector<int> l;
    ktree.getSiblings(l, romaji, romajilen);

    // The kana can still be different and the kanji must be checked as well.
    for (int ix = 0; ix != l.size(); ++ix)
    {
        WordEntry *e = words[l[ix]];
        if (e->kanji.size() != kanjilen || e->kana.size() != kanalen || qcharncmp(e->kanji.data(), kanji, kanjilen) || qcharncmp(e->kana.data(), kana, kanalen))
            continue;
        return l[ix];
    }
    return -1;
}

int Dictionary::findKanjiKanaWord(const QCharString &kanji, const QCharString &kana)
{
    return findKanjiKanaWord(kanji.data(), kana.data());
}

int Dictionary::findKanjiKanaWord(const QString &kanji, const QString &kana)
{
    return findKanjiKanaWord(kanji.constData(), kana.constData());
}

int Dictionary::findKanjiKanaWord(WordEntry *e)
{
    return findKanjiKanaWord(e->kanji.data(), e->kana.data(), e->romaji.data());
}

std::function<bool(int, const QChar*)> Dictionary::browseOrderCompareFunc(BrowseOrder order) const
{
    if (order == BrowseOrder::ABCDE)
    {
        return [this](int a, const QChar *rb) {
            //const QChar* ra = a == -1 ? r.constData() : words[a]->romaji.data();
            const QChar* ra = words[a]->romaji.data();
            return qcharcmp(ra, rb) < 0;
        };
    }
    
    
    return [this](int a, const QChar *rb) {
        //QString tmpa;
        //const QChar *ra = a == -1 ? r.constData() : (tmpa = hiraganize(words[a]->kana)).constData();
        QString tmpa = hiraganize(words[a]->kana);
        const QChar *ra = tmpa.constData();
    
        return qcharcmp(ra, rb) < 0;
    };
}

std::function<bool(int, int, const QChar*, const QChar*)> Dictionary::browseOrderCompareIndexFunc(BrowseOrder order) const
{
    if (order == BrowseOrder::ABCDE)
    {
        return [this](int a, int b, const QChar *astr, const QChar *bstr) {
            const QChar* ra = words[a]->romaji.data();
            const QChar* rb = words[b]->romaji.data();
            return qcharcmp(ra, rb) < 0;
        };
    }


    return [this](int a, int b, const QChar *astr, const QChar *bstr) {

        QString tmpa;
        if (!astr)
            tmpa = hiraganize(words[a]->kana);
        QString tmpb;
        if (!bstr)
            tmpb = hiraganize(words[b]->kana);
        const QChar *ra = astr ? astr : tmpa.constData();
        const QChar *rb = bstr ? bstr : tmpb.constData();

        return qcharcmp(ra, rb) < 0;
    };
}

namespace {
    int jpSortFuncKanaLenInc[] = { 60, 45, 35, 26, 20, 17, 15, 13, 11 };
    int jpSortFuncKanjiCntInc[] = { 60, 45, 35, 26, 20, 17, 15, 13, 11 };
}
bool Dictionary::jpSortFunc(const std::pair<WordEntry*, const std::vector<InfTypes>*> &ap, const std::pair<WordEntry*, const std::vector<InfTypes>*> &bp)
{
    // Returns a sorting order between two word entries. The base of the calculation is the
    // word frequencies. They're gradually increased depending on the properties of the word
    // entries and compared at each step. If the difference between the two values is larger
    // than a limit, the following comparisons are skipped and a result is returned.
    // The sum of maximum changes of the following comparisons to the frequency number must be
    // lower than the limit.

    const WordEntry *aw = ap.first;
    const WordEntry *bw = bp.first;

    int afreq = aw->freq;
    int bfreq = bw->freq;

    // Making less frequent words' frequency count even less, to make sure they have smaller
    // chance to get ahead of the frequent words.

    afreq /= 100;
    bfreq /= 100;

    //if (afreq < 1500)
    //    afreq = afreq * 0.3;
    //else if (afreq < 3000)
    //    afreq = afreq * 0.4;
    //else
    //    afreq = afreq * 0.6;

    //if (bfreq < 1500)
    //    bfreq = bfreq * 0.3;
    //else if (bfreq < 3000)
    //    bfreq = bfreq * 0.4;
    //else
    //    bfreq = bfreq * 0.6;

    //if (abs(afreq - bfreq) > 1500 || (abs(afreq - bfreq) > 500 && afreq + bfreq < 2400))
    //    return afreq > bfreq;

    //if (std::abs(afreq - bfreq) > 20)
    //    return afreq > bfreq;

    int alen = aw->kana.size();
    int blen = bw->kana.size();

    bool ako = (aw->defs[0].attrib.notes & (1 << (int)WordNotes::KanaOnly)) != 0;
    bool bko = (bw->defs[0].attrib.notes & (1 << (int)WordNotes::KanaOnly)) != 0;

    if (!ako)
        afreq += 20;
    if (!bko)
        bfreq += 20;

    afreq += jpSortFuncKanaLenInc[std::min(alen, 9) - 1];
    bfreq += jpSortFuncKanaLenInc[std::min(blen, 9) - 1];

    if (std::abs(afreq - bfreq) > 70)
        return afreq > bfreq;

    //if ((ako && !bko && alen < blen) || (bko && !ako && blen < alen))
    //    return alen < blen;

    int akanjicnt = 0;
    int avalidcnt = 0;
    int aklen = aw->kanji.size();
    for (int ix = 0; ix < aklen; ++ix)
        if (KANJI(aw->kanji[ix].unicode()))
            ++akanjicnt;
        else if (VALIDCODE(aw->kanji[ix].unicode()))
            ++avalidcnt;

    int bkanjicnt = 0;
    int bvalidcnt = 0;
    int bklen = bw->kanji.size();
    for (int ix = 0; ix < bklen; ++ix)
        if (KANJI(bw->kanji[ix].unicode()))
            ++bkanjicnt;
        else if (VALIDCODE(bw->kanji[ix].unicode()))
            ++bvalidcnt;

    afreq += jpSortFuncKanjiCntInc[std::min(akanjicnt + (avalidcnt / 2), 8)]; //(20 - std::min(akanjicnt + (avalidcnt / 3), 20)) * 20;
    bfreq += jpSortFuncKanjiCntInc[std::min(bkanjicnt + (bvalidcnt / 2), 8)]; //(20 - std::min(bkanjicnt + (bvalidcnt / 3), 20)) * 20;

    if (std::abs(afreq - bfreq) > 10)
        return afreq > bfreq;

    afreq += (10 - std::min(aklen, 9));
    bfreq += (10 - std::min(bklen, 9));

    if (ap.second == nullptr || ap.second->empty())
        ++afreq;
    if (bp.second == nullptr || bp.second->empty())
        ++bfreq;

    return afreq > bfreq;
}

Dictionary::WordResultSortData Dictionary::defSortDataGen(QString searchstr, WordEntry *w)
{
    // TODO: (later) Some languages might not use the parenthesis or comma for the same task.
    // Make this translatable somehow.

    WordResultSortData data;

    data.len = 1000;
    data.pos = 1000;
    data.defpos = 255;
    data.deflen = 0;

    int indexof = -1;
    QString def;

    int ix = 0;
    while (ix != w->defs.size() && data.len + data.pos != 0)
    {
        if (indexof == -1)
            def = w->defs[ix].def.toLower();

        indexof = def.indexOf(searchstr, indexof + 1);
        if (indexof == -1)
        {
            ++ix;
            continue;
        }

        // Find the number of characters before the search string till a comma or word start in
        // def. Exclude things in parentheses. If a closing parenthesis is found but no opening
        // parenthesis, try again, this time ignoring them.
        int indexpos = indexof - 1;
        int pos = 0; // Number of characters in front of searchstr, excluding parenthesised strings.
        int pos2 = 0; // Full number of characters in front of searchstr.
        int len = 0;
        int parcnt = 0;

        while (indexpos != -1)
        {
            if (parcnt == 0 && (def.at(indexpos) == ',' || def.at(indexpos) == GLOSS_SEP_CHAR))
                break;
            if (def.at(indexpos) == ')')
                ++parcnt;
            else if (parcnt != 0 && def.at(indexpos) == '(')
                --parcnt;
            if (parcnt == 0)
                ++pos;
            ++pos2;
            --indexpos;
        }

        // Found unclosed parenthesis. The whole definition before the search string counts.
        if (parcnt != 0)
            pos = pos2;

        // Find number of characters after search string till a comma or word end in def.
        // Parentheses are considered as usual.
        pos2 = 0;
        indexpos = indexof + searchstr.size();
        while (indexpos != def.size())
        {
            if (parcnt == 0 && (def.at(indexpos) == ',' || def.at(indexpos) == GLOSS_SEP_CHAR))
                break;
            if (def.at(indexpos) == '(')
                ++parcnt;
            else if (parcnt != 0 && def.at(indexpos) == ')')
                --parcnt;
            if (parcnt == 0)
                ++len;
            ++pos2;
            ++indexpos;
        }
        // Found unclosed parenthesis. The whole definition after the search string counts.
        if (parcnt != 0)
            len = pos2;

        if (pos < data.pos / 2 || pos + len < data.pos + data.len)
        {
            data.pos = pos;
            data.len = len;
            data.defpos = ix;
            data.deflen = def.size();
        }

    }

    return data;
}

bool Dictionary::defSortFunc(const std::pair<const WordEntry*, WordResultSortData> &as, const std::pair<const WordEntry*, WordResultSortData> &bs)
{
    int afreq = as.first->freq;
    int bfreq = bs.first->freq;

    // Old way of calculation:
    //if ((std::min(afreq, bfreq) < 3000 && abs(afreq - bfreq) > 1500) ||
    //    (std::min(afreq, bfreq) >= 3000 && abs(afreq - bfreq) > 2400) ||
    //    (abs(afreq - bfreq) > 500 && afreq + bfreq < 2400))
    //    return afreq > bfreq;
    //return (bfreq - afreq + (as.second.defpos - bs.second.defpos) * 500 + ((as.second.len + as.second.pos) - (bs.second.len + bs.second.pos)) * 500 + (as.second.pos - bs.second.pos) * 50) < 0;

    // Making less frequent words frequency count even less, to make sure they have smaller
    // chance to get ahead of the frequent words.

    if (afreq < 1500)
        afreq = afreq * 0.4;
    else if (afreq < 3000)
        afreq = afreq * 0.65;
    else
        afreq = afreq * 0.8;

    if (bfreq < 1500)
        bfreq = bfreq * 0.4;
    else if (bfreq < 3000)
        bfreq = bfreq * 0.65;
    else
        bfreq = bfreq * 0.8;

    afreq += (5 - std::min<int>(as.second.defpos, 5)) * 150;
    bfreq += (5 - std::min<int>(bs.second.defpos, 5)) * 150;

    afreq += (20 - std::min<int>(as.second.len + as.second.pos, 20)) * 350;
    bfreq += (20 - std::min<int>(bs.second.len + bs.second.pos, 20)) * 350;

    afreq += (10 - std::min<int>(as.second.pos, 10)) * 100;
    bfreq += (10 - std::min<int>(bs.second.pos, 10)) * 100;

    afreq += (20 - std::min<int>(as.second.deflen, 20)) * 100;
    bfreq += (20 - std::min<int>(bs.second.deflen, 20)) * 100;

    return afreq > bfreq;

}

//int Dictionary::browseIndex(QString r, BrowseOrder order, const std::vector<int> *wordlist)
//{
//    if (order == BrowseOrder::ABCDE)
//    {
//        r = romanize(r);
//        auto it = std::lower_bound(abcde.begin(), abcde.end(), -1, [&r, this](int a, int b) {
//            const QChar* ra = a == -1 ? r.constData() : words[a]->romaji.data();
//            const QChar* rb = b == -1 ? r.constData() : words[b]->romaji.data();
//            return qcharcmp(ra, rb) < 0;
//        });
//
//        if (it == abcde.end())
//            return abcde.size() - 1;
//        return it - abcde.begin();
//    }
//
//    // AIUEO order.
//
//    r = hiraganize(r);
//    // Remove non-kana.
//    for (int ix = r.size() - 1; ix != -1; --ix)
//        if (!KANA(r.at(ix).unicode()))
//            r[ix] = QChar('*');
//    r.remove(QChar('*'));
//
//    auto it = std::lower_bound(aiueo.begin(), aiueo.end(), -1, [&r, this](int a, int b) {
//        QString tmpa;
//        const QChar *ra = a == -1 ? r.constData() : (tmpa = hiraganize(words[a]->kana)).constData();
//        QString tmpb;
//        const QChar *rb = b == -1 ? r.constData() : (tmpb = hiraganize(words[b]->kana)).constData();
//
//        return qcharcmp(ra, rb) < 0;
//    });
//
//    if (it == aiueo.end())
//        return aiueo.size() - 1;
//    return it - aiueo.begin();
//}

void Dictionary::listUsedWords(std::vector<int> &result)
{
    // TODO: WHEN changing the dictionary and some new list holds words: check if all words have been looked up.
    // - originalwords is not listed here. Those are handled separately.

    // Must list indexes in:
    // - word groups
    // - long term study list
    // - words marked as kanji example
    // - user defined word study definitions
    // the list might not be full.
    for (int ix = 0; ix != groups->wordGroups().size(); ++ix)
    {
        auto vec = groups->wordGroups().items(ix)->getIndexes();
        result.insert(result.end(), vec.begin(), vec.end());
    }

    for (int ix = 0, siz = decks->size(); ix != siz; ++ix)
    {
        WordDeck *deck = decks->items(ix);
        result.reserve(result.size() + deck->wordDataSize());
        for (int iy = 0; iy != deck->wordDataSize(); ++iy)
            result.push_back(deck->wordData(iy)->index);
    }

    int s = 0;
    for (int ix = 0; ix != kanjidata.size(); ++ix)
        s += kanjidata[ix]->ex.size();

    result.reserve(result.size() + s + wordstudydefs.size());
    for (int ix = 0; ix != kanjidata.size(); ++ix)
    {
        for (int iy = 0; iy != kanjidata[ix]->ex.size(); ++iy)
            result.push_back(kanjidata[ix]->ex[iy]);
    }

    wordstudydefs.listWordIndexes(result);

    if (result.empty())
        return;

    std::sort(result.begin(), result.end());
    result.resize(std::unique(result.begin(), result.end()) - result.begin());
}

std::map<int, int> Dictionary::mapWords(Dictionary *src, const std::vector<int> &wlist)
{
    std::map<int, int> tmp;
    //tmp.reserve(wlist.size());

    for (int ix = 0; ix != wlist.size(); ++ix)
    {
        WordEntry *e = src->wordEntry(wlist[ix]);
        tmp.insert(std::make_pair(wlist[ix], findKanjiKanaWord(e)));
    }

    return tmp;
}

//std::map<int, int> Dictionary::mapOriginalWords()
//{
//    std::map<int, int> tmp;
//    //tmp.reserve(ZKanji::originals.size());
//
//    Dictionary *d = ZKanji::dictionary(0);
//
//    for (int ix = 0; ix != ZKanji::originals.size(); ++ix)
//    {
//        const OriginalWord *ori = ZKanji::originals.items(ix);
//        int pos = findKanjiKanaWord(ori->kanji, ori->kana);
//        tmp.insert(std::make_pair(ori->index, pos));
//    }
//
//    return tmp;
//}

void Dictionary::diff(Dictionary *other, std::vector<std::pair<int, int>> &result)
{
    result.clear();

    //std::vector<std::pair<int, int>> tmp;
    if (words.empty() || other->words.empty())
    {
        int siz = std::max(words.size(), other->words.size());
        result.reserve(siz);
        for (int ix = 0; ix != siz; ++ix)
        {
            if (words.empty())
                result.push_back(std::make_pair(-1, ix));
            else
                result.push_back(std::make_pair(ix, -1));
        }
        return;
    }

    // Compare words in abc ordering.

    int lpos = 0;
    int opos = 0;
    int lsiz = words.size();
    int osiz = other->words.size();

    WordEntry *l = words[abcde[0]];
    WordEntry *o = other->words[other->abcde[0]];

    int siz = std::min(lsiz, osiz);
    result.reserve(std::max(lsiz, osiz) * 1.2);

    std::map<QCharString, QString> hira;
    QString lstr;
    QString ostr;
    while (lpos != lsiz && opos != osiz)
    {
        int d = qcharcmp(l->romaji.data(), o->romaji.data());
        if (d == 0)
        {
            auto it = hira.find(l->kana);
            if (it == hira.end())
                hira[l->kana] = lstr = hiraganize(l->kana);
            else
                lstr = it->second;
            it = hira.find(o->kana);
            if (it == hira.end())
                hira[o->kana] = ostr = hiraganize(o->kana);
            else
                ostr = it->second;
            d = qcharcmp(lstr.constData(), ostr.constData());
        }
        if (d == 0)
            d = qcharcmp(l->kana.data(), o->kana.data());
        if (d == 0)
            d = qcharcmp(l->kanji.data(), o->kanji.data());

        // By this time if d is 0 the words match. Otherwise the one that
        // comes first is determined by the sign of d.
        if (d == 0)
        {
            result.push_back(std::make_pair(abcde[lpos++], other->abcde[opos++]));
            if (lpos != lsiz)
                l = words[abcde[lpos]];
            if (opos != osiz)
                o = other->words[other->abcde[opos]];
            continue;
        }

        if (d < 0)
        {
            result.push_back(std::make_pair(abcde[lpos++], -1));
            if (lpos != lsiz)
                l = words[abcde[lpos]];
            continue;
        }

        // d > 0
        result.push_back(std::make_pair(-1, other->abcde[opos++]));
        if (opos != osiz)
            o = other->words[other->abcde[opos]];
        continue;
    }

    while (lpos != lsiz || opos != osiz)
    {
        if (lpos != lsiz)
            result.push_back(std::make_pair(abcde[lpos++], -1));
        else
            result.push_back(std::make_pair(-1, other->abcde[opos++]));
    }
}

int Dictionary::addWordCopy(WordEntry *src, bool originals)
{
    if (originals && this == ZKanji::dictionary(0))
    {
        if (ZKanji::originals.createAdded(words.size(), src->kanji.data(), src->kana.data()))
            setToUserModified();
    }

    WordEntry *w = new WordEntry;
    ZKanji::cloneWordData(w, src, true);

    words.push_back(w);

    // Insert word into aiueo and abcde ordered lists.
    addWordData();

    int windex = words.size() - 1;
    emit entryAdded(windex);

    if (this != ZKanji::dictionary(0))
        setToModified();

    return windex;
}

void Dictionary::cloneWordData(int windex, WordEntry *src, bool originals, bool checkoriginals)
{
    WordEntry *w = words[windex];

    bool orichanged = false;
    if (originals && this == ZKanji::dictionary(0) && (!checkoriginals || ZKanji::originals.find(windex) == nullptr))
    {
        orichanged = true;
        if (ZKanji::originals.createModified(windex, w))
            setToUserModified();
    }

    ZKanji::cloneWordData(w, src, false);

    dtree.removeLine(windex, false);
    dtree.expandWith(windex, false);

    emit entryChanged(windex, false);

    if (!orichanged && this != ZKanji::dictionary(0))
        setToModified();
}

void Dictionary::revertEntry(int windex)
{
    if (this != ZKanji::dictionary(0))
        return;

    WordEntry *w = words[windex];

    if (!ZKanji::originals.revertModified(windex, w))
        return;

    dtree.removeLine(windex, false);
    dtree.expandWith(windex, false);

    emit entryChanged(windex, false);

    setToUserModified();
}

void Dictionary::addWordData()
{
    WordEntry *w = words.back();

    int windex = words.size() - 1;

    std::map<QCharString, QString> hira;
    auto it = std::upper_bound(abcde.begin(), abcde.end(), -1, [this, w, &hira](int a, int b) {
        WordEntry *wa = a == -1 ? w : words[a];
        WordEntry *wb = b == -1 ? w : words[b];

        int val = qcharcmp(wa->romaji.data(), wb->romaji.data());
        if (val != 0)
            return val < 0;

        QString ah;
        QString bh;
        auto it = hira.find(wa->kana);
        if (it != hira.end())
            ah = it->second;
        else
            hira[wa->kana] = ah = hiraganize(wa->kana);
        it = hira.find(wb->kana);
        if (it != hira.end())
            bh = it->second;
        else
            hira[wb->kana] = bh = hiraganize(wb->kana);
        val = qcharcmp(ah.constData(), bh.constData());
        if (val != 0)
            return val < 0;

        val = qcharcmp(wa->kana.data(), wb->kana.data());
        if (val != 0)
            return val < 0;

        return qcharcmp(wa->kanji.data(), wb->kanji.data()) < 0;
    });
    abcde.insert(it, windex);

    it = std::upper_bound(aiueo.begin(), aiueo.end(), -1, [this, w, &hira](int a, int b) {
        WordEntry *wa = a == -1 ? w : words[a];
        WordEntry *wb = b == -1 ? w : words[b];

        QString ah;
        QString bh;
        auto it = hira.find(wa->kana);
        if (it != hira.end())
            ah = it->second;
        else
            hira[wa->kana] = ah = hiraganize(wa->kana);
        it = hira.find(wb->kana);
        if (it != hira.end())
            bh = it->second;
        else
            hira[wb->kana] = bh = hiraganize(wb->kana);
        int val = qcharcmp(ah.constData(), bh.constData());
        if (val != 0)
            return val < 0;

        val = qcharcmp(wa->kana.data(), wb->kana.data());
        if (val != 0)
            return val < 0;

        return qcharcmp(wa->kanji.data(), wb->kanji.data()) < 0;
    });
    aiueo.insert(it, windex);

    // Expand kanjidata, symdata.
    for (int ix = w->kanji.size() - 1; ix != -1; --ix)
    {
        ushort ch = w->kanji[ix].unicode();
        if (KANJI(ch))
        {
            int kix = ZKanji::kanjiIndex(ch);

            std::vector<int> &wvec = kanjidata[kix]->words;
            if (!wvec.empty() && wvec.back() == windex)
                continue;
            wvec.push_back(windex);

            ZKanji::kanjis[kix]->word_freq += w->freq;
        }
        else if (!KANA(ch) && UNICODE_J(ch))
        {
            std::vector<int> &svec = symdata[ch];
            if (!svec.empty() && svec.back() == windex)
                continue;
            svec.push_back(windex);
        }
    }

    // Expand kanadata.
    QChar *romaji = w->romaji.data();
    int wlen = w->romaji.size();

    for (int ix = 0; ix != wlen; ++ix)
    {
        ushort ch;

        int dummy;
        if (!kanavowelize(ch, dummy, romaji + ix, wlen - ix))
            continue;

        std::vector<int> &kvec = kanadata[ch];
        if (!kvec.empty() && kvec.back() == windex)
            continue;
        kvec.push_back(windex);
    }

    QChar *kana = w->kana.data();
    wlen = w->kana.size();

    for (int ix = 0; ix != wlen; ++ix)
    {
        ushort ch = kana[ix].unicode();
        if (!KANA(ch))
            continue;
        if (KATAKANA(ch) && ch <= 0x30F4)
            ch -= 0x60;

        std::vector<int> &kvec = kanadata[ch];
        if (!kvec.empty() && kvec.back() == windex)
            continue;
        kvec.push_back(windex);
    }

    // Expand dtree, ktree and btree.
    if (!w->defs.empty())
        dtree.expandWith(words.size() - 1, false);
    ktree.expandWith(words.size() - 1, false);
    btree.expandWith(words.size() - 1, false);
}

void Dictionary::removeWordData(int index, int &abcdeindex, int &aiueoindex)
{
    // Remove word from alphabetic ordering.

    int *abcdat = abcde.data();
    for (int ix = 0, pos = 0, siz = abcde.size(); ix != siz; ++ix, ++pos)
    {
        if (abcdat[ix] < index)
            abcdat[pos] = abcdat[ix];
        else if (abcdat[ix] > index)
            abcdat[pos] = abcdat[ix] - 1;
        else
        {
            abcdeindex = ix;
            --pos;
        }
    }
    abcde.resize(abcde.size() - 1);

    abcdat = aiueo.data();
    for (int ix = 0, pos = 0; ix != aiueo.size(); ++ix, ++pos)
    {
        if (abcdat[ix] < index)
            abcdat[pos] = abcdat[ix];
        else if (abcdat[ix] > index)
            abcdat[pos] = abcdat[ix] - 1;
        else
        {
            aiueoindex = ix;
            --pos;
        }
    }
    aiueo.resize(aiueo.size() - 1);

    // Remove word from kanjidata, symdata and kanadata, and its frequency from kanjis' freq value.

    WordEntry *w = words[index];

    std::set<ushort> found;
    for (int ix = w->kanji.size() - 1; ix != -1; --ix)
    {
        ushort ch = w->kanji[ix].unicode();

        if (KANJI(ch) && found.count(ch) == 0)
        {
            found.insert(ch);

            int kix = ZKanji::kanjiIndex(ch);
            ZKanji::kanjis[kix]->word_freq -= w->freq;
        }
    }

    for (int ix = 0; ix != kanjidata.size(); ++ix)
    {
        removeIndexFromList(index, kanjidata[ix]->ex);
        removeIndexFromList(index, kanjidata[ix]->words);
    }

    for (auto &keyvalue : symdata)
        removeIndexFromList(index, keyvalue.second);
    for (auto &keyvalue : kanadata)
        removeIndexFromList(index, keyvalue.second);

    // Remove word from the search trees.

    dtree.removeLine(index, true);
    ktree.removeLine(index, true);
    btree.removeLine(index, true);
}

//bool Dictionary::importedWordStrings(const QString &line, int pos, int len, QString &kanji, QString &kana)
//{
//    int endpos = line.lastIndexOf(QChar(')'), pos + len - 1);
//    int startpos = line.lastIndexOf(QChar('('), endpos - 1);
//
//    // Kana string not found.
//    if (endpos < pos || startpos < pos)
//        return false;
//
//    kanji = line.mid(pos, startpos);
//    kana = line.mid(startpos + 1, endpos - startpos - 1);
//
//    return true;
//}


//-------------------------------------------------------------
