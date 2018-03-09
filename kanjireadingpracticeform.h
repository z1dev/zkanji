/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef KANJIREADINGPRACTICEFORM_H
#define KANJIREADINGPRACTICEFORM_H

#include <QMainWindow>
#include "dialogwindow.h"
#include "zdictionarylistview.h"
#include "zdictionarymodel.h"

namespace Ui {
    class KanjiReadingPracticeForm;
}

class QPainter;
class KanjiReadingPracticeListDelegate : public DictionaryListDelegate
{
    Q_OBJECT
public:
    KanjiReadingPracticeListDelegate(ZDictionaryListView *parent = nullptr);

    // Paints the definition text of an entry. If selected is true, the text is painted with
    // textcolor, otherwise only the main definition is using it.
    virtual void paintKanji(QPainter *painter, const QModelIndex &index, int left, int top, int basey, QRect r) const override;
private:
    typedef DictionaryListDelegate  base;
};

enum class KanjiReadingRoles
{
    ReadingsList = (int)DictRowRoles::Last,

    Last,
};

class KanjiReadingPracticeListModel : public DictionaryWordListItemModel
{
    Q_OBJECT
public:
    KanjiReadingPracticeListModel(QObject *parent = nullptr);
    virtual ~KanjiReadingPracticeListModel();

    // Sets a list of words to be provided by this model.
    void setWordList(Dictionary *d, std::vector<int> &&wordlist, std::vector<std::vector<int>> &&readings);
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;
private:
    std::vector<std::vector<int>> readings;

    typedef DictionaryWordListItemModel base;
};

class WordDeck;
class DictionaryWordListItemModel;
class QLabel;
class KanjiReadingPracticeForm : public DialogWindow
{
    Q_OBJECT
public:
    KanjiReadingPracticeForm(WordDeck *deck, QWidget *parent = nullptr);
    ~KanjiReadingPracticeForm();

    void exec();
private slots:
    void answerChanged();
    void readingAccepted();
    void nextClicked();
private:
    // Updates the labes, buttons etc. to show the next reading.
    void initNextRound();

    // Updates the numbers in the status bar.
    void updateLabels();

    Ui::KanjiReadingPracticeForm *ui;
    WordDeck *deck;

    //QLabel *dueLabel;
    //QLabel *correctLabel;
    //QLabel *wrongLabel;

    int tries;

    int correct;
    int wrong;

    KanjiReadingPracticeListModel *model;

    typedef DialogWindow base;
};


#endif // KANJIREADINGPRACTICEFORM_H

