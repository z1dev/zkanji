/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>

#include "groups.h"
#include "zkanjimain.h"
#include "words.h"
#include "grammar_enums.h"

void WordGroup::loadLegacy(QDataStream &stream, bool isstudy, int version)
{
    WordStudyMethod studymethod = ZKanji::defaultWordStudySettings().method;

    if (isstudy)
    {
        qint8 ssm;
        stream >> ssm;

        studymethod = (WordStudyMethod)ssm;
    }

    // Number of words in group.
    qint32 wcnt;
    stream >> wcnt;

    list.reserve(wcnt);
    //defs.reserve(wcnt);

    std::vector<int> indextmp;
    indextmp.reserve(list.size());
    for (int ix = 0; ix != wcnt; ++ix)
    {
        qint32 index;
        qint8 meaning;
        stream >> index;
        // The meaning is not used anymore since 2015.
        stream >> meaning;

        //addSingleDefinition(index, meaning);

        // Check whether we have a duplicate because another meaning of the
        // same word was added to the group.
        bool duplicate = 0;
        std::forward_list<WordGroup*> wg = owner().groupsOfWord(index);
        for (WordGroup *g : wg)
        {
            if (g == this)
            {
                duplicate = true;
                break;
            }
        }

        if (!duplicate)
        {
            indextmp.push_back(tosigned(list.size()));
            list.push_back(index);
            wg.push_front(this);
            dictionary()->wordEntry(index)->dat |= (1 << (int)WordRuntimeData::InGroup);
        }
        else
            indextmp.push_back(-1);
    }

    if (isstudy)
        study.loadLegacy(stream, studymethod, version, indextmp);
}

void WordGroups::loadLegacy(QDataStream &stream, bool study, int version)
{
    WordGroupCategory *cat = nullptr;

    // Number of word groups.
    qint32 gcnt;

    stream >> gcnt;

    if (!study)
    {
        cat = this;
        clear();
    }
    else if (gcnt != 0)
        cat = categories(addCategory(tr("Study groups")));

    for (int ix = 0; ix != gcnt; ++ix)
    {
        QString name;
        stream >> make_zstr(name, ZStrFormat::Byte);

        // Skip checkbox bool, it wasn't used for anything.
        stream.skipRawData(1);

        WordGroup *grp = cat->items(addGroup(name, cat));
        grp->loadLegacy(stream, study, version);
    }
}

void KanjiGroup::loadLegacy(QDataStream &stream)
{
    quint16 cnt;
    stream >> cnt;

    list.reserve(cnt);

    quint16 w;
    quint8 b;
    qint32 val;
    for (int ix = 0; ix != cnt; ++ix)
    {
        stream >> w;
        list.push_back(w);

        stream >> b;
        //    Items[j]->examples->Capacity = b;
        for (int j = 0; j != b; ++j)
        {
            stream >> val;
            //        AddExample(j, collection->kanjidat[Kanjis[j]->index].card->Examples[k]->ix);
        }
    }

}

void KanjiGroups::loadLegacy(QDataStream &stream)
{
    //word w;
    //byte b;
    //char *c;
    //UnicodeString newname;

    //Clear();

    quint16 cnt;
    stream >> cnt;

    for (int ix = 0; ix != cnt; ++ix)
    {
        QString name;
        stream >> make_zstr(name, ZStrFormat::Byte);

        // Skip checkbox bool, it wasn't used for anything.
        stream.skipRawData(1);

        KanjiGroup *grp = items(addGroup(name));
        grp->loadLegacy(stream);
    }
}

void Groups::loadWordsLegacy(QDataStream &stream, int version)
{
    qint32 cnt;
    // Number of global word stats for the basic word study groups. Those are not
    // used anymore, so we skip that data next.
    stream >> cnt;

    for (; cnt != 0; --cnt)
    {
        qint32 windex;
        stream >> windex;

        for (int j = 0; j != tosigned(dict->wordEntry(windex)->defs.size()); ++j)
            stream.skipRawData(sizeof(qint16) + sizeof(char) + sizeof(double));
    }

    wordgroups.loadLegacy(stream, false, version);
    wordgroups.loadLegacy(stream, true, version);
}

void Groups::loadKanjiLegacy(QDataStream &stream, int /*version*/)
{
    kanjigroups.loadLegacy(stream);
}
