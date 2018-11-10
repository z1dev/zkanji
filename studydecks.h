/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef STUDYDECKS_H
#define STUDYDECKS_H

#include <QDataStream>
#include <QDateTime>
#include <memory>
#include <unordered_map>
#include "smartvector.h"
#include "fastarray.h"

// Long-term study algorithm and structures description.
//
// The exact things to memorize are not stored by these structures as they are general, only
// study data.
//
// StudyCard: Knowledge is divided into separate small units (items). Data for a single item
// is stored in a StudyCard structure. The data corresponds to statistics for each repetition,
// date of last test, interval to wait till next test etc.
//
// StudyDeck: holds a list of StudyCard objects and global statistics about each study day.
//
// StudyDeckList: holds a list of study decks. Used to create or delete decks or to retrieve
// them.
//
// StudentProfile: computes the interval multiplier based on answer given. Holds an average of
// multipliers that helps deciding on the result.
//
//
// A few assumptions are made (from information found online) about the speed of learning and
// forgetting of items:
// - Number of items being studied doesn't influence the aquisition of other items.
// - Some items are much harder to remember than most others. If those are excluded, the ratio
//   of correct answers will be much better. This is currently not used in the algorithm. The
//   student should bear the responsibility for excluding these items.
// - The probability of correct answer to an item and the period's length or interval of not
//   studying that item is directly proportional.
// - Items being remembered for less than 3 weeks count as not memorized. Anything above is
//   not considered new.
//

// StudyCardStat: statistics for cards for each day they were studied on
struct StudyCardStat // - ex TRepStat
{
    QDate day;

    //uchar score; - score was scrapped since 2015

    uchar level;
    float multiplier;
    // Number of tenth seconds spent on this card on the test day between its display and the
    // answer given. Can't exceed 6000 seconds, even if the student went afk.
    ushort timespent;
};
QDataStream& operator<<(QDataStream &stream, const StudyCardStat &stat);
QDataStream& operator>>(QDataStream &stream, StudyCardStat &stat);

// An id given to the users of study decks for each item they store in a deck. Must be stored
// as a pointer.
struct CardId
{
private:
    CardId(int data);
    int data;

    friend class StudyDeck;
};

struct StudyCard // - ex TRepetitionItem
{
    // Indexes in answers.
    enum AnswerType { Retry = 0, Correct = 1, Wrong = 2, Easy = 3 };

    // Statistics for each day the card was tested.
    fastarray<StudyCardStat> stats;

    // User defined data for the study card. This is usually a pointer to the item that was
    // tested. This data is NOT saved. It's the user's task to restore this on load.
    intptr_t data;

    // Date of the start of the current or last test. The repetition spacing is added to this
    // date to compute the date of the next test.
    QDateTime testdate;

    // Date and time when the card should be tested next. Computed by adding interval to
    // testdate. This value is not saved and is invalid until needed. When the testdate or
    // interval changes, this date should be invalidated.
    mutable QDateTime nexttestdate;

    // Exact date and time when the item was tested the last time. This is NOT used for
    // determining when it's tested again. Only used, when searching for the next item to
    // test, to find the one tested the longest time ago. Testdate is not suitable as it is
    // the same for every item previously answered in the current test.
    QDateTime itemdate;

    // Number of times a type of answer was given in today's test. See indexes from
    // AnswerType. At most 255 is valid. Only used in last test day's statistics.
    uchar answers[4];

    // Current item difficulty. Starts at 1.0.
    //double difficulty;

    // Current repetition of item that determines interval.
    uchar level;
    // Last study interval is multiplied by this value.
    float multiplier;
    // Number of times this item was included in tests and not repeated.
    //ushort inclusion;

    // Time between the last and the next tests in seconds.
    quint32 spacing;


    // TO-NOT-DO: is wrongcnt and answercnt imported from old data? is the student also updated with it?

    // Number of answers since the last time the counting started. (After
    // an item is flagged problematic, this counter starts over.)
    //ushort answercnt;

    // Number of tests wrong answer is received for the item since it last 
    // started counting. (After an item is flagged problematic, this counter
    // starts over.)
    //ushort wrongcnt;

    // When an item gets too many wrong answers compared to the average of
    // all items, this value is set to true. Problematic items won't affect
    // the student's interval-multipliers, and can be separated in study
    // statistics.
    //bool problematic;

    bool learned;


    // Number of times the item got repeated in the last test so far. Only 255 repeats are
    // stored and nothing above this is measured.
    uchar repeats;
    // Level of item when it was first included in today's test.
    uchar testlevel;

    // Sum of all time spent on this item in tenth seconds.
    quint32 timespent;

    // Index of card in a deck.
    int index;

    // The cards of related items are connected via the 'next' pointer.
    // There is no first or last item with a zero value. The "last" item's
    // 'next' pointer points to the "first" item.
    StudyCard *next;
};

QDataStream& operator<<(QDataStream& stream, const StudyCard &card);
QDataStream& operator>>(QDataStream& stream, StudyCard &card);


// The DeckTimeStatItem, DeckTimeStat and DeckTimeStatList are all used for
// estimating the time a test will take.
// The DeckTimeStatList holds a separate DeckTimeStat object for every card
// level. (The initial level on the test day.)
// A DeckTimeStat holds a separate DeckTimeStatItem object for each repetition
// on that test day. That is, the time taken to answer a card shown for the
// first time on that day is stored at index 0, when it's first repeated it is
// stored at index 1 and so on.
// DeckTimeStatItem holds the time it took to answer the last 255 cards for
// that specific level and repetition.
// A DeckTimeStat object also holds the number of times cards were repeated
// on that level, for the last 100 answeres given.

struct DeckTimeStatItem // - ex TTimeStatItem
{
    // Number of tenth seconds spent on StudyCard items of a specific level and repeats. Only
    // the last 255 are saved.
    quint32 time[255];
    // Number of slots filled in the time[] array. When a new answer arrives,
    // the new value is inserted at the front and the array is shifted by one.
    uchar used;
};
QDataStream& operator<<(QDataStream &stream, const DeckTimeStatItem &s);
QDataStream& operator>>(QDataStream &stream, DeckTimeStatItem &s);

struct DeckTimeStat // - ex TTimeStat
{
    //DeckTimeStatItem *stat;
    fastarray<DeckTimeStatItem> timestats;
    //uchar alloced;

    uchar repeat[100];
    uchar used;

    //void removerepeat();

    //void addrepeat(uchar arepeat);
    //void addtime(int repeatix, double atime);
    //void grow(int repeatix);

    //double averagerepeat() const;
    //double averagetime(int repeatix) const;
};

QDataStream& operator<<(QDataStream &stream, const DeckTimeStat &s);
QDataStream& operator>>(QDataStream &stream, DeckTimeStat &s);

class DeckTimeStatList // -ex TTimeStatList
{
public:
    DeckTimeStatList();

    void loadLegacy(QDataStream &stream, int version);
    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    // Saves the current state of the object for the passed level, so it can be reverted when
    // the student changes the last answer.
    void createUndo(uchar level);
    // Removes the last added (first) value from repeats. Only call if addRepeat will be used
    // afterwards to add a new value.
    void revertUndo(uchar level);
    // Adds a new time in tenth seconds for a given level and repeat count.
    void addTime(uchar level, uchar repeats, quint32 time);
    // Adds a new repeat value on level. Both level and repeats must be 0 based.
    void addRepeat(uchar level, uchar repeats);

    // Returns the number of tenth seconds an item might take in a test if it was first
    // included at level and currently is past repeats number or tries. If there is not enough
    // data, returns a simple guess that might be wrong, but can be displayed.
    quint32 estimate(uchar level, uchar repeats) const;
private:
    // Returns the number of milliseconds an item's single repeat might take in a test. If
    // there's no data, returns a guess.
    quint32 _estimate(uchar level, uchar repeats) const;

    // Temporary undo data. Only use during tests.

    // Number of levels in items. If this is less than the level to undo or create undo for
    // undostat is invalid.
    uchar undocount;
    // The saved statistics item on level.
    DeckTimeStat undostat;

    // End of temporary data.

    fastarray<DeckTimeStat> items;
};

struct DeckDayStat // - ex TDayStat
{
    // Test day.
    QDate day;
    // Length of test in tenth seconds.
    int timespent;
    // Count of items that were tested (not the ones that were added to the test but not
    // tested because there was not enough time for it).
    int testcount;
    // Items that were newly added to the test on the study day and tested.
    int testednew;
    // Number of items with an incorrect answer at least once on the study day.
    int testwrong;
    // Number of items with a correct answer after a 2+ months interval.
    int testlearned;
    // Count of all items with a study card.
    int itemcount;
    // Count of groups of items. Each item is only counted separately if another item in the
    // same group hasn't yet been included.
    int groupcount;
    // Number of items marked Learned after the day.
    int itemlearned;
    // Number of items that were marked Problematic after the day.
    //int itemproblematic;
};
QDataStream& operator<<(QDataStream &stream, const DeckDayStat &s);
QDataStream& operator>>(QDataStream &stream, DeckDayStat &s);

class DeckDayStatList  // -ex TDayStatList
{
public:
    typedef size_t  size_type;

    DeckDayStatList();

    void loadLegacy(QDataStream &stream, int version);
    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    // Recreates daily statistics from the passed card data. The list should hold one item for
    // each card tested at a day in order of their date. The cards learned status is fixed as
    // well.
    // [study card, index of card stat, answer was correct]
    void fixStats(const std::vector<std::tuple<StudyCard*, int, bool>> &cards);

    bool empty() const;
    DeckDayStat& back();
    const DeckDayStat& back() const;

    size_type size() const;
    const DeckDayStat& items(int index) const;
    DeckDayStat& items(int index);

    void clear();

    void createUndo(QDate testdate);
    void revertUndo();

    // Includes a new card in the day stats for the given test date. The date must match or
    // come after the last date.
    void newCard(QDate testdate, bool newgroup);
    // Creates a new stat item for testdate if it does not exist, and updates the item and
    // group counts. If testdate is less than the last stat date, it's assumed the system time
    // has been changed and the card was deleted on the last day.
    void cardDeleted(QDate testdate, bool grouplast, bool learned);
    // Adds a new item to the daily statistics. If the day has no statistics yet, a new one is
    // created, although this is only possible during stat fixing.
    void cardTested(QDate testdate, int timespent, bool wrong, bool newcard, bool orilearned, bool newlearned);
    // Decrements the group count of the last test day. Does not create an extra empty day
    // stat for today if it doesn't exist.
    void groupsMerged();
private:
    // Creates statistics for the day of testdate. The date must match or come later than the
    // last existing date. If the dates match the function returns leaving the list unchanged,
    // otherwise every item that doesn't refer to today's test is copied.
    void addDay(QDate testdate);

    // Temporary container of undo data. It's not used outside testing.
    DeckDayStat undo;

    std::vector<DeckDayStat> list;
};

class StudyDeck;
class StudyDeckId
{
public:
    StudyDeckId();
    StudyDeckId(const StudyDeckId &orig);
    StudyDeckId(StudyDeckId &&orig);
    StudyDeckId& operator=(const StudyDeckId &orig);
    StudyDeckId& operator=(StudyDeckId &&orig);
    bool valid() const;
private:
    // Returns a new study deck id which is unique from both a and b, and comes after them.
    static StudyDeckId next(StudyDeckId a, StudyDeckId b);
    // Returns a new study deck id which comes after a.
    static StudyDeckId next(StudyDeckId a);

    StudyDeckId(intptr_t id);
    intptr_t id;

    friend bool operator==(const StudyDeckId &a, const StudyDeckId &b);
    friend QDataStream& operator<<(QDataStream &stream, const StudyDeckId &id);
    friend QDataStream& operator>>(QDataStream &stream, StudyDeckId &id);
    friend class StudyDeckList;
    friend struct StudyDeckIdHash;
};
bool operator==(const StudyDeckId &a, const StudyDeckId &b);
QDataStream& operator<<(QDataStream &stream, const StudyDeckId &id);
QDataStream& operator>>(QDataStream &stream, StudyDeckId &id);

struct StudyDeckIdHash
{
    size_t operator()(const StudyDeckId &id) const;
};

class StudyDeckList;
class StudyDeck // - ex TRepetitionList
{
public:
    StudyDeck(StudyDeckList *owner, StudyDeckId id);
    ~StudyDeck();

    void loadLegacy(QDataStream &stream, int version);

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    // Creates a copy of source, returning a mapping from source card id to those in this
    // deck. Does not change the owner.
    void copy(StudyDeck *src, std::map<CardId*, CardId*> &map);

    const DeckDayStatList& dayStats() const;

    // Fixes errors after loading from previous versions. Simulates answering the cards one
    // by one on each study date.
    void fixDayStats();

#ifdef _DEBUG
    // Checks the last item in the day statistics whether the counts are valid. Throws if any
    // discrepancies are discovered.
    void checkDayStats() const;
#endif

    // The unique id of the deck. It is used to identify the deck when requesting
    // it from the student with getDeck().
    StudyDeckId deckId() const;
    // Whether this deck holds the passed deckid;
    //bool hasId(StudyDeckId deckid) const;

    // Returns the id of a card by its index.
    CardId* cardId(int index) const;
    int cardIndex(CardId *cardid) const;

    // Makes sure that there are no more card Ids in a study deck than size. Only used during
    // legacy loading. If it does anything it's a sign of corrupted data and will show a
    // message box.
    void fixResizeCardId(int size);

    // Loads the next card id from stream and returns a pointer to it.
    CardId* loadCardId(QDataStream &stream);
    // Saves a card id to stream. Internally just saves its index in cards. If the id is null,
    // the index saved is -1.
    void saveCardId(QDataStream &stream, CardId *id) const;
    // Sets the user defined data for a given card.
    void setCardData(CardId *cardid, intptr_t data);
    // Returns the user defined data for a given card.
    intptr_t cardData(const CardId *cardid) const;
    // Returns the next card in the card group of cardid. It can be the same as cardid if the
    // group consists of that single card.
    CardId* nextCard(CardId *cardid);
    // Returns the user defined data for a given group identified by a card in the card group.
    // The result vector is not cleared, new items are appended. The vector is not checked for
    // duplicates.
    void groupData(CardId *cardid, std::vector<intptr_t> &result);

    // Creates a new card on the deck. Cards can be grouped if an item's multiple data are
    // tested. Pass a card's id in cardid_group which is in the group as the card to be
    // created. Passing null creates a new group. Set data to a user specified data.
    CardId* createCard(CardId *cardid_group, intptr_t data);

    // Deletes the card with the passed id. Returns the id of the card which comes next in the
    // same card group. If the group is empty after the delete, deleteCard() returns null.
    CardId* deleteCard(CardId *cardid);

    // Deletes the card with the passed id and every other card in the same group.
    void deleteCardGroup(CardId *cardid);

    // Returns whether g1 and g2 belongs to the same card group.
    bool sameGroup(CardId *g1, CardId *g2) const;

    // Merges the groups g1 and g2. After the merge, both ids can be used to reference the
    // same group.
    void mergeGroups(CardId *g1, CardId *g2);

    // Returns the number of day statistics stored.
    ushort dayStatSize() const;
    // Returns a day statistic for the given index. 
    const DeckDayStat& dayStat(int index) const;

    // Number of study cards stored in the deck.
    //int cardCount();

    // Returns the study card in the deck at the given index.
    // TO-NOT-DO: to be deleted. Only used as a fix for old versions.
    //StudyCard* cards(int index);

    uchar cardLevelOld(CardId *cardid) const;
    uchar cardLevel(CardId *cardid) const;

    // Returns the number of times a card was answered with a given answer. See
    // StudyCard::AnswerType for the indexes. Returned is an array of size 4.
    const uchar* cardTries(CardId *cardid) const;

    // Level of a card at the start of the last test. This level is only valid after the card
    // has been answered at least once.
    uchar cardTestLevel(CardId *cardid) const;

    // Returns whether a card was shown for the first time when an answer is given. Only valid
    // right after an answer.
    bool cardFirstShow(CardId *cardid) const;

    // Number of times a card was studied.
    ushort cardInclusion(CardId *cardid) const;
    // Study card's date of the last test.
    QDateTime cardTestDate(CardId *cardid) const;
    // Statistics of a card when it was first tested.
    QDate cardFirstStatDate(CardId *cardid) const;

    // Returns the date of the card when it was last answered.
    QDateTime cardItemDate(CardId *cardid) const;

    // Returns the date of the card when it's due next, by adding its interval
    // to its testdate.
    QDateTime cardNextTestDate(CardId *cardid) const;

    // Returns the spacing of the card in seconds.
    quint32 cardSpacing(CardId *cardid) const;

    // Returns the spacing of the card in seconds if its level increases by one without
    // changing the multiplier.
    quint32 increasedSpacing(CardId *cardid) const;
    // Returns the spacing of the card in seconds if its level decreases by one without
    // changing the multiplier.
    quint32 decreasedSpacing(CardId *cardid) const;
    // Increases the current level of the specific card and its spacing, updating the word and
    // day statistics. The card can't go above level 10.
    void increaseSpacingLevel(CardId *cardid);
    // Decreases the current level of the specific card and its spacing, updating the word and
    // day statistics. The card can't go below level 1, and a spacing of 1 day.
    void decreaseSpacingLevel(CardId *cardid);
    // Resets the study data of the card, clearing it's own statistics and removing it from
    // day statistics. If the card was included in the last test, its removed from the last
    // test list.
    void resetCardStudyData(CardId *cardid);

    // Returns the multiplier of a card from the previous test day.
    float cardMultiplierOld(CardId *cardid) const;
    // Returns the multiplier of a card.
    float cardMultiplier(CardId *cardid) const;

    // Returns the estimated time in milliseconds a card will take to answer.
    quint32 cardEta(CardId *cardid) const;
    // Returns the estimated time in milliseconds a new card will take to answer.
    quint32 newCardEta() const;

    // Must be called when the test starts for the day. It's not an error to call this
    // repeatedly in the same day but it only has an effect the first time.
    // Returns whether a new test day was started, and not just testing again on the same day.
    bool startTestDay();

    // Returns the number of days passed since the last time new items have been included.
    // Returns -1 if the statistics don't have a day when items were included.
    int daysSinceLastInclude() const;
    // Returns true if startTestDay() has not been called at all, or was only called on a
    // previous test day.
    bool firstTest() const;

    // The date of the test. While a test takes place this date never changes, even if the
    // date changes (unless startTestDay() is called again).
    QDate testDay() const;

    // Number of items tested on this or the last test day.
    int testSize() const;
    // Id of the card tested today. The cards are sorted by the time of their last answer.
    CardId* testCard(int index) const;

    // Computes the new interval of a card based on the answer given. If simulate is set to
    // true, no data is changed, but the interval is returned in seconds. Otherwise updates
    // the card's statistics and notifies the student to change the student profile.
    // The variable passed in postponed is set to true if the card is postponed after this
    // answer due to it becoming problematic. Set answertime to the number of milliseconds it
    // took the student to give an answer. When changing a previous answer, set answertime to
    // -1. Its value is ignored when simulating.
    // When simulating and the answer was wrong, the returned value should be ignored and only
    // the new value of postponed should be used.
    quint32 answer(CardId *cardid, StudyCard::AnswerType a, qint64 answertime, /*bool &postponed,*/ bool simulate = false);

    // Returns the answer passed to answer() the last time it was called during the current
    // running test. Only valid when a test is running and answer() was called at least once
    // in the same session. (Suspended tests lose this information.)
    StudyCard::AnswerType lastAnswer() const;

    // Undos the effect of the last call to answer() and calls it again with the passed value.
    // Only valid when a test is running and answer() was called at least once in the same
    // session. (Suspended tests lose the needed data.)
    void changeLastAnswer(StudyCard::AnswerType a);

    // Returns the id of the card last answered.
    const CardId* lastCard() const;
private:
    // Returns the card by its id. Passing an invalid id results in undefined behavior.
    const StudyCard* fromId(const CardId *cardid) const;
    // Returns the card by its id. Passing an invalid id results in undefined behavior.
    StudyCard* fromId(const CardId *cardid);
    // Returns the card's index in list by its id. Passing an invalid id
    // results in undefined behavior.
    int posFromId(const CardId *cardid) const;

    // Saves everything about the last item so its data can be changed. The data saved is only
    // usable in the same study session. It becomes invalid on suspend or when the test ends.
    void createUndo(StudyCard *card, StudyCard::AnswerType a, qint64 answertime);

    // Reverts the data saved with createUndo(). Only valid during a study session
    // and not for the first item in the session.
    void revertUndo();

    // Changes the last study statistics of a card to match the card's level and multiplier,
    // and adds time to the time spent on the card.
    void updateCardStat(StudyCard *card, /*StudyCard::AnswerType a,*/ int time);

    // Checks whether the card's current interval conflicts with another item in the same
    // group, and changes the interval if it does. The values of the passed card are not used
    // directly. It's only needed to identify the group it's in.
    // cardtestdate is the date of the current test, cardinterval is the current set interval
    // for the next repetition. This value is updated if another item of the same group was to
    // be tested on the day.
    void fixCardSpacing(const StudyCard *card, QDateTime cardtestdate, uchar cardlevel, quint32 &cardspacing) const;

    // Temporary container of undo data.
    struct Undo
    {
        StudyCard *card;

        // Copy of the original card. The easiest way to undo is to copy the data back.
        StudyCard cardundo;

        qint64 answertime;

        StudyCard::AnswerType lastanswer;
    };

    // Not saved. Only valid during a study session.
    Undo undodata;

    // End of temporary data

    StudyDeckList *owner;

    // A unique identifier that can be passed to ZKanji::student() to retrieve the deck.
    // Study deck id is randomly generated between 0 and intmax.
    StudyDeckId id;

    // Date and time when the last test was started for the day. It's not updated when testing
    // again on the same day.
    QDateTime testdate;

    // List of all cards in the deck.
    smartvector<StudyCard> list;
    // List of stored card ids corresponding to same index in list.
    smartvector<CardId> ids;

    // Card indexes in list that were tested during the last test. The list is ordered by the
    // card's last test time.
    std::vector<int> testcards;

    DeckTimeStatList timestats; 
    DeckDayStatList daystats;

};

// Only a single student object should exist while the program is running.
// Class managing general data about the decks the student studies, and their
// performance. Helps to decide when an item should be tested again.
class StudyDeckList
{
public:
    typedef size_t  size_type;

    StudyDeckList();
    ~StudyDeckList();

    // Legacy load function
    void loadDecksLegacy(QDataStream &stream, int version);
    StudyDeckId skipIdLegacy(QDataStream &stream);
    // End legacy.

    void load(QDataStream &stream, int version);
    void save(QDataStream &stream) const;

    void clear();

    // Forces the decks to recompute the answer count and wrong count for every
    // item, finding problematic ones.
    void fixStats();

    // Returns a deck that matches the passed id.
    StudyDeck* getDeck(StudyDeckId id);
    // Creates a new deck and returns its id.
    StudyDeckId createDeck();
    // Removes a deck with the passed id, deleting all its data.
    void removeDeck(StudyDeckId id);

    // Number of decks in the list.
    size_type size() const;

    // Returns the index-th deck.
    StudyDeck* decks(int index);
private:
    // Id of the next study deck to be created. This value is not saved.
    StudyDeckId nextid;

    smartvector<StudyDeck> list; // - ex TRepetitionCategory
    std::unordered_map<StudyDeckId, StudyDeck*, StudyDeckIdHash> mapping;
};

class StudentProfile
{
public:
    StudentProfile();

    void load(const QString &filename);
    void save(const QString &filename);

    void clear();

    float baseMultiplier() const;
    // Updates baseMultiplier() when an item with mul is deleted.
    void removeMultiplier(float mul);
    // Adds a new multiplier to the average returned by baseMultiplier().
    void addMultiplier(float mul);

    // Returns the multiplier for the next repetition of a card, if the answer to it was easy.
    float easyMultiplier(float mul) const;
    // Returns the multiplier for the next repetition of a card, if the answer to it was wrong.
    float wrongMultiplier(float mul) const;
    // Returns the multiplier for the next repetition of a card, if the answer to it was retry.
    float retryMultiplier(float mul) const;

    bool isModified() const;
    //void setModified(bool mod);

    // Saves the last state of the profile which can be restored by calling revertUndo().
    void createUndo();

    // Restores profile saved with createUndo(). Only call it once before calling
    // creatateUndo() again.
    void revertUndo();

    // Acceptable rate of correct items. 0.9 means 90% correct answers.
    double acceptRate(uchar level);
private:
    // Temporary container of undo data.
    struct Undo
    {
        int cardcount;
        double multiaverage;
    };

    Undo undodata;
    // End of temporary data

    bool mod;

    // Number of cards whose multiplier average is the multiaverage.
    int cardcount;
    // The average value of all card multipliers that had a correct answer after a 3 week
    // interval.
    double multiaverage;
};

namespace ZKanji
{
    // Global student profile.
    StudentProfile& profile();
}

#endif // STUDYDECKS_H
