/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef SEARCHTREE_H
#define SEARCHTREE_H

#include <functional>
#include <QStringList>
#include "smartvector.h"
#include "qcharstring.h"


// The abstract class TextSearchTreeBase holds 'line' indexes in a tree
// structure. It is used for fast lookup of these indexes by a search string.
// Nodes: Every node in the tree has a label string. Every child node's label
// is the same as the parent's label + 1 character. The labels are all unique.
// There is no top level root node, but several nodes of 1 character long
// label strings. The nodes are sorted in their direct parent by character
// value based string ordering. The labels must be all lower case.
// Lines: A line is a single string with one or more searchable tokens. Nodes
// have a list of integer line indexes only, not the lines themselves.
// These indexes must be unique within a single node, but can be placed in any
// number of nodes. There's no special ordering for the indexes. When
// populating the table, line indexes are added to the line list of the nodes
// that match the indexes.
// Searching in the tree: corresponding line indexes can be looked up by a
// search string. First a node is located that matches the search string the
// closest. The label will be the leading substring of the search string, but
// it doesn't have to exactly match the search string's length. Every index is
// returned from this node and all sub-nodes. The returned indexes must be
// filtered for the search string, it's not done by the tree.
// Populating the tree: The tree can be built by a TreeBuilder object (not
// described here), or by adding lines one by one. Line adding is done by
// implementing the doGetWord() function in a sub class which returns a list
// of token strings for a given line index, and calling doExpand() for that
// index.
// Line adding is done for every token one by one. The matching node is looked
// up for the token. If no such node is found, a 1 character long node is
// created for it at the root of the tree. If the found node has sub-nodes
// and the token is longer than the node's label, a new sub-node is created
// for it instead.
// If the found node has no sub-nodes and it's not full yet (has less indexes
// in the line list than a certain limit), the index is added unless it's
// already present.
// If the node is full, the lines listed are all moved to new sub-nodes. Lines
// with a token that matches the node are not moved, though they can be added
// to sub nodes if the line has multiple tokens and one of them fits in a
// sub-node.
// The line index to be added is then added to the matching sub-node.
// Removing a line: when a line is deleted, it's index must be removed from
// all nodes that hold it. Nodes that become empty with no sub-nodes are
// removed as well. If all lines in sub-nodes of a node fall below the full
// limit, those are not moved back to the node.

struct TextNode;
class TextNodeList
{
public:
    typedef smartvector<TextNode>::iterator    iterator;
    typedef size_t  size_type;

    TextNodeList() = delete;
    TextNodeList(const TextNodeList &src) = delete;
    TextNodeList& operator=(TextNodeList &&src) = delete;
    TextNodeList& operator=(const TextNodeList &src) = delete;

    TextNodeList(TextNode *owner);
    //TextNodeList(TextNodeList &&src);
    TextNodeList(TextNodeList &&src, TextNode *owner);
    virtual ~TextNodeList();

    // Acts like a move operator=, swapping the contents while keeping the old owner.
    void swap(TextNodeList &src, TextNode *owner);

    // Creates a copy of src which can work independently.
    void copy(TextNodeList *src);

    iterator begin();
    iterator end();

    void clear();
    // Adds a new node to the node list with the given label. The label must
    // be a generic lower case string.
    // Set findpos to true if the node should be added at its correct
    // position. When findpos is false, the new node is added at the end of
    // the list of nodes.
    TextNode* addNode(const QChar *label, int labellength, bool findpos);
    // Adds a node with an empty label.
    TextNode* addNode();
    // Adds a created node, which is not part of the tree to the node list.
    // Set findpos to true if the node should be added at its correct
    // position. When findpos is false, the new node is added at the end of
    // the list of nodes.
    void addNode(TextNode *node, bool findpos);
    // Removes the node at the given index, giving ownership of it to the
    // caller.
    TextNode* removeNode(int index);
    // Deletes a node and all its children. The node must be in this node
    // list.
    void deleteNode(TextNode *node);
    // Sorts the nodes by their labels with memcmp.
    void sort();

    size_type size() const;
    bool empty() const;
    void reserve(int alloc);
    TextNode* items(int index);
    const TextNode* items(int index) const;
    TextNode* ownerNode() const;

    // Returns a child node at the deepest possible level, which could store the passed string.
    TextNode* searchContainer(const QChar *str, int strlength);
    // Returns a child node at the deepest possible level, which could store the passed string.
    const TextNode* searchContainer(const QChar *str, int strlength) const;

    // Adds every line in this branch, whose node's label matches str. The result might contain
    // invalid finds (the word is longer than the label and doesn't match str), duplicates
    // and it is not sorted in any way.
    void collectLines(std::vector<int> &result, const QChar *str, int strlength = -1);

    // Removes a word from every node with the passed line index. If the word is deleted, all other
    // indices are decremented by one. Returns the number of items removed.
    int removeLine(int line, bool deleted);
private:
    int nodePosition(const QChar *label, int labellength = -1);

    smartvector<TextNode> list;
    TextNode *owner;
};

struct TextNode
{
    QCharString label;
    TextNode *parent;

    // Indexes of items added to the node.
    std::vector<int> lines;
    // Child nodes directly below this node.
    TextNodeList nodes;

    // Number of lines inside this tree node, including those in child nodes.
    // Duplicates are counted as separate.
    int sum;

    void loadLegacy(QDataStream &stream, int version);
    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    // Copies the nodes and lines from source. The label and parent are unchanged.
    void copy(TextNode *src);

    TextNode() = delete;
    TextNode(const TextNode &other) = delete;
    TextNode& operator=(const TextNode &&other) = delete;

    // Constructs a new node with the specified label and parent.
    TextNode(TextNode *parent, const QChar *label, int labellength);
    // Constructs a new node with an empty label and specified parent.
    TextNode(TextNode *parent);

    TextNode(TextNode &&src);
    TextNode& operator=(TextNode &&src);
    ~TextNode();
};

class TextSearchTreeBase
{
public:
    // Number of items in a node after which the items must be distributed in child nodes.
    // Only items with the same string as the node's label will be kept in the node and not in
    // child nodes.
    static const int NODEFULLCOUNT;
    // If at least this many same string items as the node's label are placed in a node, they
    // must be blacklisted.
    //static const int NODETOOMUCHCOUNT;

    TextSearchTreeBase(const TextSearchTreeBase&) = delete;
    TextSearchTreeBase& operator=(const TextSearchTreeBase &src) = delete;

    TextSearchTreeBase(TextSearchTreeBase &&src) = delete;
    TextSearchTreeBase& operator=(TextSearchTreeBase &&src) = delete;

    TextSearchTreeBase(/*bool createbase,*/);
    virtual ~TextSearchTreeBase();


    virtual void clear();

    void swap(TextSearchTreeBase &src);

    void copy(TextSearchTreeBase *src);

    virtual TextNodeList& getNodes();

    // Should return true if the tree indexes romanized kana.
    virtual bool isKana() const = 0;
    // Should return true if the tree is used to look up reversed text.
    virtual bool isReversed() const = 0;

    // Adds a new word to the tree, under a new node if necessary. Set insert
    // to true, if word indexes that are the same or higher should be
    // incremented by one.
    //void expand(const QString &str, int index, bool insert);

    // Removes a word from every node with the passed line index. If the word is deleted, all
    // higher indices are decremented by one.
    void removeLine(int line, bool deleted);

    // Executes a function with every TextNode in the tree. Data will be/ passed to the
    // function as second argument.
    void walkthrough(intptr_t data, std::function<void(TextNode*, intptr_t)> func);

    // Returns a list of line indexes that were in the node that holds the line index of text
    // c.
    void getSiblings(std::vector<int> &result, const QChar *c, int clen = -1);
protected:
    virtual void loadLegacy(QDataStream &stream, int version);
    virtual void load(QDataStream &stream);
    virtual void save(QDataStream &stream) const;

    // Searches for a TextNode which matches the passed string, and updates result to point to
    // it. The string must be all lowercase with a generic lowercase function that works the
    // same way on every system. Returns whether the container is an exact match for the
    // string.
    bool findContainer(const QChar *str, int strlength, TextNode* &result);
    // Searches for a TextNode which matches the passed string, and updates result to point to
    // it. The string must be all lowercase with a generic lowercase function that works the
    // same way on every system. Returns whether the container is an exact match for the
    // string.
    bool findContainer(const QChar *str, int strlength, const TextNode* &result) const;

    // Creates a root node. The caller must make sure no node with the
    // starting character of ch exists, or a duplicate will be added.
    TextNode* createRoot(QChar ch);

    // Clears the tree and refills it from data provided by derived classes. Call when the
    // tree is in an incorrect state and expanding or removing single items can't fix it.
    // Pass a callback function which will be called periodically to let the system update the
    // user interface. Return false from the callback to interrupt the rebuilding, leaving the
    // tree in an invalid state. Call rebuild to make the tree valid or make sure it's not
    // used again.
    void rebuild(const std::function<bool()> &callback = std::function<bool()>());

    // Return an item's text at index in all lowercase characters, converted with a generic
    // lower case function. The text is then used to place or remove the item from nodes. If
    // multiple strings are returned, every node that matches will be used.
    virtual void doGetWord(int index, QStringList &texts) const = 0;

    // Return the number of lines in this tree.
    virtual size_t size() const = 0;

    // Notifies the tree that a line with the passed index has been added to the data
    // referenced by the tree. When doGetWord is called with this index, derived classes must
    // be able to return a valid string for it. Set inserted to true if the index of lines
    // with equal or higher index have increased.
    void doExpand(int index, bool inserted = false);

    TextNodeList nodes;
private:
    // When expanding the tree (adding new items), and the selected node gets
    // full (above 300 items in it), moves items with longer text than the old
    // node label in that node to new sub-nodes.
    //virtual int doMoveFromFullNode(TextNode *node, int index);

    // Called when the node becomes full and lines in it should be moved to child nodes. It's
    // possible a line stays in the node and also added to a child node when it has several
    // texts associated with it that fit different nodes.
    // Possibly adds multiple levels of sub-nodes if they become full when lines are moved
    // there from parent.
    void distributeChildren(TextNode *parent);

    // Used in walkthrough.
    void walkReq(TextNode *n, intptr_t data, std::function<void(TextNode*, intptr_t)> func);

    // Has nodes a to z on top of the nodes list.
    //bool createbase;

    // Stores the last accessed node. This value is only used for checking whether we try to
    // access the same node again.
    mutable TextNode *cache;
};

#endif

