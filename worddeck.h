/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDDECK_H
#define WORDDECK_H

#include <QDataStream>
#include <QRunnable>
#include <QSet>
#include <atomic>

#include "smartvector.h"
#include "qcharstring.h"
#include "searchtree.h"
#include "studydecks.h"
#include "ranges.h"

// Parts of a word entry. Used as questions/hints in the long-term study list and word study
// groups. Used as numbers in studysettings.h. Update when this is changed.
enum class WordParts : uchar { Kanji, Kana, Definition, Default };

enum class WordPartBits : uchar;
struct WordDeckWord // -ex TWordStudyDefinition
{
    // Index of word in the dictionary's word list.
    int index;

    // Type added to the long term study list, represented by the bits set. See WordPartBits
    // for the available types.
    uchar types;

    // Date and time the word was last included in a test. Used for spacing items of the same
    // word, so they don't follow each other too early.
    QDateTime lastinclude;

    // An id returned by the study deck for this word. Each word can have multiple id's for
    // each question type. Only one is saved here because the others can be located with it.
    CardId *groupid;
};
QDataStream& operator<<(QDataStream &stream, const WordDeckWord &w);
QDataStream& operator>>(QDataStream &stream, WordDeckWord &w);

// Data of item added to the long term study list. For every word, each type
// of question (kanji, kana or meaning) can have its own.
struct WordDeckItem // -ex TWordStudyItem
{
    // Date the item was added to the long term study list.
    QDateTime added;
    // General data of the word item. Word index, studied types etc.
    WordDeckWord *data;
    // Part of word shown when asking for another part (questiontype).
    WordParts mainhint;
    // Part of word the student must recall. WordDeckWord::types data refers to this type.
    WordPartBits questiontype;
};

struct FreeWordDeckItem : public WordDeckItem // - ex TFreeWordStudyItem
{
    uchar priority;
};

struct LockedWordDeckItem : public WordDeckItem // - ex TLockedWordStudyItem
{
    // id of this item in a StudyDeck.
    CardId *cardid;
};

class WordDeck;
template<typename T> // T is either FreeWordDeckItem or LockedWordDeckItem
class WordDeckItems //: public TextSearchTreeBase // - ex TItemTree
{
public:
    typedef T   item_type;
    typedef size_t  size_type;

    WordDeckItems(WordDeck *owner);

    void loadLegacy(QDataStream &stream);

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    void clear();
    bool empty() const;

    // Called when the dictionary is updated. Updates the list of items to
    // match the changes. The values of each word data are a combination of
    // question types. Only items with word data of the given question
    // types are kept, the others are deleted.
    // mainword: [new index, word data]
    // kept: [(old?) word data, question type used from that word data]
    void applyChanges(const std::map<int, int> &changes, const std::map<int, WordDeckWord*> &mainword, const std::map<WordDeckWord*, int> &kept);

    // Creates an exact copy of source, apart from the owner, which is kept unchanged. The
    // copied data will be created for this list and not just moved by pointer.
    // Wordmap contains [source word, new word] items.
    void copy(WordDeckItems<T> *src, const std::map<CardId*, CardId*> &cardmap, const std::map<WordDeckWord*, WordDeckWord*> &wordmap);

    size_type size() const
    {
        return list.size();
    }

    T* items(int index)
    {
        return list[index];
    }

    const T* items(int index) const
    {
        return list[index];
    }

    // Adds the passed item to the list, taking ownership of the pointer.
    int add(T *item);

    // Removes an item with the passed index from the list of items. If a card
    // id was associated with the item, it should be deleted elsewhere.
    void remove(int index);

    int indexOf(T*) const;

    //virtual bool isKana() const override;
    //virtual bool isReversed() const override;
//protected:
    //virtual QString doGetWord(int index) const override;
private:
    void _copy(item_type *item, item_type *srcitem, const std::map<CardId*, CardId*> &cardmap);

    WordDeck *owner;
    smartvector<item_type> list;

    void loadItem(QDataStream &stream, item_type *item, StudyDeck *deck);
    void saveItem(QDataStream &stream, const item_type *item, StudyDeck *deck) const;

    void loadItemLegacy(QDataStream &stream, item_type *item, int index);
    //typedef TextSearchTreeBase  base;
};

typedef WordDeckItems<FreeWordDeckItem> FreeWordDeckItems;
typedef WordDeckItems<LockedWordDeckItem> LockedWordDeckItems;

// Word tested in the daily test which has an associated kanji with a reading to be tested.
// WARNING: when this is changed, change the ReadingTestList::add() undo creation code too.
struct KanjiReadingWord // - ex TTestReadingWord
{
    // Index of word in the dictionary to be tested for kanji readings.
    int windex;

    // Character position of kanji in the word's written form.
    //uchar kanjipos;

    // The word was newly added in the long term study session.
    bool newword;
    // There was at least one failure when answering the locked item's question.
    bool failed;
};
QDataStream& operator<<(QDataStream &stream, const KanjiReadingWord &w);
QDataStream& operator>>(QDataStream &stream, KanjiReadingWord &w);

// Kanji readings that will be tested.
// WARNING: when this is changed, change the ReadingTestList::add() undo creation code too.
struct KanjiReadingItem // - ex TTestReadingItem
{
    // List of words this reading was tested in in the daily test.
    smartvector<KanjiReadingWord> words;

    // Index of kanji in the dictionary kanji list.
    int kanjiindex;
    // Reading of kanji to be tested.
    uchar reading;
};
QDataStream& operator<<(QDataStream &stream, const KanjiReadingItem &r);
QDataStream& operator>>(QDataStream &stream, KanjiReadingItem &r);

class WordResultList;
class Dictionary;

// List of readings to be tested after a study session.
class ReadingTestList
{
public:
    typedef size_t  size_type;

    ReadingTestList(WordDeck *owner);

    void loadLegacy(QDataStream &stream);

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    // Removes all data from the reading test list.
    void clear();
    // Returns whether there are no more readings that can be tested on the current test day.
    bool empty() const;
    // Returns the number of kanji/reading pair to be tested.
    size_type size() const;

    // Called when the dictionary is updated.
    // mainword: [new index, word data] - word data which will be kept in deck and its new
    //           index.
    // match: [new index, old kanji and kana matches new] - whether the word data in mainword
    //           with the same new index is exact match.
    // Every reading for words where the match is not exact, or mergedest does not contain it
    // must be removed from the readings test.
    void applyChanges(const std::map<int, int> &changes, const std::map<int, WordDeckWord*> &mainword, const std::map<int, bool> &match);

    void copy(ReadingTestList *src);

    // Called when a word was deleted from the dictionary. Words with an index above this
    // value must be updated, and the word's data removed.
    void processRemovedWord(int windex);

    // Called during the long term study test, when a word is tested, to include the readings
    // of kanji found in the word to be tested. windex is the index of the word in the main
    // dictionary. The newitem should be true when the word was included for the first time.
    // Set failed to true when there was at least one incorrect answer given to the word
    // during the test. When undo is true, if the same word was added last, it'll be removed
    // removed and added again with the new values.
    void add(int windex, bool newitem, bool failed, bool undo = false);

    // Removes a word with the passed index. If a tested kanji reading was only associated
    // with this word, it's also removed. This function shouldn't be called during tests.
    void removeWord(int windex);

    // Returns the index of the next kanji to be tested.
    int nextKanji();
    // Returns the reading to be tested next for the given kanji.
    uchar nextReading();

    // Returns whether the answer is correct for the currently tested kanji reading. The
    // answer can be either correct if it exactly matches the (hiragana version of the)
    // original kanji reading, or if it is found in a similar form in the word being tested.
    bool readingMatches(QString answer);
    // Returns the correct answer to the currently tested kanji reading.
    QString correctReading();
    // Removes the current reading item from the list, after it has been answered.
    void readingAnswered();
    // Returns the list of words to be shown with the current kanji's reading in a kanji
    // reading practice.
    void nextWords(std::vector<int> &words);
private:
    WordDeck *owner;

    // List of readings to be tested.
    smartvector<KanjiReadingItem> list;

    // Word indexes in the main dictionary already added to the readings test for the day, so
    // they won't be added a second time. 
    QSet<int> words;

    // Temporary data only used during testing. It is never saved.

    // Word index of the last added item, if it was added, or -1. This value is not saved and
    // only used during testing.
    int undoindex;

    // The word with undoindex was previously in the words set.
    bool undoexists;

    // [changed kanji reading items, original form] If original form is null, this item was
    // not previously in the readings list.
    std::vector<std::pair<KanjiReadingItem*, std::unique_ptr<KanjiReadingItem>>> undolist;

    // End of temporary data.
};

class WordDeckList : public QObject
{
    Q_OBJECT
signals:
    // Emited when a deck was renamed.
    void deckRenamed(WordDeck *deck, const QString &newname);

    // Emited before a deck is deleted.
    void deckToBeRemoved(int index, WordDeck *deck);
    // Emited after a deck was deleted.
    void deckRemoved(int index, void *oldaddress);

    // Emited when decks were moved.
    void decksMoved(const smartvector<Range> &ranges/*int first, int last*/, int pos);
public:
    typedef size_t  size_type;

    WordDeckList(Dictionary *dict);
    ~WordDeckList();

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    void clear();

    void applyChanges(Dictionary *olddict, std::map<int, int> &changes);
    //void swap(WordDeckList *other);

    void copy(WordDeckList *src);

    Dictionary* dictionary();
    void processRemovedWord(int windex);

    // Number of long-term study decks in the dictionary.
    size_type size() const;
    // Returns true if size() is 0.
    bool empty() const;
    // Returns the long-term study deck at index.
    WordDeck* items(int index);
    // Last deck that has been selected as destination to add words to. If the inner value is
    // null, returns the first deck if it exists.
    WordDeck* lastSelected() const;
    // Sets the deck which had words added to it last. If deckname is empty or not found, the
    // last selected will be set to null, but lastSelected() can still return a valid deck
    // then.
    void setLastSelected(const QString &deckname);
    // Sets the deck to be returned as having words added to it last. If deck is not part of
    // this list, it's not set.
    void setLastSelected(WordDeck *deck);
    // Returns the index of deck in items.
    int indexOf(WordDeck *deck) const;
    // Returns the index of deck in items.
    int indexOf(const QString &name) const;
    // Changes the name of the deck at index if it's valid and doesn't conflict with the names
    // of other decks. Returns whether the change was made.
    bool rename(int index, const QString &name);
    // Creates a new word deck with name and returns whether it has been created. Deck
    // creation can fail if the name is invalid or conflicts with another deck.
    bool add(const QString &name);
    // Returns whether the passed name is not valid when adding a new deck.
    bool nameTakenOrInvalid(const QString &name);
    // Removes an existing word deck with all data.
    void remove(int index);
    // Reorders the decks, moving those in ranges to the new position of newpos. The newpos
    // index must point to a valid position before the move. The ranges must be sorted. If
    // they overlap or are invalid, the function's behavior is undefined.
    void move(const smartvector<Range> &ranges, int newpos);
private:
    Dictionary *dict;

    smartvector<WordDeck> list;

    WordDeck* lastdeck;

    typedef QObject base;
};

class NextWordDeckItemThread : public QRunnable
{
public:
    NextWordDeckItemThread(WordDeck *owner);
    ~NextWordDeckItemThread();

    virtual void run() override;

private:
    WordDeck *owner;

    typedef QRunnable base;
};

class WordDeck : public QObject
{
    Q_OBJECT
signals:
    // Emited when new items were added to the queue. Count holds the number of items added to
    // the end of the queue.
    void itemsQueued(int count);
    // Emited when removing items either from the queue or the study list. The list of indexes
    // is sorted by value.
    void itemsRemoved(const std::vector<int> &indexes, bool queued);
    // Signaled when one or more data of items change, and a model listing them must be
    // updated. The signal is only emited for limited cases when functions were called from
    // the WordStudyListForm. The indexes are sorted by value. Queued is set depending on
    // whether queued or studied items changed.
    void itemDataChanged(const std::vector<int> &indexes, bool queued);
public:
    WordDeck(WordDeckList *owner);
    ~WordDeck();

    void loadLegacy(QDataStream &stream);

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    void saveWordDeckItem(QDataStream &stream, const WordDeckItem *w) const;
    void loadWordDeckItem(QDataStream &stream, WordDeckItem *w);
    void loadFreeDeckItem(QDataStream &stream, FreeWordDeckItem *w);
    void saveFreeDeckItem(QDataStream &stream, const FreeWordDeckItem *w) const;
    void loadLockedDeckItem(QDataStream &stream, LockedWordDeckItem *w, StudyDeck *deck);
    void saveLockedDeckItem(QDataStream &stream, const LockedWordDeckItem *w, StudyDeck *deck) const;

    void setName(const QString &newname);
    const QString &getName() const;

    WordDeckList* owner() const;
    WordDeckList* owner();

    Dictionary* dictionary();

    // Returns true if there are no words added to the deck.
    bool empty() const;

    // Called when the dictionary is updated. Changes holds pairs of old and new word indexes.
    // If more words are mapped to a single new index, their data is merged when possible.
    // Otherwise some data will be lost. Olddict is the dictionary holding the old word
    // entries. The dictionary() is already updated.
    void applyChanges(Dictionary *olddict, std::map<int, int> &changes);

    // Copies the deck data from source. Requests a new study deck for itself and fills the
    // deck with data copied from src as well. The owner is not changed.
    void copy(WordDeck *src);

    // Called when a word was deleted from the dictionary. Words with an index above this
    // value must be updated, and the word's data removed.
    void processRemovedWord(int windex);

    // Sets up the deck for addig new items to test for the day. If the number
    // of new items is not 0, num is added to it.
    void initNewStudy(int num);

    // Returns the word data stored in the deck associated with a given word index.
    WordDeckWord* wordFromIndex(int windex);

    // Converts the passed item indexes to word indexes. The result is unsorted.
    void itemsToWords(const std::vector<int> &items, bool queue, std::vector<int> &result);

    // Call once on each day before showing the study window. It's not an
    // error to call this more than once when taking breaks during tests.
    // Avoid calling it in the middle of tests.
    void startTest();

    // Returns the number of days passed since the last study day.
    int daysSinceLastInclude() const;
    // Returns true if startTest() has not been called on this study day yet.
    bool firstTest() const;

    // Returns whether there are kanji readings to be tested from words tested
    // today.
    int readingsQueued() const;

    // Number of word data in the deck.
    int wordDataSize() const;

    // Returns data of a word added to the deck. Only a single data is added for each word
    // even if multiple items are added for them.
    WordDeckWord* wordData(int index);

    // Returns whether the word marked by word data at index has ever been studied or not.
    bool wordDataStudied(int index) const;


    //StudyDeckId deckId();
    StudyDeck* getStudyDeck();

    // Returns the romanized kana of a word by the given word index.
    //QString wordRomaji(int windex);

    // Number of tenth seconds spent on study in total.
    int totalTime() const;

    // Average time in tenth seconds spent on study each day.
    int studyAverage() const;

    // Average time spent on a single item in tenth seconds.
    int answerAverage() const;

    // Number of queued items.
    int queueSize() const;

    // Number of items due today.
    int dueSize() const;
    // Number of tenth seconds estimated for all the due and new items in the current test.
    quint32 dueEta() const;

    // Number of new items to be tested today.
    int newSize() const;

    // Number of failed items in today's test that must be tested again.
    int failedSize() const;

    FreeWordDeckItem* queuedItems(int index);

    // Removes items from the queue. If every item of a given word is deleted, the word's data
    // is removed as well.
    void removeQueuedItems(const std::vector<int> &items);
    // Removes items from the studied list, including their study history. If every item of a
    // given word is deleted, the word's data is removed as well.
    void removeStudiedItems(const std::vector<int> &items);
    // Removes items from the studied list, including their study history. If every item of a
    // given word is deleted, the word's data is removed as well.
    // The removed items are then added at the end of the queue.
    void requeueStudiedItems(const std::vector<int> &items);

    // Fills sel with the priorities the items in the queue have selected.
    void queuedPriorities(const std::vector<int> &items, QSet<uchar> &sel) const;
    // Updates the priority of items in the queue between 1 and 9 and emits the
    // itemDataChanged signal. The indexes list in the signal is sorted by value.
    void setQueuedPriority(const std::vector<int> &items, uchar priority);

    // Sets hints to the possible main hints of the passed items and sel with hints
    // currently selected for them. The values are taken from WordPartBits. Set queued to true
    // to get the main hint of queued items, and false to get those of the studied items.
    void itemHints(const std::vector<int> &items, bool queued, uchar &hints, uchar &sel) const;
    // Updates the main hints for the passed items. If the main hint matches the question
    // type, that item is not changed. Emits the itemDataChanged signal. The indexes list in
    // the signal is sorted by value. Set queued to true to change the main hint of queued
    // items, and false to change the studied items.
    void setItemHints(const std::vector<int> &items, bool queued, WordParts hint);

    // Increases the current level and spacing of the studied item at index, updating the word
    // and day statistics. The item can't go above level 10.
    void increaseSpacingLevel(int index);
    // Decreases the current level and spacing of the studied item at index, updating the word
    // and day statistics. The item can't go below level 1.
    void decreaseSpacingLevel(int index);
    // Resets the study data of the studied item at index, clearing its own statistics and
    // removing it from day statistics. If the item was included in the last test, its removed
    // from the last test list.
    void resetCardStudyData(const std::vector<int> &items);


    // Adds items of words to the queue, adding the words to the list of words if necessary.
    // The passed vector should hold [dictionary word index, word parts] pairs. Set the bits
    // in types to an OR-ed combination of WordPartBits. Only those items will be added that
    // were missing. Returns the number of word parts that were added.
    int queueWordItems(const std::vector<std::pair<int, int>> &parts);

    // Number of words in the queue, only counting one of the question types.
    int queueUniqueSize() const;

    // Returns item indexes in the passed vector for studiedItems() to be studied in order
    // they are to be included in tests by the current standing. The list always starts with
    // failed items from the past test if found.
    void dueItems(std::vector<int> &items) const;

    // Number of studied items.
    int studySize() const;
    // Deck items already in the study list and not the queue.
    LockedWordDeckItem* studiedItems(int index);
    // Returns the index of the passed item in the list of studied items.
    //int studiedIndex(LockedWordDeckItem *item) const;

    // Returns the level of the studied item at index.
    int studyLevel(int index) const;

    // Number of items tested during the last test.
    int testedSize() const;
    // Studied deck items tested in the last test.
    LockedWordDeckItem* testedItems(int index);
    // Deck index of items tested in the last test, ordered by the time of their last test.
    int testedIndex(int index) const;

    // Returns the date of the last day the deck was studied.
    QDate lastDay() const;

    // Returns the date of the first day the deck was studied.
    QDate firstDay() const;

    // Number of days when test took place since the first test day.
    int testDayCount() const;
    // Number of days when no test took place since the first test date till today.
    int skippedDayCount() const;

    // Sorts the duelist so the items are in order of the next test date. Only used in legacy
    // data loading and shouldn't be used elsewhere.
    void sortDueList();

    // Returns the index of the locked item in the sorted duelist if found.
    // If the item is not in duelist, the result is -[insert index] - 1. For
    // example a result of -1 means the index should be 0.
    int dueIndex(int lockindex) const;

#ifdef _DEBUG
    // Checks the correct ordering of the due list and throws on error.
    void checkDueList();
#endif

    // Returns the locked item of worddat with the given type. If the item is not in the
    // duelist yet or such type does not exist for the word, null is returned.
    //LockedWordDeckItem* itemOfType(WordDeckWord *worddat, WordPartBits type);

    // Returns the item's main hint. If the main hint is default, the current
    // default is returned depending on the question. Otherwise the item's 
    // main hint is returned unchanged.
    WordParts resolveMainHint(const WordDeckItem *item) const;

    // Index of next word in the dictionary. If there are no words left to be
    // tested for the day, -1 is returned.
    // To correctly use nextIndex(), nextType() and nextHint(), their values
    // must be stored, and generateNextItem() called immediately afterwards.
    // Calling nextIndex() will block the next time until generateNextItem()
    // has finished.
    int nextIndex();

    // Hint of next word to be tested. Only valid if nextIndex() returns a
    // valid index.
    WordParts nextHint() const;

    // Type of next word to be tested. Only valid if nextIndex() returns a
    // valid index.
    WordPartBits nextQuestion() const;

    // Main hint (initially shown part) of next word to be tested. If the main
    // hint is 'Default', the returned value is converted to the current
    // default value for the given question. Only valid if nextIndex() returns
    // a valid index.
    WordParts nextMainHint() const;

    // Simulated interval for the next item if it is answered correctly.
    quint32 nextCorrectSpacing() const;

    // Simulated interval for the next item if it is answered as easy.
    quint32 nextEasySpacing() const;

    // Returns whether the currently/next shown card is new for the student.
    bool nextNewCard() const;
    // Returns whether the currently/next shown card had a failed answer.
    bool nextFailedCard() const;

    // Computes the item to be shown next in a word test. Runs in its separate
    // thread while the previous item is being tested.
    // It is not safe to call this function before calling nextIndex() first
    // every time.
    void generateNextItem();

    // Called when clicking one of the answer buttons on the study form. Asks the study deck
    // to update the current item's information, and puts it in the duelist or failedlist to
    // its correct position. To avoid race conditions, the function will wait for the next
    // item generating thread if it's running. Pass the number of milliseconds it took to
    // answer.
    // Returns whether the card has been set as postponed.
    void answer(StudyCard::AnswerType a, qint64 answertime);

    // Returns the answer passed to answer() the last time it was called during the current
    // running test. Only valid when a test is running and answer() was called at least once
    // in the same session. (Suspended tests lose this information.)
    StudyCard::AnswerType lastAnswer() const;

    // Undoes the effect of the last call to answer() and calls it again with the passed
    // value. Only valid when a test is running and answer() was called at least once in the
    // same session. (Suspended tests lose the needed data.)
    void changeLastAnswer(StudyCard::AnswerType a);

    // Returns whether calling changeLastAnswer() is ready to be called, and
    // the current item is not the same as the previous one.
    bool canUndo() const;

    // Returns the index of the kanji to be tested next in a kanji reading
    // practice.
    int nextPracticeKanji();

    // Returns the reading to be tested next for the given kanji in a kanji
    // reading practice.
    uchar nextPracticeReading();

    // Returns whether the answer is correct for the currently tested kanji
    // reading.
    bool practiceReadingMatches(QString answer);

    // Returns the correct answer to the currently tested kanji reading.
    QString correctReading();

    // Removes the current reading item from the list, after it has been
    // answered.
    void practiceReadingAnswered();

    // Updates the list of words to be shown with the current kanji's reading in a kanji
    // reading practice.
    void nextPracticeReadingWords(std::vector<int> &words);
private:
    // Returns a word data for the passed windex. If the data exists, it just calls
    // wordFromIndex. Otherwise the data is created and inserted to the word data list at the
    // correct location.
    WordDeckWord* addWordWithIndex(int windex);

    // Deletes the data from list. It's important that no items reference this data when it is
    // removed and the cards of its items must be deleted separately.
    void removeWordData(WordDeckWord *dat);

    // Blocks the main thread while a next item is being computed, unless
    // it's already found. Returns false if the item thread was running
    // but found no items to be shown next. Returns true if no item thread
    // was running or there was a running thread and it set a valid nextitem.
    bool waitForNextItem() const;

    // Aborts the thread looking for the item to be tested next and sets
    // nextitem and nextindex to -1. Call generateNextItem() in either case.
    void abortGenerateNextItem();

    // Computes the item to be shown next in a word test. It is called from
    // a separate thread and should only change the value of nextitem and
    // nextindex.
    // Accesses the following values: (that must be protected in the main thread)
    //      LockedWordDeckItem::data->lastinclude.
    //      items list.
    void doGenerateNextItem();

    // Returns an item from either freeitems or lockitems, that is currently
    // being shown in a test.
    WordDeckItem* currentItem();
    // Returns an item from either freeitems or lockitems, that was selected
    // to be the next item shown in a test.
    WordDeckItem* nextItem();
    // Returns an item from either freeitems or lockitems, that is currently
    // being shown in a test.
    const WordDeckItem* currentItem() const;
    // Returns an item from either freeitems or lockitems, that was selected
    // to be the next item shown in a test.
    const WordDeckItem* nextItem() const;

    const StudyDeck* studyDeck() const;
    StudyDeck* studyDeck();

    // -- Temporary values used only during test taking. These values are never
    // saved and can differ between sessions.

    struct FLIndex
    {
        int free;
        int locked;

        FLIndex() : free(-1), locked(-1) {}
        void reset() { free = -1; locked = -1; }

        void setFree(int index) { free = index; locked = -1; };
        void setLocked(int index) { locked = index; free = -1; }
        bool isValid() const { return free != -1 || locked != -1; }
        bool operator==(const FLIndex &other) const { return other.free == free && other.locked == locked; }
    };

    // Item index, returned in the previous nextIndex() call. Refers to an item either from
    // freeitems or lockitems. Its value is safe to access while generating an item to show
    // next.
    FLIndex currentix;

    // Item index used for nextIndex(), nextType() and nextHint() for items from either
    // freeitems or lockitems. It's computed in generateNextItem() calls, which runs in a
    // separate thread.
    FLIndex nextix;

    // Next item is being generated. Don't access nextix from the main thread.
    std::atomic_bool generatingnext;
    // Set to true to abort generating the next item, and call the blocking
    // waitForNextItem(). Or call abortGenerateNextItem() which does that.
    std::atomic_bool abortgenerating;

    // -- End of temporary values.

    // Name of the deck. At most 255 characters long.
    QString name;

    // Identifier for fetching the deck from the student's deck list. It's invalid when not
    // set.
    StudyDeckId deckid;

    // Day when last test was taken. Computed with get_day for taking user preferences into
    // account.
    QDate lastday;

    // Number of items that were included in the last test.
    int lastcnt;

    // Number of items to be added in the current test. Decremented as they are added during
    // testing.
    int newcnt;

    // Data for each word in the deck. Items can share the same data if they refer to the same
    // word. The list is sorted by word index.
    smartvector<WordDeckWord> list;

    // List of items on the queue.
    FreeWordDeckItems freeitems;

    // List of studied items with an existing card id. These items have been included in a
    // test before.
    LockedWordDeckItems lockitems;

    ReadingTestList testreadings;

    // Item indexes in lockitems in order of their next due time. All items are either listed
    // in duelist or in failedlist, but not both.
    std::vector<int> duelist;

    // Items indexes in lockitems failed in this or the last test. These are removed from
    // duelist while they are considered failed.
    std::vector<int> failedlist;

    friend class NextWordDeckItemThread;

    typedef QObject base;
};


#endif // WORDDECK_H
