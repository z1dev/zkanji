/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <set>
#include "searchtree.h"
#include "zkanjimain.h"
#include "treebuilder.h"

#include "checked_cast.h"


//-------------------------------------------------------------


TextNode::TextNode(TextNode *parent, const QChar *newlabel, int lblength) : parent(parent), nodes(this), sum(0)
{
    if (newlabel != nullptr && newlabel[0] != 0)
        label.copy(newlabel, lblength);
}

TextNode::TextNode(TextNode *parent) : parent(parent), nodes(this), sum(0)
{
}

TextNode::TextNode(TextNode &&src) : parent(nullptr), lines(std::move(src.lines)), nodes(std::move(src.nodes), this), sum(0)
{
    std::swap(label, src.label);
    std::swap(parent, src.parent);
    std::swap(sum, src.sum);
}

TextNode& TextNode::operator=(TextNode &&src)
{
    std::swap(label, src.label);
    std::swap(parent, src.parent);
    nodes.swap(src.nodes, this);
    std::swap(lines, src.lines);
    std::swap(sum, src.sum);

    return *this;
}

TextNode::~TextNode()
{
}

void TextNode::load(QDataStream &stream)
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

    quint16 u16;
    stream >> u16;

    nodes.reserve(u16);
    for (int ix = 0; ix < u16; ++ix)
    {
        TextNode *n = new TextNode(this);
        nodes.addNode(n, false);
        n->load(stream);
        sum += n->sum;
    }
}

void TextNode::save(QDataStream &stream) const
{
    stream << make_zstr(label, ZStrFormat::Byte);

    stream << (quint32)lines.size();
    for (int ix = 0, siz = tosigned(lines.size()); ix != siz; ++ix)
        stream << (qint32)lines[ix];

    stream << (quint16)nodes.size();
    for (int ix = 0, siz = tosigned(nodes.size()); ix != siz; ++ix)
        nodes.items(ix)->save(stream);
}

void TextNode::copy(TextNode *src)
{
    lines = src->lines;
    nodes.clear();
    nodes.copy(&src->nodes);
}


//-------------------------------------------------------------


TextNodeList::TextNodeList(TextNode *owner) : owner(owner)
{
}

//TextNodeList::TextNodeList(TextNodeList &&src) : list(std::move(src.list))
//{
//    owner = src.owner;
//}

TextNodeList::TextNodeList(TextNodeList &&src, TextNode *owner) : list(), owner(nullptr)
{
    swap(src, owner);
}

//TextNodeList& TextNodeList::operator=(TextNodeList &&src)
//{
//    std::swap(std::move(list), std::move(src.list));
//    std::swap(owner, src.owner);
//    return *this;
//}

//TextNodeList& TextNodeList::operator=(TextNodeList &&src)
//{
//    std::swap(std::move(list), std::move(src.list));
//    std::swap(parent, src.parent);
//
//    return *this;
//}

TextNodeList::~TextNodeList()
{
    clear();
}

void TextNodeList::swap(TextNodeList &src, TextNode *o)
{
    std::swap(list, src.list);
    owner = o;
}

void TextNodeList::copy(TextNodeList *src)
{
    if (src == this)
        return;

    list.clear();
    list.reserve(src->list.size());

    for (int ix = 0, siz = tosigned(src->list.size()); ix != siz; ++ix)
    {
        list.push_back(new TextNode(owner, src->list[ix]->label.data(), -1));
        list.back()->copy(src->list[ix]);
    }
}

TextNodeList::iterator TextNodeList::begin()
{
    return list.begin();
}

TextNodeList::iterator TextNodeList::end()
{
    return list.end();
}

void TextNodeList::clear()
{
    list.clear();
}

TextNode* TextNodeList::addNode(const QChar *label, int length, bool findpos)
{
    // label has to be lower case. Originally with GenLower

    TextNode *n = new TextNode(owner, label, length);
    if (!findpos)
        list.push_back(n);
    else
        list.insert(list.begin() + nodePosition(label, length), n);

    return n;
}

TextNode* TextNodeList::addNode()
{
    TextNode *n = new TextNode(owner);
    list.push_back(n);

    return n;
}

void TextNodeList::addNode(TextNode *node, bool findpos)
{
    node->parent = owner;
    if (!findpos)
        list.push_back(node);
    else
        list.insert(list.begin() + nodePosition(node->label.data()), node);
}

TextNode* TextNodeList::removeNode(int index)
{
    TextNode *node;
    list.removeAt(list.begin() + index, node);
    return node;
}

void TextNodeList::deleteNode(TextNode *node)
{
    list.erase(list.findPointer(node));
}

void TextNodeList::sort()
{
    std::sort(list.begin(), list.end(), [](const TextNode *a, const TextNode *b) {
        return a->label.compare(b->label) < 0;
    });
}

TextNode* TextNodeList::items(int index)
{
    return list[index];
}

const TextNode* TextNodeList::items(int index) const
{
    return list[index];
}

TextNodeList::size_type TextNodeList::size() const
{
    return list.size();
}

bool TextNodeList::empty() const
{
    return list.empty();
}

void TextNodeList::reserve(int alloc)
{
    list.reserve(alloc);
}

TextNode* TextNodeList::ownerNode() const
{
    return owner;
}

TextNode* TextNodeList::searchContainer(const QChar *str, int length)
{
    TextNode *node1, *node2 = nullptr;

    if (length == -1)
        length = tosigned(qcharlen(str));

    //node1 = nodeSearch(c);
    int mid = 0;
    int n;
    int min = 0;
    int max = tosigned(list.size()) - 1;
    QChar *label;
    int lblen;

    while (max >= min)
    {
        mid = (min + max) / 2;

        label = list[mid]->label.data();
        lblen = tosigned(qcharlen(label));
        n = qcharncmp(label, str, std::min(lblen, length)); //GenCompareIN
        if (length < lblen && n == 0)
            n = 1;

        if (n > 0)
            max = mid - 1;
        else if (n < 0)
            min = mid + 1;
        else
            break;
    }

    if (min > max)
        return nullptr;

    node1 = list[mid];

    while (node1)
    {
        node2 = node1;
        node1 = node1->nodes.searchContainer(str, length);
    }

    return node2;
}

 const TextNode* TextNodeList::searchContainer(const QChar *str, int length) const
{
    const TextNode *node1, *node2 = nullptr;

    if (length == -1)
        length = tosigned(qcharlen(str));

    //node1 = nodeSearch(c);
    int mid = 0;
    int n;
    int min = 0;
    int max = tosigned(list.size()) - 1;
    QChar *label;
    int lblen;

    while (max >= min)
    {
        mid = (min + max) / 2;

        label = list[mid]->label.data();
        lblen = tosigned(qcharlen(label));
        n = qcharncmp(label, str, std::min(lblen, length)); //GenCompareIN
        if (length < lblen && n == 0)
            n = 1;

        if (n > 0)
            max = mid - 1;
        else if (n < 0)
            min = mid + 1;
        else
            break;
    }

    if (min > max)
        return nullptr;

    node1 = list[mid];

    while (node1)
    {
        node2 = node1;
        node1 = node1->nodes.searchContainer(str, length);
    }

    return node2;
}

void TextNodeList::collectLines(std::vector<int> &result, const QChar *str, int length)
{
    if (length == -1)
        length = tosigned(qcharlen(str));

    TextNode *n;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        n = list[ix];
        int nlen = tosigned(n->label.size());
        if (qcharncmp(str, n->label.data(), std::min(length, nlen)))
            continue;

        result.insert(result.end(), n->lines.begin(), n->lines.end());
        n->nodes.collectLines(result, str, length);
    }

}

int TextNodeList::removeLine(int line, bool deleted)
{
    int removed = 0;
    for (int ix = tosigned(size()) - 1; ix != -1; --ix)
    {
        removed += list[ix]->nodes.removeLine(line, deleted);
        if (list[ix]->lines.empty() && list[ix]->nodes.empty() && owner != nullptr)
            list.erase(list.begin() + ix);
    }

    if (owner != nullptr)
    {
        auto it = owner->lines.begin();
        while (it != owner->lines.end())
        {
            int &ix = *it;
            if (ix == line)
            {
                it = owner->lines.erase(it);
                ++removed;
                continue;
            }
            else if (deleted && ix > line)
                --ix;
            ++it;
        }

        owner->sum -= removed;
    }
    return removed;
}

int TextNodeList::nodePosition(const QChar *label, int length)
{
    if (length == -1)
        length = tosigned(qcharlen(label));

    int min = 0, max = tosigned(list.size()) - 1, mid;
    int cmp;
    TextNode *n;

    while (min <= max)
    {
        mid = (max + min) / 2;

        n = list[mid];
        cmp = qcharncmp(label, n->label.data(), length);
        if (cmp == 0 && length < tosigned(n->label.size()))
            cmp = -1;

        if (cmp < 0)
            max = mid - 1;
        if (cmp > 0)
            min = mid + 1;
        if (cmp == 0)
        {
            min = mid;
            break;
        }
    }

    return min;
}


//-------------------------------------------------------------

const int TextSearchTreeBase::NODEFULLCOUNT = 300;
//const int TextSearchTreeBase::NODETOOMUCHCOUNT = 5000;

TextSearchTreeBase::TextSearchTreeBase(/*bool createbase,*/) : nodes(nullptr),
/*createbase(createbase),*/ cache(nullptr)
{
    //if (createbase)
    //{
    //    for (ushort c = 'a'; c <= 'z'; ++c)
    //        nodes.addNode(&QChar(c), 1, false);
    //}
}

//TextSearchTreeBase::TextSearchTreeBase(TextSearchTreeBase &&src) : nodes(std::move(src.nodes), nullptr), cache(nullptr),
//        blacklisting(src.blacklisting), blmin(src.blmin), blmax(src.blmax), blwords(std::move(src.blwords))
//{
//
//}
//
//TextSearchTreeBase& TextSearchTreeBase::operator=(TextSearchTreeBase &&src)
//{
//    nodes.swap(src.nodes, nullptr);
//    cache = nullptr;
//    std::swap(blacklisting, src.blacklisting);
//    std::swap(blmin, src.blmin);
//    std::swap(blmax, src.blmax);
//    std::swap(blwords, src.blwords);
//    return *this;
//}

TextSearchTreeBase::~TextSearchTreeBase()
{
}

void TextSearchTreeBase::load(QDataStream& stream)
{
    cache = nullptr;
    qint32 nodecnt;

    quint16 ui;
    stream >> ui;
    nodecnt = ui;
    for (int ix = 0; ix < nodecnt; ++ix)
    {
        nodes.addNode();

        nodes.items(ix)->parent = nullptr;
        nodes.items(ix)->load(stream);
    }

}

void TextSearchTreeBase::save(QDataStream &stream) const
{
    stream << (quint16)nodes.size();
    for (int ix = 0, siz = tosigned(nodes.size()); ix != siz; ++ix)
        nodes.items(ix)->save(stream);

}

void TextSearchTreeBase::clear()
{
    cache = nullptr;
    nodes.clear();
}

void TextSearchTreeBase::swap(TextSearchTreeBase &src)
{
    nodes.swap(src.nodes, nullptr);
    cache = nullptr;
}

void TextSearchTreeBase::copy(TextSearchTreeBase *src)
{
    if (this == src)
        return;

    nodes.copy(&src->nodes);
    cache = nullptr;
}

TextNodeList& TextSearchTreeBase::getNodes()
{
    return nodes;
}

bool TextSearchTreeBase::findContainer(const QChar *str, int length, TextNode* &result)
{
    if (str == nullptr || length == 0) // Error
        throw "Replace throws with some other thingy.";

    if (length == -1)
        length = tosigned(qcharlen(str));

    result = cache;
    TextNode *n;

    QChar cfirst = str[0];

    if (result == nullptr || (result != nullptr && result->label[0] != cfirst))
    {
        int min = 0;
        int mid = 0;
        int max = tosigned(nodes.size()) - 1;
        int cmp;
        while (min <= max)
        {
            mid = (max + min) / 2;
            cmp = cfirst.unicode() - nodes.items(mid)->label[0].unicode();

            if (cmp < 0)
                max = mid - 1;
            else if (cmp > 0)
                min = mid + 1;
            else
                break;
        }
        if (min > max)
        {
            result = nullptr;
            return false;
        }

        result = nodes.items(mid);

        if (length == 1)
        {
            cache = result;
            return true; // Exact match
        }
    }

    int slen = tosigned(result->label.size());

    // Get out of selected node while it's not a container of the searched one.
    while (slen > length || qcharncmp(str, result->label.data(), slen)) // GenCompareIN
    {
        if (!result->parent)
            return false;
        result = result->parent;
        slen = result->label.size();
    }
    if (length == slen && !qcharncmp(str, result->label.data(), length)) // GenCompareI
        return true;

    n = result->nodes.searchContainer(str, length);
    if (n)
    {
        result = n;
        slen = result->label.size();
    }

    if (length == slen && !qcharncmp(str, result->label.data(), length)) //!GenCompareI(selected->label, c);
    {
        cache = result;
        return true;
    }
    return false;
}

bool TextSearchTreeBase::findContainer(const QChar *str, int length, const TextNode* &result) const
{
    if (str == nullptr || length == 0) // Error
        throw "Replace throws with some other thingy.";

    if (length == -1)
        length = tosigned(qcharlen(str));

    result = cache;
    const TextNode *n;

    QChar cfirst = str[0];

    if (result == nullptr || (result != nullptr && result->label[0] != cfirst))
    {
        int min = 0;
        int mid = 0;
        int max = tosigned(nodes.size()) - 1;
        int cmp;
        while (min <= max)
        {
            mid = (max + min) / 2;
            cmp = cfirst.unicode() - nodes.items(mid)->label[0].unicode();

            if (cmp < 0)
                max = mid - 1;
            else if (cmp > 0)
                min = mid + 1;
            else
                break;
        }
        if (min > max)
        {
            result = nullptr;
            return false;
        }

        result = nodes.items(mid);

        if (length == 1)
        {
            cache = const_cast<TextNode*>(result);
            return true; // Exact match
        }
    }

    int slen = result->label.size();

    // Get out of selected node while it's not a container of the searched one.
    while (slen > length || qcharncmp(str, result->label.data(), slen)) // GenCompareIN
    {
        if (!result->parent)
            return false;
        result = result->parent;
        slen = result->label.size();
    }
    if (length == slen && !qcharncmp(str, result->label.data(), length)) // GenCompareI
        return true;

    n = result->nodes.searchContainer(str, length);
    if (n)
    {
        result = n;
        slen = result->label.size();
    }

    if (length == slen && !qcharncmp(str, result->label.data(), length)) //!GenCompareI(selected->label, c);
    {
        cache = const_cast<TextNode*>(result);
        return true;
    }
    return false;
}

TextNode* TextSearchTreeBase::createRoot(QChar ch)
{
#ifdef _DEBUG
    for (int ix = tosigned(nodes.size()) - 1; ix != -1; --ix)
        if (nodes.items(ix)->label[0] == ch && nodes.items(ix)->label[1] == 0)
            throw "This node already exists.";
#endif

    return nodes.addNode(&ch, 1, true);
}

//int TextSearchTreeBase::doMoveFromFullNode(TextNode *node, int index)
//{
//    if (isKana())
//    {
//        QStringList texts;
//        doGetWord(node->lines[index], texts); // AnsiLowerCase - must be done in the implementation of doGetWord
//        QString k = texts.at(0);
//
//        int slen = node->label.size();
//        QString s;
//        TextNode *n;
//
//        int klen = k.size();
//        if (klen == slen) // Don't do anything.
//            return 0;
//
//        int len = slen + 1;
//        while (len <= klen && blacklisted(k.constData(), len))
//            len++;
//        if (len > klen)
//            return 0;
//
//        const QChar *kc = k.constData();
//        n = node->nodes.searchContainer(kc, len);
//        if (!n || qcharncmp(n->label.data(), kc, len) || n->label.data()[len].unicode() != 0)
//        {
//            n = node->nodes.addNode(kc, len, true);
//            //node->nodes.sort();
//        }
//        n->lines.push_back(node->lines[index]);
//        ++n->sum;
//
//        node->lines.erase(node->lines.begin() + index);
//
//        return 0;
//    }
//
//    int result = 0;
//    //QChar *k;
//    int slen = node->label.size();
//    QChar *l;
//    const QChar *s;
//    TextNode *n;
//    
//    //const WordEntry* w = dict->wordEntry(node->lines[index]);
//    
//    // Add every word to this first.
//    //QStringList ms;
//    //
//    //bool fullmove = true;
//    //QCharString k;
//    //for (int ix = 0; ix < w->defs.size(); ++ix)
//    //{
//    //    k.copy(w->defs[ix].def.toLower().constData()); // AnsiStrLower
//    //    QCharTokenizer tokens(k.data());
//    //
//    //    while (tokens.next())
//    //    {
//    //        if (tokens.tokenSize() >= slen && !qcharncmp(node->label.data(), tokens.token(), slen))
//    //        {
//    //            if (tokens.tokenSize() == slen) // Fits this node well
//    //                fullmove = false;
//    //            else if (!blacklisted(tokens.token(), tokens.tokenSize()))
//    //                ms << QString(tokens.token(), tokens.tokenSize());
//    //        }
//    //    }
//    //}
//
//    QStringList texts;
//    doGetWord(node->lines[index], texts); // AnsiLowerCase - must be done in the implementation of doGetWord
//
//    bool fullmove = true;
//    for (int ix = texts.size() - 1; ix != -1; --ix)
//    {
//        if (texts.at(ix).size() < slen)
//        {
//            texts.removeAt(ix);
//            continue;
//        }
//        if (texts.at(ix).size() == slen)
//        {
//            fullmove = false;
//            texts.removeAt(ix);
//            continue;
//        }
//        if (blacklisted(texts.at(ix).constData(), texts.at(ix).size()))
//            texts.removeAt(ix);
//    }
//
//    std::sort(texts.begin(), texts.end(), [](const QString &a, const QString &b) { return a < b; });
//    texts.erase(std::unique(texts.begin(), texts.end()), texts.end());
//    
//    for (int ix = 0; ix < texts.size(); ++ix)
//    {
//        // Find length of new node to create
//        int len = slen + 1;
//        while (len <= texts.at(ix).size() && blacklisted(texts.at(ix).constData(), len))
//            len++;
//    
//            
//        if (len > texts.at(ix).size())
//        {
//            // If blacklisted, delete.
//            texts.removeAt(ix);
//        }
//        else
//        { 
//            // Delete everything that would go to the same node.
//            while (texts.size() > ix + 1 && !qcharncmp(texts.at(ix).constData(), texts.at(ix + 1).constData(), len))
//                texts.removeAt(ix + 1);
//    
//            s = texts.at(ix).constData();
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

void TextSearchTreeBase::doExpand(int index, bool inserted)
{
    if (inserted)
    {
        // Increase all lines with index equal or higher by one.
        walkthrough((intptr_t)index, [](TextNode *n, intptr_t value)
        {
            for (int ix = 0, siz = tosigned(n->lines.size()); ix != siz; ++ix)
                if (n->lines[ix] >= (int)value)
                    ++n->lines[ix];
        });
    }

    QStringList texts;
    doGetWord(index, texts);
    std::sort(texts.begin(), texts.end(), [](const QString &a, const QString &b) { return a < b; });
    texts.erase(std::unique(texts.begin(), texts.end()), texts.end());

#ifdef _DEBUG
    if (texts.empty())
        throw "Can't expand with empty string.";
#endif

    TextNode *node;

    for (const QString &s : texts)
    {
        int ssiz = s.size();
        const QChar *str = s.constData();

        findContainer(str, ssiz, node);
        if (node == nullptr)
            cache = node = createRoot(s.at(0).unicode());
        else if (std::find(node->lines.begin(), node->lines.end(), index) != node->lines.end())
            continue;

        // Line to be added to the node matches the node's label. It couldn't be put in a
        // child node.
        bool match = (ssiz == tosigned(node->label.size()));

        if ((node->lines.size() < NODEFULLCOUNT && node->nodes.empty()) || (match && !node->nodes.empty()))
        {
            // Nothing to do if there's still space in the node or its items have already been
            // distributed.
            node->lines.push_back(index);

            ++node->sum;
            TextNode *n = node;
            while (n->parent)
            {
                ++n->parent->sum;
                n = n->parent;
            }

            continue;
        }

        if (node->lines.size() >= NODEFULLCOUNT && node->nodes.empty())
        {
            TextNode *newnode = node;
            TextNode *newcache = node;

            // Node is full and there are no sub-nodes created yet.
            while (newnode->lines.size() >= NODEFULLCOUNT && newnode->nodes.empty())
            {
                distributeChildren(newnode);
                if (match)
                    break;

                findContainer(str, ssiz, newnode);

                match = (ssiz == tosigned(newnode->label.size()));
                if (!match && ((newcache == newnode && newnode->lines.size() >= NODEFULLCOUNT) || !newnode->nodes.empty()))
                {
                    newnode = newnode->nodes.addNode(str, newnode->label.size() + 1, true);
                    break;
                }
                newcache = newnode;
            }

            newnode->lines.push_back(index);

            ++newnode->sum;
            TextNode *n = newnode;
            while (n->parent)
            {
                ++n->parent->sum;
                n = n->parent;
            }

            cache = node;
            continue;
        }

        // When: (!match && !node->nodes.empty())
        // Create a new sub node where the line can be put.
        node = node->nodes.addNode(str, node->label.size() + 1, true);
        node->lines.push_back(index);

        ++node->sum;
        TextNode *n = node;
        while (n->parent)
        {
            ++n->parent->sum;
            n = n->parent;
        }
    }
}

void TextSearchTreeBase::distributeChildren(TextNode *parent)
{
    // Collect the text of every line in parent which fits in the parent node
    // and future sub-nodes.

    QString label = parent->label.toQStringRaw();
    int labellen = tosigned(label.size());

    std::vector<std::pair<QString, int>> lines;

    for (int ix = 0, siz = tosigned(parent->lines.size()); ix != siz; ++ix)
    {
        int index = parent->lines[ix];

        // Get texts of line.
        QStringList texts;
        doGetWord(index, texts);
        std::sort(texts.begin(), texts.end(), [](const QString &a, const QString &b) { return a < b; });
        auto textsend = std::unique(texts.begin(), texts.end());

        // Find interval in texts which fits in parent.
        auto textsbegin = std::lower_bound(texts.begin(), textsend, label);

        textsend = std::upper_bound(textsbegin, textsend, label, [&label, labellen](const QString &a, const QString &b) {
            bool aeq = !qcharncmp(a.constData(), label.constData(), labellen);
            bool beq = !qcharncmp(b.constData(), label.constData(), labellen);
            if (aeq == beq)
                return false;
            return aeq;
        });

        // Save the texts that fit in parent with the current line index.
        for (auto it = textsbegin; it != textsend; ++it)
            lines.emplace_back(*it, index);
    }

    // Lines contain all the texts with the associated line indexes that fit
    // in parent.
    std::sort(lines.begin(), lines.end(), [](const std::pair<QString, int> &a, const std::pair<QString, int> &b) {
        int cmp = qcharcmp(a.first.constData(), b.first.constData());
        if (cmp == 0)
            return a.second < b.second;
        return cmp < 0;
    });

    // Lines is sorted, the first ones are those that match the parent node
    // and should stay there too.
    parent->lines.clear();

    int ix = 0;
    for (int siz = tosigned(lines.size()); ix != siz; ++ix)
    {
        if (lines[ix].first.size() != labellen)
            break;
        parent->lines.push_back(lines[ix].second);
    }

    // The sum of lines must be valid at every step as the function can call
    // itself recursively.
    TextNode *n = parent;
    while (n->parent)
    {
        n->parent->sum += tosigned(parent->lines.size()) - parent->sum;
        n = n->parent;
    }
    parent->sum = tosigned(parent->lines.size());

    std::set<int> added;
    int lsiz = tosigned(lines.size());
    while (ix != lsiz)
    {
        QChar c = lines[ix].first.at(labellen);
        TextNode *node = parent->nodes.addNode(lines[ix].first.constData(), labellen + 1, false);
        while (ix != lsiz && lines[ix].first.at(labellen) == c)
        {
            if (added.count(lines[ix].second) == 0)
            {
                node->lines.push_back(lines[ix].second);
                added.insert(lines[ix].second);
            }
            ++ix;
        }

        node->sum = tosigned(node->lines.size());
        // Fixing the sum.
        n = node;
        while (n->parent)
        {
            n->parent->sum += node->sum;
            n = n->parent;
        }

        // It's possible that every line has been added to a single node and
        // made it full.
        if (node->lines.size() > NODEFULLCOUNT)
            distributeChildren(node);
        added.clear();
    }
}

//void TextSearchTreeBase::expand(const QString &str, int index, bool insert)
//{
//    if (str.isEmpty())
//        throw "Congralutations! You've found a bug! Expanding dictionary with empty string.";
//
//    if (insert)
//    {
//        auto func = [](TextNode *n, void *value)
//        {
//            for (int ix = 0; ix != n->lines.size(); ++ix)
//                if (n->lines.at(ix) >= (int)value)
//                    ++n->lines[ix];
//        };
//        walkthrough(func, (void*)index);
//    }
//
//    if (blacklisted(str.constData(), str.size()))
//        return;
//
//    QString lstr = str.toLower(); // AnsiStrLower
//
//    int cntdiff = 0;
//
//    TextNode *node;
//    findContainer(lstr.constData(), lstr.size(), node);
//    if (node == nullptr)
//        cache = node = createRoot(lstr.at(0).unicode());
//
//    if (std::find(node->lines.begin(), node->lines.end(), index) != node->lines.end())
//        return;
//
//    uint slen = node->label.size();
//    bool match = !qcharcmp(lstr.constData(), node->label.data());
//
//    if (match || (node->lines.size() < NODEFULLCOUNT && !node->nodes.size()))
//    {
//        if (node->parent && node->lines.size() >= NODETOOMUCHCOUNT) // Can only happen on match, blacklist!
//        {
//            blacklist(lstr.constData(), lstr.size());
//            cntdiff = -node->lines.size();
//            while (node->nodes.size())
//                node->parent->nodes.addNode(node->nodes.removeNode(0), false);
//            node->parent->nodes.sort();
//            TextNode *sel = node->parent;
//            node->parent->nodes.deleteNode(node);
//
//            //delete[] c;
//            //c = nullptr;
//
//            cache = node = sel;
//        }
//        else
//        {
//            node->lines.push_back(index);
//            cntdiff = 1;
//        }
//    }
//    else if (!node->nodes.size()) // The node is full, move everything to a new level, do everything else only after that.
//    {
//        for (int ix = node->lines.size() - 1; ix != -1; --ix)
//            cntdiff += doMoveFromFullNode(node, ix);
//    }
//
//    if (!match && node->nodes.size()) // We have to create a new node for this word only.
//    {
//
//        unsigned int len = slen + 1;
//        const QChar *ldata = lstr.constData();
//        while (len <= lstr.size() && blacklisted(ldata, len))
//            len++;
//        if (len <= lstr.size())
//        {
//            TextNode *n = node->nodes.searchContainer(ldata, len);
//            if (!n || qcharncmp(n->label.data(), ldata, len) || n->label.data()[len].unicode() != 0)
//            {
//                n = node->nodes.addNode(ldata, len, true);
//                //node->nodes.sort();
//            }
//            n->lines.push_back(index);
//            ++n->sum;
//
//            ++cntdiff;
//        }
//    }
//
//    do
//    {
//        node->sum += cntdiff;
//        cache = node = node->parent;
//    } while (node);
//}

void TextSearchTreeBase::removeLine(int line, bool deleted)
{
    nodes.removeLine(line, deleted);
}

void TextSearchTreeBase::walkReq(TextNode *n, intptr_t data, std::function<void(TextNode*, intptr_t)> func)
{
    func(n, data);
    for (int ix = 0, siz = tosigned(n->nodes.size()); ix != siz; ++ix)
        walkReq(n->nodes.items(ix), data, func);
}

void TextSearchTreeBase::walkthrough(intptr_t data, std::function<void(TextNode*, intptr_t)> afunc)
{
    for (int ix = 0, siz = tosigned(nodes.size()); ix != siz; ++ix)
        walkReq(nodes.items(ix), data, afunc);
}

void TextSearchTreeBase::rebuild(const std::function<bool()> &callback)
{
    cache = nullptr;
    nodes.clear();

    TreeBuilder rebuilder(*this, tosigned(size()), [this](int ix, QStringList &list) { doGetWord(ix, list); }, callback);

    while (rebuilder.initNext())
    {
        if (callback && !callback())
            return;
    }
    while (rebuilder.sortNext())
    {
        if (callback && !callback())
            return;
    }
}

void TextSearchTreeBase::getSiblings(std::vector<int> &result, const QChar *c, int clen)
{
    TextNode *n;
    findContainer(c, clen, n);

    if (n == nullptr)
    {
        result.clear();
        return;
    }

    result = n->lines;
}


//-------------------------------------------------------------


