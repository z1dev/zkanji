/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZDICTIONARYMODEL_H
#define ZDICTIONARYMODEL_H

#include <memory>
#include <functional>
#include "fastarray.h"
#include "zabstracttablemodel.h"
#include "smartvector.h"

#define SHOWDEBUGCOLUMN 0

// Custom column roles for displayed data in a dictionary listing view.
// Type: The column's type determines the displayed part of a word. See: DictColumnTypes.
enum class DictColumnRoles {
    Type = (int)CellRoles::User,

    Last
};

// Custom row roles for displayed data in a dictionary listing view.
// WordEntry:   WordEntry*. The entry of the displayed word.
// WordIndex:   int. The index of the word in its dictionary.
// DefIndex:    int. The index of the definition to be displayed in a given row. If the value
//              is -1, all definitions should be shown at once.
// Inflection:  int, to be converted to a pointer of type smartvector<std::vector<InfTypes>>*.
//              0 when no inflection was specified for the WordEntry.
enum class DictRowRoles {
    WordEntry = (int)DictColumnRoles::Last,
    WordIndex,
    DefIndex,
    Inflection,

    // Used as the first value in kanjireadingpracticeform.h
    Last 
};

// Part of word displayed in the column:
// Default:     Lets the default painting take place.
// Frequency:   frequency of use number of word. 0 or positive.
// DEBUGIndex:  only used when debugging. Index of word
// Kanji:       Written string with kanji if word contains it.
// Kana:        Kana reading of word.
// Definition:  Word definition listing.
enum class DictColumnTypes
{
    Default,

    Frequency,
#if SHOWDEBUGCOLUMN
    DEBUGIndex,
#endif
    Kanji,
    Kana,
    Definition,

    // Used as the first value in other custom models (e.g. DeckColumnTypes. See zworddeckmodel.h)
    Last = 255
};

class WordResultList;
class WordGroup;
struct WordEntry;
class Dictionary;
class QDragEnterEvent;
enum class ResultOrder : uchar;

struct DictColumnData
{
    // Type of the column that the list view and delegates understand.
    int type;

    // Horizontal alignment of the header text or image.
    Qt::AlignmentFlag align;

    // Whether the table view should resize the column and how. If auto sizing is not set in
    // the view, the Auto and HeaderAuto values are ignored, like NoAuto was set.
    ColumnAutoSize autosize;

    // Whether the user can change the width of the column.
    bool sizable;

    // Desired size of a column. Columns are sized to this value initially,
    // unless the column is auto sized and has contents for it.
    int width;

    // Text shown for the column in its header.
    QString text;
};

// Base class for models used in ZDictionaryListView widgets. Provides everything necessary
// for listing words in a table. To show special data in columns override the data functions
// and specify each column by setColumns(), addColumn() and removeColumn().
class DictionaryItemModel : public ZAbstractTableModel
{
    Q_OBJECT

public:
    DictionaryItemModel(QObject *parent = nullptr);
    virtual ~DictionaryItemModel();

    // Returns the dictionary the model shows items for.
    virtual Dictionary* dictionary() const = 0;

    // Returns the word group if such is represented by the model. The default implementation
    // returns null.
    virtual WordGroup* wordGroup() const;

    // Returns the index of a word in its dictionary at the passed row.
    virtual int indexes(int pos) const = 0;

    // Returns the word entry at the passed row. This must be overriden if dictionary returns
    // null, or indexes returns -1.
    virtual WordEntry* items(int pos) const;

    void insertColumn(int index, const DictColumnData &column);
    void setColumn(int index, const DictColumnData & column);
    void setColumns(const std::initializer_list<DictColumnData> &columns);
    void setColumns(const std::vector<DictColumnData> &columns);
    void addColumn(const DictColumnData &column);
    void removeColumn(int index);
    void setColumnText(int index, const QString &str);
    // Returns the index of the Nth column with the passed type.
    int columnByType(int columntype, int n = 0);

    // Overriden inherited functions:

    // IMPORTANT: must be included in derived classes:
    // virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
    //virtual QStringList mimeTypes() const override;

    virtual int statusCount() const override;
    virtual StatusTypes statusType(int statusindex) const override;
    virtual QString statusText(int statusindex, int labelindex, int rowpos) const override;
    virtual int statusSize(int statusindex, int labelindex) const override;
    virtual bool statusAlignRight(int statusindex) const override;
protected:
    virtual bool event(QEvent *e) override;
    virtual void setColumnTexts();

    // Creates the connections for word data change and removal to the current dictionary.
    void connect();
    // Releases all connections to the current dictionary.
    void disconnect();
protected slots:
    //virtual void entryAboutToBeRemoved(int windex) = 0;
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) = 0;
    virtual void entryChanged(int windex, bool studydef) = 0;
    virtual void entryAdded(int windex) = 0;
    virtual void dictionaryToBeRemoved(int ind, int oldind, Dictionary *dict);
    //virtual void dictionaryReplaced(int ind, int ordind, void *olddict, Dictionary *dict);
private:
    fastarray<DictColumnData> cols;

    // Whether connected() was called to connect the entry slots to the dictionary's signals.
    bool connected;

    typedef ZAbstractTableModel base;
protected:
    using base::connect;
    using base::disconnect;
};

// Manages data about a word list in a given dictionary. Use setWordList() to change contents.
// Only use it for lists without a specific function. For group etc. listing use a different
// derived class of DictionaryItemModel. When deriving a model from this class that will be
// used for displaying a default ordered list of words, make sure that reordering of the model
// due to sorting change or other reason is handled, by overriding the orderChanged()
// function. If inclusion of new entries in an ordered list should
// This model does not support drag and drop.
class DictionaryWordListItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    DictionaryWordListItemModel(QObject *parent = nullptr);
    virtual ~DictionaryWordListItemModel();

    // Sets a list of words to be provided by this model. Set ordered to true to keep the
    // items of the model in the order specified in the dictionary settings. The list will be
    // ordered by the model automatically.
    void setWordList(Dictionary *d, const std::vector<int> &wordlist, bool ordered = false);
    // Sets a list of words to be provided by this model. The list is moved from the passed
    // argument. Set ordered to true to keep theitems of the model in the order specified in
    // the dictionary settings. The list will be ordered by the model automatically.
    void setWordList(Dictionary *d, std::vector<int> &&wordlist, bool ordered = false);
    const std::vector<int>& getWordList() const;

    // Returns the dictionary the model shows items for.
    virtual Dictionary* dictionary() const override;
    // Returns the index of a word in its dictionary at the passed row.
    virtual int indexes(int pos) const override;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;

    // Returns the row of a given word index or -1 if not found. Calling this function is slow
    // if the model is not sorted.
    int wordRow(int windex) const;

    // Removes a row at index position from the word list and signals the change.
    virtual void removeAt(int index);
protected:
    // Old index for every new index of the contained words. I.e. if the word originally at
    // position 5 is now at position 0, then ordering[0] will be 5. Derived classes should
    // reorder any list they have that corresponds for the word listing. Default view signals
    // are handled by the base model.
    virtual void orderChanged(const std::vector<int> &ordering);
    // Derived classes should check whether the word entry in the dictionary should be listed
    // in the model and set the insert position. If the insert position is left at the default
    // value, the new word will be added at the end of the model's list. When showing an
    // ordered word list, the position value is ignored.
    virtual bool addNewEntry(int windex, int &position);
    // Returns the position of a new entry added, when includeNewEntry() returned true for it.
    // The position can be the same as the passed position argument in that function, but if
    // automatic ordering is enabled, the return value can be different.
    virtual void entryAddedPosition(int pos);
protected slots:
    void settingsChanged();
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;
private:
    void sortList();

    Dictionary *dict;
    std::vector<int> list;

    bool ordered;
    ResultOrder resultorder;

    typedef DictionaryItemModel base;
};

class DictionaryDefinitionListItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    DictionaryDefinitionListItemModel(QObject *parent = nullptr);
    virtual ~DictionaryDefinitionListItemModel();

    void setWord(Dictionary *d, int windex);
    int getWord();

    virtual Dictionary* dictionary() const override;
    virtual int indexes(int pos) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // Separately returns the definition of the entry at each index.
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;
protected slots:
    //virtual void entryAboutToBeRemoved(int windex) override;
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;
private:
    Dictionary *dict;
    int index;

    typedef DictionaryItemModel base;
};

enum class SearchMode : uchar;
enum class SearchWildcard : uchar;
Q_DECLARE_FLAGS(SearchWildcards, SearchWildcard);
struct WordFilterConditions;
class Dictionary;

// Lists words resulting from dictionary searches.
class DictionarySearchResultItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    DictionarySearchResultItemModel(QObject *parent = nullptr);
    virtual ~DictionarySearchResultItemModel();

    // Populates the model by searching the dictionary according to the given conditions. Only
    // does a new search if the passed parameters are different from a previous call to this
    // function.
    void search(SearchMode mode, Dictionary *dict, QString searchstr, SearchWildcards wildcards, bool strict, bool inflections, bool studydefs, WordFilterConditions *cond);

    // Prepares the model for a new search in case the filter conditions changed, but does not
    // update the listed words. Call search() again with the new conditions.
    void resetFilterConditions();

    //const std::vector<int>& getWordList() const;

    // Returns the dictionary the model shows items for.
    virtual Dictionary* dictionary() const override;
    // Returns the index of a word in its dictionary at the passed row.
    virtual int indexes(int pos) const override;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
protected slots:
    void settingsChanged();
    //virtual void entryAboutToBeRemoved(int windex) override;
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;

    virtual void filterMoved(int index, int to);
private:
    std::unique_ptr<WordResultList> list;

    // Saved search parameters. When calling search, if these match, the list is not updated.

    std::unique_ptr<WordFilterConditions> scond;
    SearchMode smode;
    Dictionary *sdict;
    QString ssearchstr;
    SearchWildcards swildcards;
    bool sstrict;
    bool sinflections;
    bool sstudydefs;

    ResultOrder resultorder;

    typedef DictionaryItemModel base;
};

enum class BrowseOrder : uchar;

// Model listing the words in a dictionary in browse (alphabetic or aiueo) order.
class DictionaryBrowseItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    DictionaryBrowseItemModel(Dictionary *dict, BrowseOrder order, QObject *parent = nullptr);
    virtual ~DictionaryBrowseItemModel();

    // Sets the filter used when displaying the words in dictionary browsing order. Only
    // updates the model if the passed conditions differ from the already set ones.
    void setFilterConditions(WordFilterConditions *cond);
    // Sets a list of words to be provided by this model. The list is moved from the passed
    // argument.
    //const std::vector<int>& getWordList() const;

    BrowseOrder browseOrder() const;

    // Returns the order (row number) of the word that matches the searchstr, or the order
    // index of the very last word if the search string would come after it.
    int browseRow(QString searchstr) const;
    // Returns the order (row number) of the word with the given dictionary word index. If not
    // found, returns the index to the word that would be the result of a browseRow search
    // with the same kana form as the word at windex.
    int browseRow(int windex) const;

    // Returns the dictionary the model shows items for.
    virtual Dictionary* dictionary() const override;
    // Returns the index of a word in its dictionary at the passed row.
    virtual int indexes(int pos) const override;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
protected slots:
    //virtual void entryAboutToBeRemoved(int windex) override;
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;
private:
    Dictionary *dict;
    BrowseOrder order;
    std::vector<int> list;
    std::unique_ptr<WordFilterConditions> cond;

    typedef DictionaryItemModel base;
};


struct WordDefAttrib;
// Manages data about a single word entry, used when displaying a word being edited. Because
// the edited words are only copies, and changing them does not affect dictionaries and other
// views in any way, only a single model should exist for a single entry, and that entry
// shouldn't be shown anywhere else.
// Must be used in a dictionary list view with a DictionaryListEditDelegate set as its
// delegate. Any delegate that can't paint the row after the last entry definition will cause
// an access violation.
class DictionaryEntryItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    DictionaryEntryItemModel(Dictionary *dict, WordEntry *entry, QObject *parent = nullptr);
    virtual ~DictionaryEntryItemModel();

    virtual Dictionary* dictionary() const override;
    virtual int indexes(int pos) const override;

    virtual WordEntry* items(int pos) const override;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;

    // Call when the entry has been modified outside the model.
    void entryChanged();
    // Call when a definition of the entry has been modified. If this as the last (empty)
    // definition line, a new empty definition line is added by this model.
    void changeDefinition(int ix, QString defstr, const WordDefAttrib &defattrib);
    // Delete the definition of the entry at the given index.
    void deleteDefinition(int ix);
    // Creates a copy of the entry's definition at index at the end of the entry's def list.
    void cloneDefinition(int ix);
    // Clones the data of the src entry into the stored entry.
    void copyFromEntry(WordEntry *src);

    // Separately returns the definition of the entry at each index.
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;

    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
    //virtual QStringList mimeTypes() const override;
protected slots:
    //virtual void entryAboutToBeRemoved(int windex) override {};
virtual void entryRemoved(int /*windex*/, int /*abcdeindex*/, int /*aiueoindex*/) override {};
virtual void entryChanged(int /*windex*/, bool /*studydef*/) override {};
virtual void entryAdded(int /*windex*/) override {};
private:
    Dictionary *dict;
    WordEntry *entry;

    typedef DictionaryItemModel base;
};

class GroupBase;
// Manages data of a word group.
class DictionaryGroupItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    DictionaryGroupItemModel(QObject *parent = nullptr);
    virtual ~DictionaryGroupItemModel();

    // Sets the group whose words are to be provided by this model.
    void setWordGroup(WordGroup *group);
    virtual WordGroup* wordGroup() const override;

    // Returns the dictionary the model shows items for.
    virtual Dictionary* dictionary() const override;

    // Returns the index of a word in its dictionary at the passed row.
    virtual int indexes(int pos) const override;

    // Adds word with passed word index to the currently set group. If no group
    // is set the behavior is undefined. (Throws an unhandled exception.)
    //void wordToGroup(int windex);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;

    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
protected slots:
    //virtual void entryAboutToBeRemoved(int windex) override;
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;
    void dictionaryReset();
private slots:
    void itemsInserted(GroupBase *parent, const smartvector<Interval> &intervals);
    void itemsRemoved(GroupBase *parent, const smartvector<Range> &ranges);
    void itemsMoved(GroupBase *parent, const smartvector<Range> &ranges, int pos);

    //void beginItemsRemove(GroupBase *parent, int first, int last);
    //void itemsRemoved(GroupBase *parent);
    //void beginItemsMove(GroupBase *parent, int first, int last, int pos);
    //void itemsMoved(GroupBase *parent);
private:
    WordGroup *group;

    // Temporarily saved row index when Dictionary::entryAboutToBeRemoved is signaled.
    //int removalrow;

    typedef DictionaryItemModel base;
};

//class GroupCategoryBase;
//// Lists words in all the groups passed in the constructor. The groups must all come from the
//// same dictionary, and must be distinct. The order of listed words depends on the order of
//// the groups.
//class DictionaryGroupListItemModel : public DictionaryItemModel
//{
//    Q_OBJECT
//public:
//    DictionaryGroupListItemModel(const std::vector<GroupBase*> &grouplist, QWidget *parent = nullptr);
//
//    virtual Dictionary* dictionary() const override;
//    virtual int indexes(int pos) const override;
//    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
//protected slots:
//    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
//    virtual void entryChanged(int windex, bool studydef) override;
//    virtual void entryAdded(int windex) override;
//private slots:
//    void groupDeleted(GroupCategoryBase *parent, int index, void *oldaddress);
//    void itemsInserted(GroupBase *parent, const smartvector<Interval> &intervals);
//    void itemsRemoved(GroupBase *parent, const smartvector<Range> &ranges);
//    void itemsMoved(GroupBase *parent, const smartvector<Range> &ranges, int pos);
//private:
//    // Groups whose items are listed.
//    std::vector<WordGroup*> groups;
//    // Groups and the positions in them. [Group, word position]
//    std::vector<std::pair<WordGroup*, int>> list;
//    // Start of the word items for each group in list.
//    std::vector<int> poslist;
//
//    Dictionary *dict;
//
//    typedef DictionaryItemModel base;
//};

// Lists words with written forms containing a given kanji.

/*
class KanjiWordsItemModel : public DictionaryItemModel
{
    Q_OBJECT
public:
    KanjiWordsItemModel(QObject *parent = nullptr);
    virtual ~KanjiWordsItemModel();

    // Changes the list to display a kanji at kindex and limit the list to words where the
    // kanji appears with the passed reading index.
    void setKanji(Dictionary *d, int kindex, int reading = -1);

    bool showOnlyExamples() const;
    void setShowOnlyExamples(bool val);

    virtual Dictionary* dictionary() const override;
    virtual int indexes(int pos) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
protected slots:
    //virtual void entryAboutToBeRemoved(int windex) override;
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;

    void kanjiExampleAdded(int kindex, int windex);
    //void kanjiExampleAboutToBeRemoved(int kindex, int windex);
    void kanjiExampleRemoved(int kindex, int windex);

    // Change sorting of items in the model when a settings change require it.
    void settingsChanged();
private:
    //std::function<bool(int, int)> sortFunc() const;

    // Returns the row of the word with windex in the model, or -1 when not found.
    int wordRow(int windex) const;

    void fillList();

    Dictionary *dict;

    // Current ordering of the word list.
    ResultOrder order;

    // Index of kanji.
    int kix;
    // Index of kanji "reading" that must be in words to be listed.
    // -1 = no reading, 0 = irregulars, 1-... = ON readings, the rest = KUN
    int rix;

    // Indexes of words in dict that contain the kanji at kix. Depending on onlyex and rix,
    // the items in the list might be limited to only example words or words with a given
    // kanji reading.
    std::vector<int> list;
    // When set, only example words are listed.
    bool onlyex;

    typedef DictionaryItemModel base;
};
*/

class KanjiWordsItemModel : public DictionaryWordListItemModel
{
    Q_OBJECT
public:
    KanjiWordsItemModel(QObject *parent = nullptr);
    virtual ~KanjiWordsItemModel();

    // Changes the list to display a kanji at kindex and limit the list to words where the
    // kanji appears with the passed reading index.
    void setKanji(Dictionary *d, int kindex, int reading = -1);

    bool showOnlyExamples() const;
    void setShowOnlyExamples(bool val);

    //virtual int indexes(int pos) const override;
    //virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
protected slots:
    void kanjiExampleAdded(int kindex, int windex);
    void kanjiExampleRemoved(int kindex, int windex);
protected:
    virtual void orderChanged(const std::vector<int> &ordering) override;
    virtual bool addNewEntry(int windex, int &position) override;
    virtual void entryAddedPosition(int pos) override;
private:
    //std::function<bool(int, int)> sortFunc() const;

    void fillList(Dictionary *d);

    // Index of kanji.
    int kix;
    // Index of kanji "reading" that must be in words to be listed.
    // -1 = no reading, 0 = irregulars, 1-... = ON readings, the rest = KUN
    int rix;

    // When set, only example words are listed.
    bool onlyex;

    typedef DictionaryWordListItemModel base;
};


#endif
