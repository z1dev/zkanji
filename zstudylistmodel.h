/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZWORDDECKMODEL_H
#define ZWORDDECKMODEL_H

#include <QAbstractItemModel>
#include "zdictionarymodel.h"

class WordDeck;
class WordDeckItem;

enum class WordPartBits : uchar;

//enum class DeckColumnRoles {
//    ColumnType = (int)RowRoles::Last,
//};

enum class DeckColumnTypes
{
    // General columns
    Index = (int)DictColumnTypes::Last,
    AddedDate,
    StudiedPart,

    // Queued items list columns
    Priority,

    // Studied items list columns
    //Score,
    OldLevel,
    Level,
    Tries,
    RetryCount,
    WrongCount,
    EasyCount,
    FirstDate,
    LastDate,
    NextDate,
    Interval,
    OldMultiplier,
    Multiplier,

    Last = Multiplier + 255
};

enum class DeckRowRoles
{
    DeckIndex = (int)DeckColumnTypes::Last,
    // The tested part of an item. Value is of type WordPartBits.
    ItemQuestion,
    // The selected hint for the item which can be default. Value is of type WordParts.
    ItemHint,
};

enum class DeckViewModes : int { Queued, Studied, Tested };

class StudyListModel : public DictionaryItemModel
{
    Q_OBJECT

public:
    StudyListModel() = delete;
    StudyListModel(WordDeck *deck, QObject *parent = nullptr);
    virtual ~StudyListModel();

    // Repopulates the model.
    void reset();

    void setViewMode(DeckViewModes newmode);
    DeckViewModes viewMode() const;

    // Fills the passed list with the default column sizes in the requested mode.
    void defaultColumnWidths(DeckViewModes mode, std::vector<int> &result) const;
    // Returns the default column width of a column at columnindex. Returns -1 on invalid
    // index.
    int defaultColumnWidth(DeckViewModes mode, int columnindex) const;

    //int deckColumnCount() const;
    //int testColumnCount() const;
    //int queueColumnCount() const;
    //int deckColumnCount() const;
    //int testColumnCount() const;

    // Returns whether "row a" is in front of "row b" when sorting by the passed column and
    // with the given sort order.
    bool sortOrder(int column, int rowa, int rowb);

    void setShownParts(bool showkanji, bool showkana, bool showdefinition);

    // Returns the dictionary the model shows items for.
    virtual Dictionary* dictionary() const override;

    // Returns the index of a word in its dictionary at the passed row.
    virtual int indexes(int pos) const override;

    // Overriden inherited functions:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
    //virtual QStringList mimeTypes() const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
protected slots:;
    //virtual void entryAboutToBeRemoved(int windex) override;
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
    virtual void entryChanged(int windex, bool studydef) override;
    virtual void entryAdded(int windex) override;
private slots:
    void itemsQueued(int count);
    void itemsRemoved(const std::vector<int> &indexes, bool queued, bool decrement = true);
    void itemDataChanged(const std::vector<int> &indexes, bool queue);
private:
    // Fills dest with the index of items in the deck (queue or studied) that should be shown.
    void fillList(std::vector<int> &dest);

    bool matchesFilter(WordPartBits part) const;

    WordDeck *deck;
    DeckViewModes mode;

    // Indexes into the deck's items sorted by value. Used when one or more parts of the item
    // have to be hidden.
    std::vector<int> list;

    // Items with kanji question part are shown.
    bool kanjion;
    // Items with kana question part are shown.
    bool kanaon;
    // Items with definition question part are shown.
    bool defon;

    QString shortHintString(WordDeckItem *item) const;
    QString shortQuestionString(WordDeckItem *item) const;

    // Generates the studied definition string for the word in the dictionary at the given index, or
    // returns the user defined value if it exists.
    QString definitionString(int index) const;

    // Temporary value set to true in entryAboutToBeRemoved() only if that entry is displayed
    // by this model.
    //bool entryremoving;

    typedef DictionaryItemModel base;
};

// QVariant initialization.
//Q_DECLARE_METATYPE(DeckColumnTypes);

//bool operator==(int, DeckColumnRoles);
//bool operator==(DeckColumnRoles, int);
//bool operator!=(int, DeckColumnRoles);
//bool operator!=(DeckColumnRoles, int);

extern const int queuecolcount;
extern const int studiedcolcount;
extern const int testedcolcount;


#endif // ZWORDDECKMODEL_H

