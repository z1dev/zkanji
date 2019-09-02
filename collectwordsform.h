/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef COLLECTWORDSFORM_H
#define COLLECTWORDSFORM_H

#include <vector>
#include "dialogwindow.h"
#include "smartvector.h"
#include "zdictionarymodel.h"
#include "zlistboxmodel.h"

namespace Ui {
    class CollectWordsForm;
}

class Dictionary;
class WordGroup;
class GroupCategoryBase;
class QPushButton;
class KanjiListModel;
struct Interval;

class CheckableKanjiReadingsListModel : public ZAbstractListBoxModel
{
    Q_OBJECT
signals:
    // Signalled when user the is checking or unchecking readings in the list. Checkstate
    // holds bits corresponding to the checkboxes.
    void readingsChecked(ushort checkstate);
public:
    CheckableKanjiReadingsListModel(QObject *parent = nullptr);
    virtual ~CheckableKanjiReadingsListModel();

    void setKanji(int kanjiindex, ushort checkstate = 0);
    int kanji();

    // Overrides
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index = QModelIndex()) const override;
private:
    int kindex;
    // Bits set for each reading to mark them checked or unchecked. When all readings are
    // checked this value is always 0xffff, when none are checked it is set to 0.
    ushort checks;

    typedef ZAbstractListBoxModel  base;
};

class CollectedWordListModel : public DictionaryWordListItemModel
{
    Q_OBJECT
signals:
    void checkStateChanged();
public:
    CollectedWordListModel(QObject *parent = nullptr);
    virtual ~CollectedWordListModel();

    void setWordList(Dictionary *d, std::vector<int> &&wordlist);
    bool hasCheckedWord() const;
    // Fills result with the dictionary word indexes of the checked words.
    void getCheckedWords(std::vector<int> &result) const;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index = QModelIndex()) const override;

    virtual void removeAt(int index) override;

    virtual int statusCount() const override;
    virtual StatusTypes statusType(int statusindex) const override;
    virtual QString statusText(int statusindex, int labelindex, int rowpos) const override;
    virtual int statusSize(int statusindex, int labelindex) const override;
    virtual bool statusAlignRight(int statusindex) const override;
protected:
    virtual void setColumnTexts() override;

    virtual void orderChanged(const std::vector<int> &ordering) override;
    virtual bool addNewEntry(int windex, int &position) override;
    virtual void entryAddedPosition(int pos) override;
protected slots:
    virtual void entryRemoved(int windex, int abcdeindex, int aiueoindex) override;
private:
    std::vector<char> checks;
    int checkedcnt;

    typedef DictionaryWordListItemModel base;
};

class CollectWordsForm : public DialogWindow
{
    Q_OBJECT
public:
    CollectWordsForm(QWidget *parent = nullptr);
    virtual ~CollectWordsForm();

    void exec(Dictionary *d, const std::vector<ushort> &kanjis);
protected:
    virtual void closeEvent(QCloseEvent *e) override;
protected slots:
    //void on_groupButton_clicked();
    //void groupDeleted(GroupCategoryBase *parent, int index, void *oldaddress);
    void kanjiInserted(const smartvector<Interval> &intervals);
    void readingsChecked(ushort checkstate);
    void positionBoxChanged();
    void kanjiSelectionChanged();
    void on_generateButton_clicked();
    //void on_jlptMinCBox_currentTextChanged();
    //void on_jlptMaxCBox_currentTextChanged();
    void on_addButton_clicked();
    void wordChecked();
    void on_dictCBox_currentIndexChanged(int index);

    void dictionaryReset();
    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
    void dictionaryReplaced(Dictionary *old, Dictionary *newdict, int index);
private:
    enum KanjiPlacement { Anywhere, Front, Middle, End, FrontEnd, FrontMiddle, MiddleEnd };

    Ui::CollectWordsForm *ui;

    Dictionary *dict;
    KanjiListModel *kmodel;
    CollectedWordListModel *wmodel;
    CheckableKanjiReadingsListModel *readingsmodel;
    // Bits representing the checked status of kanji readings.
    std::vector<ushort> readings;
    std::vector<KanjiPlacement> placement;

    WordGroup *dest;

    QPushButton *addButton;

    typedef DialogWindow    base;
};


void collectKanjiWords(Dictionary *dict, const std::vector<ushort> &kanjis);

#endif // COLLECTWORDSFORM_H
