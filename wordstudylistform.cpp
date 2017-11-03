/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include <QPushButton>
#include <QSignalMapper>
#include <QMenu>
#include <QtEvents>
#include <QtCharts/QBarSeries> 
#include <QtCharts/QStackedBarSeries> 
#include <QtCharts/QBarSet> 
#include <QtCharts/QBarCategoryAxis> 
#include <QtCharts/QAreaSeries> 
#include <QtCharts/QLineSeries> 
#include <QtCharts/QValueAxis> 
#include <QtCharts/QDateTimeAxis> 
#include <map>

#include "wordstudylistform.h"
#include "ui_wordstudylistform.h"
#include "zstudylistmodel.h"
#include "dictionarywidget.h"
#include "globalui.h"
#include "worddeck.h"
#include "wordtodeckform.h"
#include "zlistview.h"
#include "zdictionarylistview.h"
#include "wordstudyform.h"
#include "words.h"
#include "zui.h"
#include "grouppickerform.h"
#include "groups.h"
#include "formstate.h"
#include "dialogs.h"


//-------------------------------------------------------------

// TODO: column sizes not saved in list form.


// Single instance of WordDeckForm 
std::map<WordDeck*, WordStudyListForm*> WordStudyListForm::insts;

WordStudyListForm* WordStudyListForm::Instance(WordDeck *deck, DeckStudyPages page, bool createshow)
{
    auto it = insts.find(deck);

    WordStudyListForm *inst = nullptr;
    if (it != insts.end())
        inst = it->second;

    if (inst == nullptr && createshow)
    {
        inst = insts[deck] = new WordStudyListForm(deck, page, gUI->activeMainForm());
        //i->connect(worddeckform, &QMainWindow::destroyed, i, &GlobalUI::formDestroyed);

        inst->show();
    }
    else if (page != DeckStudyPages::None && inst != nullptr)
        inst->showPage(page);

    if (createshow)
    {
        inst->raise();
        inst->activateWindow();
    }


    return inst;
}

WordStudyListForm::WordStudyListForm(WordDeck *deck, DeckStudyPages page, QWidget *parent) : base(parent), ui(new Ui::WordStudyListForm), dict(deck->dictionary()), deck(deck), itemsinited(false), statsinited(false), ignoresort(false)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    ui->dictWidget->setSaveColumnData(false);

    QPushButton *startButton = ui->buttonBox->addButton(tr("Start the test"), QDialogButtonBox::AcceptRole);
    QPushButton *closeButton = ui->buttonBox->button(QDialogButtonBox::Close);

    connect(startButton, &QPushButton::clicked, this, &WordStudyListForm::startTest);
    connect(closeButton, &QPushButton::clicked, this, &WordStudyListForm::close);

    queuesort = { 0, Qt::AscendingOrder };
    studiedsort = { 0, Qt::AscendingOrder };
    testedsort = { 0, Qt::AscendingOrder };

    ui->dictWidget->addFrontWidget(ui->buttonsWidget);
    ui->dictWidget->setExamplesVisible(false);
    ui->dictWidget->setMultilineVisible(false);
    ui->dictWidget->setAcceptWordDrops(true);
    ui->dictWidget->setSortIndicatorVisible(true);
    ui->dictWidget->setWordFiltersVisible(false);
    ui->dictWidget->setSelectionType(ListSelectionType::Extended);
    ui->dictWidget->setInflButtonVisible(false);
    ui->dictWidget->setStudyDefinitionUsed(true);
    ui->dictWidget->setDictionary(dict);

    StudyListModel::defaultColumnWidths(DeckViewModes::Queued, queuesizes);
    StudyListModel::defaultColumnWidths(DeckViewModes::Studied, studiedsizes);
    StudyListModel::defaultColumnWidths(DeckViewModes::Tested, testedsizes);

    queuecols.assign(queuesizes.size() - 2, 1);
    studiedcols.assign(studiedsizes.size() - 2, 1);
    testedcols.assign(testedsizes.size() - 2, 1);

    ui->dictWidget->view()->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->dictWidget->view()->horizontalHeader(), &QWidget::customContextMenuRequested, this, &WordStudyListForm::showColumnContextMenu);
    connect(ui->dictWidget, &DictionaryWidget::customizeContextMenu, this, &WordStudyListForm::showContextMenu);

    connect(ui->queuedButton, &QToolButton::toggled, this, &WordStudyListForm::modeButtonClicked);
    connect(ui->studiedButton, &QToolButton::toggled, this, &WordStudyListForm::modeButtonClicked);
    connect(ui->testedButton, &QToolButton::toggled, this, &WordStudyListForm::modeButtonClicked);

    connect(ui->wButton, &QToolButton::toggled, this, &WordStudyListForm::partButtonClicked);
    connect(ui->kButton, &QToolButton::toggled, this, &WordStudyListForm::partButtonClicked);
    connect(ui->dButton, &QToolButton::toggled, this, &WordStudyListForm::partButtonClicked);

    connect(deck->owner(), &WordDeckList::deckToBeRemoved, this, &WordStudyListForm::closeCancel);

    if (!FormStates::emptyState(FormStates::wordstudylist))
        resize(FormStates::wordstudylist.siz);

    ui->tabWidget->setCurrentIndex(-1);

    if (page == DeckStudyPages::None)
        page = DeckStudyPages::Items;
    showPage(page, true);
}

WordStudyListForm::~WordStudyListForm()
{
    delete ui;
    insts.erase(insts.find(deck));
}

void WordStudyListForm::saveState(WordStudyListFormData &data) const
{
    data.siz = isMaximized() ? normalGeometry().size() : rect().size();

    if (itemsinited)
    {
        data.showkanji = ui->wButton->isChecked();
        data.showkana = ui->kButton->isChecked();
        data.showdef = ui->dButton->isChecked();

        data.mode = ui->queuedButton->isChecked() ? DeckViewModes::Queued : ui->studiedButton->isChecked() ? DeckViewModes::Studied : DeckViewModes::Tested;

        ui->dictWidget->saveState(data.dict);

        data.queuesort.column = queuesort.column;
        data.queuesort.order = queuesort.order;
        data.studysort.column = studiedsort.column;
        data.studysort.order = studiedsort.order;
        data.testedsort.column = testedsort.column;
        data.testedsort.order = testedsort.order;

        data.queuesizes = queuesizes;
        data.studysizes = studiedsizes;
        data.testedsizes = testedsizes;
        data.queuecols = queuecols;
        data.studycols = studiedcols;
        data.testedcols = testedcols;
    }
}

void WordStudyListForm::restoreItemsState(const WordStudyListFormData &data)
{
    ignoresort = true;

    ui->wButton->setChecked(data.showkanji);
    ui->kButton->setChecked(data.showkana);
    ui->dButton->setChecked(data.showdef);

    if (data.mode == DeckViewModes::Queued)
        ui->queuedButton->setChecked(true);
    else if (data.mode == DeckViewModes::Studied)
        ui->studiedButton->setChecked(true);
    else if (data.mode == DeckViewModes::Tested)
        ui->testedButton->setChecked(true);

    ui->dictWidget->restoreState(data.dict);

    queuesort.column = data.queuesort.column;
    queuesort.order = data.queuesort.order;
    studiedsort.column = data.studysort.column;
    studiedsort.order = data.studysort.order;
    testedsort.column = data.testedsort.column;
    testedsort.order = data.testedsort.order;

    queuesizes = data.queuesizes;
    studiedsizes = data.studysizes;
    testedsizes = data.testedsizes;
    queuecols = data.queuecols;
    studiedcols = data.studycols;
    testedcols = data.testedcols;

    ignoresort = false;

    model->setViewMode(data.mode);

    model->setShownParts(ui->wButton->isChecked(), ui->kButton->isChecked(), ui->dButton->isChecked());

    auto &s = data.mode == DeckViewModes::Queued ? queuesort : data.mode == DeckViewModes::Studied ? studiedsort : testedsort;
    if (s.column == -1)
        s.column = 0;

    // Without the forced update (happening in the sort) the widget won't have columns to
    // restore below.
    ui->dictWidget->forceUpdate();

    ui->dictWidget->setSortIndicator(s.column, s.order);
    ui->dictWidget->sortByIndicator();

    restoreColumns();
}

void WordStudyListForm::showPage(DeckStudyPages newpage, bool forceinit)
{
    switch (newpage)
    {
    case DeckStudyPages::Items:
        if (ui->tabWidget->currentIndex() != 0)
            ui->tabWidget->setCurrentIndex(0);
        else if (forceinit)
            on_tabWidget_currentChanged(0);
        break;
    case DeckStudyPages::Stats:
        if (ui->tabWidget->currentIndex() != 1)
            ui->tabWidget->setCurrentIndex(1);
        else if (forceinit)
            on_tabWidget_currentChanged(1);
        break;
    default:
        return;
    }
}

void WordStudyListForm::showQueue()
{
    if (!ui->queuedButton->isChecked())
        ui->queuedButton->toggle();
}

void WordStudyListForm::startTest()
{
    // WARNING: if later this close() is removed, the model of the dictionary table must be
    // destroyed and recreated when the window is shown again. The test changes the deck items
    // and that doesn't show up in the model.
    close();
    WordStudyForm::studyDeck(deck);
}

void WordStudyListForm::addQuestions(const std::vector<int> &wordlist)
{
    addWordsToDeck(dict, deck, wordlist, this);
}

void WordStudyListForm::removeItems(const std::vector<int> &items, bool queued)
{
    QString msg;
    if (queued)
        msg = tr("Do you want to remove the selected items?\n\nThe items will be removed from the queue, but their custom definitions shared between tests won't change.");
    else
        msg = tr("Do you want to remove the selected items?\n\nThe study data for the selected items will be reset and the items will be removed from this list. The custom definitions shared between tests won't change.\n\nThis can't be undone.");
    if (QMessageBox::question(this, "zkanji", msg, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    if (queued)
        deck->removeQueuedItems(items);
    else
        deck->removeStudiedItems(items);
}

void WordStudyListForm::requeueItems(const std::vector<int> &items)
{
    if (QMessageBox::question(this, "zkanji", tr("Do you want to move the selected items back to the queue?\n\nThe study data of the items will be reset, the items will be removed from this list and moved to the end of the queue. The custom definitions shared between tests won't change.\n\nThis can't be undone."), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    deck->requeueStudiedItems(items);
}

void WordStudyListForm::changePriority(const std::vector<int> &items, uchar val)
{
    deck->setQueuedPriority(items, val);
}

void WordStudyListForm::changeMainHint(const std::vector<int> &items, bool queued, WordParts val)
{
    deck->setItemHints(items, queued, val);
}

void WordStudyListForm::increaseLevel(int deckitem)
{
    deck->increaseSpacingLevel(deckitem);
}

void WordStudyListForm::decreaseLevel(int deckitem)
{
    deck->decreaseSpacingLevel(deckitem);
}

void WordStudyListForm::resetItems(const std::vector<int> &items)
{
    if (QMessageBox::question(this, "zkanji", tr("All study data, including past statistics, item level and difficulty will be reset for the selected items. The items will be shown like they were new the next time the test starts. Consider moving the items back to the queue instead.\n\nDo you want to reset the study data?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
        deck->resetCardStudyData(items);
}

void WordStudyListForm::closeEvent(QCloseEvent *e)
{
    if (itemsinited)
        saveColumns();

    saveState(FormStates::wordstudylist);
    base::closeEvent(e);
}

void WordStudyListForm::keyPressEvent(QKeyEvent *e)
{
    if (itemsinited && ui->tabWidget->currentIndex() == 0)
    {
        bool queue = model->viewMode() == DeckViewModes::Queued;
        if (queue && e->modifiers().testFlag(Qt::ControlModifier) && e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9)
        {
            std::vector<int> rowlist;
            ui->dictWidget->selectedRows(rowlist);
            if (rowlist.empty())
            {
                base::keyPressEvent(e);
                return;
            }
            for (int &ix : rowlist)
                ix = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt();
            deck->setQueuedPriority(rowlist, e->key() - Qt::Key_1 + 1);
        }
    }

    base::keyPressEvent(e);
}

bool WordStudyListForm::eventFilter(QObject *o, QEvent *e)
{
    if (o == ui->statView && e->type() == QEvent::Resize && ui->statView->chart() != nullptr && ui->statView->chart()->axisY() != nullptr)
    {
        ((QValueAxis*)ui->statView->chart()->axisY())->setTickCount(std::max(2, ui->statView->height() / 70));
        if (ui->itemsButton->isChecked())
            ((QDateTimeAxis*)ui->statView->chart()->axisX())->setTickCount(std::max(2, ui->statView->width() / 70));
    }
    return base::eventFilter(o, e);
}

void WordStudyListForm::on_tabWidget_currentChanged(int index)
{
    if (index == -1 || (index == 0 && itemsinited) || (index == 1 && statsinited))
        return;

    if (index == 0)
    {
        itemsinited = true;

        model = new StudyListModel(deck, this);
        ui->dictWidget->setModel(model);
        ui->dictWidget->setSortFunction([this](DictionaryItemModel *d, int c, int a, int b) { return model->sortOrder(c, a, b); });

        connect(ui->dictWidget, &DictionaryWidget::rowSelectionChanged, this, &WordStudyListForm::rowSelectionChanged);
        connect(ui->dictWidget, &DictionaryWidget::sortIndicatorChanged, this, &WordStudyListForm::headerSortChanged);

        auto &s = ui->queuedButton->isChecked() ? queuesort : ui->studiedButton->isChecked() ? studiedsort : testedsort;
        ui->dictWidget->setSortIndicator(s.column, s.order);
        ui->dictWidget->sortByIndicator();

        connect(dict, &Dictionary::dictionaryReset, this, &WordStudyListForm::dictReset);
        connect(gUI, &GlobalUI::dictionaryRemoved, this, &WordStudyListForm::dictRemoved);

        restoreItemsState(FormStates::wordstudylist);
    }
    else
    {
        statsinited = true;

        ui->dueLabel->setText(QString::number(deck->dueSize()));
        ui->queueLabel->setText(QString::number(deck->queueSize()));

        ui->studiedLabel->setText(QString::number(deck->studySize()));

        // Calculate unique words and kanji count by checking every word data.
        int wordcnt = 0;
        QSet<ushort> kanjis;
        for (int ix = 0, siz = deck->wordDataSize(); ix != siz; ++ix)
        {
            if (!deck->wordDataStudied(ix))
                continue;
            ++wordcnt;
            WordEntry *e = dict->wordEntry(deck->wordData(ix)->index);
            for (int iy = 0, siy = e->kanji.size(); iy != siy; ++iy)
                if (KANJI(e->kanji[iy].unicode()))
                    kanjis.insert(e->kanji[iy].unicode());
        }

        ui->wordsLabel->setText(QString::number(wordcnt));
        ui->kanjiLabel->setText(QString::number(kanjis.size()));

        ui->firstLabel->setText(DateTimeFunctions::formatPastDay(deck->firstDay()));
        ui->lastLabel->setText(DateTimeFunctions::formatPastDay(deck->lastDay()));
        ui->testDaysLabel->setText(QString::number(deck->testDayCount()));
        ui->skippedDaysLabel->setText(QString::number(deck->skippedDayCount()));

        ui->studyTimeLabel->setText(DateTimeFunctions::formatLength((int)((deck->totalTime() + 5) / 10)));
        ui->studyTimeAvgLabel->setText(DateTimeFunctions::formatLength((int)((deck->studyAverage() + 5) / 10)));
        ui->answerTimeAvgLabel->setText(DateTimeFunctions::formatLength((int)((deck->answerAverage() + 5) / 10)));

        showStat(DeckStatPages::Items);

        ui->statView->installEventFilter(this);

        QSignalMapper *map = new QSignalMapper(this);
        map->setMapping(ui->itemsButton, (int)DeckStatPages::Items);
        map->setMapping(ui->testsButton, (int)DeckStatPages::Tests);
        map->setMapping(ui->decksButton, (int)DeckStatPages::Decks);
        connect(ui->itemsButton, &QToolButton::clicked, map, (void (QSignalMapper::*)())&QSignalMapper::map);
        connect(ui->testsButton, &QToolButton::clicked, map, (void (QSignalMapper::*)())&QSignalMapper::map);
        connect(ui->decksButton, &QToolButton::clicked, map, (void (QSignalMapper::*)())&QSignalMapper::map);
        connect(map, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), [this](int val) { showStat((DeckStatPages)val);  });
    }
}

void WordStudyListForm::headerSortChanged(int column, Qt::SortOrder order)
{
    if (ignoresort)
        return;

    auto &s = model->viewMode() == DeckViewModes::Queued ? queuesort : model->viewMode() == DeckViewModes::Studied ? studiedsort : testedsort;
    if (order == s.order && column == s.column)
        return;

    if (column == model->columnCount() - 1)
    {
        // The last column shows the definitions of words and cannot be used for sorting. The
        // original column should be restored.
        ui->dictWidget->setSortIndicator(s.column, s.order);
        return;
    }

    s.column = column;
    s.order = order;

    ui->dictWidget->sortByIndicator();
}

void WordStudyListForm::rowSelectionChanged()
{
    std::vector<int> indexes;
    ui->dictWidget->selectedIndexes(indexes);
    ui->defEditor->setWords(dict, indexes);

    ui->addButton->setEnabled(!indexes.empty());
    ui->delButton->setEnabled(!indexes.empty());
    ui->backButton->setEnabled((ui->studiedButton->isChecked() || ui->testedButton->isChecked()) && !indexes.empty());
}

void WordStudyListForm::modeButtonClicked(bool checked)
{
    if (!checked)
        return;

    saveColumns();

    DeckViewModes mode = ui->queuedButton->isChecked() ? DeckViewModes::Queued : ui->studiedButton->isChecked() ? DeckViewModes::Studied : DeckViewModes::Tested;

    auto &s = mode == DeckViewModes::Queued ? queuesort : mode == DeckViewModes::Studied ? studiedsort : testedsort;
    model->setViewMode(mode);

    restoreColumns();

    ui->dictWidget->setSortIndicator(s.column, s.order);
    ui->dictWidget->sortByIndicator();
}

void WordStudyListForm::partButtonClicked()
{
    model->setShownParts(ui->wButton->isChecked(), ui->kButton->isChecked(), ui->dButton->isChecked());
}

void WordStudyListForm::on_addButton_clicked()
{
    std::vector<int> wordlist;
    ui->dictWidget->selectedIndexes(wordlist);
    addQuestions(wordlist);
}

void WordStudyListForm::on_delButton_clicked()
{
    std::vector<int> rowlist;
    ui->dictWidget->selectedRows(rowlist);
    for (int &ix : rowlist)
        ix = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt();

    removeItems(rowlist, ui->queuedButton->isChecked());
}

void WordStudyListForm::on_backButton_clicked()
{
    if (ui->queuedButton->isChecked())
        return;

    std::vector<int> rowlist;
    ui->dictWidget->selectedRows(rowlist);
    for (int &ix : rowlist)
        ix = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt();

    requeueItems(rowlist);
}

void WordStudyListForm::showColumnContextMenu(const QPoint &p)
{
    std::vector<char> &vec = model->viewMode() == DeckViewModes::Queued ? queuecols : model->viewMode() == DeckViewModes::Studied ? studiedcols : testedcols;
    QMenu m;

    for (int ix = 0, siz = vec.size(); ix != siz; ++ix)
    {
        QAction *a = m.addAction(ui->dictWidget->view()->model()->headerData(ix, Qt::Horizontal, Qt::DisplayRole).toString());
        a->setCheckable(true);
        a->setChecked(vec[ix]);
    }

    QAction *a = m.exec(((QWidget*)sender())->mapToGlobal(p));
    if (a == nullptr)
        return;
    int aix = m.actions().indexOf(a);
    
    std::vector<int> &sizvec = model->viewMode() == DeckViewModes::Queued ? queuesizes : model->viewMode() == DeckViewModes::Studied ? studiedsizes : testedsizes;
    if (vec[aix])
        sizvec[aix] = ui->dictWidget->view()->columnWidth(aix);

    vec[aix] = !vec[aix];

    ui->dictWidget->view()->setColumnHidden(aix, !vec[aix]);

    if (vec[aix])
        ui->dictWidget->view()->setColumnWidth(aix, sizvec[aix]);
}

void WordStudyListForm::showContextMenu(QMenu *menu, QAction *insertpos, Dictionary *dict, DictColumnTypes coltype, QString selstr, const std::vector<int> &windexes, const std::vector<ushort> &kindexes)
{
    if (ui->queuedButton->isChecked())
    {
        QAction *actions[9];

        QMenu *m = new QMenu(tr("Main hint"));
        menu->insertMenu(insertpos, m);

        std::vector<int> selrows;
        std::vector<int> rowlist;
        ui->dictWidget->selectedRows(selrows);

        // Used in checking if all items have the same hint type. In case this is false, it
        // will be set to something else than the valid values. (=anything above 3)
        uchar mainhint = 255;
        // Lists all hint types that should be shown in the context menu.
        uchar shownhints = (uchar)WordPartBits::Default;
        for (int &ix : selrows)
        {
            uchar question = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::ItemQuestion).toInt();
            uchar hint = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::ItemHint).toInt();
            if (mainhint == 255)
                mainhint = hint;
            else if (mainhint != hint) // No match. Set mainhint to invalid.
                mainhint = 254;

            shownhints |= (int)WordPartBits::AllParts - question;

            rowlist.push_back(ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt());
        }

        QSignalMapper *map = new QSignalMapper(menu);
        connect(menu, &QMenu::aboutToHide, map, &QObject::deleteLater);
        QString strhint[4] = { tr("Default"), tr("Written"), tr("Kana"), tr("Definition") };
        for (int ix = 0; ix != 4; ++ix)
        {
            if (ix != 0 && ((shownhints & (1 << (ix - 1))) == 0))
                continue;

            actions[ix] = m->addAction(strhint[ix]);
            // The Default value in mainhint is 3 but we want to place default at the front.
            // Because of that ix must be converted to match mainhint.
            if ((mainhint == 3 && ix == 0) || (ix != 0 && ix - 1 == mainhint))
            {
                actions[ix]->setCheckable(true);
                actions[ix]->setChecked(true);
            }

            connect(actions[ix], &QAction::triggered, map, (void (QSignalMapper::*)())&QSignalMapper::map);
            map->setMapping(actions[ix], ix == 0 ? 3 : ix - 1);
        }

        connect(map, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, [this, rowlist](int val) {
            changeMainHint(rowlist, true, (WordParts)val);
        });
        m->setEnabled(ui->dictWidget->hasSelection());

        m = new QMenu(tr("Priority"));
        menu->insertMenu(insertpos, m)->setEnabled(ui->dictWidget->hasSelection());
        menu->insertSeparator(insertpos);

        map = new QSignalMapper(menu);
        connect(menu, &QMenu::aboutToHide, map, &QObject::deleteLater);

        connect(map, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, [this, rowlist](int val) {
            changePriority(rowlist, val);
        });

        QString strpriority[9] = { tr("Highest"), tr("Very high"), tr("High"), tr("Higher"), tr("Normal"), tr("Lower"), tr("Low"), tr("Very low"), tr("Lowest") };
        for (int ix = 0; ix != 9; ++ix)
        {
            actions[ix] = m->addAction(strpriority[ix]);
            actions[ix]->setShortcut(QKeySequence(tr("Ctrl+%1").arg(9 - ix)));
            connect(actions[ix], &QAction::triggered, map, (void (QSignalMapper::*)())&QSignalMapper::map);
            map->setMapping(actions[ix], 9 - ix);
        }

        QAction *a = new QAction(tr("Remove from deck..."), this);
        menu->insertAction(insertpos, a);
        connect(a, &QAction::triggered, this, [this, rowlist]() {
            removeItems(rowlist, true);
        });
        a->setEnabled(ui->dictWidget->hasSelection());

        menu->insertSeparator(insertpos);

        return;
    }

    QMenu *m = new QMenu(tr("Main hint"));
    menu->insertMenu(insertpos, m);

    std::vector<int> selrows;
    std::vector<int> rowlist;
    ui->dictWidget->selectedRows(selrows);

    // Used in checking if all items have the same hint type. In case this is false, it
    // will be set to something else than the valid values. (=anything above 3)
    uchar mainhint = 255;
    // Lists all hint types that should be shown in the context menu.
    uchar shownhints = (uchar)WordPartBits::Default;
    for (int &ix : selrows)
    {
        uchar question = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::ItemQuestion).toInt();
        uchar hint = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::ItemHint).toInt();
        if (mainhint == 255)
            mainhint = hint;
        else if (mainhint != hint) // No match. Set mainhint to invalid.
            mainhint = 254;

        shownhints |= (int)WordPartBits::AllParts - question;

        rowlist.push_back(ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt());
    }

    QAction *actions[4];

    QSignalMapper *map = new QSignalMapper(menu);
    connect(menu, &QMenu::aboutToHide, map, &QObject::deleteLater);
    QString strhint[4] = { tr("Default"), tr("Written"), tr("Kana"), tr("Definition") };
    for (int ix = 0; ix != 4; ++ix)
    {
        if (ix != 0 && ((shownhints & (1 << (ix - 1))) == 0))
            continue;

        actions[ix] = m->addAction(strhint[ix]);
        // The Default value in mainhint is 3 but we want to place default at the front.
        // Because of that ix must be converted to match mainhint.
        if ((mainhint == 3 && ix == 0) || (ix != 0 && ix - 1 == mainhint))
        {
            actions[ix]->setCheckable(true);
            actions[ix]->setChecked(true);
        }

        connect(actions[ix], &QAction::triggered, map, (void (QSignalMapper::*)())&QSignalMapper::map);
        map->setMapping(actions[ix], ix == 0 ? 3 : ix - 1);
    }

    connect(map, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, [this, rowlist](int val) {
        changeMainHint(rowlist, false, (WordParts)val);
    });
    m->setEnabled(ui->dictWidget->hasSelection());

    //menu->insertSeparator(insertpos);

    m = new QMenu(tr("Study options"));
    menu->insertMenu(insertpos, m);

    QAction *a = m->addAction(tr("Increase level"));
    m->setEnabled(rowlist.size() == 1);
    if (rowlist.size() == 1)
    {
        int dix = rowlist.front();
        connect(a, &QAction::triggered, this, [this, dix]() { increaseLevel(dix); });
    }

    a = m->addAction(tr("Decrease level"));
    m->setEnabled(rowlist.size() == 1);
    if (rowlist.size() == 1)
    {
        int dix = rowlist.front();
        connect(a, &QAction::triggered, this, [this, dix]() { decreaseLevel(dix); });
    }

    m->addSeparator();
    a = m->addAction(tr("Reset study data"));
    connect(a, &QAction::triggered, this, [this, rowlist]() { resetItems(rowlist); });
    a->setEnabled(ui->dictWidget->hasSelection());

    m->setEnabled(ui->dictWidget->hasSelection());

    menu->insertSeparator(insertpos);

    a = new QAction(tr("Remove from deck..."), this);
    menu->insertAction(insertpos, a);
    connect(a, &QAction::triggered, this, [this, rowlist]() {
        removeItems(rowlist, false);
    });
    a->setEnabled(ui->dictWidget->hasSelection());

    a = new QAction(tr("Move back to queue..."), this);
    menu->insertAction(insertpos, a);
    connect(a, &QAction::triggered, this, [this, rowlist]() {
        requeueItems(rowlist);
    });
    a->setEnabled(ui->dictWidget->hasSelection());

    menu->insertSeparator(insertpos);
}

//void WordStudyListForm::dictContextMenu(const QPoint &pos, const QPoint &globalpos, int selindex)
//{
//    int ix = ui->dictWidget->view()->rowAt(pos.y());
//    if (ix < 0)
//        return;
//
//    bool queue = model->viewMode() == DeckViewModes::Queued;
//    //WordDeckItem *item = nullptr;
//
//    QMenu m;
//    QAction *a = nullptr;
//    QMenu *sub = nullptr;
//
//    std::vector<int> rowlist;
//    ui->dictWidget->selectedRows(rowlist);
//    if (rowlist.empty())
//        rowlist.push_back(ix);
//    for (int ix = 0, siz = rowlist.size(); ix != siz; ++ix)
//        rowlist[ix] = ui->dictWidget->view()->model()->data(ui->dictWidget->view()->model()->index(rowlist[ix], 0), (int)DeckRowRoles::DeckIndex).toInt();
//
//    // Single item clicked. Even if selection is 0, the mouse was over a row.
//    if (rowlist.size() < 2)
//    {
//        if (rowlist.size() == 1)
//            ix = rowlist.front();
//        else
//            ix = ui->dictWidget->view()->model()->rowData(ix, (int)DeckRowRoles::DeckIndex).toInt();
//        //item = queue ? (WordDeckItem*)deck->queuedItems(ix) : (WordDeckItem*)deck->studiedItems(ix);
//    }
//
//    if (queue)
//    {
//        sub = new QMenu(tr("Priority"), &m);
//
//        QActionGroup *prioritygroup = new QActionGroup(sub);
//        prioritygroup->addAction(sub->addAction(QString(tr("Highest") + "\tShift+9")));
//        prioritygroup->addAction(sub->addAction(QString(tr("Very high") + "\tShift+8")));
//        prioritygroup->addAction(sub->addAction(QString(tr("High") + "\tShift+7")));
//        prioritygroup->addAction(sub->addAction(QString(tr("Higher") + "\tShift+6")));
//        prioritygroup->addAction(sub->addAction(QString(tr("Medium") + "\tShift+5")));
//        prioritygroup->addAction(sub->addAction(QString(tr("Lower") + "\tShift+4")));
//        prioritygroup->addAction(sub->addAction(QString(tr("Low") + "\tShift+3")));
//        prioritygroup->addAction(sub->addAction(QString(tr("Very low") + "\tShift+2")));
//        prioritygroup->addAction(sub->addAction(QString(tr("Lowest") + "\tShift+1")));
//        prioritygroup->setExclusive(false);
//
//        QSet<uchar> priset;
//        deck->queuedPriorities(rowlist, priset);
//
//        for (uchar ix = 0; ix != 9; ++ix)
//        {
//            a = prioritygroup->actions().at(ix);
//            if (priset.contains(9 - ix))
//            {
//                a->setCheckable(true);
//                a->setChecked(true);
//            }
//        }
//
//        connect(prioritygroup, &QActionGroup::triggered, this, [this, &rowlist](QAction *action)
//        {
//            int priority = action->actionGroup()->actions().indexOf(action);
//            deck->setQueuedPriority(rowlist, 9 - priority);
//        });
//
//        m.addMenu(sub);
//        m.addSeparator();
//    }
//
//
//    uchar hintset;
//    uchar selset;
//    deck->itemHints(rowlist, queue, hintset, selset);
//
//    sub = new QMenu(tr("Primary hint"));
//    QActionGroup *hintgroup = new QActionGroup(sub);
//    QAction *defa = nullptr;
//    hintgroup->addAction(defa = a = sub->addAction(tr("Default")));
//    hintgroup->setExclusive(false);
//    a->setCheckable(true);
//    if (selset & (int)WordPartBits::Default)
//        a->setChecked(a);
//    QAction *kanjia = nullptr;
//    if (hintset & (int)WordPartBits::Kanji)
//    {
//        hintgroup->addAction(kanjia = a = sub->addAction(tr("Written")));
//        a->setCheckable(true);
//        if (selset & (int)WordPartBits::Kanji)
//            a->setChecked(a);
//    }
//    QAction *kanaa = nullptr;
//    if (hintset & (int)WordPartBits::Kana)
//    {
//        hintgroup->addAction(kanaa = a = sub->addAction(tr("Kana")));
//        a->setCheckable(true);
//        if (selset & (int)WordPartBits::Kana)
//            a->setChecked(a);
//    }
//    QAction *defna = nullptr;
//    if (hintset & (int)WordPartBits::Definition)
//    {
//        hintgroup->addAction(defna = a = sub->addAction(tr("Definition")));
//        a->setCheckable(true);
//        if (selset & (int)WordPartBits::Definition)
//            a->setChecked(a);
//    }
//
//    connect(hintgroup, &QActionGroup::triggered, this, [this, &rowlist, queue, defa, kanjia, kanaa, defna](QAction *action)
//    {
//        deck->setItemHints(rowlist, queue, action == kanjia ? WordParts::Kanji : action == kanaa ? WordParts::Kana : action == defna ? WordParts::Definition : WordParts::Default);
//    });
//
//    m.addMenu(sub);
//
//    a = m.addAction(tr("Add question..."));
//    connect(a, &QAction::triggered, this, [this, &rowlist, queue](bool checked) {
//        std::vector<int> wordlist;
//        deck->itemsToWords(rowlist, queue, wordlist);
//        addQuestions(wordlist);
//    });
//    m.addSeparator();
//
//    StudyDeck *study = deck->getStudyDeck();
//
//    a = m.addAction(tr("Remove..."));
//    connect(a, &QAction::triggered, this, [this, &rowlist, queue](bool checked){
//        removeItems(rowlist, queue);
//    });
//
//    if (!queue)
//    {
//        a = m.addAction(tr("Back to queue..."));
//        connect(a, &QAction::triggered, this, [this, &rowlist](bool checked){
//            requeueItems(rowlist);
//        });
//
//        m.addSeparator();
//
//        sub = new QMenu(tr("Study options"));
//        if (rowlist.size() < 2)
//        {
//            a = sub->addAction(tr("Level+") + QString(" (%1)").arg(formatSpacing(study->increasedSpacing(deck->studiedItems(ix)->cardid))));
//            connect(a, &QAction::triggered, this, [this, &rowlist, ix, study](bool checked) {
//                deck->increaseSpacingLevel(ix);
//            });
//            a = sub->addAction(tr("Level-") + QString(" (%1)").arg(formatSpacing(study->decreasedSpacing(deck->studiedItems(ix)->cardid))));
//            connect(a, &QAction::triggered, this, [this, &rowlist, ix, study](bool checked) {
//                deck->decreaseSpacingLevel(ix);
//            });
//        }
//        else
//        {
//            a = sub->addAction(tr("Level+"));
//            a->setEnabled(false);
//            a = sub->addAction(tr("Level-"));
//            a->setEnabled(false);
//        }
//        sub->addSeparator();
//
//        a = sub->addAction(tr("Reset study data"));
//        connect(a, &QAction::triggered, this, [this, &rowlist/*, item*/, study](bool checked) {
//            if (QMessageBox::question(this, "zkanji", tr("All study data, including past statistics, item level and difficulty will be reset for the selected items. The items will be shown like they were new the next time the test starts. Consider moving the items back to the queue instead.\n\nDo you want to reset the study data?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
//            {
//                deck->resetCardStudyData(rowlist);
//            }
//        });
//
//        m.addMenu(sub);
//        m.addSeparator();
//    }
//    a = m.addAction(tr("Add to group..."));
//    connect(a, &QAction::triggered, this, [this, &rowlist, queue](bool checked) {
//        std::vector<int> indexes;
//        QSet<int> added;
//        for (int ix : rowlist)
//        {
//            WordDeckItem *item = queue ? (WordDeckItem*)deck->queuedItems(ix) : (WordDeckItem*)deck->studiedItems(ix);
//            if (added.contains(item->data->index))
//                continue;
//            indexes.push_back(item->data->index);
//            added << item->data->index;
//        }
//
//        WordGroup *group = (WordGroup*)GroupPickerForm::select(GroupWidget::Words, tr("Select a word group for the words of the selected items."), dict, false, false, this);
//        if (group == nullptr)
//            return;
//        
//        int r = group->add(indexes);
//        if (r == 0)
//            QMessageBox::information(this, "zkanji", tr("No new words from the selected items were added to the group."), QMessageBox::Ok);
//        else
//            QMessageBox::information(this, "zkanji", tr("%1 words from the selected items were added to the group.").arg(r), QMessageBox::Ok);
//    });
//
//    a = m.exec(globalpos);
//}

void WordStudyListForm::dictReset()
{
    if (!itemsinited)
        return;

    auto &s = ui->queuedButton->isChecked() ? queuesort : ui->studiedButton->isChecked() ? studiedsort : testedsort;
    model->reset();

    ui->dictWidget->setSortIndicator(s.column, s.order);
    ui->dictWidget->sortByIndicator();
}

void WordStudyListForm::dictRemoved(int index, int orderindex, void *oldaddress)
{
    if (dict == oldaddress)
        close();
}

void WordStudyListForm::saveColumns()
{
    std::vector<int> &oldvec = model->viewMode() == DeckViewModes::Queued ? queuesizes : model->viewMode() == DeckViewModes::Studied ? studiedsizes : testedsizes;
    oldvec.resize((model->viewMode() == DeckViewModes::Queued ? queuecolcount : model->viewMode() == DeckViewModes::Studied ? studiedcolcount : testedcolcount) - 1, -1);
    for (int ix = 0, siz = oldvec.size(); ix != siz; ++ix)
    {
        bool hid = ui->dictWidget->view()->isColumnHidden(ix);
        ui->dictWidget->view()->setColumnHidden(ix, false);
        oldvec[ix] = ui->dictWidget->view()->columnWidth(ix);
        if (hid)
            ui->dictWidget->view()->setColumnHidden(ix, true);
    }

    std::vector<char> &oldcolvec = model->viewMode() == DeckViewModes::Queued ? queuecols : model->viewMode() == DeckViewModes::Studied ? studiedcols : testedcols;
    oldcolvec.resize((model->viewMode() == DeckViewModes::Queued ? queuecolcount : model->viewMode() == DeckViewModes::Studied ? studiedcolcount : testedcolcount) - 3, 1);
    for (int ix = 0, siz = oldcolvec.size(); ix != siz; ++ix)
        oldcolvec[ix] = !ui->dictWidget->view()->isColumnHidden(ix);
}

void WordStudyListForm::restoreColumns()
{
    std::vector<char> &colvec = model->viewMode() == DeckViewModes::Queued ? queuecols : model->viewMode() == DeckViewModes::Studied ? studiedcols : testedcols;
    colvec.resize((model->viewMode() == DeckViewModes::Queued ? queuecolcount : model->viewMode() == DeckViewModes::Studied ? studiedcolcount : testedcolcount) - 3, 1);
    for (int ix = 0, siz = colvec.size(); ix != siz; ++ix)
        ui->dictWidget->view()->setColumnHidden(ix, !colvec[ix]);

    std::vector<int> &vec = model->viewMode() == DeckViewModes::Queued ? queuesizes : model->viewMode() == DeckViewModes::Studied ? studiedsizes : testedsizes;
    vec.resize((model->viewMode() == DeckViewModes::Queued ? queuecolcount : model->viewMode() == DeckViewModes::Studied ? studiedcolcount : testedcolcount) - 1, -1);
    for (int ix = 0, siz = vec.size(); ix != siz; ++ix)
    {
        bool hid = ui->dictWidget->view()->isColumnHidden(ix);
        ui->dictWidget->view()->setColumnHidden(ix, false);
        if (vec[ix] != -1)
            ui->dictWidget->view()->setColumnWidth(ix, vec[ix]);
        else
            ui->dictWidget->view()->setColumnWidth(ix, model->defaultColumnWidth(model->viewMode(), ix));
        if (hid)
            ui->dictWidget->view()->setColumnHidden(ix, true);
    }
}

void WordStudyListForm::showStat(DeckStatPages page)
{
    //if (viewed == page)
    //    return;

    //viewed = page;

    if (ui->statView->chart() != nullptr)
        ui->statView->chart()->deleteLater();
    QChart *chart = new QChart();

    switch (page)
    {
    case DeckStatPages::Decks:
    {
        QBarSeries *bars = new QBarSeries(chart);
        QBarSet *dataset = new QBarSet("Levels", chart);

        std::vector<int> levels;
        for (int ix = 0, siz = deck->studySize(); ix != siz; ++ix)
        {
            int lv = deck->studyLevel(ix);
            if (levels.size() <= lv)
                levels.resize(lv + 1);
            ++levels[lv];
        }
        for (int ix = 0, siz = levels.size(); ix != siz; ++ix)
            (*dataset) << levels[ix];

        for (int ix = levels.size(), siz = 12; ix < siz; ++ix)
            (*dataset) << 0;


        bars->append(dataset);
        bars->setLabelsVisible(true);
        bars->setLabelsPosition(QBarSeries::LabelsOutsideEnd);
        chart->addSeries(bars);
        chart->setTitle(tr("Number of items at each level"));
        chart->legend()->hide();
        chart->createDefaultAxes();
        ((QValueAxis*)chart->axisY())->setLabelFormat("%i");
        ((QValueAxis*)chart->axisY())->setTickCount(std::max(2, ui->statView->height() / 70));
        break;
    }
    case DeckStatPages::Items:
    {
        QLineSeries *l1 = new QLineSeries(chart);
        QLineSeries *l2 = new QLineSeries(chart);
        QLineSeries *l3 = new QLineSeries(chart);

        const StudyDeck *study = deck->getStudyDeck();

        qreal hi = 0;

        QDate last;
        for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
        {
            const DeckDayStat &stat = study->dayStat(ix);
            last = stat.day;
            qint64 timesince = QDateTime(stat.day, QTime()).toMSecsSinceEpoch();

            hi = std::max(hi, (qreal)stat.itemcount);
            l1->append(timesince, stat.itemcount);
            l2->append(timesince, stat.itemcount - stat.itemlearned);
            l3->append(timesince, stat.testcount);
        }
        QDateTime now = QDateTime(ltDay(QDateTime::currentDateTimeUtc()), QTime());
        qint64 timesince = now.toMSecsSinceEpoch();
        if (last != now.date())
        {
            l1->append(timesince, static_cast<const QVector<QPointF>>(l1->pointsVector()).last().y());
            l2->append(timesince, static_cast<const QVector<QPointF>>(l2->pointsVector()).last().y());
            l3->append(timesince, static_cast<const QVector<QPointF>>(l3->pointsVector()).last().y());
        }

        QDateTimeAxis *xaxis = new QDateTimeAxis(chart);
        xaxis->setTickCount(10);
        xaxis->setFormat("MMM yyyy");
        xaxis->setTitleText("Study days");

        QValueAxis *yaxis = new QValueAxis(chart);
        yaxis->setLabelFormat("%i");
        yaxis->setTitleText("Items");
        yaxis->setRange(0, hi + 100);

        QAreaSeries *area1 = new QAreaSeries(chart);
        area1->setUpperSeries(l1);
        area1->setLowerSeries(l2);
        chart->addSeries(area1);

        QAreaSeries *area2 = new QAreaSeries(chart);
        area2->setUpperSeries(l2);
        area2->setLowerSeries(l3);
        chart->addSeries(area2);

        QAreaSeries *area3 = new QAreaSeries(chart);
        area3->setUpperSeries(l3);
        chart->addSeries(area3);

        xaxis->setTickCount(std::max(2, ui->statView->width() / 70));
        yaxis->setTickCount(std::max(2, ui->statView->height() / 70));

        chart->setTitle(tr("Number of items on each day"));
        chart->legend()->hide();

        chart->setAxisX(xaxis);
        chart->setAxisY(yaxis);

        area1->attachAxis(xaxis);
        area1->attachAxis(yaxis);
        area2->attachAxis(xaxis);
        area2->attachAxis(yaxis);
        area3->attachAxis(xaxis);
        area3->attachAxis(yaxis);

        break;
    }
    case DeckStatPages::Tests:
    {
        const StudyDeck *study = deck->getStudyDeck();
        QStackedBarSeries *bars = new QStackedBarSeries(chart);

        QBarSet *s1 = new QBarSet(tr("Learned"));
        QBarSet *s2 = new QBarSet(tr("Correct"));
        QBarSet *s3 = new QBarSet(tr("Wrong"));

        QBarCategoryAxis *xaxis = new QBarCategoryAxis(chart);
        QStringList days;

        QDate last;
        for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
        {
            const DeckDayStat &stat = study->dayStat(ix);
            qint64 timesince = QDateTime(stat.day, QTime()).toMSecsSinceEpoch();

            s1->append(stat.testlearned);
            s2->append(stat.testcount - stat.testwrong);
            s3->append(stat.testwrong);

            if (last.isValid() && last.daysTo(stat.day) > 1)
            {
                for (int iy = 1, siz = last.daysTo(stat.day) - 1; iy != siz; ++iy)
                {
                    s1->append(0);
                    s2->append(0);
                    s3->append(0);
                    days.append(DateTimeFunctions::formatDay(last.addDays(iy)));
                }
            }

            last = stat.day;
            days.append(DateTimeFunctions::formatDay(stat.day));
        }
        QDate now = ltDay(QDateTime::currentDateTimeUtc());
        if (last.isValid() && last.daysTo(now) > 0)
        {
            for (int iy = 0, siz = last.daysTo(now); iy != siz; ++iy)
            {
                s1->append(0);
                s2->append(0);
                s3->append(0);
                days.append(DateTimeFunctions::formatDay(last.addDays(iy + 1)));
            }
        }

        bars->append(s1);
        bars->append(s2);
        bars->append(s3);

        xaxis->append(days);
        if (!days.empty())
            xaxis->setRange(days.at(std::max(0, days.size() - 7)), days.last());
        QValueAxis *yaxis = new QValueAxis(chart);
        yaxis->setLabelFormat("%i");
        yaxis->setTitleText("Tested items");
        //yaxis->setRange(0, hi + 100);

        chart->addSeries(bars);
        chart->setTitle(tr("Number of items tested"));
        chart->legend()->hide();

        chart->setAxisX(xaxis, bars);
        chart->setAxisY(yaxis, bars);


        break;
    }
    }

    ui->statView->setChart(chart);
    ui->statView->setRubberBand(QChartView::HorizontalRubberBand);
}


//-------------------------------------------------------------
