/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QDataStream>

#include "searchtree.h"
#include "zkanjimain.h"

void TextNode::loadLegacy(QDataStream &stream, int version)
{
    quint32 cnt;

    label.clear();

    stream >> make_zstr(label, ZStrFormat::Byte);

    stream >> cnt;
    sum = cnt;

#ifdef _DEBUG
    if (!lines.empty())
        throw "THIS IS NOT AN ERROR, REMOVE THIS AND THE LINE ABOVE IF WE HIT THIS AND IT TURNS OUT TO BE USEFUL!";
#endif

    lines.reserve(cnt);

    for (int ix = 0; ix < sum; ++ix)
    {
        qint32 val;
        stream >> val;
        lines.push_back(val);
    }

    stream >> cnt;

    nodes.reserve(cnt);
    for (int ix = 0; ix < tosigned(cnt); ++ix)
    {
        TextNode *n = new TextNode(this);
        nodes.addNode(n, false);
        n->loadLegacy(stream, version);
        sum += n->sum;
    }

    nodes.sort();
}

void TextSearchTreeBase::loadLegacy(QDataStream& stream, int version)
{
    cache = nullptr;
    qint32 nodecnt;

    int nversion = 4;
    char tmp[6];
    tmp[4] = 0;
    stream.readRawData(tmp, 4);

    if (strcmp(tmp, "zsct")) // Version above 4 - added 2011.04.04
        throw "ERROR: Not supported"; // Replace these with something useful.

    stream.readRawData(tmp, 3);
    tmp[3] = 0;
    nversion = strtol(tmp, 0, 10);

    if (nversion < 5)
        throw "Error, using throw!";
    stream >> nodecnt;

    for (int ix = 0; ix < nodecnt; ++ix)
    {
        nodes.addNode();

        nodes.items(ix)->parent = nullptr;
        nodes.items(ix)->loadLegacy(stream, version);
    }

    if (version <= 1)
        nodes.sort();
    
    int bcnt;
    quint8 blcnt;
    stream >> blcnt;

    bcnt = blcnt;
    //if (!blacklisting && blcnt != 0)
    //    throw "Cannot have skipped words in a non-blacklisting tree!"; // Do something with throws already.

    // Skip blacklisting.

    for (int ix = 0; ix < bcnt; ++ix)
    {
        QString str;
        stream >> make_zstr(str, ZStrFormat::Byte);
    }

}
