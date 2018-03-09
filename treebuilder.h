/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef TREEBUILDER_H
#define TREEBUILDER_H

#include "words.h"

class DictImport;

// Class used for fast building a tree from custom data, avoiding having to
// add every item one by one.
// Start with calling initNext() until it returns false. Then do the same with
// sortNext().
class TreeBuilder
{
public:
    // Pass the number of strings in size and a function that returns the
    // strings of a given index. The first value is the index, and the second
    // is a string list that should be filled with all lower case tokenized
    // parts of the item at index.
    // Pass a function in callback which will be called during some stages
    // of the tree building, to let the user interface respond to user input
    // or repainting. If callback returns false, the building of the tree
    // is interrupted.
    TreeBuilder(TextSearchTreeBase &tree, int size, const std::function<void (int, QStringList&)> &func, const std::function<bool()> &callback = std::function<bool()>());

    // Call initNext() until it returns false to collect the strings of
    // every item.
    bool initNext();
    // Number of words to init.
    int initSize() const;
    // Position in the initialization process.
    int initPos() const;

    // Creates a new node and places the unprocessed items in it that fit.
    // Returns true if the job is not finished yet. Call sortNext() until it
    // returns false to place every item in their correct node.
    bool sortNext();

    // Number of items that were imported and need to be sorted in the tree.
    int importSize() const;
    // Current item position.
    int importPos() const;
private:
    TextSearchTreeBase &tree;
    
    int size;
    std::function<void (int, QStringList&)> func;
    std::function<bool()> callback;

    // List of strings already added to textmap pointing to their textmap
    // position. After the initialization this map is cleared and becomes
    // invalid.
    std::map<const QCharString, int> added;

    // Holds every string in every word used for distributing the words in
    // the nodes. The textmap is a huge vector of characters without spaces
    // or delimiters. The Item struct's strpos refers to positions in textmap.
    std::vector<QChar> textmap;

    // Position in the initialization. Words of initpos count have already
    // been added to the textmap.
    int initpos;

    struct Item
    {
        // Index in textmap to the start of the string.
        int strpos;
        // Length of string.
        int len;
        // Index of word.
        int index;
    };
    // Each item in list holds a string and a word index. The string is found
    // in the word.
    std::vector<Item> list;

    // An ordering of list. Each value represents an index in list.
    std::vector<int> indexes;

    // The text node that will be the parent of the indexes added in it.
    TextNode *current;
    // Length of the labels being considered for insertion in current.
    int lablen;
    // Position in indexes. Items before pos are distributed in nodes already.
    int pos;

    typedef TextSearchTree  base;
};


#endif // TREEBUILDER_H
