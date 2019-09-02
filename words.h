/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDS_H
#define WORDS_H

//#include <QGlobal>
#include <QDateTime>
#include <QDataStream>
#include <QStringList>
//#include <qvector.h>

#include <memory>
#include <map>

#include "zkanjimain.h"
#include "fastarray.h"
#include "searchtree.h"

// Parts of a word entry used as flags. Default is only used for main hints.
enum class WordPartBits : uchar { Kanji = 0x01, Kana = 0x02, Definition = 0x04, Default = 0x08, AllParts = Kanji | Kana | Definition };

struct WordDefAttrib
{
    // Flags for the word's grammatical types. I.e. verb. See WordTypes in grammar.enums.h
    uint types = 0;
    // Flags for the word's notes. I.e. kana only. See WordNotes in grammar.enums.h
    uint notes = 0;
    // Flags for the word's fields. I.e. physics. See WordFields in grammar.enums.h
    uint fields = 0;
    // Flags for the word's dialect(s). I.e Kansai. See WordDialects in grammar.enums.h
    ushort dialects = 0;
};

bool operator!=(const WordDefAttrib &a, const WordDefAttrib &b);
bool operator==(const WordDefAttrib &a, const WordDefAttrib &b);
//QDataStream& operator<<(QDataStream &stream, const WordDefAttrib &a);
//QDataStream& operator>>(QDataStream &stream, WordDefAttrib &a);

struct WordDefinition
{
    // Text of a word's definition. The definition can be made up of multiple glosses, which
    // are then separated by the constant GLOSS_SEP_CHAR character.
    QCharString def;
    // Attributes of a word's definition.
    WordDefAttrib attrib;

    WordDefinition();
    WordDefinition(WordDefinition &&src);
    WordDefinition& operator=(WordDefinition &&src);
#if !defined(_MSC_VER) || _MSC_VER >= 1900
	WordDefinition(const WordDefinition &src) = default;
	WordDefinition& operator=(const WordDefinition &src) = default;
#endif
};

bool operator!=(const WordDefinition &a, const WordDefinition &b);
bool operator==(const WordDefinition &a, const WordDefinition &b);

// General use word flags not stored on disk.
// InGroup: the word is placed in at least one word group.
enum class WordRuntimeData
{
    InGroup
};

// Data of a single word entry in dictionaries.
struct WordEntry
{
    QCharString kanji;
    QCharString kana;
    // Simplified reading of the word's kana.
    QCharString romaji;

    // Frequency of usage. The higher the more frequent.
    ushort freq;
    // Flags of the word's information field. See WordInfo in grammar_enums.h.
    //// The bits above WordInfo::Count should not be saved in a dictionary. They represent
    //// the current state of a word. For example that it's in a group.
    uchar inf;

    // Flags from WordRuntimeData.
    uchar dat;

    fastarray<WordDefinition> defs;

    WordEntry() : freq(0), inf(0), dat(0) { ; }
};

// Structures for filtering in word listings.

// AnyCanMatch: Accept result if any property specified is found.
// AllMustMatch: Accept result if all properties specified are found.
enum class FilterMatchType { AnyCanMatch, AllMustMatch };

// Filter properties used to limit the words shown.
struct WordAttributeFilter
{
    QString name;

    WordDefAttrib attrib;
    uchar inf;
    uchar jlpt;

    FilterMatchType matchtype;
};

enum class Inclusion : uchar { Ignore, Include, Exclude };
struct WordFilterConditions
{
    std::vector<Inclusion> inclusions;

    Inclusion examples;
    Inclusion groups;
};

bool operator==(const WordFilterConditions &a, const WordFilterConditions &b);
bool operator!=(const WordFilterConditions &a, const WordFilterConditions &b);
bool operator!(const WordFilterConditions &a);

struct WordCommons;
class QXmlStreamWriter;
class QXmlStreamReader;

// TODO: rearranging filters.
class WordAttributeFilterList : public QObject
{
    Q_OBJECT
public:
    typedef size_t  size_type;

    WordAttributeFilterList(QObject *parent = nullptr);
    virtual ~WordAttributeFilterList();

    void saveXMLSettings(QXmlStreamWriter &writer) const;
    void loadXMLSettings(QXmlStreamReader &reader);

    // Number of filters in the list.
    size_type size() const;
    // Erases every filter and signals filterErased for each of them.
    void clear();
    // No filters found in filters list. The "in examples" and "in groups" filters are not
    // included.
    bool empty() const;
    // Returns the filter at index.
    const WordAttributeFilter& items(int index) const;
    // Returns the index of the filter with the passed name or -1 if the filter is not found.
    int itemIndex(const QString &name);
    // Returns the index of the filter with the passed name or -1 if the filter is not found.
    int itemIndex(const QStringRef &name);
    // Destroys a filter, removing it from the list and emiting filterErased.
    void erase(int index);
    // Updates the position of a filter at index, moving it in front of the to filter. If the
    // move doesn't make sense, the call returns without changing the filter list.
    void move(int index, int to);

    // Changes the name of the filter at index and emits the filterRenamed signal. It's the
    // caller's responsibility to avoid conflicts.
    void rename(int index, const QString &newname);
    // Changes the attributes of a word filter and emits the filterChanged signal.
    void update(int index, const WordDefAttrib &attrib, uchar info, uchar jlpt, FilterMatchType matchtype);
    // Creates a new filter with the given name and attributes and emits the filterCreated
    // signal.
    void add(const QString &name, const WordDefAttrib &attrib, uchar info, uchar jlpt, FilterMatchType matchtype);

    // Returns whether the passed word matches the filters inclusion list. Calls the other
    // match function with every filter not ignored and evaluates the result.
    bool match(const WordEntry *w, const WordFilterConditions *conditions) const;
signals:
    // Signaled when a new filter has been added.
    void filterCreated();
    // Signaled when a filter at index has been erased.
    void filterErased(int index);
    // Signaled when a filter at index has been updated. When the name changed filterRenamed
    // is signaled instead.
    void filterChanged(int index);
    // Signaled when a filter's name changed.
    void filterRenamed(int index);
    // Signaled after a filter was moved.
    void filterMoved(int index, int to);
private:
    // Returns whether the passed word matches the filter at index. Commons must be set if
    // it's needed (to look up JLPT of word).
    bool domatch(const WordEntry *w, const WordCommons *commons, int index) const;

    std::vector<WordAttributeFilter> list;

    typedef QObject base;
};


// The original version of a changed word in the main dictionary.
// TODO: when updating the dictionary fix the original words' kanji and kana.
struct OriginalWord
{
    // * Added: the user's own word, which was not part of the original dictionary.
    // In this case defs is empty. Only added words can be removed by the user.
    //
    // * Modified: the user modified a word in the dictionary. Every member must be set.
    enum ChangeType { Added = 1, Modified };
    ChangeType change;

    // Index of word in main dictionary.
    int index;

    QCharString kanji;
    QCharString kana;

    // Frequency of usage.
    ushort freq;
    // Flags of the word's info field
    uint inf;

    // The meanings data originally in the dictionary when the change type is Modified.
    fastarray<WordDefinition> defs;
};

class OriginalWordsList
{
public:
    typedef size_t  size_type;

    // Skips the legacy data of the originals list in the passed stream.
    static bool skip(QDataStream &stream);

    ~OriginalWordsList();

    void swap(OriginalWordsList &other);

    // Returns whether there's data in the originals list.
    bool empty() const;
    // Clears the originals list. Does not change dictionaries.
    void clear();

    // Legacy load. There is no new load function as loading the original words list is done
    // when loading the user data.
    void loadLegacy(QDataStream &stream, int version);
    // Used when loading the legacy data, to update the word index and frequency of saved
    // items. When wfreq or windex is negative, frequency is not updated.
    void updateItemLegacy(int index, int windex, ushort wfreq);

    // Number of original words in the list.
    size_type size() const;

    // Returns the original word at the index position in list.
    const OriginalWord* items(int index) const;
    // Returns the index of the item with the passed dictionary index, that can be used with
    // items() and other functions that use list indexes.
    int wordIndex(int windex) const;

    // Returns the original word item with the passed word index.
    const OriginalWord* find(int windex) const;

    // Deletes the originalword item at index.
    //void remove(int index);

    // Adds a new word with the passed dictionary index, kanji and kana as "Added". Does not
    // check for conflicting words added before. Returns whether changes were made.
    void createAdded(int windex, const QChar *kanji, const QChar *kana, int kanjilen = -1, int kanalen = -1);

    // Adds a word with the passed dictionary index and data as "Modified". Does not check for
    // conflicting words added before.
    void createModified(int windex, WordEntry *w);

    // If the word with windex was user modified and found in this list, its data is copied to
    // w, and then it's removed. Returns whether the word was successfully reverted. When the
    // result is false, the list is not changed. The w entry must correspond to the item in
    // this list, but it is not checked.
    bool revertModified(int windex, WordEntry *w);

    // Decrements every stored word's index if they have a value above windex. Removes the
    // originals data of the word with windex if found. Returns whether changes were made.
    bool processRemovedWord(int windex);
private:
    smartvector<OriginalWord> list;
};

// QVariant requires registration for the types it stores with Q_DECLARE_METATYPE.
Q_DECLARE_METATYPE(WordEntry*);

bool definitionsMatch(WordEntry *e1, WordEntry *e2);

//QDataStream& operator<<(QDataStream &stream, const WordEntry &w);
//QDataStream& operator>>(QDataStream &stream, WordEntry &w);


class Dictionary;
enum class InfTypes;

class WordResultList
{
public:
    typedef size_t  size_type;

    //WordResultList();
    WordResultList(Dictionary *dict);
    WordResultList(WordResultList &&src);
    WordResultList& operator=(WordResultList &&src);

    WordResultList(const WordResultList&) = delete;
    WordResultList& operator=(const WordResultList&) = delete;

    // Sets the word indexes stored in this results list to the new list. The inflections list
    // is cleared.
    void set(const std::vector<int> &wordindexes);

    // Sets the word indexes stored in this results list to the new list. The inflections list
    // is cleared.
    void set(std::vector<int> &&wordindexes);

    Dictionary* dictionary();
    const Dictionary* dictionary() const;
    void setDictionary(Dictionary *newdict);

    void clear();
    bool empty() const;

    size_type size() const;
    std::vector<int>& getIndexes();
    const std::vector<int>& getIndexes() const;
    //int indexAt(int ix) const;

    smartvector<std::vector<InfTypes>>& getInflections();
    const smartvector<std::vector<InfTypes>>& getInflections() const;

    WordEntry* items(int ix);
    const WordEntry* items(int ix) const;
    WordEntry* operator[](int ix);
    const WordEntry* operator[](int ix) const;

    //WordEntry* operator[](int ix);
    //const WordEntry* operator[](int ix) const;

    // Sorts indexes and infs to match the order of list. The list must have the same size as
    // indexes with each number in (0, indexes.size()].
    void sortByList(const std::vector<int> &list);

    // Sorts indexes by value. The corresponding elements in infs will be sorted the same way.
    void sortByIndex();

    // Sort list of results using the words' kanji, kana and frequency. Pass a vector with
    // indexes, and the indexes will be converted to their new sorted position.
    void jpSort(std::vector<int> *pindexes = nullptr); 
    // Returns the insert position of windex with the passed inflections using the same
    // conditions as jpSort(). If passed, the value of oldpos is updated to the current
    // position of the word, or -1 if it's not already in the list. The insert position is
    // computed with windex removed from the list.
    int jpInsertPos(int windex, const std::vector<InfTypes> &winfs, int *oldpos);

    // Sort list of results according to the words' definitions. The search string is used in
    // the sort to prioritize results which have a higher probability of being relevant. Pass
    // a vector with indexes, and the indexes will be converted to their new sorted position.
    void defSort(QString searchstr, std::vector<int> *pindexes = nullptr);

    // Returns the insert position of windex when it was looked up with searchstr using the
    // same conditions as defSort(). If passed, the value of oldpos is updated to the current
    // position of the word, or -1 if it's not already in the list. The insert position is
    // computed with windex removed from the list.
    int defInsertPos(QString searchstr, int windex, int *oldpos);

    void removeAt(int ix);

    void insert(int pos, int wordindex);
    void insert(int pos, int wordindex, const std::vector<InfTypes> &inf);

    void reserve(int newreserve, bool infs_too);
    // Expands the list with the word.
    void add(int wordindex);
    // Expands the list with a new word and its inflections.
    void add(int wordindex, const std::vector<InfTypes> &inf);
private:
    std::vector<int> indexes;
    smartvector<std::vector<InfTypes>> infs;

    Dictionary *dict;
};


class TextSearchTree : public TextSearchTreeBase
{
public:
    typedef size_t  size_type;

    TextSearchTree(const TextSearchTree&) = delete;
    TextSearchTree& operator=(const TextSearchTree&) = delete;
    TextSearchTree(TextSearchTree&&) = delete;
    TextSearchTree& operator=(TextSearchTree&&) = delete;

    TextSearchTree(Dictionary *dict, bool kana, bool reversed);
    TextSearchTree(Dictionary *dict, TextSearchTree &&src);
    ~TextSearchTree();

    void swap(TextSearchTree &src, Dictionary *dict);
    void swap(TextSearchTree &src);

    void copy(TextSearchTree *src);

    using TextSearchTreeBase::loadLegacy;
    using TextSearchTreeBase::load;
    using TextSearchTreeBase::save;

    // Returns a list of words starting with the search string. If exact is true, the word
    // can't be longer than the romanized search. If sameform is true, the kana/kanji or
    // lower/upper case of the original search must match the word. In this case only the last
    // infsize characters can differ, because they were altered when the word was deinflected.
    // The search string should be in Japanese form for kana trees, and not reversed. Pass a
    // list for the results in result. Pass a list of word indexes in wordpool to limit the
    // possible results to the words in that list. This list must be sorted.
    void findWords(std::vector<int> &result, QString search, bool exact, bool sameform, const std::vector<int> *wordpool, const WordFilterConditions *conditions, int infsize = 0);
    // Returns whether the result of findWords() would hold windex with the passed arguments.
    // Filter conditions and word filtering list are not supported. This function can be fast
    // for a single value, but it's slow to use in place of findWords(). Pass a boolean
    // value's address in found to check whether the given windex was found in the tree.
    bool wordMatches(int windex, QString search, bool exact, bool sameform, int infsize = 0, bool *found = nullptr);


    virtual bool isKana() const override;
    virtual bool isReversed() const override;
    Dictionary* dictionary() const;

    // Notifies the tree that the dictionary has a new word that must be added. The word must
    // already be in the dictionary so the tree can access its strings.
    // Set inserted to true if the word was added in the middle of the words list of the owner
    // dictionary.
    void expandWith(int windex, bool inserted = false);
protected:
    // Should return the word index for the given line index stored in the tree. Used in
    // findWords and wordMatches.
    // Trees implementing this should only be used for definition search. It's invalid to
    // search for kana in them.
    virtual int wordForLine(int line) const;
    // Should return the line stored in tree nodes for the passed word index. If a word is not
    // found, the result should be -1.
    // Trees implementing this should only be used for definition search. It's invalid to
    // search for kana in them.
    virtual int lineForWord(int windex) const;
    // Should return the number of string items (definitions) for the given line index.
    // Trees implementing this should only be used for definition search. It's invalid to
    // search for kana in them.
    virtual int lineDefinitionCount(int line) const;
    // Should return the string item (definitions) at the given def index for the given line.
    // Trees implementing this should only be used for definition search. It's invalid to
    // search for kana in them.
    virtual QString lineDefinition(int line, int def) const;

    virtual void doGetWord(int index, QStringList &texts) const override;
    virtual size_type size() const override;
    //virtual int doMoveFromFullNode(TextNode *node, int index) override;
private:
	typedef TextSearchTreeBase base;

    Dictionary *dict;
	bool kana;
	bool reversed;
};

// Search tree for user defined word definitions used for studying.
class StudyDefinitionTree : public TextSearchTree
{
public:
    StudyDefinitionTree(const StudyDefinitionTree&) = delete;
    StudyDefinitionTree& operator=(const StudyDefinitionTree&) = delete;
    StudyDefinitionTree(StudyDefinitionTree&&) = delete;
    StudyDefinitionTree& operator=(StudyDefinitionTree&&) = delete;

    StudyDefinitionTree(Dictionary *dict);
    //StudyDefinitionTree(Dictionary *dict, StudyDefinitionTree &&src);

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    void swap(StudyDefinitionTree &src, Dictionary *dict);

    // Creates an exact copy of src, apart from the owner dictionary, which is unchanged.
    void copy(StudyDefinitionTree *src);

    // Updates the definition list's word indexes by the changes mapping and rebuilds the
    // tree. If multiple definitions are changed to the same new word index, the first one is
    // kept and the rest will be lost. Definitions with a new index of -1 are removed.
    void applyChanges(std::map<int, int> &changes);

    // Called when a word was removed from the dictionary.
    void processRemovedWord(int windex);

    // Number of definitions stored.
    virtual size_type size() const override;
    // Returns the user definition of an item with the passed word index. If not defined for
    // the word, null is returned.
    const QCharString* itemDef(int windex) const;
    // Updates the user given definition of the word. Returns whether change was necessary.
    bool setDefinition(int windex, QString def);

    // Adds the indexes in the tree to a vector. The vector is not cleared first.
    void listWordIndexes(std::vector<int> &result) const;
protected:
    virtual int wordForLine(int line) const;
    virtual int lineForWord(int windex) const;
    virtual int lineDefinitionCount(int line) const;
    virtual QString lineDefinition(int line, int def) const;

    virtual void doGetWord(int index, QStringList &texts) const override;
private:
    // Returns an iterator of a word in list. The iterator can point to the element holding
    // the word's index if found, otherwise the position where it would be inserted. To check
    // whether the word is found, the pair's first member pointed to by the iterator should
    // match windex.
    std::vector<std::pair<int, QCharString>>::iterator wordIt(int windex);
    std::vector<std::pair<int, QCharString>>::const_iterator wordIt(int windex) const;

    // Holds word indexes and user definitions specified for them. The items in the list are
    // ordered by word index. The tree nodes hold indexes of this list and not the dictionary.
    std::vector<std::pair<int, QCharString>> list;

    typedef TextSearchTree  base;
};

// Block, index and word index of a word in an example sentence. Stored in the word commons
// tree for each word with examples data.
struct WordCommonsExample
{
    // Block a word is found in the examples file.
    ushort block;
    // Sentence inside the block.
    uchar line;
    // Index of the word in the sentence.
    uchar wordindex;
};

// Word data that is same through every dictionary for the words with the same kanji and kana.
struct WordCommons
{
    QCharString kanji;
    QCharString kana;

    // JLPT N level. 0 means no N level is specified for the word.
    uchar jlptn;

    fastarray<WordCommonsExample, ushort> examples;
};

class WordCommonsTree : public TextSearchTreeBase
{
public:
    typedef size_t  size_type;

    WordCommonsTree();
    virtual ~WordCommonsTree();

    virtual void clear() override;

    void loadLegacy(QDataStream &stream, int version);
    void load(QDataStream &stream);

    // Saves the commons data to stream. Only call this if the tree only contains JLPT words
    // and data. SOD elements indexes are not saved.
    void save(QDataStream &stream) const;

    void clearJLPTData();
    void clearExamplesData();

    // Adds a word with jlpt data to the list. This can cause duplicates, and wrong sort
    // order. The tree is not expanded unless insertsorted is set to true. After finishing
    // with the last word, it must be rebuilt either with rebuild() or with a TreeBuilder that
    // receives the romanized kana form of each word.
    int addJLPTN(const QChar *kanji, const QChar *kana, int jlptN, bool insertsorted = false);

    // Removes a word's jlpt data with the given index in the commons word list. Pass in the
    // index gained with addJLPTN. If there are indexes saved above the passed index, those
    // might change due to the remove. The function returns true if the data for the jlpt word
    // have been deleted and higher indexes must be decremented by one.
    // Calling this function is only valid for fully functional and built trees.
    bool removeJLPTN(int commonsindex);

    // Adds a single sentence data to a word. If not found, the word is inserted at the
    // correct place. At most 65535 sentence data can be added for a single word. This
    // operation is slow for loading. Instead, add a new commons data with addWord() and call
    // rebuild() afterwards, setting checkandsort to true.
    int addExample(const QChar *kanji, const QChar *kana, const WordCommonsExample &data);

    // Rebuilds the commons tree. Set checkandsort to true if duplicate words might have been
    // added or they were not sorted with wordcompare(). If a callback function is passed that
    // returns a single bool value, it's called at every step. The function should return
    // false to interrupt rebuilding.
    void rebuild(bool checkandsort, const std::function<bool()> &callback = std::function<bool()>());

    virtual bool isKana() const override;
    virtual bool isReversed() const override;

    // Searches the commons tree and returns the data exactly matching the passed kanji and
    // kana. Returns null when no such word is found.
    WordCommons* findWord(const QChar *kanji, const QChar *kana, const QChar *romaji = nullptr);

    // Adds a commons data with the passed kanji and kana. This call can cause duplicates and
    // the added data is not sorted. Call rebuild() with checkandsort set to true after
    // finished adding new data.
    WordCommons* addWord(const QChar *kanji, const QChar *kana);

    // Returns a read-only list storing the data in the commons tree.
    const smartvector<WordCommons>& getItems();
protected:
    virtual void doGetWord(int index, QStringList &texts) const override;
    virtual size_type size() const override;
private:
    smartvector<WordCommons> list;

    // Stores the index where a word with the kanji and kana is found or would be inserted to
    // if not found, when the tree has a sorted list. Returns false if the word was found at
    // index and shouldn't be inserted again.
    bool insertIndex(const QChar *kanji, const QChar *kana, int &index) const;

    typedef TextSearchTreeBase base;
};

struct WordExamples
{
    QCharString kanji;
    QCharString kana;

    // Stores the examples selected by the user for the word. The original IDs are saved to be
    // able to restore them even when the examples and the dictionary has been rebuilt. The
    // last value is the current index in all the available example sentences, loaded when the
    // sentences load. If a sentence is not found, this value is -1
    // [ Japanese ID, English ID, current example index]
    fastarray<std::tuple<int, int, int>> data;
};

// Holds example sentences linked to words.
class WordExamplesTree : public TextSearchTreeBase
{
public:
    typedef size_t  size_type;

    WordExamplesTree();
    virtual ~WordExamplesTree();

    virtual void clear() override;

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    virtual bool isKana() const override;
    virtual bool isReversed() const override;

    // Removes the current example index from the stored data, but keeps the original linked
    // sentence IDs intact.
    void reset();

    // Resets the data then goes through every word trying to find the current example index.
    void rebuild();

    // Links or unlinks the example to the word depending on the value of `set`.
    void linkExample(const QChar *kanji, const QChar *kana, int exampleindex, bool set);
    // Returns whether the example is linked to the word.
    bool isExample(const QChar *kanji, const QChar *kana, int exampleindex) const;
    // Returns whether the passed kanji and kana pair has any examples linked to them.
    bool hasExample(const QChar *kanji, const QChar *kana) const;
protected:
    virtual void doGetWord(int index, QStringList &texts) const override;
    virtual size_type size() const override;
private:
    smartvector<WordExamples> list;

    // Searches the tree and returns the index to the data exactly matching the passed kanji
    // and kana. Returns null when no such word was found.
    int findWordIndex(const QChar *kanji, const QChar *kana, const QChar *romaji = nullptr) const;

    typedef TextSearchTreeBase base;
};

// Additional data for kanji in each dictionary. 
struct KanjiDictData
{
    // Index of words containing the kanji.
    std::vector<int> words;
    // Meanings assigned to the kanji by users of a given dictionary.
    QCharStringList meanings;
    // Words that contain the kanji and were selected by the user as examples.
    std::vector<int> ex;
};

class Groups;
class WordGroups;
class KanjiGroups;
class WordDeckList;
class KanjiGroup;
class WordGroup;
struct Range;

enum class SearchMode : uchar { Browse, Japanese, Definition };
enum class BrowseOrder : uchar;
enum class SearchWildcard : uchar { AnyBefore = 0x0001, AnyAfter = 0x0002 };
Q_DECLARE_FLAGS(SearchWildcards, SearchWildcard);

class StudyDeckList;
class Dictionary : public QObject
{
    Q_OBJECT
signals:
    void dictionaryModified(bool mod);
    void userDataModified(bool mod);

    // Emited when the dictionary changed so much, any model using it should be reset.
    void dictionaryReset();

    // Emited before removing an entry.
    //void entryAboutToBeRemoved(int windex);
    // Emited after an entry was removed.
    void entryRemoved(int windex, int abcdeindex, int aiueoindex);

    // Emited after a word entry's frequency or word attributes (inf) changed.
    //void entryAttribChanged(int windex);
    // Emited after a definition was added to an entry. If the entry has 1 definition
    // when this is signaled, the entry has just been created.
    //void entryDefinitionAdded(int windex);
    // Emited after an entry's definition was updated.
    //void entryDefinitionChanged(int windex, int dindex);
    // Emited after an entry's definition was deleted.
    //void entryDefinitionRemoved(int windex, int dindex);

    // Emited after an entry's any property (apart from its kanji and kana) has changed. When
    // the study definition changes, studydef is set to true.
    void entryChanged(int windex, bool studydef);
    // Emited after a new entry's been added to the dictionary.
    void entryAdded(int windex);
    // Emited after a word was marked as kanji example.
    void kanjiExampleAdded(int kindex, int windex);

    // Emited when a word is to be removed from the list of the word examples of a kanji.
    //void kanjiExampleAboutToBeRemoved(int kindex, int windex);
    // Emited after a word was removed from the list of the word examples of a kanji.
    void kanjiExampleRemoved(int kindex, int windex);

    // Emited when the user changed the meaning of kanji at the given index.
    void kanjiMeaningChanged(int kindex);
public:

    Dictionary(const Dictionary&) = delete;
    Dictionary& operator=(const Dictionary&) = delete;

    Dictionary();
    ~Dictionary();
    // Constructs a dictionary from imported data.
    Dictionary(smartvector<WordEntry> &&words, TextSearchTree &&dtree, TextSearchTree &&ktree, TextSearchTree &&btree,
        smartvector<KanjiDictData> &&kanjidata, std::map<ushort, std::vector<int>> &&symdata, std::map<ushort, std::vector<int>> &&kanadata,
        std::vector<int> &&abcde, std::vector<int> &&aiueo);

    // Loads the base dictionary data which mainly consists of data for kanji.
    void loadBaseFile(const QString &filename);
    // Loads the main dictionary data which can change for user dictionaries.
    // Set maindict to true for the English base dictionary.
    // Set skiporiginals to true for user dictionaries.
    // Both basedict and skiporiginals are only used for the old data formats.
    void loadFile(const QString &filename, bool maindict, bool skiporiginals);
    void loadUserDataFile(const QString &filename, bool emitreset);

    void loadBaseLegacy(QDataStream &stream, int version);
    void loadBase(QDataStream &stream);

    void loadLegacy(QDataStream &stream, int version, bool basedict, bool skiporiginals);
    void load(QDataStream &stream);

    void loadUserDataLegacy(QDataStream &stream, int version);
    void loadUserData(QDataStream &stream, int version);

    void clearUserData();

    // Returns the last write date of the dictionary. For the base dictionary this is the date
    // of generation from JMdict. Pass the name of a zkj or zkdict file. On error an invalid
    // date is returned.
    static QDateTime fileWriteDate(const QString &filename);

    //void clear();

    // Returns true for dictionaries created with old zkanji versions before 2015.
    bool pre2015() const;

    // Updates its long-term deck data, setting correct/wrong answer ratio.
    //void fixStudyData();

    // Saves the base file with the given name and returns false if there was an error.
    Error saveBase(const QString &filename);

    // Saves the dictionary with the given name and returns false if there was an error.
    // Updates modified status to false.
    Error save(const QString &filename);

    // Saves the user data, including changed dictionary words for the main dictionary.
    // Updates user data modified status to false.
    Error saveUserData(const QString &filename);

    // Writes an export file of user data that can be imported later.  Pass the kanji groups
    // to write in kgroups and the words groups to write in wgroups. Set kexamples to true to
    // write the word examples selected for the kanji in the groups. Set usermeanings to true
    // to write the user defined word meanings.
    void exportUserData(const QString &filename, std::vector<KanjiGroup*> &kgroups, bool kexamples, std::vector<WordGroup*> &wgroups, bool usermeanings);

    // Writes a full or partial dictionary export to file. When limit is true, the export is
    // only partial with the kanji and words found in the passed lists. The items in the lists
    // should be unique or they will be exported multiple times.
    void exportDictionary(const QString &filename, bool limit, const std::vector<ushort> &kanjilimit, const std::vector<int> wordlimit);

    // Reads an export file of user data and merges it with the dictionary.
    //void importUserData(const QString &filename, KanjiGroupCategory *kanjiroot, WordGroupCategory *wordsroot);

    // Writes both the base dictionary and the dictionary itself in separate files after an
    // import. The groups and other user data are not saved as those should be empty when this
    // function is called.
    void saveImport(const QString &path);

    // Set the name of the dictionary in user friendly string format.
    void setName(const QString &newname);
    // The name of the dictionary in user friendly string format.
    QString name() const;

    // Returns the date when the dictionary base was last written to file. Only valid for the
    // base dictionary.
    QDateTime baseDate() const;
    // Returns the date when the dictionary was last written to file. The main dictionary is
    // only written on updates.
    QDateTime lastWriteDate() const;
    // Updates the program version of the dictionary to the current version.
    void setProgramVersion();
    // Returns the program version string used to create the data file for the dictionary.
    QString programVersion() const;
    // Returns the about text for the dictionary.
    QString infoText() const;
    // Changes the about text of the dictionary to text.
    void setInfoText(QString text);

    // Swaps the dictionary data of src with this dictionary, keeping old groups and study
    // data. The word indexes in the user data is then updated with the changes mapping.
    // After the swap, src will hold a copy of the original dictionary data, and user data.
    // This data can be restored by calling restoreChanges() with the same dictionary.
    void swapDictionaries(Dictionary *src, std::map<int, int> &changes);
    // Restores the dictionary after an update made with applyChanges(). Passing any other
    // source dictionary than that in applyChanges() will result in undefined behavior.
    void restoreChanges(Dictionary *src);

    // Whether the dictionary has been modified since the last load or save.
    bool isModified() const;
    // Changes the dictionary data modified flag to true.
    void setToModified();
    // Whether the user data has been modified since the last load or save.
    bool isUserModified() const;
    // Changes the user data modified flag to true.
    void setToUserModified();

    WordGroups& wordGroups();
    KanjiGroups& kanjiGroups();
    const WordGroups& wordGroups() const;
    const KanjiGroups& kanjiGroups() const;

    // Number of entries found in the dictionary. Each entry can hold multiple translations.
    int entryCount() const;
    WordEntry* wordEntry(int ix);
    const WordEntry* wordEntry(int ix) const;
    // Creates a new word entry with the passed kanji and kana, and single definition, and
    // adds it to the dictionary. Returns the index of the newly created word. If there is
    // already a word with the same kanji and kana, no word is created and -1 is returned.
    // The kanji and kana must only contain valid characters.
    //int createEntry(const QString &kanji, const QString &kana, ushort freq, uint inf/*, QString defstr, const WordDefAttrib &attrib*/);

    // Removes a word entry from the dictionary, removing it from every index.
    void removeEntry(int windex);

    // Returns whether the entry at windex is found in the originals list and was created by
    // the user. Only valid for the base dictionary().
    bool isUserEntry(int windex) const;
    // Returns whether the entry at windex is found in the originals list and was created by
    // the user. Only valid for the base dictionary().
    bool isModifiedEntry(int windex) const;

    // Changes a word entry's frequency and word attributes (inf).
    //void changeWordAttrib(int windex, int wfreq, int winf);
    // Adds a new definition to an existing word entry. The passed string must have glosses
    // separated with the GLOSS_SEP_CHAR character.
    //void addWordDefinition(int windex, QString defstr, const WordDefAttrib &attrib);
    // Updates the existing definition of a word entry.
    //void changeWordDefinition(int windex, int dindex, QString defstr, const WordDefAttrib &attrib);
    // Removes a definition from the word at windex. The word should keep at least one
    // definition to stay valid. If it's not possible, call removeEntry instead.
    //void removeWordDefinition(int windex, int dindex);

    // Returns an ordering of the dictionary word indexes based on the passed order.
    const std::vector<int>& wordOrdering(BrowseOrder order) const;

    WordDeckList *wordDecks();
    StudyDeckList *studyDecks();

    // Returns the definitions of a word without grammar tags separated by comma. Set numbers
    // to true to use numbers as the separator.
    QString wordDefinitionString(int windex, bool numbers) const;

    // Returns the custom definition for the word in study groups if found. Otherwise the
    // returned string will contain all definitions. To only get the custom definition, use
    // studyDefinition() instead.
    QString displayedStudyDefinition(int windex) const;

    // Returns the custom definition for the word in study groups if found. Otherwise returns
    // null. This is faster than displayedStudyDefinition() because QString conversion isn't
    // necessary.
    const QCharString* studyDefinition(int windex) const;

    // Sets a user defined definition to the word at index, which will be used in study decks
    // and groups. Passing an empty string or a string matching the dictionary definition will
    // remove the study definition. When a study definition is not set, the dictionary
    // definition is used in tests.
    void setWordStudyDefinition(int windex, QString def);

    // Removes the user defined word definition for the word with index.
    //void clearWordStudyDefinition(int index);

    //const KanjiDictData* kanjiData(int index) const;

    // Fills a list with every word that contains the given kanji.
    void getKanjiWords(short kindex, std::vector<int> &dest) const;
    // Fills a list with every word that contains the given kanji. Each word only appears
    // once in the result which will be sorted by value.
    void getKanjiWords(const std::vector<ushort> &kanji, std::vector<int> &dest) const;
    // Returns the number of words that contain a given kanji.
    int kanjiWordCount(short kindex) const;
    // Returns the index of the word in the list of words which contain the passed kanji.
    int kanjiWordAt(short kindex, int ix) const;

    // Lists a word as an example for kanji at kindex. Checks whether the same word has been
    // added, but not whether the word contains the kanji or not.
    void addKanjiExample(short kindex, int windex);
    // Checks whether a word is an example of the kanji at index, and if found, removes it
    // from the list of examples.
    void removeKanjiExample(short kindex, int windex);
    // Returns whether a word of windex is selected as example for the kanji at kindex.
    bool isKanjiExample(short kindex, int windex) const;

    // Returns whether the kanji at the passed index has a user defined meaning in the
    // dictionary.
    bool hasKanjiMeaning(short kindex) const;
    // Returns the meaning string of a kanji. If the local kanji data in the dictionary
    // doesn't contain a meaning string for a kanji, the global (English) meaning is returned.
    // Set the joinstr to the string that should separate the meanings. By default this is a
    // comma followed by space.
    QString kanjiMeaning(short kindex, QString joinstr = ", ") const;
    // Sets a meaning string for the kanji at kindex. The string should be separated by
    // newline characters between each distinct meaning. It's invalid to set a meaning for the
    // base dictionary. Setting an empty string clears the meaning and the dictionary meaning
    // will be used for the kanji.
    void setKanjiMeaning(short kindex, QString str);
    // Sets a meanings for the kanji at kindex. It's invalid to set a meaning for the base
    // dictionary. Setting an empty list clears the meaning and the dictionary meaning will be
    // be used for the kanji.
    void setKanjiMeaning(short kindex, QStringList &list);

    // Fills result with the word indexes in the dictionary in the specified browse order.
    // Pass a list for the results in result. The same (updated) list is returned for
    // convenience. Pass filter conditions to return a subset of the dictionary word indexes.
    // This can be a slow operation.
    //WordResultList&& browseWords(WordResultList &&result, BrowseOrder order, const WordFilterConditions *conditions) const;

    // Calls the other find**Words functions that return a list of unsorted words that match
    // the search conditions in `result`. (The result can be sorted with jpSort() or defSort()
    // afterwards, depending on the searchmode if necessary.) Set sameform to true if the
    // results must contain the search string exactly as it was entered. Pass a list of word
    // indexes in wordpool to limit the possible results to the words in that list. Pass a
    // list of/ inclusion/exclusion in filters to limit the possible results by word
    // attributes.
    // When studydefs is true, and the search mode is Definition, the search string is matched
    // with the user defined word definitiones first. If a word has a user word definition its
    // dictionary version is not checked.
    // WARNING: Passing a search string made with QString::fromRawData() might not be null
    // terminated, or the null might come too late. In that case this function can fail.
    void findWords(WordResultList &result, SearchMode searchmode, QString search, SearchWildcards wildcards, bool sameform, bool inflections, bool studydefs, const std::vector<int> *wordpool, const WordFilterConditions *conditions);

    // Determines whether the passed word index would be listed in the result of findWords(),
    // if searching with the same parameters. Fills inftypes with the inflections affecting
    // the result if inflections is true and inftypes is not null.
    bool wordMatches(int windex, SearchMode searchmode, QString search, SearchWildcards wildcards, bool sameform, bool inflections, bool studydefs, const WordFilterConditions *conditions, std::vector<InfTypes> *inftypes = nullptr);

    // Returns a list of words that match the given search string. The search string must
    // contain a kanji or other searchable non-kana unicode character.
    // Set sameform to true if the results must contain the search string exactly as it was entered.
    // If sameform is true, only the last infsize characters can differ, because they were altered
    // when the word was deinflected.
    // The result list receives the words. Pass a list of word indexes in wordpool, to limit the
    // possible results to the words in that list. This list must be sorted.
    // WARNING: Passing a search string made with QString::fromRawData() might not be null terminated,
    // or the null might come too late. In that case this function can fail.
    void findKanjiWords(std::vector<int> &result, QString search, SearchWildcards wildcards, bool sameform, const std::vector<int> *wordpool, const WordFilterConditions *conditions, int infsize = 0) const;
    // Returns whether the result of findKanjiWords() would contain windex. This check is fast
    // for a single value, but much slower than findKanjiWords() when filling a results list.
    bool wordMatchesKanjiSearch(int windex, QString search, SearchWildcards wildcards, bool sameform, int infsize = 0) const;

    // Returns a list of words that match the given search string. The search string must
    // only contain kana or the Japanese long-vowel dash symbol.
    // Set sameform to true if the results must contain the search string exactly as it was entered.
    // If sameform is true, only the last infsize characters can differ, because they were altered
    // when the word was deinflected.
    // The result list receives the words. Pass a list of word indexes in wordpool, to limit the
    // possible results to the words in that list. This list must be sorted.
    // WARNING: Passing a search string made with QString::fromRawData() might not be null terminated,
    // or the null might come too late. In that case this function can fail.
    void findKanaWords(std::vector<int> &result, QString search, SearchWildcards wildcards, bool sameform, const std::vector<int> *wordpool, const WordFilterConditions *conditions, int infsize = 0);
    // Returns whether the result of findKanaWords() would contain windex. This check is fast
    // for a single value, but much slower than findKanaWords() when filling a results list.
    bool wordMatchesKanaSearch(int windex, QString search, SearchWildcards wildcards, bool sameform, const int infsize = 0);


    // Returns the index of the word with the exact kanji, kana and romaji. Romaji must
    // correspond to the kana, but it's not required when not available. If the word is not
    // found, -1 is returned.
    int findKanjiKanaWord(const QChar *kanji, const QChar *kana, const QChar *romaji = nullptr, int kanjilen = -1, int kanalen = -1, int romajilen = -1);

    // Returns the index of the word with the exact kanji and kana. If the word is not found,
    // -1 is returned.
    int findKanjiKanaWord(const QCharString &kanji, const QCharString &kana);

    // Returns the index of the word with the exact kanji and kana. If the word is not found,
    // -1 is returned.
    int findKanjiKanaWord(const QString &kanji, const QString &kana);

    // Returns the index of a word in this dictionary matching the kanji and kana of a word in
    // another dictionary. If the word is not found, -1 is returned.
    int findKanjiKanaWord(WordEntry *e);

    // Returns a function that can be used for comparing a word at an index to a QChar string,
    // returning true if the word at the passed index is ordered less than the string in the
    // given browse order.
    std::function<bool(int, const QChar*)> browseOrderCompareFunc(BrowseOrder order) const;
    // Returns a function that can be used for comparing a word at an index to another word at
    // a different index, returning true if the first index is ordered less than the second one
    // in the given browse order.
    // In AIUEO order, the words' kana must be hiraganized before comparison. If that form is
    // available for either of the indexes, passing a pointer to the hiraganized string can
    // speed the search up. It's valid to pass nullptr to those strings if not available.
    // In the ABCDE order, the passed strings are ignored.
    std::function<bool(int, int, const QChar *astr, const QChar *bstr)> browseOrderCompareIndexFunc(BrowseOrder order) const;

    // Helper struct holding data about a single word for sorting word results by kanji/kana
    // with jpSortFunc().
    struct JPResultSortData
    {
        //The word.
        WordEntry *w;
        const std::vector<InfTypes> *inf;
        uchar jlpt;
    };

    // Generates data used for speeding up sorting of words with jpSortFunc() found in a
    // dictionary search.
    static JPResultSortData jpSortDataGen(WordEntry *entry, const std::vector<InfTypes> *inf);

    // Returns a function for sorting words displayed in a dictionary listing in a user
    // friendly order. The function's arguments are 2 pairs of word entry and inflection
    // types to be compared. The function returns true if the first pair should be listed
    // first. If The inflection type list can be null.
    static bool jpSortFunc(const JPResultSortData &a, const JPResultSortData &b);

    // Helper struct holding data about a single word for sorting word results by definition
    // with defSortFunc().
    struct DefResultSortData
    {
        //The word.
        WordEntry *w;
        // Number of characters after the search string before the word end or a comma is
        // encountered, not counting the part in parentheses where possible.
        int len;
        // Number of characters in front of the search string before the word start or a comma
        // is encountered, not counting the part in parentheses where possible.
        int pos;
        // Definition the search string was found in.
        uchar defpos;
        // The length of the whole definition.
        int deflen;

        // JLPT level of word. Only used when sorting by JLPT in the settings.
        uchar jlpt;
    };

    // Generates data used for speeding up sorting of words with defSortFunc() found in a
    // dictionary definition search. Calculating this data takes time so it should be stored
    // for every word taking part in a sort. The searchstr should be in lower case.
    static DefResultSortData defSortDataGen(QString searchstr, WordEntry *entry);

    // Returns a function for sorting words displayed in a dictionary listing in a user
    // friendly order, when searching the dictionary for translated definition parts. The
    // function's arguments are 2 pairs of word entry and its pre-computed data. The data can
    // be created with the defSortDataGen() function. The function returns true if the first
    // pair should be listed first. To speed up comparison, the word result sort data should
    // be created and stored before sorting.
    static bool defSortFunc(const DefResultSortData &a, const DefResultSortData &b);

    // Returns the index of the word that either matches or starts with the passed kana
    // string, or, if no such word is found, the index returned is the position where such
    // word would be inserted at. The comparison depends on the order. If the order is ABCDE,
    // the romanized form of words are compared to the romanized search string, and the index
    // is from the abcde list. If the order is AIUEO, the hiraganized kana form of the words
    // are compared to the hiraganized search string, and the index is from the aiueo list.
    // Any non-kana character in the search string is ignored. A word list can be passed as
    // optional parameter if the returned index must come from that list. The order is taken
    // into account.
    //int browseIndex(QString search, BrowseOrder order, const std::vector<int> *wordlist);

    // Returns a list of words that are in groups, study lists etc. to check at dictionary
    // imports. The returned list is sorted by index and every index is unique.
    void listUsedWords(std::vector<int> &result);

    // Returns a mapping of word indexes from src to this dictionary, for the
    // word indexes passed in wlist.
    std::map<int, int> mapWords(Dictionary *src, const std::vector<int> &wlist);

    // Returns a mapping of word indexes from the main dictionary to this dictionary for
    // originalwords.
    //std::map<int, int> mapOriginalWords();

    // Returns a diff of the two dictionaries. Each item in the returned list holds the index
    // in this dictionary and the index of the same word in the other. If either dictionary is
    // missing a word, the corresponding index of the other one is set to -1. Only compares
    // words by their kanji and kana.
    void diff(Dictionary *other, std::vector<std::pair<int, int>> &result);

    // Adds the exact copy of the word w to the dictionary. Does not check for conflicts. The
    // global originals list is only updated when originals is true. Returns the index of the
    // added word in the dictionary.
    int addWordCopy(WordEntry *w, bool originals);
    // Modifies the word at windex with data of the word w. The kanji and kana forms are not
    // updated. The global originals list is only updated when originals is true. When
    // originals is true, set checkoriginals to true if the word might exist in the originals
    // list. If found, nothing is changed in the originals list.
    void cloneWordData(int windex, WordEntry *w, bool originals, bool checkoriginals);

    // Only valid for the base dictionary. Restores the word originally in the imported
    // dictionary, if it was modified by the user. User created words are not modified.
    void revertEntry(int windex);
private:
    // Adds a word to necessary lists and maps. The abcde and aiueo lists, kanji, kana and
    // symbol data lists/maps, and the kana and definition trees. The word must be the latest
    // added word to the dictionary with an index of words.size() - 1.
    // If the added word lacks any definitions, the definition tree will be unchanged. Any
    // definitions created later must be added to the definition tree separately.
    void addWordData();
    // Erases every trace of a word with the given index from lists and maps. Sets abcde and
    // aiueo indexes to the word's index in these lists.
    void removeWordData(int index, int &abcdeindex, int &aiueoindex);

    //// Sets the kanji and kana strings to those found in line starting at pos up to len
    //// characters. The format of the line's substring should be kanji(kana). Returns whether
    //// the strings were found and filled correctly.
    //bool importedWordStrings(const QString &line, int pos, int len, QString &kanji, QString &kana);

    // Date and time when the dictionary base was saved. Only shown in dictionary statistics.
    QDateTime basedate;

    // Date and time when the dictionary data was saved. When loading the groups data, the
    // date in that file must match this one.
	QDateTime writedate;

    // Program version that was used to create the dictionary files. For formats before 2015,
    // this string is "pre2015".
    QString prgversion;

    // Dictionary name, usually the language.
    QString dictname;

    // Custom information. I.e. copyright, authors.
	QString info;

    // Dictionary was modified since last save.
    bool mod;

    // User data was modified since last save.
    bool usermod;

	smartvector<WordEntry> words;

    // Definitions tree.
    TextSearchTree dtree;

    // Kana tree.
    TextSearchTree ktree;

    // Kana tree for word endings.
    TextSearchTree btree;

    // Additional data for every kanji. The indexes in this list are the same as that in the
    // global kanjis list.
    smartvector<KanjiDictData> kanjidata;

    // List of words containing the unicode symbols in their written form, kana excluded. The
    // key is the unicode value for the symbols. The lists must be sorted by word index and
    // every item must be unique. The symbols are those in the ZKanji::validcode array.
    std::map<ushort, std::vector<int>> symdata;

    // List of words containing each hiragana (or small ka/ke or vu not in hiragana). The
    // hiragana characters are taken both from the romanized form of the word's kana, using
    // kanavowelize(), and its kana string. In the latter case katakana is converted to
    // hiragana unless it's larger than 0x30F4 (small ka ke etc.) Look into importJMdict() for
    // details. The lists must be sorted by word index and every item must be unique.
    std::map<ushort, std::vector<int>> kanadata;

    // Alphabetic ordering of words.
    std::vector<int> abcde;

    // Japanese aiueo syllable ordering of words.
    std::vector<int> aiueo;

    // User defined meaning of words to be studied.
    StudyDefinitionTree wordstudydefs;

    // Word and kanji groups.
    Groups *groups;

    // Long-term study lists for words.
    //smartvector<WordDeck> worddecks;

    WordDeckList *decks;
    std::unique_ptr<StudyDeckList> studydecks;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SearchWildcards)

namespace ZKanji
{
    extern WordCommonsTree commons;
    extern OriginalWordsList originals;
    extern WordExamplesTree wordexamples;

    // Minimum frequency needed to mark a word as popular.
    extern const int popularFreqLimit;
    // Minimum frequency needed to mark a word as medium frequency.
    extern const int mediumFreqLimit;


    // See findEntriesByKana().
    struct WordEntriesResult
    {
        int index;
        Dictionary *dict;

        WordEntriesResult(int index, Dictionary *dict) : index(index), dict(dict) { ; }
        WordEntriesResult() { ; }
    };

    // Searches every dictionary and fills the result list with words whose kana matches.
    // If multiple dictionaries contain the same kanji/kana pairing, only one of them
    // is added to result. The list is not cleared and no checking is made to avoid duplication
    // if it already contains results.
    // The main purpose of the function is to find all possible kanji/kana combinations with a given
    // kana, to be used in furigana finding for word parts whose furigana cannot be determined by
    // normal means.
    void findEntriesByKana(std::vector<WordEntriesResult> &result, const QString &kana);

    WordAttributeFilterList& wordfilters();

    // Adds dictionary to the dictionaries list. If a dictionary is already present, an
    // exception is thrown which shouldn't be handled, as this is a serious coding error.
    // No signal is sent out.
    //void addImportDictionary(Dictionary *dict);
    // Removes the sole dictionary from the dictionaries list. If the dictionaries list holds
    // multiple dictionaries, or dict is not the single dictionary already added, an exception
    // is thrown which shouldn't be handled, as this is a serious coding error.
    // No signal is sent out.
    //void removeImportDictionary(Dictionary *dict);

    // Number of dictionaries loaded.
    int dictionaryCount();
    // Adds a fully constructed dictionary to the list of dictionaries. Used in import only.
    void addDictionary(Dictionary *dict);
    // Adds a new dictionary.
    Dictionary* addDictionary();
    // Replaces the existing dictionary at index with `replacement`, returning the original.
    Dictionary* replaceDictionary(int index, Dictionary *replacement);
    // Deletes the dictionary at index, and places dict in its place in the dictionaries list.
    // Using an invalid index, replacing a dictionary with itself or placing a dictionary at
    // a different location is an error.
    //void replaceDictionary(int index, Dictionary *dict);
    // Returns whether the passed string is already used as a name for an existing dictionary.
    bool dictionaryNameTaken(const QString &name);
    // Returns whether the passed string is valid as dictionary name (file name.) Returns true
    // for everything, except a few characters that are considered either bad practice or
    // are invalid on Windows.
    bool dictionaryNameValid(const QString &name, bool allowused = false);
    // Destroys the dictionary at index, removing it from the dictionary list.
    void deleteDictionary(int index);
    // Returns the dictionary with the given index.
    Dictionary* dictionary(int index);
    // Returns the index of the passed dictionary.
    int dictionaryIndex(Dictionary *dict);
    // Returns the index of the passed dictionary.
    int dictionaryIndex(QString dictname);
    // Returns the index of the passed dictionary in the user ordering.
    int dictionaryOrderIndex(Dictionary *dict);
    // Returns the index of the passed dictionary in the user ordering.
    int dictionaryOrderIndex(const QString &dictname);
    // Returns the real index of the dictionary at the user defined index.
    int dictionaryPosition(int index);
    // Returns the user defined index of the dictionary at the real index.
    int dictionaryOrder(int index);
    // Moves the dictionary in the dictionary ordering. The index passed is the current order
    // index of the dictionary, and to is the order position, where it should be moved.
    void moveDictionaryOrder(int from, int to);
    // Changes the name of dictionary at index. Does no checking for name conflict.
    void renameDictionary(int index, const QString &str);
    // Reorders the dictionaries to be in the order defined by the passed list. The list must
    // contain the index of every dictionary exactly once. No signal is emitted when the
    // dictionaries are reordered. This must be called at startup.
    void changeDictionaryOrder(const std::list<quint8> &order);

    // Saves every modified dictionary and group to the user data folder. Set forced to true
    // to save unmodified data too.
    void saveUserData(bool forced = false);

    // Checks whether the user data files should be backed up according to the user settings,
    // and creates a backup of the current files in so. Removes any extra backup files first,
    // if necessary.
    void backupUserData();

    // Modifies word entry dest to be the exact copy of src. Only the temporary saved data in
    // the destination entry's inf is kept. If a dictionary is using this, it must make sure
    // the inner trees, originals etc. are up to date after the change.
    // Set copykanjikana to also clone the written and kana form of src. Otherwise this step
    // is skipped for speed.
    void cloneWordData(WordEntry *dest, WordEntry *src, bool copykanjikana);

    // Returns true if a and b have the same kanji/kana, definitions, frequency, types etc.
    bool sameWord(WordEntry *a, WordEntry *b);

    // Returns the meanings of a kanji as specified in the passed dictionary.
    QString kanjiMeanings(Dictionary *d, int index);
}

extern const QChar GLOSS_SEP_CHAR;

#endif
