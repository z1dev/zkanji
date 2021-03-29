/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef GROUPS_H
#define GROUPS_H

#include <QCoreApplication>
#include <QDateTime>
#include <memory>
#include <forward_list>
#include <functional>
#include <memory>
#include "smartvector.h"
#include "groupstudy.h"

enum class GroupTypes { Kanji, Words };

class DictionaryGroupItemModel;
class KanjiGroupModel;
class GroupCategoryBase;
class Dictionary;
class Groups;
struct Range;
struct Interval;
class GroupBase : public QObject
{
    Q_OBJECT
public:
    GroupBase() = delete;
    GroupBase(const GroupBase&) = delete;
    GroupBase& operator=(const GroupBase&) = delete;

    virtual ~GroupBase();

    virtual void load(QDataStream &stream);
    virtual void save(QDataStream &stream) const;

    virtual bool isEmpty() const = 0;

    virtual bool isCategory() const;
    virtual GroupTypes groupType() const = 0;

    virtual Dictionary* dictionary() = 0;
    virtual const Dictionary* dictionary() const = 0;

    virtual void copy(GroupBase *src);

    GroupCategoryBase* parentCategory();
    const GroupCategoryBase* parentCategory() const;

    const QString& name() const;

    virtual void setName(QString newname);

    // Returns the name of the group including its parent categories as a directory path with
    // the / character as separator. The /, ^ and [ characters in the names are escaped with
    // the ^ character.
    const QString fullEncodedName() const;
    // Returns the name of the group including its parent categories as a directory path with
    // the / character as separator. The name is not escaped and shouldn't be used in function
    // calls, besides displaying it to the user. Set a root where the path should start.
    // If root is not a parent category, the whole path is returned.
    const QString fullName(GroupCategoryBase *root = nullptr) const;

protected:
    GroupBase(GroupCategoryBase *parent);
    GroupBase(GroupCategoryBase *parent, QString name);

    GroupBase(GroupBase &&src);
    GroupBase& operator=(GroupBase &&src);

    //virtual Groups* getOwner();
    //virtual const Groups* getOwner() const;

    //GroupCategoryBase* topParent();
    //const GroupCategoryBase* topParent() const;
private:
    GroupCategoryBase *parent;

    QString _name;

    friend class GroupCategoryBase;
};

// Container for other group categories and kanji/word groups.
class GroupCategoryBase : public GroupBase
{
    Q_OBJECT
        signals:
    void groupsReseted();
    // Signaled when a category has been added to a parent category. Index is its new position
    // in the parent category list.
    void categoryAdded(GroupCategoryBase *parent, int index);
    // Signaled when a group has been added to a parent category. Index is its new position
    // in the parent category list.
    void groupAdded(GroupCategoryBase *parent, int index);
    // Signaled before a category is deleted from a parent category. Index is its position
    // in parent before it was deleted.
    void categoryAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupCategoryBase *cat);
    // Signaled when a category has been deleted from a parent category. Index is its
    // position before it was deleted.
    void categoryDeleted(GroupCategoryBase *parent, int index, void *oldaddress);
    // Signaled before a group is deleted from a parent category. Index is its position
    // in parent before it's deleted.
    // When deleting a category, this signal is sent out for every group in its branch, with
    // parent set to null.
    void groupAboutToBeDeleted(GroupCategoryBase *parent, int index, GroupBase *group);
    // Signaled when a group has been deleted from a parent category. Index is its position
    // in parent before it was deleted. oldaddress is the old pointer of the deleted group. It
    // shouldn't be used apart from identifying it.
    // When deleting a category, this signal is sent out for every group in its branch, with
    // parent set to null.
    // Note: do not directly emit this signal. Call emitGroupDeleted() instead.
    void groupDeleted(GroupCategoryBase *parent, int index, void *oldaddress);

    // Signaled when a category's name has been changed.
    void categoryRenamed(GroupCategoryBase *parent, int index);
    // Signaled when a group's name has been changed.
    void groupRenamed(GroupCategoryBase *parent, int index);

    // Signaled when items were added to a group at the passed positions. Each interval holds
    // the item index before the insertion where the new items are inserted.
    void itemsInserted(GroupBase *parent, const smartvector<Interval> &interval/*int first, int last*/);
    // Signaled when items were removed from a group, with the index of the items in their
    // group.
    void itemsRemoved(GroupBase *parent, const smartvector<Range> &ranges);
    // Signaled before items are removed from a group. First and last marks the position
    // interval of the items removed.
    //void beginItemsRemove(GroupBase *parent, int first, int last);

    // Signaled when items were moved inside the group.
    void itemsMoved(GroupBase *parent, const smartvector<Range> &ranges, int destpos);
    // Signaled before items are moved inside the group. First and last marks the interval of
    // moved items. Pos is the new position of the first moved item. The position must be
    // valid before the items are moved, and shouldn't fall inside [first, last].
    //void beginItemsMove(GroupBase *parent, int first, int last, int pos);

    // Signalled when a category with the parent of srcparent at index srcindex was moved
    // to destparent at destindex. The category was placed in front of the item originally at
    // destindex. If srcparent and destparent are identical, and destindex was higher than
    // srcindex, this means the real new index is one less than destindex.
    void categoryMoved(GroupCategoryBase *srcparent, int srcindex, GroupCategoryBase *destparent, int destindex);
    // Signalled when a group with the parent of srcparent at index srcindex was moved to
    // destparent at destindex. The group was placed in front of the item originally at
    // destindex. If srcparent and destparent are identical, and destindex was higher than
    // srcindex, this means the real new index is one less than destindex.
    void groupMoved(GroupCategoryBase *srcparent, int srcindex, GroupCategoryBase *destparent, int destindex);

    void categoriesMoved(GroupCategoryBase *srcparent, const Range &range, GroupCategoryBase *destparent, int destindex);
    void groupsMoved(GroupCategoryBase *srcparent, const Range &range, GroupCategoryBase *destparent, int destindex);
public:
    typedef size_t  size_type;

    GroupCategoryBase() = delete;
    GroupCategoryBase(const GroupCategoryBase&) = delete;
    GroupCategoryBase& operator= (const GroupCategoryBase&) = delete;

    virtual ~GroupCategoryBase();

    virtual void load(QDataStream &stream);
    virtual void save(QDataStream &stream) const;

    virtual bool isEmpty() const override;
    virtual bool isCategory() const override;

    virtual Dictionary* dictionary();
    virtual const Dictionary* dictionary() const;

    // Creates an exact copy of src apart from the dictionary, which is unchanged. Creates new
    // groups as necessary instead of copying pointers.
    void copy(GroupCategoryBase *src);

    // Creates a child category and returns its index.
    int addCategory(QString name);

    // Creates a group and returns its index.
    int addGroup(QString name);

    // Removes the child category at the given index.
    void deleteCategory(int index);

    // Removes the passed child category. Child must be a sub-category.
    //virtual void deleteCategory(GroupCategoryBase *child);

    // Removes the group at the given index.
    void deleteGroup(int ix);

    // Removes categories and grouops in the list
    void remove(const std::vector<GroupBase*> &which);


    // Moves the category 'what' from its original parent and index to a new parent and index.
    // If the original parent of 'what' and destparent are the same, 'what' will be moved in
    // front of the element originally at destindex. It's possible that its final index will
    // be one less than destindex after the move, if it had a lower index than destindex.
    // If there is a name conflict with other categories, the move fails.
    bool moveCategory(GroupCategoryBase *what, GroupCategoryBase *destparent, int destindex);
    // Moves the group 'what' from its original parent and index to a new parent and index.
    // If the original parent of 'what' and destparent are the same, 'what' will be moved in
    // front of the element originally at destindex. It's possible that its final index will
    // be one less than destindex after the move, if it had a lower index than destindex.
    // If there is a name conflict with other groups, the move fails.
    bool moveGroup(GroupBase *what, GroupCategoryBase *destparent, int destindex);

    // Moves the categories from their original parent to a new parent at destindex. Every
    // category in the list must come from the same parent or the result will be wrong.
    // Ignores name conflicts. Check with GroupCategoryBase::categoryNameTaken().
    void moveCategories(const std::vector<GroupCategoryBase*> &list, GroupCategoryBase *destparent, int destindex);
    // Moves the groups from their original parent to a new parent at destindex. Every group
    // in the list must come from the same parent or the result will be wrong.
    // Ignores name conflicts. Check with GroupCategoryBase::groupNameTaken().
    void moveGroups(const std::vector<GroupBase*> &list, GroupCategoryBase *destparent, int destindex);

    // Returns the 0 based index of the given child category, or -1 if it's not found directly
    // inside this category.
    int categoryIndex(GroupCategoryBase *child);

    // Returns the index of a child category with the specified name. Names are checked
    // without case-sensitivity. Returns -1 if no category was found in this category with the
    // passed name.
    int categoryIndex(QString name);

    // Number of direct sub-categories.
    int categoryCount() const;

    // Returns the group at index.
    GroupCategoryBase* categories(int index);

    // Returns the group at index.
    const GroupCategoryBase* categories(int index) const;

    // The total number of groups below this category at any level.
    int groupCount() const;

    // Number of groups in the category.
    size_type size() const;

    // Returns the group at index.
    GroupBase* items(int index);

    // Returns the group at index.
    const GroupBase* items(int index) const;

    // Returns the index of a child group with the specified name. Names are checked without
    // case-sensitivity. Returns -1 if no group was found in this category with the passed
    // name.
    int groupIndex(QString name) const;

    // Returns the index of a child group. Only direct children indexes are returned. If the
    // group is not found in this category, -1 is returned.
    int groupIndex(GroupBase *group) const;

    // Returns whether the passed name is valid for the given child. The name can't
    // be empty or match the name of any child. The check is case-insensitive.
    virtual bool categoryNameTaken(QString name) const;
    // Returns whether the passed name is valid for the given child. The name can't
    // be empty or match the name of any child. The check is case-insensitive.
    virtual bool groupNameTaken(QString name) const;

    // Emits the top level category's signal when this category has been renamed.
    virtual void setName(QString newname) override;

    // Static:

    // Helper function for adding a new group to any category.
    static int addGroup(QString name, GroupCategoryBase *cat);
    // Helper function for adding a child category to any category.
    static int addCategory(QString name, GroupCategoryBase *cat);

    // Executes a function on every group under this category.
    void walkGroups(const std::function<void(GroupBase*)> &callback);

    // Returns whether the passed item is a child of this category. If recursive is false,
    // only checks direct children, otherwise checks the whole tree below this category.
    bool isChild(GroupBase *item, bool recursive = false) const;
protected:
    virtual void clear();

    //GroupCategoryBase(GroupCategoryBase *parent);
    GroupCategoryBase(GroupCategoryBase *parent, QString name);

    GroupCategoryBase(GroupCategoryBase &&src);
    GroupCategoryBase& operator=(GroupCategoryBase &&src);

    // Should return a newly constructed WordGroupCategory or KanjiGroupCategory.
    // The object is added to the inner list and will be deleted automatically.
    virtual GroupCategoryBase* createCategory(QString name) = 0;

    // Should return a newly constructed WordGroup or KanjiGroup. The object's ownership is
    // handed to this category.
    virtual GroupBase* createGroup(QString name) = 0;

    // Emits the groupDeleted signal with the passed arguments. Override for other operations.
    virtual void emitGroupDeleted(GroupCategoryBase *parent, int index, void *oldaddress);
private:
    // Emits the groupDeleted() signal for every child group in the category and its child
    // categories.
    void emitDeleted();

    // Fills result with groups and categories from src. Any group or category inside another
    // category in src is excluded from the result.
    void selectTopItems(const std::vector<GroupBase*> &src, std::vector<GroupBase*> &result) const;

    // Child categories under this category.
    smartvector<GroupCategoryBase> list;

    // Groups belonging to this category. The items in the list are either WordGroup or
    // KanjiGroup objects. They are created with the abstract virtual createGroup().
    smartvector<GroupBase> groups;

    // The category at the top of the hierarchy. Only the owner has access to the dictionary's
    // groups object, and only the owner's signals are emitted.
    GroupCategoryBase *owner;

    typedef GroupBase   base;
};

class WordGroup;
class KanjiGroup;

template<typename T>
class GroupCategory : public GroupCategoryBase
{
    typedef GroupCategory<T> self_type;
public:
    GroupCategory() = delete;
    GroupCategory(const GroupCategory&) = delete;
    //GroupCategory(GroupCategory&&) = delete;
    GroupCategory& operator=(const GroupCategory&) = delete;
    //GroupCategory& operator=(GroupCategory&&) = delete;

    virtual GroupTypes groupType() const override;

    T* items(int index)
    {
        return dynamic_cast<T*>(base::items(index));
    }

    const T* items(int index) const;

    self_type* categories(int index);
    const self_type* categories(int index) const;

    // Returns a group with the full name (includes paths and encoded with ^.) If the  group
    // does not exist, the value of create determines whether a group will be created and
    // returned. Otherwise returns the existing or created group. This category is used as
    // root for the returned or created category.
    T* groupFromEncodedName(const QString &fullname, int pos = 0, int len = -1, bool create = false);
protected:
    GroupCategory(self_type *parent, QString name);

    GroupCategory(self_type &&src);
    GroupCategory& operator=(self_type &&src);

    virtual GroupCategoryBase* createCategory(QString name) override;
    virtual GroupBase* createGroup(QString name) override;
private:
    template<typename TT = T>
    typename std::enable_if<std::is_same</*typename*/ TT, WordGroup>::value, GroupTypes>::type _groupType() const
    {
        return GroupTypes::Words;
    }

    template<typename TT = T>
    typename std::enable_if<std::is_same</*typename*/ TT, KanjiGroup>::value, GroupTypes>::type _groupType() const
    {
        return GroupTypes::Kanji;
    }

    typedef GroupCategoryBase   base;
};

typedef GroupCategory<WordGroup>    WordGroupCategory;
typedef GroupCategory<KanjiGroup>   KanjiGroupCategory;

struct WordEntry;
struct WordStudySettings;

class WordGroups;
class WordGroup : public GroupBase
{
    Q_OBJECT
public:
    typedef size_t  size_type;

    WordGroup() = delete;
    WordGroup(const WordGroup &orig) = delete;
    WordGroup& operator=(const WordGroup&) = delete;

    virtual ~WordGroup();

    // Legacy load functions
    void loadLegacy(QDataStream &stream, bool study, int version);
    // End legacy load functions

    virtual void load(QDataStream &stream);
    virtual void save(QDataStream &stream) const;

    virtual bool isEmpty() const override;
    virtual GroupTypes groupType() const override;

    virtual void copy(GroupBase *src) override;

    // Changes the word indexes from the original values to the mapped values.
    // Words with no original or a mapped value of -1 will be removed.
    // Duplicates are removed too.
    void applyChanges(const std::map<int, int> &changes);

    // Removes the passed index from the group's words list, decrements the index of words
    // above this value, and checks whether study is valid after the removal.
    void processRemovedWord(int windex);

    void clear();

    size_type size() const;
    //bool empty() const;

    Dictionary* dictionary();
    const Dictionary* dictionary() const;

    DictionaryGroupItemModel* groupModel();

    WordEntry* items(int index);
    const WordEntry* items(int index) const;
    //WordEntry* operator[](int index);
    //const WordEntry* operator[](int index) const;

    // Gives direct access to the indexes of stored words in a group.
    const std::vector<int>& getIndexes() const;
    // Returns the word at the given position in the group.
    int indexes(int pos) const;

    // Returns the index of a word in the group. Returns -1 if the word is not in the group.
    int indexOf(int windex) const;
    // Fills positions with the group index of every word index. If a word is not found, it's
    // excluded from positions. The resulting positions list is ordered by group index.
    // The source and destination vectors can be the same.
    void indexOf(const std::vector<int> &windexes, std::vector<int> &positions) const;

    //void setStudyMethod(WordStudy::Method method);

    // Adds a word to the group if it's not already added. Returns its current position.
    int add(int windex);

    // Inserts a word to the group at pos if it's not already added. The pos position must be
    // valid in the bounds of [0, size of group], or -1, to insert at the end of the group.
    // Returns the word's position.
    int insert(int windex, int pos);

    // Adds multiple words to the group, filling positions with their position in the group.
    // Words already in the group are not added, but their positions are set. Returns the
    // number of words actually added.
    int add(const std::vector<int> &windexes, std::vector<int> *positions = nullptr);

    // Inserts multiple words to the group in front of the word at pos, filling positions with
    // their position in the group. Words already in the group are not inserted, but their
    // positions are set. Returns the number of words actually inserted.
    // The pos position must be valid in the bounds of [0, size of group], or -1, to insert at
    // the end of the group.
    int insert(const std::vector<int> &windexes, int pos, std::vector<int> *positions = nullptr);

    // Removes a word from the group if it's found.
    void remove(WordEntry *e);

    // Removes the words at the passed indexes from the group. The ranges must be ordered by
    // position. The result is undefined if the ranges overlap or are invalid.
    void remove(const smartvector<Range> &ranges/*const std::vector<int> &indexes*/);
    // Removes the words at the given ranges from the group, and moves them in front of the
    // word at pos. The behavior is undefiend if ranges overlap, touch or are invalid.
    // The ranges must be sorted.
    // If pos is -1, the words at the given indexes are moved to the end of the group.
    void move(const smartvector<Range> &ranges, int pos);

    // Removes a word at the index position from the group.
    void removeAt(int index);
    // Removes the words in the interval [first, last] from the group.
    void removeRange(int first, int last);

    WordStudy& studyData();

    // Emits the top level category's signal when this group has been renamed.
    virtual void setName(QString newname) override;

    // Returns the top level category holding every sub-category and group in its dictionary.
    WordGroups& owner();

    // Returns the top level category holding every sub-category and group in its dictionary.
    const WordGroups& owner() const;

    // Returns the last set test settings of this group. If nothing was set,
    // the default settings are returned.
    //const WordStudySettings& studySettings() const;

    // Notifies the study data to update its word list to fit the group's
    // current state. Any data of a suspended test is also deleted.
    //void resetStudy();

    // Returns whether the group has study data for a test that was suspended,
    // but can continue.
    //bool studySuspended() const;
protected:
    //WordGroup(WordGroupCategory *parent);
    WordGroup(WordGroupCategory *parent, QString name);

    WordGroup(WordGroup &&src);
    WordGroup& operator=(WordGroup &&src);
private:
    // Removes this group from the word's list of groups. If the group doesn't remain in any
    // group, the list is deleted and the flag marking the word as being in a group is
    // cleared. In _DEBUG mode, throws exception if group is not in the global list of groups
    // for that word.
    void removeFromGroup(int index);

    // Moves items between first and last to pos. The passed range is guaranteed to be outside
    // pos, but can be below or above it. Pos refers to an item position before the move.
    // First and last are both inclusive positions.
    //void _move(int first, int last, int pos);

    std::vector<int> list;

    std::unique_ptr<DictionaryGroupItemModel> modelptr;

    WordStudy study;

    friend WordGroupCategory;
    typedef GroupBase base;
};


class Groups;
class WordGroups : public WordGroupCategory
{
    Q_OBJECT
public:
    WordGroups() = delete;
    WordGroups(const WordGroups&) = delete;
    WordGroups(WordGroups&&) = delete;
    WordGroups& operator=(const WordGroups&) = delete;
    WordGroups& operator=(WordGroups&&) = delete;

    WordGroups(Groups *owner);
    virtual ~WordGroups();

    virtual void clear() override;

    // Legacy load functions
    void loadLegacy(QDataStream &stream, bool study, int version);
    // End legacy load functions

    void applyChanges(const std::map<int, int> &changes);

    void copy(WordGroups *src);

    // Removes any data of the passed word index from groups and decrements the index of words
    // above this value.
    void processRemovedWord(int windex);

    virtual Dictionary* dictionary() override;
    virtual const Dictionary* dictionary() const override;

    // Returns the list of groups a word is placed in. If the word was not added to a group,
    // an empty list is created and returned that can be used for expanding. To avoid creating
    // this list for words not in a group, check first with wordHasGroups().
    std::forward_list<WordGroup*>& groupsOfWord(int windex);
    // Returns whether a word is placed in any group.
    bool wordHasGroups(int windex) const;


    // Returns the total number of words added to a group.
    int wordsInGroups() const;

    // Checks whether the word of windex is still in a group, and if not, removes its
    // groupsOfWord entry, and unsets the inGroup tag for the word.
    void checkGroupsOfWord(int windex);

    // Returns a word group with the full name (includes paths and encoded with ^.) If the
    // group does not exist, the value of create determines whether a group will be created
    // and returned. Otherwise returns the existing or created group.
    WordGroup* groupFromEncodedName(const QString &fullname, int pos = 0, int len = -1, bool create = false);

    // Returns the group which had words last added to it or null.
    WordGroup* lastSelected() const;
    // Sets which group to return in lastSelected(), when accepting the word to group dialog.
    void setLastSelected(WordGroup *g);
    // Sets which group to return in lastSelected(), when accepting the word to group dialog.
    // The passed name is a full encoded name.
    void setLastSelected(const QString &name);
private:
    virtual void emitGroupDeleted(GroupCategoryBase *parent, int index, void *oldaddress) override;

    // Calls applyChanges for the passed category's items and sub categories.
    void groupsApplyChanges(GroupCategoryBase *cat, const std::map<int, int> &changes);

    // Newly creates the data in wordsgroups.
    void fixWordsGroups();

    Groups *owner;

    // [word index, groups list] where the word is found in.
    std::map<int, std::forward_list<WordGroup*>> wordsgroups;

    // Group last selected in a word to group dialog as destination.
    WordGroup *lastgroup;

    typedef WordGroupCategory base;
};

struct KanjiEntry;
class KanjiGroups;
class KanjiGroup : public GroupBase
{
    Q_OBJECT
public:
    typedef size_t  size_type;

    KanjiGroup() = delete;
    KanjiGroup(const KanjiGroup &orig) = delete;
    KanjiGroup& operator=(const KanjiGroup&) = delete;

    virtual ~KanjiGroup();

    // Legacy load functions
    void loadLegacy(QDataStream &stream);
    // End legacy load functions

    virtual void load(QDataStream &stream);
    virtual void save(QDataStream &stream) const;

    virtual bool isEmpty() const override;
    virtual GroupTypes groupType() const override;

    virtual void copy(GroupBase *src) override;

    KanjiEntry* items(int index);
    const KanjiEntry* items(int index) const;

    size_type size() const;

    Dictionary* dictionary();
    const Dictionary* dictionary() const;

    KanjiGroupModel* groupModel();

    // Gives direct access to the indexes of stored kanji in a group.
    const std::vector<ushort>& getIndexes() const;

    // Returns the index of kanji in the group. Returns -1 if the kanji is not in the group.
    int indexOf(ushort kindex) const;
    // Returns the group positions of every kanji in kindexes. If a kanji is not found in the
    // group, it's omited from the return value. The vectors don't need to be distinct.
    void indexOf(const std::vector<ushort> &kindexes, std::vector<int> &positions) const;

    // Adds a kanji to the group and returns its position. If a kanji has been
    // added before, its old index is returned and it's not added again.
    int add(ushort kindex);
    // Inserts a kanji to the group and returns its position. If a kanji has been inserted
    // already, its old index is returned and not inserted again.
    // The pos position must be valid in the bounds of [0, size of group], or -1, to insert at
    // the end of the group.
    int insert(ushort kindex, int pos);
    // Adds every kanji from the indexes list to the group, filling positions
    // (if its not null) with the new position of each kanji. Kanji already in
    // the group won't be added again, only their positions. Returns the
    // number of kanji added.
    int add(const std::vector<ushort> &kindexes, std::vector<int> *positions = nullptr);
    // Removes the kanji at the passed indexes from the group. The result is undefined if
    // indexes hold values out of range or if it has duplicates.
    void remove(const smartvector<Range> &ranges);
    // Inserts every kanji from the indexes list to the group at pos position, filling
    // positions (if its not null) with the new position of each kanji. Kanji already in the
    // group won't be added again, only their positions. Returns the number of kanji added.
    // The pos position must be valid in the bounds of [0, size of group], or -1, to insert at
    // the end of the group.
    int insert(const std::vector<ushort> &kindexes, int pos, std::vector<int> *positions = nullptr);
    // Removes the kanji at the given indexes from the group, and moves them in front of the
    // kanji at pos. The kanji at pos must not be in indexes. The call is invalid if indexes
    // hold values out of range or if it has duplicates.
    // If pos is -1, the kanji at the given indexes are moved to the end of the group.
    void move(const smartvector<Range> &ranges, int pos);

    // Removes a kanji at index position.
    void removeAt(int index);
    // Removes multiple kanji found at the passed positions.
    void removeRange(int first, int last);

    // Emits the top level category's signal when this group has been renamed.
    virtual void setName(QString newname) override;

    // Returns the top level category holding every sub-category and group in its dictionary.
    KanjiGroups& owner();
    // Returns the top level category holding every sub-category and group in its dictionary.
    const KanjiGroups& owner() const;
protected:
    //KanjiGroup(KanjiGroupCategory *parent);
    KanjiGroup(KanjiGroupCategory *parent, QString name);

    KanjiGroup(KanjiGroup &&src);
    KanjiGroup& operator=(KanjiGroup &&src);
private:
    // Moves items between first and last to pos. The passed range is guaranteed to be outside
    // pos, but can be below or above it. Pos refers to an item position before the move.
    // First and last are both inclusive positions.
    //void _move(int first, int last, int pos);

    std::vector<ushort> list;

    std::unique_ptr<KanjiGroupModel> modelptr;

    friend KanjiGroupCategory;
    typedef GroupBase   base;
};

class KanjiGroups : public KanjiGroupCategory
{
    Q_OBJECT
public:
    KanjiGroups() = delete;
    KanjiGroups(const KanjiGroups&) = delete;
    KanjiGroups(KanjiGroups&&) = delete;
    KanjiGroups& operator=(const KanjiGroups&) = delete;
    KanjiGroups& operator=(KanjiGroups&&) = delete;

    KanjiGroups(Groups *owner);
    virtual ~KanjiGroups();

    virtual void clear() override;

    // Legacy load functions
    void loadLegacy(QDataStream &stream);
    // End legacy load functions

    virtual Dictionary* dictionary() override;
    virtual const Dictionary* dictionary() const override;

    // Returns a kanji group with the full name (includes paths and encoded with ^.) If the
    // group does not exist, the value of create determines whether a group will be created
    // and returned. Otherwise returns the existing or created group.
    KanjiGroup* groupFromEncodedName(const QString &fullname, int pos = 0, int len = -1, bool create = false);

    // Returns the group which had kanji last added to it or null.
    KanjiGroup* lastSelected() const;
    // Sets which group to return in lastSelected(), when accepting the kanji to group dialog.
    void setLastSelected(KanjiGroup *g);
    // Sets which group to return in lastSelected(), when accepting the kanji to group dialog.
    // The passed name is a full encoded name.
    void setLastSelected(const QString &name);
private:
    virtual void emitGroupDeleted(GroupCategoryBase *parent, int index, void *oldaddress);

    Groups *owner;

    // Group last selected in a kanji to group dialog as destination.
    KanjiGroup *lastgroup;

    typedef KanjiGroupCategory base;
};

class Groups
{
public:
    Groups() = delete;
    Groups(const Groups&) = delete;
    Groups& operator=(const Groups&) = delete;
    Groups(Groups&&) = delete;
    Groups& operator=(Groups&&) = delete;

    Groups(Dictionary *dict);
    ~Groups();

    // Returns the value of version which is set in load() when reading the user group file.
    //int fileVersion();

    // Legacy load functions
    void loadWordsLegacy(QDataStream &stream, int version);
    void loadKanjiLegacy(QDataStream &stream, int version);
    // End legacy load functions

    void load(QDataStream &stream);
    void save(QDataStream &stream) const;

    void clear();

    WordGroups& wordGroups();
    KanjiGroups& kanjiGroups();
    const WordGroups& wordGroups() const;
    const KanjiGroups& kanjiGroups() const;

    Dictionary* dictionary();
    const Dictionary* dictionary() const;

    // Changes the original index to the new index after a dictionary update. The changes map
    // contains the words' original index in the old dictionary, and the new index in newdict.
    // If the new index is -1, the word was not found in the new dictionary and its data
    // should be removed.
    void applyChanges(std::map<int, int> &changes);

    void copy(Groups *src);

    // Called when a word is deleted from the main dictionary. Notifies the groups to erase
    // their data of this word.
    void processRemovedWord(int windex);
private:
    Dictionary *dict;
    WordGroups wordgroups;
    KanjiGroups kanjigroups;

    //int version; // Base version.
};


#endif
