/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef WORDSTUDYLISTFORM_H
#define WORDSTUDYLISTFORM_H

#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QHeaderView>

#include <QtCharts/QChart>
QT_CHARTS_USE_NAMESPACE

#include <map>
#include "dialogwindow.h"
#include "zabstractstatmodel.h"

namespace Ui {
    class WordStudyListForm;
}

enum class DeckItemViewModes : int;
class WordDeck;
class StudyListModel;
class DictionaryItemModel;
class Dictionary;
class QMenu;
struct WordStudyListFormData;
struct WordStudyListFormDataItems;
struct WordStudyListFormDataStats;
QT_CHARTS_BEGIN_NAMESPACE
    class QDateTimeAxis;
QT_CHARTS_END_NAMESPACE

class WordStudyTestsModel : public ZAbstractStatModel
{
    Q_OBJECT
public:
    WordStudyTestsModel(WordDeck *deck, QObject *parent = nullptr);
    virtual ~WordStudyTestsModel();

    virtual int count() const override;
    virtual int barWidth(ZStatView *view, int col) const override;
    virtual int maxValue() const override;
    virtual QString axisLabel(Qt::Orientation ori) const override;
    virtual QString barLabel(int ix) const override;

    virtual int valueCount() const override;
    virtual int value(int col, int valpos) const override;
    virtual QString tooltip(int col) const override;
private:
    WordDeck *deck;

    // For each displayed column, links to the given index in the study deck's day statistics.
    // If a value is -1, it marks a break between tests when the user didn't study. Other
    // values are indexes to study->dayStat().
    std::vector<int> stats;
    
    // The value at the top of the scale.
    int maxval;

    typedef ZAbstractStatModel  base;
};

class WordStudyLevelsModel : public ZAbstractStatModel
{
    Q_OBJECT
public:
    WordStudyLevelsModel(WordDeck *deck, QObject *parent = nullptr);
    virtual ~WordStudyLevelsModel();

    virtual ZStatType type() const override;

    virtual int count() const override;
    virtual int maxValue() const override;
    virtual QString axisLabel(Qt::Orientation ori) const override;
    virtual QString barLabel(int ix) const override;

    virtual int valueCount() const override;
    virtual int value(int col, int valpos) const override;
private:
    WordDeck *deck;

    // Number of items in each level from deck.
    std::vector<int> list;

    // The value at the top of the scale.
    int maxval;

    typedef ZAbstractStatModel  base;
};

struct WordStudySorting {
    int column;
    Qt::SortOrder order;
};

enum class DeckStudyPages { Items, Stats, None };
enum class DeckStatPages : int { Items, Forecast, Levels, Tests };
enum class DeckStatIntervals : int { All, Year, HalfYear, Month };

class QMenu;
class QAction;
enum class DictColumnTypes;
enum class WordParts : uchar;
class WordStudyListForm : public DialogWindow
{
    Q_OBJECT
public:
    // Shows and returns a WordStudyListForm displaying deck if one exists. Pass true in
    // createshow to make sure the form gets created if it doesn't exist. If a form is already
    // shown, it's only activated when createshow is true.
    static WordStudyListForm* Instance(WordDeck *deck, DeckStudyPages page, bool createshow);

    /* constructor is private */

    virtual ~WordStudyListForm();

    void saveState(WordStudyListFormData &data) const;
    void restoreFormState(const WordStudyListFormData &data);
    void restoreItemsState(const WordStudyListFormDataItems &data);
    void restoreStatsState(const WordStudyListFormDataStats &data);

    // Changes the active page shown in the window.
    void showPage(DeckStudyPages newpage, bool forceinit = false);

    // Switches to the queue tab.
    void showQueue();
    // Starts the long term study.
    void startTest();
    // Opens the add words to deck window with the word indexes in list.
    void addQuestions(const std::vector<int> &wordlist);
    // Shows a message box for comfirmation and removes the passed deck items from either the
    // queued items or studied items.
    void removeItems(const std::vector<int> &items, bool queued);
    // Shows a message box for comfirmation and moves the passed studied deck items back to
    // the queue.
    void requeueItems(const std::vector<int> &items);
    // Changes the priority of the passed deck items in the queued list to val.
    void changePriority(const std::vector<int> &items, uchar val);
    // Changes the shown main hint of the passed deck items in the queued or studied lists.
    void changeMainHint(const std::vector<int> &items, bool queued, WordParts val);
    // Increases the level and the interval of the item at deckitem index in the study deck.
    void increaseLevel(int deckitem);
    // Decreases the level and the interval of the item at deckitem index in the study deck.
    void decreaseLevel(int deckitem);
    // Removes the study data for the selected items, like they were not tested before.
    void resetItems(const std::vector<int> &items);
protected:
    virtual void closeEvent(QCloseEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual bool eventFilter(QObject *o, QEvent *e) override;
protected slots:
    void on_tabWidget_currentChanged(int index);
    void headerSortChanged(int index, Qt::SortOrder order);
    void rowSelectionChanged();
    void modeButtonClicked(bool checked);
    void partButtonClicked();
    void on_addButton_clicked();
    void on_delButton_clicked();
    void on_backButton_clicked();
    void showColumnContextMenu(const QPoint &p);
    void showContextMenu(QMenu *menu, QAction *insertpos, Dictionary *dict, DictColumnTypes coltype, QString selstr, const std::vector<int> &windexes, const std::vector<ushort> &kindexes);

    void on_int1Radio_toggled(bool checked);
    void on_int2Radio_toggled(bool checked);
    void on_int3Radio_toggled(bool checked);
    void on_int4Radio_toggled(bool checked);

    void on_itemsButton_clicked();
    void on_forecastButton_clicked();
    void on_levelsButton_clicked();
    void on_testsButton_clicked();

    void dictReset();
    void dictRemoved(int index, int orderindex, void *oldaddress);
private:
    // Saves the column sizes and visibility for the current mode from the dictionary view.
    void saveColumns();
    // Restores the column sizes and visibility for the current mode to the dictionary view.
    void restoreColumns();

    void showStat(DeckStatPages page);
    void updateStat();

    // When the X axis is a QDateTimeAxis, updates the axis' visible range.
    void normalizeXAxis(QDateTimeAxis *axis);

    // Changes the number of visible ticks on a QDateTimeAxis to make each tick fall on midnight.
    void autoXAxisTicks();

    WordStudyListForm(WordDeck *deck, DeckStudyPages page, QWidget *parent = nullptr);
    WordStudyListForm() = delete;

    // Instances of this form for each word deck, to avoid multiple forms for the same deck.
    static std::map<WordDeck*, WordStudyListForm*> insts;

    Ui::WordStudyListForm *ui;

    Dictionary *dict;
    WordDeck *deck;

    bool itemsinited;
    bool statsinited;

    DeckStatPages statpage;
    DeckStatIntervals itemsint;
    DeckStatIntervals forecastint;

    StudyListModel *model;

    // Saved sort column and order for the different tabs.
    WordStudySorting queuesort;
    WordStudySorting studiedsort;
    WordStudySorting testedsort;

    // Set during some operations when the sorting is changed programmatically, to ignore the
    // signal sent by the header, or while collecting statistics.
    bool ignoreop;

    std::vector<int> queuesizes;
    std::vector<int> studiedsizes;
    std::vector<int> testedsizes;

    std::vector<char> queuecols;
    std::vector<char> studiedcols;
    std::vector<char> testedcols;

    typedef DialogWindow base;
};

#endif // WORDSTUDYLISTFORM_H
