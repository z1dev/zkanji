/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef ZKANJIGRIDMODEL_H
#define ZKANJIGRIDMODEL_H

#include <QObject>
#include <QColor>
#include "smartvector.h"

class KanjiGroup;
class Dictionary;
class GroupBase;
struct Range;
struct Interval;
class QMimeData;
enum class StatusTypes;

// A model (data provider) of a list of kanji for ZKanjiGridViews. While not a Qt model,
// mimics the view/model design, but as it doesn't have to be so general, its functions
// are only useful for the ZKanjiGridView.

// Implementing drag and drop for KanjiGridModels:
// drop: implement supportedDropActions(), dropMimeData() and canDropMimeData().
// canDropMimeData() can be called multiple times. When index is 0 or greater, must return
// whether dropping in front of that item or end of model is valid. When index is -1, the view
// tests whether dropping onto the grid at unspecified position is valid.
// When the action of the DropAction is Qt::MoveAction and the sources are different, the drag
// source will be notified to remove the dragged items. If the source and destination are the
// same, the model must handle the move within itself. In this case remove won't be called.
class KanjiGridModel : public QObject
{
    Q_OBJECT
signals:
    void modelReset();
    void itemsInserted(const smartvector<Interval> &intervals);
    void itemsRemoved(const smartvector<Range> &ranges);
    void itemsMoved(const smartvector<Range> &ranges, int pos);
    void layoutToChange();
    void layoutChanged();
public:
    typedef size_t  size_type;

    KanjiGridModel(QObject *parent = nullptr);
    virtual ~KanjiGridModel();

    // Returns the number of kanji represented by the model.
    virtual size_type size() const = 0;
    // Returns whether there are no kanji to be shown.
    bool empty() const;

    // The kanji group represented by the model.
    virtual KanjiGroup* kanjiGroup() const;

    // Kanji index at the passed cell position. Return -1 if pos is outside range of cells.
    virtual int kanjiAt(int pos) const = 0;

    // The number of status widgets needed on a status bar for this grid. Excludes the number
    // of kanji widget, as it's always included.
    virtual int statusCount() const;
    // Type of status widget at statusindex position on a status bar.
    virtual StatusTypes statusType(int statusindex) const;
    // Text of status labels on widget at statusindex position on a status bar. Negative
    // value means the title text for this status widget. In that case the labelindex is
    // ignored. There can be one or two labels after the title text, denoted by label index,
    // depending on statusType(). Kanjipos says which value the text should refer to.
    virtual QString statusText(int statusindex, int labelindex, int kanjipos) const;
    // Size of status label on widget at statusindex position on a status bar. Labelindex can
    // be negative to mean the title label.
    virtual int statusSize(int statusindex, int labelindex) const;
    // Whether the first status label after the title label is text-aligned right or not. Only
    // called when the statusType() is TitleValue.
    virtual bool statusAlignRight(int statusindex) const;

    // Color of the kanji to be drawn at pos position. Return an invalid color to
    // use the default color from the settings.
    virtual QColor textColorAt(int /*post*/) const { return QColor(); }
    // Color to be used as background for kanji at pos position. Return an invalid color to
    // use the default color from the settings.
    virtual QColor backColorAt(int /*pos*/) const { return QColor(); }

    virtual QMimeData* mimeData(const std::vector<int> &indexes) const;

    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int index) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int index);

    virtual Qt::DropActions supportedDragActions() const;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const;

    // Saves the indexes received in indexes and returns an id that can be used to restore the
    // list after a layout change with restorePersistentList(). The indexes list can hold
    // duplicates.
    // The function must be called during layoutToChange signal handling. Always call
    // restorePersistentList() after the change to clear the inner data.
    int savePersistentList(std::vector<int> &indexes);
    // Restores indexes previously resulted in a call to savePersistentList(). Pass the same
    // id to the function that was received in savePersistentList().
    void restorePersistentList(int id, std::vector<int> &indexes);
protected:
    // Updates results to hold all the saved indexes with savePersistentList(). The returned
    // list holds unique items ordered by value.
    void getPersistentIndexes(std::vector<int> &result);
    // Call in a derived class before signalling the layoutChanged() to update persistent
    // indexes. Pass old indexes in from and new indexes in to. The from list must be ordered
    // and only hold unique items or the behavior of this function is undefined. The size of
    // to must match the size of from.
    void changePersistentLists(const std::vector<int> &from, const std::vector<int> &to);

    // Imitates Qt models. Notifies about the beginning of a complete data
    // reset operation.
    //void beginResetModel();
    // Imitates Qt models. Notifies about the completion of a data reset
    // operation.
    void signalModelReset();

    // Notifies about item addition or item change.
    void signalItemsInserted(const smartvector<Interval> &intervals);
    // Notifies about the end of removal of multiple items.
    void signalItemsRemoved(const smartvector<Range> &ranges);
    // Notifies about the end of move of multiple items.
    void signalItemsMoved(const smartvector<Range> &ranges, int destpos);
    // Notifies about the start of a layout change.
    void signalLayoutToChange();
    // Notifies about a layout change.
    void signalLayoutChanged();
private:
    int permapindex;
    std::map<int, std::vector<int>> persistents;

    typedef QObject base;
};

// Model for all the main kanji in order.
class MainKanjiListModel : public KanjiGridModel
{
    Q_OBJECT
public:
    typedef KanjiGridModel::size_type  size_type;

    MainKanjiListModel(QObject *parent = nullptr);
    virtual ~MainKanjiListModel();

    virtual size_type size() const override;

    // Returns the index of the kanji in the main kanji list at pos position.
    virtual int kanjiAt(int pos) const override;

    virtual int statusCount() const override;
    virtual StatusTypes statusType(int statusindex) const override;
    virtual QString statusText(int statusindex, int kanjipos, int labelindex) const override;
    virtual int statusSize(int statusindex, int labelindex) const override;
    virtual bool statusAlignRight(int statusindex) const override;
private:
    typedef KanjiGridModel base;
};

// Handles a generic list of kanji from a list of main kanji indexes passed to setList(). 
class KanjiListModel : public KanjiGridModel
{
    Q_OBJECT
public:
    typedef KanjiGridModel::size_type  size_type;

    KanjiListModel(QObject *parent = nullptr);
    virtual ~KanjiListModel();

    // Copies the list of kanji indexes to the model to be shown. This replaces the previous
    // list and removes any group that was set.
    void setList(const std::vector<ushort> &newlist);
    // Moves the list of kanji indexes to the model to be shown. This replaces the previous
    // list and removes any group that was set.
    void setList(std::vector<ushort> &&newlist);

    const std::vector<ushort>& getList() const;

    // Whether drag dropping kanji to this model can expand the stored list.
    bool acceptDrop();
    // Set whether drag dropping kanji to this model can expand the stored list. Only append
    // will be supported.
    void setAcceptDrop(bool acceptdrop);

    virtual size_type size() const override;

    // Returns the index of the kanji in the main kanji list at pos position.
    virtual int kanjiAt(int pos) const override;

    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int index) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int index) override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
private:
    std::vector<ushort> list;

    bool candrop;

    typedef KanjiGridModel base;
};

// Model of a kanji group's kanjis.
class KanjiGroupModel : public KanjiGridModel
{
    Q_OBJECT
public:
    typedef KanjiGridModel::size_type  size_type;

    KanjiGroupModel(QObject *parent = nullptr);
    virtual ~KanjiGroupModel();

    // Changes which kanji group is represented by this model.
    void setKanjiGroup(KanjiGroup *g);

    // The group that is represented by the model after a call to setGroup(). This can be null
    // if the kanji don't come from a group.
    virtual KanjiGroup* kanjiGroup() const override;

    virtual size_type size() const override;

    virtual int kanjiAt(int index) const override;

    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int index) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int index) override;

    virtual Qt::DropActions supportedDragActions() const override;
    virtual Qt::DropActions supportedDropActions(bool samesource, const QMimeData *mime) const override;
protected slots:
    void itemsInserted(GroupBase *parent, const smartvector<Interval> &intervals);
    void itemsRemoved(GroupBase *parent, const smartvector<Range> &ranges);
    void itemsMoved(GroupBase *parent, const smartvector<Range> &ranges, int pos);
private:
    KanjiGroup *group;
    //std::vector<ushort> list;

    typedef KanjiGridModel base;
};

enum class KanjiGridSortOrder { Jouyou, JLPT, Freq, Words, WordFreq, Stroke, Rad, Unicode, JISX0208, NoSort };

// Sorting model of a kanji grid model. Only used to provide data for the main kanji grid, and
// not used for groups. It doesn't support signal forwarding only relevant to group models.
// The sort model can't handle duplicate items.
class KanjiGridSortModel : public KanjiGridModel
{
    Q_OBJECT
public:
    typedef KanjiGridModel::size_type  size_type;

    KanjiGridSortModel(KanjiGridModel *basemodel, KanjiGridSortOrder order = KanjiGridSortOrder::NoSort, Dictionary *dict = nullptr, QObject *parent = nullptr);
    KanjiGridSortModel(KanjiGridModel *basemodel, const std::vector<ushort> &filter, KanjiGridSortOrder order = KanjiGridSortOrder::NoSort, Dictionary *dict = nullptr, QObject *parent = nullptr);
    KanjiGridSortModel(KanjiGridModel *basemodel, std::vector<ushort> &&filter, KanjiGridSortOrder order = KanjiGridSortOrder::NoSort, Dictionary *dict = nullptr, QObject *parent = nullptr);
    ~KanjiGridSortModel();

    void sort(KanjiGridSortOrder order, Dictionary *dict);

    //KanjiGridModel* baseModel();

    virtual size_type size() const override;
    virtual int kanjiAt(int pos) const override;

    virtual int statusCount() const override;
    virtual StatusTypes statusType(int statusindex) const override;
    virtual QString statusText(int statusindex, int kanjipos, int labelindex) const override;
    virtual int statusSize(int statusindex, int labelindex) const override;
    virtual bool statusAlignRight(int statusindex) const override;

    virtual QColor textColorAt(int pos) const override;
    // Returns the unsorted background color for kanji not found by the sorting criteria.
    virtual QColor backColorAt(int pos) const override;

    // Fills the passed (empty) vector with the kanji indexes handled by the model.
    //virtual void fillIndexes(std::vector<ushort> &kindexes) const override;

    // The kanji group of the base model.
    virtual KanjiGroup* kanjiGroup() const override;
private:
    KanjiGridModel *basemodel;
    KanjiGridSortOrder order;

    // Indexes in basemodel.
    std::vector<ushort> list;
    // Number of kanji that had data for the current sort order.
    int sortcount;

    typedef KanjiGridModel base;
};


#endif // ZKANJIGRIDMODEL_H
