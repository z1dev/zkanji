/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GROUPSTUDY_H
#define GROUPSTUDY_H

#include <QDataStream>
#include <memory>

class WordGroup;


enum class WordStudyMethod : uchar { Gradual, Single };
enum class WordStudyQuestion { Kanji = 0x0001, Kana = 0x0002, Definition = 0x0004, KanjiKana = 0x0008, DefKanji = 0x0010, DefKana = 0x0020 };
enum class WordStudyAnswering : uchar { CorrectWrong, Choices5, Choices8 };

struct WordStudyItem
{
    int windex = -1;
    bool excluded = false;

    int correct = 0;
    int incorrect = 0;
};

QDataStream& operator<<(QDataStream &stream, const WordStudyItem &item);
QDataStream& operator>>(QDataStream &stream, WordStudyItem &item);

struct WordStudyGradualSettings
{
    // Initial number of items in first round.
    int initnum;
    // Increase in each round.
    int incnum;
    // Maximum number of questions in a single round.
    int roundlimit;
    // Shuffle words in each round.
    bool roundshuffle;
};

struct WordStudySingleSettings
{
    // When randomly selecting words to test, prefer inclusion of words with less answers.
    bool prefernew;
    // When randomly selecting words to test, prefer inclusion of words that had more relative
    // difficult answers.
    bool preferhard;
};

struct WordStudySettings
{
    // Combined flags from WordStudyQuestions. When testing, these parts of the words are
    // hidden. When more than one flag is present, one is randomly selected for each word.
    int tested;
    // Combined flags from WordPartBits. Represents the parts that will be typed in by the
    // student. Only valid for the Choices answering method.
    int typed;
    // Hide kana characters in written form when the kana part is tested.
    bool hidekana;
    // Kana character usage must match word form. I.e. must type katakana for katakana in
    // word.
    bool matchkana;

    // Either correct/wrong, or pick-one-from-8-choices.
    WordStudyAnswering answer;

    // Method of the test.
    // Gradual inclusion (test is made up of rounds from an increasing word pool) or Single
    // (shows all words once unless answer is wrong.)
    WordStudyMethod method;

    // Limit the time available for each answer.
    bool usetimer;
    // Length of timer in seconds.
    int timer;

    // Limit the time of the whole test. When the time is up, the test is suspended.
    bool uselimit;
    // Length of a test in minutes.
    int limit;
    // Limit the number of words asked in a test.
    bool usewordlimit;
    // Number of words shown in a test.
    int wordlimit;

    // Shuffle the words before the test starts.
    bool shuffle;

    // Settings only used for the gradual study method.
    WordStudyGradualSettings gradual;
    WordStudySingleSettings single;
};

// Writes each member of WordStudySettings in stream.
QDataStream& operator<<(QDataStream& stream, const WordStudySettings &s);
// Reads each member of WordStudySettings from stream.
QDataStream& operator>>(QDataStream& stream, WordStudySettings &s);

class WordStudyGradual;
class WordStudySingle;
class WordStudyState;
class Dictionary;
class WordStudy
{
public:
    WordStudy(const WordStudy&) = delete;
    WordStudy& operator=(const WordStudy&) = delete;

    WordStudy(WordGroup *owner);
    ~WordStudy();
    WordStudy(WordStudy&&);
    WordStudy& operator=(WordStudy&&);

    // Legacy load. Mapping contains indexes in the owner group. If an index is -1, the word
    // at that position was skipped because it was a duplicate with a different meaning.
    void loadLegacy(QDataStream &stream, WordStudyMethod method, int version, const std::vector<int> &mapping);

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    WordGroup *getOwner();

    // Changes the word indexes from the original values to the mapped values. Words with no
    // original or a mapped value of -1 will be removed. Duplicates are removed too.
    // It's possible a suspended study becomes unusable after too many changes.
    void applyChanges(const std::map<int, int> &changes);

    // Creates an exact copy of source, apart from the owner group, which stays the same.
    void copy(WordStudy *src);

    // Called when a word has been removed from the main dictionary. Updates the indexes of
    // studied words, decrementing those with a higher value. 
    void processRemovedWord(int windex);

    // Returns the list of the current words and their states.
    const std::vector<WordStudyItem>& indexes() const;

    // Generates a new items list that matches the owner group without changing this word
    // study. The result replaces any existing items in clist, but the excluded status of
    // words in clist are kept if the same word exists in the group. If clist is empty,
    // the current list of excluded words in the study group are used for the excluded items.
    void correctedIndexes(std::vector<WordStudyItem> &clist) const;

    // Generates a list and order of words to test and creates an inner object for the test
    // method. Does nothing if the study is suspended.
    void initTest();

    // Advances to the next item after answering the previous one. 
    bool initNext();

    // Cleans up the study state when aborting or finishing a test, so it can be resumed or
    // reset safely.
    void finish();

    // Returns the current study settings if the settings are valid. Otherwise returns the
    // global default settings.
    const WordStudySettings& studySettings() const;

    Dictionary* dictionary() const;

    // Number of items tested at most in the current test. If no word limit
    // is set, returns the number of all items not excluded.
    int testSize() const;

    // Saves new study settings and included/excluded word index lists. The lists are checked
    // and if their items do not match the group, the result is undefined.
    void setup(const WordStudySettings &newsettings, const std::vector<WordStudyItem> &newlist);

    // Aborts a running test. Any score is applied first and saved.
    void abort();

    // Removes the word with word index from the included word indexes list, and adds it to
    // the excluded indexes list.
    void excludeWord(int windex);
    // Removes the word with word index from the excluded word indexes list, and adds it to
    // the included indexes list.
    void includeWord(int windex);

    // Returns whether the test can continue from a suspended state. (Returns true during a
    // test as well.)
    bool suspended() const;

    // Returns the word index of the next word in the test. If -1 is returned, that either
    // marks the end of the test, or end of the round, depending on the study method.
    int nextIndex();

    // Returns the combination of the tested word parts for the next word. These are hidden
    // for the student before they answer.
    int nextQuestion();

    // Fills the passed array with random word indexes from this study data, to be shown in
    // multiple choice tests. One of the indexes will be the same as the index of the next
    // tested word.
    void randomIndexes(int *arr, int cnt);

    // Updates study data for the current item and moves to the next item.
    void answer(bool correct);
    // Updates study data for the item that was answered last.
    void changeAnswer(bool correct);
    // Returns whether calling changeAnswer() is valid and the current item/ is not the same
    // as the previous one.
    bool canUndo() const;

    // Returns whether there are no more rounds left from the test.
    bool finished() const;

    // Copies the score of the tested items to the list of main items, resetting the test. The
    // number of correct or incorrect answers don't matter, only whether there was an answer
    // and whether there was any wrong answer. Only applicable when the test method was
    // WordStudyMethod::Single.
    void applyScore();

    // Returns the number of items in a finished test or finished round.
    int testedCount() const;

    // Returns the study item which is at pos in the finished test.
    const WordStudyItem& testedItems(int pos) const;

    // Returns the word index of items in a finished test or finished round.
    int testedIndex(int pos) const;

    // Returns whether the item at position was answered correctly.
    bool testedCorrect(int pos) const;

    // Updates the answer of the item at position to correct or incorrect.
    void setTestedCorrect(int pos, bool correct);

    // Returns whether the last answered item in this round was correct or not.
    bool previousCorrect() const;

    // Returns the current round during a gradual study. If the study is not gradual, the
    // value is always 0.
    int currentRound() const;
    // Number of items first included in the current gradual study round. Returns 0 if the
    // study is of a different method.
    int newCount() const;
    // Number of items still not answered.
    int dueCount() const;
    // Number of items answered correctly.
    int correctCount() const;
    // Number of items answered wrong.
    int wrongCount() const;

    // Returns whether the currently/next shown item is new for the student.
    bool nextNew() const;
    // Returns whether the currently/next shown item had a failed answer.
    bool nextFailed() const;

private:
    // Returns a random bit that was set to 1 in choices.
    int createRandomTested(int listindex, int choices);

    bool settingsValid() const;

    struct TestItem
    {
        // Index of word in 'list'.
        int pos;
        // Part of word to be tested.
        WordStudyQuestion tested;

        // Number of times the answer was correct during the current test.
        int correct;
        // Number of times the answer was incorrect during the current test.
        int incorrect;

        TestItem(int pos, WordStudyQuestion tested) : pos(pos), tested(tested), correct(0), incorrect(0) { ; }
        TestItem(int pos, WordStudyQuestion tested, int correct, int incorrect) : pos(pos), tested(tested), correct(correct), incorrect(incorrect) { ; }
        TestItem() {};

        TestItem(const TestItem &src) : pos(src.pos), tested(src.tested), correct(src.correct), incorrect(src.incorrect) { ; }
        TestItem& operator=(const TestItem &src) { pos = src.pos; tested = src.tested; correct = src.correct; incorrect = src.incorrect; return *this; }
    };

    WordGroup *owner;
    
    WordStudySettings settings;

    // Contains word items to be tested. The list is taken directly from the group at the time
    // of creation and does not change when the group changes. The student can exclude words
    // from the test, marking them as excluded.
    std::vector<WordStudyItem> list;

    // Items in the order they are to be tested. Contains an item for every index in list that
    // wasn't excluded, even if the test is limited to less words. Use testSize() to get the
    // number of items tested. Only used when test is in progress or is suspended.
    std::vector<TestItem> testitems;

    // A pointer to either study method, while a test is taking place or is suspended. Its
    // value can be null.
    std::unique_ptr<WordStudyState> state;
};

class WordStudyState
{
public:
    virtual ~WordStudyState() {}

    virtual void loadLegacy(QDataStream &stream, int version) = 0;

    virtual void load(QDataStream &stream) = 0;
    virtual void save(QDataStream &stream) const = 0;
    
    virtual void copy(WordStudyState *src) = 0;

    // General initialization for new tests or new rounds. The implementation
    // depends on the study method.
    virtual void init(int indexcnt) = 0;

    // Prepares the next item to test.
    virtual bool initNext(int indexcnt) = 0;

    // Cleans up the study state when aborting or finishing a test, so it can be resumed or
    // reset safely.
    virtual void finish(int indexcnt) = 0;

    // Called by WordStudy::applyChanges when the main dictionary has been
    // updated and words were removed or merged into others. The passed
    // index is relative to the WordStudy::indexes list.
    // Returns whether the suspended study is still valid after the removal.
    virtual bool itemRemoved(int index, int indexcnt) = 0;

    // Returns the index of the next word to be tested in WordStudy::indexes.
    // The returned value can be negative if the test ended or the last word
    // was answered in the last round. Returns -1 when there are no more items
    // to test in this round.
    virtual int index(int indexcnt) const = 0;

    // Returns the number of items in the current round. roundPosition() can
    // return index over the returned value, in case items get repeated.
    // To test whether there are no more items, check the value of index(),
    // which should return -1 when the round is over.
    virtual int roundSize(int indexcnt) const = 0;

    // Returns the index of the next item in the current round. Only values
    // smaller than the returned number are valid when calling indexAt() or
    // correctAt() etc.
    virtual int roundPosition() const = 0;

    // Returns the index of a word in WordStudy::indexes in the current round.
    virtual int indexAt(int pos, int indexcnt) const = 0;

    // Updates study data for the current item and moves to the next item.
    virtual void answer(bool correct, int indexcnt) = 0;

    // Returns true if the last word has been answered and the test ended.
    // Use together with index(), that returns -1 for both when the test
    // ended and when a round ended.
    virtual bool finished(int indexcnt) const = 0;

    // Returns whether the item at position has been correctly answered.
    virtual bool correctAt(int pos, int indexcnt) const = 0;

    // Sets whether the answer given for the item at pos was correct or not.
    // Only works for items answered already.
    virtual void setCorrectAt(int pos, bool correct, int indexcnt) = 0;

    // Number of items still not answered.
    virtual int dueCount(int indexcnt) const = 0;
    // Number of items answered correctly.
    virtual int correctCount() const = 0;
    // Number of items answered wrong.
    virtual int wrongCount() const = 0;
    // Returns whether the currently/next shown item is new for the student.
    virtual bool nextNew() const = 0;
    // Returns whether the currently/next shown item had a failed answer.
    virtual bool nextFailed(int indexcnt) const = 0;
};

class WordStudyGradual : public WordStudyState
{
public:
    WordStudyGradual(const WordStudyGradualSettings &settings);
    
    virtual void loadLegacy(QDataStream &stream, int version) override;
    virtual void load(QDataStream &stream) override;
    virtual void save(QDataStream &stream) const override;
    virtual void copy(WordStudyState *src) override;
    virtual void init(int indexcnt) override;
    virtual bool initNext(int indexcnt) override;
    virtual void finish(int indexcnt) override;
    virtual bool itemRemoved(int index, int indexcnt) override;
    virtual int index(int indexcnt) const override;
    virtual int roundPosition() const override;
    virtual int roundSize(int indexcnt) const override;
    virtual int indexAt(int pos, int indexcnt) const override;
    virtual void answer(bool correct, int indexcnt) override;
    virtual bool finished(int indexcnt) const override;
    virtual bool correctAt(int pos, int indexcnt) const override;
    virtual void setCorrectAt(int pos, bool correct, int indexcnt) override;

    // Returns the round currently studied. The return value is zero based.
    int currentRound() const;
    int newCount(int indexcnt) const;
    virtual int dueCount(int indexcnt) const override;
    virtual int correctCount() const override;
    virtual int wrongCount() const override;
    virtual bool nextNew() const override;
    virtual bool nextFailed(int indexcnt) const override;
private:
    struct Item
    {
        // Index in WordStudy::indexes.
        int index;
        // The item was included newly in this round and hasn't been answered yet.
        bool newitem;
        // The answer was correct in the round.
        bool correct;

        Item() {}
        explicit Item(int index, bool newitem) : index(index), newitem(newitem), correct(true) {}
        explicit Item(int index, bool newitem, bool correct) : index(index), newitem(newitem), correct(correct) {}
        Item(const Item &src) : index(src.index), newitem(src.newitem), correct(src.correct) { ; }
        Item& operator=(const Item &src) { index = src.index; newitem = src.newitem; correct = src.correct; return *this; }
    };

    const WordStudyGradualSettings &settings;

    // Current round of the test. If the student makes a mistake, the round's
    // value won't increase at the end of the previous round.
    int round;

    // Number of times the current round started. Not saved and only used to
    // show it to the user when item stats are shown at the end of a round.
    int repeats;

    // Number of items from WordStudy::indexes that were added till the
    // current round. This can be different from what the round might indicate
    // in case a word has been deleted due to dictionary changes.
    int included;

    // Position in the current round, which is also an index in testitems.
    // If this value is over the size of testitems, the round is over.
    int pos;

    // True when the next item is initialized but not answered yet. Only valid during a test
    // and not saved.
    bool unanswered;

    // Item indexes in WordStudy::indexes to be tested in the current round.
    // Only contains items during or right after a round.
    std::vector<Item> testitems;
};

class WordStudySingle : public WordStudyState
{
public:
    WordStudySingle();

    virtual void loadLegacy(QDataStream &stream, int version) override;
    virtual void load(QDataStream &stream) override;
    virtual void save(QDataStream &stream) const override;
    virtual void copy(WordStudyState *src) override;
    virtual void init(int indexcnt) override;
    virtual bool initNext(int indexcnt) override;
    virtual void finish(int indexcnt) override;
    virtual bool itemRemoved(int index, int indexcnt) override;
    virtual int index(int indexcnt) const override;
    virtual int roundPosition() const override;
    virtual int roundSize(int indexcnt) const override;
    virtual int indexAt(int pos, int indexcnt) const override;
    virtual void answer(bool correct, int indexcnt) override;
    virtual bool finished(int indexcnt) const override;
    virtual bool correctAt(int pos, int indexcnt) const override;
    virtual void setCorrectAt(int pos, bool correct, int indexcnt) override;

    virtual int dueCount(int indexcnt) const override;
    virtual int correctCount() const override;
    virtual int wrongCount() const override;
    virtual bool nextNew() const override;
    virtual bool nextFailed(int indexcnt) const override;
private:
    // Either index in WordStudy::indexes (if smaller than its size), or,
    // after subtracting indexcnt from it, position in repeated.
    int pos;

    // True when the next item is initialized but not answered yet. Only valid during a test
    // and not saved.
    bool unanswered;

    // Stores 1 or 0 depending on the answer to the items in the test. Correct
    // answer is 1, incorrect is 0. This list is filled as answers are given.
    std::vector<uchar> correctlist;

    // Filled with indexes to WordStudy::indexes which got a wrong answer
    // and should be repeated. If pos goes over the size of WordStudy::indexes
    // it points to items in repeated.
    std::vector<int> repeated;
};

namespace ZKanji
{
    const WordStudySettings& defaultWordStudySettings();
    void setDefaultWordStudySettings(const WordStudySettings &newsettings);
}

#endif // GROUPSTUDY_H

