/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include <set>

#include "zkanjimain.h"
#include "treebuilder.h"

#include "checked_cast.h"

//-------------------------------------------------------------


TreeBuilder::TreeBuilder(TextSearchTreeBase &tree, int size, const std::function<void (int, QStringList&)> &func, const std::function<bool()> &callback)
    :
    tree(tree), size(size), func(func), callback(callback), initpos(0), current(nullptr), lablen(1), pos(0)
{
    if (tree.isKana())
    {
        textmap.reserve(size * 10);

        list.resize(size);
        indexes.resize(size);
    }
    else
    {
        textmap.reserve(size * 50);
        list.reserve(size * 5);
    }
}

bool TreeBuilder::initNext()
{
    if (initpos == size)
        return false;

    if (tree.isKana())
    {
        indexes[initpos] = initpos;

        auto &elem = list[initpos];
        elem.index = initpos;
        QCharString strch;

        QStringList texts;
        func(initpos, texts);
        strch.copy(texts.at(0).constData());

        elem.len = strch.size();

        auto it = added.find(strch);
        if (it == added.end())
        {
            elem.strpos = tosigned(textmap.size());
            added.insert(std::make_pair(strch, elem.strpos));
            textmap.insert(textmap.end(), strch.data(), strch.data() + elem.len);

            if (tree.isReversed())
                std::reverse(textmap.data() + elem.strpos, textmap.data() + elem.strpos + elem.len);
        }
        else
            elem.strpos = it->second;

        ++initpos;
    }
    else
    {
        std::set<int> defs;

        QStringList texts;
        func(initpos, texts);
        int defcnt = texts.size();

        for (int ix = 0; ix != defcnt; ++ix)
        {
            QCharString str;
            str.copy(texts[ix].constData());
            auto it = added.find(str);
            bool newelem;
            if ((newelem = (it == added.end())) == true || (defs.count(it->second) == 0))
            {
                list.push_back(Item());
                auto &elem = list.back();
                elem.len = str.size();
                elem.index = initpos;
                if (newelem)
                {
                    elem.strpos = tosigned(textmap.size());
                    added.insert(std::make_pair(str, elem.strpos));
                    textmap.insert(textmap.end(), str.data(), str.data() + elem.len);
                }
                else
                    elem.strpos = it->second;
                defs.insert(elem.strpos);
            }
        }

        ++initpos;
    }

    if (initpos == size)
    {
        added.clear();

        if (!tree.isKana())
        {
            list.shrink_to_fit();
            indexes.resize(list.size());
            for (int ix = 0, siz = tosigned(indexes.size()); ix != siz; ++ix)
                indexes[ix] = ix;
        }

        const QChar *textdata = textmap.data();
        const Item *listdata = list.data();

        interruptSort(indexes.begin(), indexes.end(), [this, textdata, listdata](int a, int b, bool &stop) {
            // There's a callback function and it returned false (=suspend).
            if (callback && !callback())
            {
                stop = true;
                return false;
            }

            int val = qcharncmp(textdata + (listdata + a)->strpos, textdata + (listdata + b)->strpos, std::min((listdata + a)->len, (listdata + b)->len));
            if (val == 0)
            {
                if ((listdata + a)->len != (listdata + b)->len)
                    return (listdata + a)->len < (listdata + b)->len;
                return (listdata + a)->index < (listdata + b)->index;
            }
            return val < 0;
        });

        return false;
    }
    return true;
}

int TreeBuilder::initSize() const
{
    return size;
}

int TreeBuilder::initPos() const
{
    return initpos;
}

bool TreeBuilder::sortNext()
{
    if (pos == tosigned(indexes.size()))
        return false;

    int *ixdata = &indexes[0];
    Item *listdata = &list[0];

    Item &positem = listdata[ixdata[pos]];

    const QChar *textdata = textmap.data();

    // The item 'positem' cannot be put in the current node with no node
    // (obviously), or the label of the current node does not match positem's.
    while (current != nullptr && (tosigned(current->label.size()) >= positem.len || qcharncmp(current->label.data(), textdata + positem.strpos, current->label.size()) ))
        current = current->parent;

    if (current == nullptr)
        lablen = 1;
    else
        lablen = current->label.size() + 1;

    TextNodeList &nodelist = (current == nullptr) ? tree.getNodes() : current->nodes;

    // Finding the number of words starting at pos, that start with the
    // same string as 'positem', up to lablen characters.
    int maxpos = std::min(pos + 5000, (int)indexes.size() - 1);
    while (maxpos != tosigned(indexes.size()) - 1 && listdata[ixdata[maxpos]].len >= lablen && !qcharncmp(textdata + listdata[ixdata[maxpos]].strpos, textdata + positem.strpos, lablen))
        maxpos = std::min(int(maxpos * 1.5), tosigned(indexes.size()) - 1);

    int min = pos;
    int max = maxpos;

    while (max >= min)
    {
        maxpos = (min + max) / 2;
        bool over = listdata[ixdata[maxpos]].len < lablen || qcharncmp(textdata + listdata[ixdata[maxpos]].strpos, textdata + positem.strpos, lablen) != 0;
        if (!over)
            min = maxpos + 1;
        else
            max = maxpos - 1;
    }

    // Contains first index where label is not matching
    maxpos = min;

    // Items with the exact label as positem's label.
    int match = 0;
    // Non matching items starting with lablen length label of positem.
    int startcnt = 0;
    for (int ix = pos; ix != maxpos; ++ix)
    {
        if (listdata[ixdata[ix]].len == lablen)
            ++match;
        else
            ++startcnt;
    }

    // Used for checking whether an index has been added in the new node.
    std::set<int> found;

    current = nodelist.addNode(textdata + positem.strpos, lablen, false);

    if (startcnt || match)
    {
        current->lines.reserve(std::min(std::max(match, (int)TextSearchTreeBase::NODEFULLCOUNT), match + startcnt));

        // Add exact matches that will have to be placed in the new node even
        // if it gets full.
        for (int ix = 0; ix != match; ++ix, ++pos)
        {
            if (found.count(listdata[ixdata[pos]].index) == 0)
            {
                found.insert(listdata[ixdata[pos]].index);
                current->lines.push_back(listdata[ixdata[pos]].index);
            }
        }

        // Add the rest if they fit.
        for (int ix = 0; ix != startcnt; ++ix, ++pos)
        {
            if (found.count(listdata[ixdata[pos]].index) == 0)
            {
                if (tosigned(current->lines.size()) >= TextSearchTreeBase::NODEFULLCOUNT)
                {
                    // Node is full. Backtrack so the rest of the items can be
                    // distributed in sub-nodes.
                    pos -= ix;
                    break;
                }

                found.insert(listdata[ixdata[pos]].index);
                current->lines.push_back(listdata[ixdata[pos]].index);
            }
        }

        current->lines.shrink_to_fit();
    }

    current->sum = tosigned(current->lines.size());
    TextNode *n = current;
    while (n->parent)
    {
        n->parent->sum += n->sum;
        n = n->parent;
    }

    ++lablen;

    return pos != tosigned(indexes.size());
}

int TreeBuilder::importSize() const
{
    return tosigned(indexes.size());
}

int TreeBuilder::importPos() const
{
    return pos;
}




