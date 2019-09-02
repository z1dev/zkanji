/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMenu>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "dictionarywidget.h"
#include "ui_dictionarywidget.h"
#include "words.h"
#include "zui.h"
#include "zdictionarymodel.h"
#include "zevents.h"
#include "groups.h"
#include "recognizerform.h"
#include "sentences.h"
#include "filterlistform.h"
#include "romajizer.h"
#include "zkanjimain.h"
#include "zproxytablemodel.h"
#include "zlistview.h"
#include "dictionarysettings.h"
#include "globalui.h"
#include "formstates.h"
#include "zkanjiform.h"
#include "zkanjiwidget.h"
#include "fontsettings.h"
#include "popupdict.h"
#include "popupsettings.h"

#include "checked_cast.h"


//-------------------------------------------------------------


QStringListModel DictionaryWidget::jpsearches;
QStringListModel DictionaryWidget::ensearches;

//-------------------------------------------------------------


DictionaryWidget::DictionaryWidget(QWidget *parent) : base(parent), ui(new Ui::DictionaryWidget),
dict(ZKanji::dictionaryCount() != 0 ? ZKanji::dictionary(0) : nullptr), model(nullptr), commandmap(nullptr), /*browsemodel(nullptr), grp(nullptr), filtermodel(nullptr),*/
updatepending(false), updateforced(true), savecolumndata(true), listmode(DictSearch), mode(SearchMode::Japanese), browseorder(Settings::dictionary.browseorder)
{
    ui->setupUi(this);

    // Scaling would break these fonts.
    //ui->jpCBox->lineEdit()->setFont(Settings::kanaFont(false));
    //ui->browseEdit->setFont(Settings::kanaFont(false));

    ui->jpCBox->setDictionary(dict);
    ui->browseEdit->setDictionary(dict);

    conditions.reset(new WordFilterConditions);
    conditions->examples = Inclusion::Ignore;
    conditions->groups = Inclusion::Ignore;

    ui->filterButton->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->wordsTable->setShowGrid(false);
    ui->wordsTable->setSelectionType(ListSelectionType::Single);

    ui->jpCBox->setValidator(&japaneseValidator());
    ui->jpCBox->installEventFilter(this);
    ui->jpCBox->setCompleter(nullptr);
    ui->jpCBox->setModel(&jpsearches);
    ui->jpCBox->setCurrentIndex(-1);
    ui->browseEdit->setValidator(&japaneseValidator());
    ui->enCBox->installEventFilter(this);
    ui->enCBox->setCompleter(nullptr);
    ui->enCBox->setModel(&ensearches);
    ui->enCBox->setCurrentIndex(-1);
    ui->browseEdit->installEventFilter(this);

    ui->wordsTable->installEventFilter(this);

    connect((ZLineEdit*)ui->jpCBox->lineEdit(), &ZLineEdit::textChanged, this, &DictionaryWidget::searchEdited);
    connect((ZLineEdit*)ui->enCBox->lineEdit(), &ZLineEdit::textChanged, this, &DictionaryWidget::searchEdited);

    connect((ZLineEdit*)ui->jpCBox->lineEdit(), &ZLineEdit::textEdited, this, &DictionaryWidget::updateWords);
    connect((ZLineEdit*)ui->enCBox->lineEdit(), &ZLineEdit::textEdited, this, &DictionaryWidget::updateWords);
    connect(ui->browseEdit, &ZLineEdit::textEdited, this, &DictionaryWidget::updateWords);

    connect(ui->jpButton, &QToolButton::clicked, this, &DictionaryWidget::updateSearchMode);
    connect(ui->enButton, &QToolButton::clicked, this, &DictionaryWidget::updateSearchMode);
    connect(ui->browseButton, &QToolButton::clicked, this, &DictionaryWidget::updateSearchMode);

    connect(ui->multilineButton, &QToolButton::clicked, this, &DictionaryWidget::updateMultiline);

    connect(ui->examples, &ExampleWidget::wordSelected, this, &DictionaryWidget::exampleWordSelected);

    connect(ui->filterButton, &QWidget::customContextMenuRequested, this, &DictionaryWidget::showFilterContext);

    connect(ui->wordsTable, &ZDictionaryListView::wordDoubleClicked, this, &DictionaryWidget::wordDoubleClicked);
    connect(ui->wordsTable, &ZDictionaryListView::rowSelectionChanged, this, &DictionaryWidget::rowSelectionChanged);
    connect(ui->wordsTable, &ZDictionaryListView::currentRowChanged, this, &DictionaryWidget::tableRowChanged);
    connect(ui->wordsTable, &ZDictionaryListView::contextMenuCreated, this, &DictionaryWidget::showContextMenu);

    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterErased, this, &DictionaryWidget::filterErased);
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterChanged, this, &DictionaryWidget::filterChanged);
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterMoved, this, &DictionaryWidget::filterMoved);

    connect(gUI, &GlobalUI::settingsChanged, this, &DictionaryWidget::settingsChanged);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &DictionaryWidget::dictionaryRemoved);
    connect(gUI, &GlobalUI::dictionaryFlagChanged, this, &DictionaryWidget::dictionaryFlagChanged);
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &DictionaryWidget::dictionaryReplaced);

    installRecognizer(ui->recognizeButton, ui->jpCBox);

    updateSearchMode();

    qApp->postEvent(this, new StartEvent());

    if (dynamic_cast<ZKanjiForm*>(window()) != nullptr || (dynamic_cast<PopupDictionary*>(window()) != nullptr && Settings::popup.statusbar))
        ui->wordsTable->assignStatusBar(ui->listStatus);
    else
        ui->listStatus->hide();
}

DictionaryWidget::~DictionaryWidget()
{
    delete ui;
}

void DictionaryWidget::saveXMLSettings(QXmlStreamWriter &writer) const
{
    DictionaryWidgetData dat;
    saveState(dat);
    FormStates::saveXMLSettings(dat, writer);
}

void DictionaryWidget::loadXMLSettings(QXmlStreamReader &reader)
{
    DictionaryWidgetData dat;
    FormStates::loadXMLSettings(dat, reader);
    restoreState(dat);
}

void DictionaryWidget::saveState(DictionaryWidgetData &dat) const
{
    dat.mode = mode;
    dat.multi = ui->multilineButton->isVisibleTo(ui->multilineButton->parentWidget()) && ui->multilineButton->isChecked();
    dat.filter = ui->filterWidget->isVisibleTo(ui->filterWidget->parentWidget()) && ui->filterButton->isChecked();
    dat.showex = ui->examplesButton->isVisibleTo(this) && ui->examplesButton->isChecked();
    dat.exmode = ui->examplesButton->isVisibleTo(this) ? ui->examples->displayed() : (ExampleDisplay)0;

    dat.frombefore = ui->jpBeforeButton->isChecked();
    dat.fromafter = ui->jpAfterButton->isChecked();
    dat.fromstrict = ui->jpStrictButton->isChecked();
    dat.frominfl = inflButtonVisible() && ui->inflButton->isChecked();

    dat.toafter = ui->enAfterButton->isChecked();
    dat.tostrict = ui->enStrictButton->isChecked();

    dat.conditionex = Inclusion::Ignore;
    dat.conditiongroup = Inclusion::Ignore;
    if (ui->filterWidget->isVisibleTo(ui->filterWidget->parentWidget()) && !conditionsEmpty())
    {
        dat.conditionex = conditions->examples;
        dat.conditiongroup = conditions->groups;
    }

    dat.conditions.clear();

    for (int ix = 0, siz = tosigned(conditions->inclusions.size()); ix != siz; ++ix)
    {
        if (conditions->inclusions[ix] == Inclusion::Ignore)
            continue;
        dat.conditions.push_back(std::make_pair(ZKanji::wordfilters().items(ix).name, conditions->inclusions[ix]));
    }

    dat.savecolumndata = savecolumndata;

    if (ui->wordsTable->horizontalHeader()->isSortIndicatorShown())
    {
        dat.sortcolumn = ui->wordsTable->horizontalHeader()->sortIndicatorSection();
        dat.sortorder = ui->wordsTable->horizontalHeader()->sortIndicatorOrder();
    }
    else
        dat.sortcolumn = -1;

    ui->wordsTable->saveColumnState(dat.tabledata);
}

void DictionaryWidget::restoreState(const DictionaryWidgetData &dat)
{
    if (dat.mode == SearchMode::Browse)
        ui->browseButton->setChecked(true);
    else if (dat.mode == SearchMode::Japanese)
        ui->jpButton->setChecked(true);
    else if (dat.mode == SearchMode::Definition)
        ui->enButton->setChecked(true);
    updateSearchMode();

    if (ui->multilineButton->isVisibleTo(ui->multilineButton->parentWidget()))
    {
        ui->multilineButton->setChecked(dat.multi);
        updateMultiline();
    }

    bool usefilters = false;
    if (ui->filterWidget->isVisibleTo(ui->filterWidget->parentWidget()))
        usefilters = dat.filter;

    if (ui->examplesButton->isVisibleTo(this))
    {
        ui->examplesButton->setChecked(dat.showex);
        emit ui->examplesButton->clicked(ui->examplesButton->isChecked());
        ui->examples->setDisplayed(dat.exmode);
    }

    ui->jpBeforeButton->setChecked(dat.frombefore);
    ui->jpAfterButton->setChecked(dat.fromafter);
    ui->jpStrictButton->setChecked(dat.fromstrict);
    if (inflButtonVisible())
        ui->inflButton->setChecked(dat.frominfl);

    ui->enAfterButton->setChecked(dat.toafter);
    ui->enStrictButton->setChecked(dat.tostrict);

    conditions->examples = dat.conditionex;
    conditions->groups = dat.conditiongroup;

    for (int ix = 0, siz = tosigned(dat.conditions.size()); ix != siz; ++ix)
    {
        int filterix = ZKanji::wordfilters().itemIndex(dat.conditions[ix].first);
        if (filterix != -1)
        {
            int csiz = tosigned(conditions->inclusions.size());
            while (csiz++ != filterix + 1)
                conditions->inclusions.push_back(Inclusion::Ignore);
            conditions->inclusions[filterix] = dat.conditions[ix].second;
        }
    }
    ui->filterButton->setChecked(usefilters);

    if (savecolumndata)
    {
        ui->wordsTable->horizontalHeader()->setSortIndicator(dat.sortcolumn, dat.sortorder);
        ui->wordsTable->restoreColumnState(dat.tabledata, model);
    }

    updateWords();
}

void DictionaryWidget::assignStatusBar(ZStatusBar *bar)
{
    ui->wordsTable->assignStatusBar(bar);
}

void DictionaryWidget::showStatusBar()
{
    if (ui->listStatus->isVisibleTo(this))
        return;

    ui->listStatus->setVisible(true);
    assignStatusBar(ui->listStatus);
}

bool DictionaryWidget::isSavingColumnData() const
{
    return savecolumndata;
}

void DictionaryWidget::setSaveColumnData(bool save)
{
    savecolumndata = save;
}

//void DictionaryWidget::makeModeSpace(const QSize &size)
//{
//    ui->barLayout->setContentsMargins(size.width(), 0, 0, 0);
//}

ZDictionaryListView* DictionaryWidget::view()
{
    return ui->wordsTable;
}

const ZDictionaryListView* DictionaryWidget::view() const
{
    return ui->wordsTable;
}

Dictionary* DictionaryWidget::dictionary()
{
    if (listmode == DictSearch)
        return dict;

    if (model == nullptr)
        return nullptr;

    return model->dictionary();
}

const Dictionary* DictionaryWidget::dictionary() const
{
    if (listmode == DictSearch)
        return dict;

    if (model == nullptr)
        return nullptr;

    return model->dictionary();
}

void DictionaryWidget::setDictionary(Dictionary *newdict)
{
    if (dict == newdict)
        return;

    //if (newdict->name() == QStringLiteral("English"))
    //{
    //    ui->jpButton->setIcon(QIcon(":/flagjpen.svg"));
    //    ui->enButton->setIcon(QIcon(":/flagenjp.svg"));
    //    ui->browseButton->setIcon(QIcon(":/flagen.svg"));
    //}
    //else
    //{
    //    ui->jpButton->setIcon(QIcon(":/flagjpgen.svg"));
    //    ui->enButton->setIcon(QIcon(":/flaggenjp.svg"));
    //    ui->browseButton->setIcon(QIcon(":/flaggen.svg"));
    //}

    ui->jpButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), newdict->name(), Flags::FromJapanese)));
    ui->enButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), newdict->name(), Flags::ToJapanese)));

    ui->browseButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), QString(), Flags::Browse)));

    if (dict != nullptr)
        disconnect(dict, &Dictionary::dictionaryReset, this, &DictionaryWidget::dictionaryReset);
    dict = newdict;
    ui->jpCBox->setDictionary(dict);
    ui->browseEdit->setDictionary(dict);
    if (dict != nullptr)
        connect(dict, &Dictionary::dictionaryReset, this, &DictionaryWidget::dictionaryReset);

    if (listmode == Filtered)
        return;

    if (searchmodel)
    {
        searchmodel->deleteLater();
        searchmodel.release();
    }

    if (browsemodel)
    {
        browsemodel->deleteLater();
        browsemodel.release();
    }

    model = nullptr;

    updateWords();
}

void DictionaryWidget::setDictionary(int index)
{
    setDictionary(ZKanji::dictionary(index));
}

void DictionaryWidget::executeCommand(int command)
{
    if (command < (int)CommandLimits::SearchBegin || command >= (int)CommandLimits::SearchEnd)
        return;

    switch (command)
    {
    case (int)Commands::FromJapanese:
        setSearchMode(SearchMode::Japanese);
        break;
    case (int)Commands::ToJapanese:
        setSearchMode(SearchMode::Definition);
        break;
    case (int)Commands::BrowseJapanese:
        setSearchMode(SearchMode::Browse);
        break;
    case (int)Commands::ToggleExamples:
        if (ui->examplesButton->isVisibleTo(this))
        {
            ui->examplesButton->toggle();
            emit ui->examplesButton->clicked(ui->examplesButton->isChecked());
        }
        break;
    case (int)Commands::ToggleAnyStart:
        if (ui->jpBeforeButton->isVisibleTo(this))
        {
            ui->jpBeforeButton->toggle();
            emit ui->jpBeforeButton->clicked(ui->jpBeforeButton->isChecked());
        }
        break;
    case (int)Commands::ToggleAnyEnd:
        if (ui->jpAfterButton->isVisibleTo(this))
        {
            ui->jpAfterButton->toggle();
            emit ui->jpAfterButton->clicked(ui->jpAfterButton->isChecked());
        }
        else if (ui->enAfterButton->isVisibleTo(this))
        {
            ui->enAfterButton->toggle();
            emit ui->enAfterButton->clicked(ui->enAfterButton->isChecked());
        }
        break;
    case (int)Commands::ToggleDeinflect:
        if (ui->inflButton->isVisibleTo(this))
        {
            ui->inflButton->toggle();
            emit ui->inflButton->clicked(ui->inflButton->isChecked());
        }
        break;
    case (int)Commands::ToggleStrict:
        if (ui->jpStrictButton->isVisibleTo(this))
        {
            ui->jpStrictButton->toggle();
            emit ui->jpStrictButton->clicked(ui->jpStrictButton->isChecked());
        }
        else if (ui->enStrictButton->isVisibleTo(this))
        {
            ui->enStrictButton->toggle();
            emit ui->enStrictButton->clicked(ui->enStrictButton->isChecked());
        }
        break;
    case (int)Commands::ToggleFilter:
        if (ui->filterButton->isVisibleTo(this))
        {
            ui->filterButton->toggle();
            emit ui->filterButton->clicked(ui->filterButton->isChecked());
        }
        break;
    case (int)Commands::EditFilters:
        if (ui->filterButton->isVisibleTo(this))
            showFilterContext(QPoint());
        break;
    case (int)Commands::ToggleMultiline:
        if (ui->multilineButton->isVisibleTo(this))
        {
            ui->multilineButton->toggle();
            emit ui->multilineButton->clicked(ui->multilineButton->isChecked());
        }
        break;
    }
}

void DictionaryWidget::commandState(int command, bool &enabled, bool &checked, bool &visible) const
{
    if (command < (int)CommandLimits::SearchBegin || command >= (int)CommandLimits::SearchEnd)
        return;
    enabled = true;
    checked = false;
    visible = true;
    switch (command)
    {
    case (int)Commands::FromJapanese:
        checked = searchMode() == SearchMode::Japanese;
        break;
    case (int)Commands::ToJapanese:
        checked = searchMode() == SearchMode::Definition;
        break;
    case (int)Commands::BrowseJapanese:
        enabled = ui->browseButton->isVisibleTo(this);
        checked = enabled && searchMode() == SearchMode::Browse;
        break;
    case (int)Commands::ToggleExamples:
        enabled = ui->examplesButton->isVisibleTo(this);
        checked = enabled && ui->examplesButton->isChecked();
        break;
    case (int)Commands::ToggleAnyStart:
        enabled = ui->jpBeforeButton->isVisibleTo(this);
        checked = enabled && ui->jpBeforeButton->isChecked();
        break;
    case (int)Commands::ToggleAnyEnd:
        enabled = ui->jpAfterButton->isVisibleTo(this) || ui->enAfterButton->isVisibleTo(this);
        checked = enabled && (ui->jpAfterButton->isChecked() || ui->enAfterButton->isChecked());
        break;
    case (int)Commands::ToggleDeinflect:
        enabled = ui->inflButton->isVisibleTo(this);
        checked = enabled && ui->inflButton->isChecked();
        break;
    case (int)Commands::ToggleStrict:
        enabled = ui->jpStrictButton->isVisibleTo(this) || ui->enStrictButton->isVisibleTo(this);
        checked = enabled && (ui->jpStrictButton->isChecked() || ui->enStrictButton->isChecked());
        break;
    case (int)Commands::ToggleFilter:
        enabled = ui->filterButton->isVisibleTo(this);
        checked = enabled && ui->filterButton->isChecked();
        break;
    case (int)Commands::EditFilters:
        enabled = ui->filterButton->isVisibleTo(this);
        checked = false;
        break;
    case (int)Commands::ToggleMultiline:
        enabled = ui->multilineButton->isVisibleTo(this);
        checked = enabled && ui->multilineButton->isChecked();
        break;
    }
}

DictionaryWidget::ListMode DictionaryWidget::listMode() const
{
    return listmode;
}

void DictionaryWidget::setListMode(ListMode newmode)
{
    if (listmode == newmode)
        return;

    listmode = newmode;

    if (browsemodel)
        browsemodel->deleteLater();
    browsemodel.release();
    if (searchmodel)
        searchmodel->deleteLater();
    searchmodel.release();
    if (filtermodel)
        filtermodel->deleteLater();
    filtermodel.release();
    sortfunc = nullptr;

    model = nullptr;

    setTableModel(nullptr);

    if (listmode == DictSearch)
    {
        // We are not ready for list mode changes mid program. This part only tries to achive
        // a safe state just in case.

        //if (mode != SearchMode::Browse)
        //    searchmodel.reset(new DictionarySearchResultItemModel(this));

        ui->browseButton->show();

        setAcceptWordDrops(false);
    }
    else if (listmode == Filtered)
    {
        // Browse is an invalid mode when not browsing a dictionary. The Japanese button must take over.
        if (mode == SearchMode::Browse)
        {
            ui->jpButton->setChecked(true);
            DictionaryWidget::updateSearchMode();
        }
        ui->browseButton->hide();
    }

    updateWords();
}

SearchMode DictionaryWidget::searchMode() const
{
    return mode;
}


void DictionaryWidget::setSearchMode(SearchMode newmode)
{
    if (newmode == mode || (newmode == SearchMode::Browse && !ui->browseButton->isVisibleTo(ui->browseButton->parentWidget())))
        return;

    if (newmode == SearchMode::Browse)
        ui->browseButton->setChecked(true);
    else if (newmode == SearchMode::Japanese)
        ui->jpButton->setChecked(true);
    else if (newmode == SearchMode::Definition)
        ui->enButton->setChecked(true);

    updateSearchMode();
}

ListSelectionType DictionaryWidget::selectionType() const
{
    return ui->wordsTable->selectionType();
}

void DictionaryWidget::setSelectionType(ListSelectionType seltype)
{
    //if (multiSelect() == multi)
    //    return;

    //if (!multi)
    //    ui->wordsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    //else
    //    ui->wordsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->wordsTable->setSelectionType(seltype);
}

bool DictionaryWidget::acceptWordDrops() const
{
    return ui->wordsTable->acceptDrops();
}

void DictionaryWidget::setAcceptWordDrops(bool droptarget)
{
    ui->wordsTable->setAcceptDrops(droptarget);
}

bool DictionaryWidget::wordDragEnabled() const
{
    return ui->wordsTable->dragEnabled();
}

void DictionaryWidget::setWordDragEnabled(bool enablestate)
{
    ui->wordsTable->setDragEnabled(enablestate);
}

bool DictionaryWidget::isSortIndicatorVisible() const
{
    return ui->wordsTable->horizontalHeader()->isSortIndicatorShown();
}

void DictionaryWidget::setSortIndicatorVisible(bool showit)
{
    if (showit == ui->wordsTable->horizontalHeader()->isSortIndicatorShown())
        return;

    if (!showit)
        disconnect(ui->wordsTable->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &DictionaryWidget::sortIndicatorChanged);
    ui->wordsTable->horizontalHeader()->setSortIndicatorShown(showit);
    if (showit)
        connect(ui->wordsTable->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &DictionaryWidget::sortIndicatorChanged);
}

void DictionaryWidget::setSortFunction(ProxySortFunction func)
{
    sortfunc = func;
    if (filtermodel != nullptr)
        filtermodel->prepareSortFunction(sortfunc);
}

void DictionaryWidget::setSortIndicator(int index, Qt::SortOrder order)
{
    ui->wordsTable->horizontalHeader()->setSortIndicator(index, order);
}

void DictionaryWidget::sortByIndicator()
{
    if (filtermodel == nullptr)
        updateWords();
    else
        filtermodel->sortBy(ui->wordsTable->horizontalHeader()->sortIndicatorSection(), ui->wordsTable->horizontalHeader()->sortIndicatorOrder(), sortfunc);
}

int DictionaryWidget::checkBoxColumn() const
{
    return ui->wordsTable->checkBoxColumn();
}

void DictionaryWidget::setCheckBoxColumn(int newcol)
{
    ui->wordsTable->setCheckBoxColumn(newcol);
}

void DictionaryWidget::addFrontWidget(QWidget *widget, bool separated)
{
    if (separated)
    {
        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Sunken);
        ui->barLayout->insertWidget(0, line);
    }
    ui->barLayout->insertWidget(0, widget);
}

void DictionaryWidget::addBackWidget(QWidget *widget, bool separated)
{
    if (separated)
    {
        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Sunken);
        ui->barLayout->addWidget(line);
    }
    ui->barLayout->addWidget(widget);
}

//ZAbstractTableModel* DictionaryWidget::currentModel()
//{
//    return ui->wordsTable->model();
//}

void DictionaryWidget::setModel(DictionaryItemModel *newmodel)
{
    setListMode(Filtered);

    if (model == newmodel)
        return;

    if (filtermodel)
        filtermodel->deleteLater();
    filtermodel.release();
    sortfunc = nullptr;

    model = newmodel;

    if (model == nullptr)
        setTableModel(nullptr);
    else
        updateWords();
}

void DictionaryWidget::setGroup(WordGroup *newgroup)
{
    //setMultiSelect(true);
    setSelectionType(ListSelectionType::Extended);
    setAcceptWordDrops(true);

    if (newgroup == nullptr)
    {
        updateforced = true;
        setModel(nullptr);
        updateforced = false;
    }
    else
        setModel(newgroup->groupModel());
}

WordEntry* DictionaryWidget::words(int row) const
{
    if (row < 0 || model == nullptr)
        return nullptr;

    //if (ui->wordsTable->isMultiLine() && model)
    //    row = ui->wordsTable->multiModel()->mapToSourceRow(row); // mapToSource(ui->wordsTable->multiModel()->index(row, 0)).row();

    if (listmode == DictSearch || !filtermodel)
    {
        if (row >= model->rowCount())
            return nullptr;
        return model->items(row);

    }
    else if (filtermodel != nullptr)
    {
        if (row >= filtermodel->rowCount())
            return nullptr;
        return model->items(filtermodel->mapToSource(filtermodel->index(row, 0)).row());
    }
    return nullptr;
}

int DictionaryWidget::wordIndex(int row) const
{
    if (row < 0 || model == nullptr)
        return -1;

    //if (ui->wordsTable->isMultiLine() && model)
    //    row = ui->wordsTable->multiModel()->mapToSourceRow(row); // mapToSource(ui->wordsTable->multiModel()->index(row, 0)).row();

    if (listmode == DictSearch || filtermodel == nullptr)
    {
        if (row >= model->rowCount())
            return -1;
        return model->indexes(row);
    }
    else if (filtermodel != nullptr)
    {
        if (row >= filtermodel->rowCount())
            return -1;
        return model->indexes(filtermodel->mapToSource(filtermodel->index(row, 0)).row());
    }
    return -1;
}

WordEntry* DictionaryWidget::currentWord() const
{
    return words(currentRow());
}

int DictionaryWidget::currentIndex() const
{
    return wordIndex(currentRow());
}

int DictionaryWidget::currentRow() const
{
    int row = ui->wordsTable->currentRow();
    if (row == -1)
        return -1;
    if (ui->wordsTable->isMultiLine() && model)
        return ui->wordsTable->multiModel()->mapToSourceRow(row);
    return row;
}

bool DictionaryWidget::hasSelection() const
{
    return ui->wordsTable->hasSelection();
}

int DictionaryWidget::selectionSize() const
{
    return ui->wordsTable->selectionSize();
}

void DictionaryWidget::selectedIndexes(std::vector<int> &indexes) const
{
    ui->wordsTable->selectedRows(indexes);
    for (int &ix : indexes)
    {
        if (listmode == DictSearch || filtermodel == nullptr)
            ix = model->indexes(ix);
        else if (filtermodel != nullptr)
            ix = model->indexes(filtermodel->mapToSource(filtermodel->index(ix, 0)).row());
    }
}

void DictionaryWidget::selectedRows(std::vector<int> &rows) const
{
    ui->wordsTable->selectedRows(rows);
    //if (filtermodel != nullptr)
    //    for (int &row : rows)
    //        row = filtermodel->mapToSource(filtermodel->index(row, 0)).row();
}

void DictionaryWidget::unfilteredRows(const std::vector<int> &filteredrows, std::vector<int> &result)
{
    if (filtermodel == nullptr)
    {
        result = filteredrows;
        return;
    }
    result.resize(filteredrows.size());
    for (int ix = 0, siz = tosigned(filteredrows.size()); ix != siz; ++ix)
        result[ix] = filtermodel->mapToSource(filtermodel->index(filteredrows[ix], 0)).row();
}

bool DictionaryWidget::filtered() const
{
    return filtermodel != nullptr;
}

int DictionaryWidget::rowCount() const
{
    if (ui->wordsTable->model() == nullptr)
        return 0;
    if (ui->wordsTable->multiModel() != nullptr)
        return ui->wordsTable->multiModel()->rowCount();
    return ui->wordsTable->model()->rowCount();
}

bool DictionaryWidget::continuousSelection() const
{
    return ui->wordsTable->continuousSelection();
}

bool DictionaryWidget::rowSelected(int rowindex) const
{
    return ui->wordsTable->rowSelected(rowindex);
}

void DictionaryWidget::setSearchText(const QString &str)
{
    if (mode == SearchMode::Japanese)
        ui->jpCBox->setText(str);
    else if (mode == SearchMode::Definition)
        ui->enCBox->setText(str);
    else
        ui->browseEdit->setText(str);

    updateWords();
}

QString DictionaryWidget::searchText() const
{
    if (mode == SearchMode::Japanese)
        return ui->jpCBox->text();
    else if (mode == SearchMode::Definition)
        return ui->enCBox->text();
    else
        return ui->browseEdit->text();
}

bool DictionaryWidget::inflButtonVisible() const
{
    return ui->inflButton->isVisibleTo((QWidget*)ui->inflButton->parent());
}

void DictionaryWidget::setInflButtonVisible(bool visible)
{
    ui->inflButton->setChecked(false);
    ui->inflButton->setVisible(visible);
}

bool DictionaryWidget::examplesVisible() const
{
    return ui->examplesButton->isVisibleTo(this);
}

void DictionaryWidget::setExamplesVisible(bool shown)
{
    if (ui->examplesButton->isVisibleTo(this) == shown)
        return;

    ui->examplePanel->setVisible(shown);
    ui->examples->setVisible(shown);
    if (!shown)
        ui->examples->setItem(nullptr, 0);
}

void DictionaryWidget::setExamplesInteraction(bool allow)
{
    ui->examples->setInteraction(allow);
}

bool DictionaryWidget::multilineVisible() const
{
    return ui->multilineButton->isVisibleTo(this);
}

void DictionaryWidget::setMultilineVisible(bool shown)
{
    if (ui->multilineButton->isVisibleTo(this) == shown)
        return;

    if (!shown)
        ui->multilineButton->setChecked(false);
    ui->multilineButton->setVisible(shown);
}

bool DictionaryWidget::wordFiltersVisible() const
{
    return ui->filterWidget->isVisibleTo(ui->filterWidget->parentWidget());
}

void DictionaryWidget::setWordFiltersVisible(bool shown)
{
    if (ui->filterWidget->isVisibleTo(ui->filterWidget->parentWidget()) == shown)
        return;

    if (!shown)
        ui->filterButton->setChecked(false);
    ui->filterWidget->setVisible(shown);
}

void DictionaryWidget::turnOnWordFilter(int index, Inclusion inc)
{
    if (index < 0 || index >= tosigned(ZKanji::wordfilters().size()))
        return;

    int csiz = tosigned(conditions->inclusions.size());
    while (csiz++ <= index)
        conditions->inclusions.push_back(Inclusion::Ignore);
    conditions->inclusions[index] = inc;

    ui->filterButton->setChecked(true);
    on_filterButton_clicked(true);
}

void DictionaryWidget::setStudyDefinitionUsed(bool study)
{
    ui->wordsTable->setStudyDefinitionUsed(study);
}

bool DictionaryWidget::isStudyDefinitionUsed() const
{
    return ui->wordsTable->isStudyDefinitionUsed();
}

void DictionaryWidget::browseTo(Dictionary *d, int windex)
{
#ifdef _DEBUG
    if (listmode != DictSearch)
        throw "Only browse to a given word in dictionary search list mode.";
#endif

    if (listmode != DictSearch)
        return;

    if (mode == SearchMode::Definition)
    {
        ui->jpButton->setChecked(true);
        mode = SearchMode::Japanese;
        ui->searchStack->setCurrentIndex(0);
    }

    setDictionary(d);

    if (updatepending)
    {
        updateforced = true;
        updateWords();
    }

    WordEntry *e = dict->wordEntry(windex);
    QString kana = e->kana.toQString();

    int browserow = -1;

    if (mode == SearchMode::Japanese)
    {
        ui->jpCBox->setText(kana);
        updateWords();
        for (int ix = 0; ix != model->rowCount(); ++ix)
            if (model->indexes(ix) == windex)
            {
                browserow = ix;// model->index(ix, 0);
                break;
            }
    }
    else // Browse
    {
        ui->browseEdit->setText(e->kana.toQString());

        browserow = browsemodel->browseRow(kana);

        if (browserow != -1)
        {
            int row = browserow;
            bool found = false;

            while (!found && row < model->rowCount())
            {
                if (model->indexes(row) == windex)
                    found = true;
                else
                    ++row;
            }
            if (found)
                browserow = row;
        }
    }

    if (browserow == -1)
        return;

    if (ui->wordsTable->isMultiLine())
        browserow = ui->wordsTable->multiModel()->mapFromSourceRow(browserow);

    ui->wordsTable->setCurrentRow(browserow);
}

void DictionaryWidget::forceUpdate()
{
    updateforced = true;
}

void DictionaryWidget::showEvent(QShowEvent *e)
{
    if (updatepending)
        updateWords();
    base::showEvent(e);
}

void DictionaryWidget::timerEvent(QTimerEvent *e)
{
    base::timerEvent(e);

    if (e->timerId() != historytimer.timerId() || (!ui->jpCBox->hasFocus() && !ui->enCBox->hasFocus()))
        return;

    storeLastSearch(ui->jpCBox->hasFocus() ? ui->jpCBox : ui->enCBox);
}

bool DictionaryWidget::event(QEvent *e)
{
    if (e->type() == StartEvent::Type())
    {
        installCommands();

        if (dict != nullptr)
        {
            ui->jpButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), dict->name(), Flags::FromJapanese)));
            ui->enButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), dict->name(), Flags::ToJapanese)));
            ui->browseButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), QString(), Flags::Browse)));
        }

        return true;
    }

    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        installCommands();
    }

    return base::event(e);
}

bool DictionaryWidget::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == ui->wordsTable)
    {
        if (e->type() == QEvent::KeyPress && ((QKeyEvent*)e)->text() != QString() && (((QKeyEvent*)e)->modifiers() & ~Qt::ShiftModifier) == Qt::NoModifier)
        {
            if ((
                //TODO: (later with Qt5.6 or newer) replace all Key_Escape keys where appropriate with QKeySequence::Cancel
#if (QT_VERSION > 0x050500)
                ((QKeyEvent*)e)->matches(QKeySequence::Cancel) &&
#else
                ((QKeyEvent*)e)->key() == Qt::Key_Escape &&
#endif
                ui->wordsTable->cancelActions()) || qApp->mouseButtons().testFlag(Qt::RightButton))
                return base::eventFilter(obj, e);

            QKeyEvent *ke = (QKeyEvent*)e;
            ZLineEdit *edit = ui->jpCBox->isVisible() ? (ZLineEdit*)ui->jpCBox->lineEdit() : ui->enCBox->isVisible() ? (ZLineEdit*)ui->enCBox->lineEdit() : ui->browseEdit;
            edit->setFocus();
            qApp->postEvent(edit, new QKeyEvent(ke->type(), ke->key(), ke->modifiers(), ke->text(), ke->isAutoRepeat(), ke->count()));
            return true;
        }

        return base::eventFilter(obj, e);
    }

    if (obj != ui->jpCBox && obj != ui->enCBox && obj != ui->browseEdit)
        return base::eventFilter(obj, e);

    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = (QKeyEvent*)e;
        switch (ke->key())
        {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (obj != ui->jpCBox && obj != ui->enCBox)
            {
                if (obj == ui->browseEdit)
                    ui->browseEdit->setText(QString());
                break;
            }
            storeLastSearch((ZComboBox*)obj);
            ((ZComboBox*)obj)->setText(QString());
            updateWords();
            return true;
        case Qt::Key_Up:
        case Qt::Key_Down:
            ui->wordsTable->setFocus();
            ui->wordsTable->setCurrentRow(ui->wordsTable->currentRow() + (ke->key() == Qt::Key_Up ? -1 : 1));
            return true;
        }
        return base::eventFilter(obj, e);
    }

    if (obj != ui->jpCBox && obj != ui->enCBox)
        return base::eventFilter(obj, e);

    if (e->type() == QEvent::FocusOut || e->type() == QEvent::MouseButtonPress)
        storeLastSearch((ZComboBox*)obj);

    return base::eventFilter(obj, e);
}

void DictionaryWidget::tableRowChanged(int row, int prev)
{
    if (ui->wordsTable->isMultiLine() && model && prev != -1)
        prev = ui->wordsTable->multiModel()->mapToSourceRow(prev);

    if (row == -1)
    {
        ui->examples->setItem(dictionary(), -1);
        if (prev != -1)
            emit rowChanged(-1, prev);
        return;
    }

    if (ui->wordsTable->isMultiLine() && model)
        row = ui->wordsTable->multiModel()->mapToSourceRow(row);

    if (examplesVisible())
        ui->examples->setItem(dictionary(), wordIndex(row));

    emit rowChanged(row, prev);
}

void DictionaryWidget::exampleWordSelected(ushort block, uchar line, int wordpos, int wordform)
{
    if (dictionary() == nullptr)
        return;

    auto sentence = ZKanji::sentences.getSentence(block, line);
    ExampleWordsData::Form &f = sentence.words[wordpos].forms[wordform];
    int ix = dictionary()->findKanjiKanaWord(f.kanji, f.kana);
    if (ix == -1)
        return;

    ui->examples->lock();
    browseTo(dictionary(), ix);
    ui->examples->unlock(dictionary(), ix, wordpos, wordform);
}

void DictionaryWidget::showFilterContext(const QPoint &/*pos*/)
{
    if (!ui->filterButton->isChecked())
    {
        ui->filterButton->setChecked(true);
        if (!conditionsEmpty())
            updateWords();
    }

    FilterListForm *form = new FilterListForm(conditions.get(), QRect(ui->filterButton->mapToGlobal(QPoint(0, 0)), ui->filterButton->mapToGlobal(QPoint(ui->filterButton->width() - 1, ui->filterButton->height() - 1))), this);
    connect(form, &FilterListForm::includeChanged, this, &DictionaryWidget::filterInclude);
    connect(form, &FilterListForm::conditionChanged, this, &DictionaryWidget::conditionChanged);
    form->show();

}

void DictionaryWidget::filterInclude(int /*index*/, Inclusion /*oldinclude*/)
{
    if (ui->filterButton->isChecked())
        updateWords();
}

void DictionaryWidget::conditionChanged()
{
    if (ui->filterButton->isChecked())
        updateWords();
}

void DictionaryWidget::filterErased(int index)
{
#ifdef _DEBUG
    if (conditions == nullptr)
        throw "Conditions not initialized. Shouldn't happen, unless filters can change after program shutdown.";
#endif

    if (index < 0 || index >= tosigned(conditions->inclusions.size()))
        return;

    bool changed = conditions->inclusions[index] != Inclusion::Ignore && ui->filterButton->isChecked();
    conditions->inclusions.erase(conditions->inclusions.begin() + index);

    if (changed)
    {
        if (listmode == DictSearch)
        {
            switch (mode)
            {
            case SearchMode::Browse:
                if (browsemodel)
                    browsemodel->setFilterConditions(nullptr);
                break;
            case SearchMode::Japanese:
            case SearchMode::Definition:
                if (searchmodel)
                    searchmodel->resetFilterConditions();
                break;
            }
        }
        updateWords();
    }
}

void DictionaryWidget::filterChanged(int index)
{
#ifdef _DEBUG
    if (conditions == nullptr)
        throw "Conditions not initialized. Shouldn't happen, unless filters can change after program shutdown.";
#endif

    if (index < 0 || index >= tosigned(conditions->inclusions.size())  || conditions->inclusions[index] == Inclusion::Ignore || !ui->filterButton->isChecked())
        return;

    if (listmode == DictSearch)
    {
        switch (mode)
        {
        case SearchMode::Browse:
            if (browsemodel)
                browsemodel->setFilterConditions(nullptr);
            break;
        case SearchMode::Japanese:
        case SearchMode::Definition:
            if (searchmodel)
                searchmodel->resetFilterConditions();
            break;
        }
    }

    updateWords();
}

void DictionaryWidget::filterMoved(int index, int to)
{
#ifdef _DEBUG
    if (conditions == nullptr)
        throw "Conditions not initialized. Shouldn't happen, unless filters can change after program shutdown.";
#endif

    std::vector<Inclusion> &inc = conditions->inclusions;
    if ((index < 0 || index >= tosigned(inc.size())) && (to < 0 || to >= tosigned(inc.size())))
        return;

    if (to > index)
        --to;

    Inclusion moved = index >= tosigned(inc.size()) ? Inclusion::Ignore : inc[index];
    if (index < tosigned(inc.size()))
        inc.erase(inc.begin() + index);

    if (to >= tosigned(inc.size()))
    {
        if (moved == Inclusion::Ignore)
            return;
        while (tosigned(inc.size()) <= to)
            inc.push_back(Inclusion::Ignore);
        inc[to] = moved;
        return;
    }

    inc.insert(inc.begin() + to, moved);
    if (moved == Inclusion::Ignore)
        return;
    while (!inc.empty() && inc.back() == Inclusion::Ignore)
        inc.pop_back();
}

void DictionaryWidget::showContextMenu(QMenu *menu, QAction *insertpos, Dictionary *d, DictColumnTypes coltype, QString selstr, const std::vector<int> &windexes, const std::vector<ushort> &kindexes)
{
    if (d == nullptr || (!ui->jpButton->isVisibleTo(this) && !ui->browseButton->isVisibleTo(this) && searchMode() == SearchMode::Definition))
    {
        emit customizeContextMenu(menu, insertpos, d, coltype, selstr, windexes, kindexes);
        return;
    }

    if (!selstr.isEmpty() || (windexes.size() == 1 && (ui->jpButton->isVisibleTo(this) || ui->browseButton->isVisibleTo(this)) && (coltype == DictColumnTypes::Kanji || coltype == DictColumnTypes::Kana)))
    {
        QString str = !selstr.isEmpty() ? selstr : coltype == DictColumnTypes::Kanji ? d->wordEntry(windexes[0])->kanji.toQString() : d->wordEntry(windexes[0])->kana.toQString();

        bool haskanji = false;
        for (int ix = 0, siz = str.size(); !haskanji && ix != siz; ++ix)
            if (KANJI(str.at(ix).unicode()))
                haskanji = true;

        if (!haskanji || ui->jpButton->isVisibleTo(this) || mode == SearchMode::Japanese)
        {
            QAction *asrc = new QAction(tr("Search"), menu);
            menu->insertAction(insertpos, asrc);
            connect(asrc, &QAction::triggered, [this, str, coltype, haskanji]() {
                if (mode == SearchMode::Definition)
                {
                    if (ui->jpButton->isVisibleTo(this))
                        setSearchMode(SearchMode::Japanese);
                    else if (!haskanji && ui->browseButton->isVisibleTo(this))
                        setSearchMode(SearchMode::Browse);
                    else
                        return;
                }
                else if (mode == SearchMode::Browse && haskanji)
                {
                    if (ui->jpButton->isVisibleTo(this))
                        setSearchMode(SearchMode::Japanese);
                    else
                        return;
                }
                setSearchText(str);
            });
            insertpos = asrc;
        }
    }

    emit customizeContextMenu(menu, insertpos, d, coltype, selstr, windexes, kindexes);
}

void DictionaryWidget::searchEdited()
{
    historytimer.stop();

    if (!Settings::dictionary.historytimer)
        return;

    ZLineEdit *ed = (ZLineEdit*)sender();

    QString str = ed->text();
    if (str.isEmpty())
        return;
    if (ed->validator() != nullptr)
    {
        int p = 0;
        ed->validator()->validate(str, p);
        ed->validator()->fixup(str);
    }

    int pos = (ed == ui->jpCBox->lineEdit() ? ui->jpCBox : ui->enCBox)->findData(str, Qt::DisplayRole);
    if (pos == 0)
        return;

    historytimer.start(1000 * Settings::dictionary.historytimeout, this);
}

void DictionaryWidget::settingsChanged()
{
    if (dynamic_cast<PopupDictionary*>(window()) != nullptr)
    {
        ui->listStatus->setVisible(Settings::popup.statusbar);
        ui->wordsTable->assignStatusBar(Settings::popup.statusbar ? ui->listStatus : nullptr);
    }

    if (browseorder != Settings::dictionary.browseorder)
    {
        browseorder = Settings::dictionary.browseorder;

        if (listmode == DictSearch && mode == SearchMode::Browse)
            updateWords();
    }
}

void DictionaryWidget::dictionaryReset()
{
    if (listmode == Filtered)
        return;

    if (searchmodel)
    {
        searchmodel->deleteLater();
        searchmodel.release();
    }

    if (browsemodel)
    {
        browsemodel->deleteLater();
        browsemodel.release();
    }

    model = nullptr;

    updateWords();
}

void DictionaryWidget::dictionaryRemoved(int /*index*/, int /*order*/, void *oldaddress)
{
    if (dict != oldaddress)
        return;
    dict = nullptr;
    ui->jpCBox->setDictionary(nullptr);
    ui->browseEdit->setDictionary(nullptr);
    updateWords();
}

void DictionaryWidget::dictionaryFlagChanged(int index, int /*order*/)
{
    if (ZKanji::dictionary(index) != dict)
        return;

    ui->jpButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), dict->name(), Flags::FromJapanese)));
    ui->enButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), dict->name(), Flags::ToJapanese)));
    ui->browseButton->setIcon(QIcon(ZKanji::dictionaryFlag(ui->jpButton->iconSize(), QString(), Flags::Browse)));
}

void DictionaryWidget::dictionaryReplaced(Dictionary *olddict, Dictionary *newdict, int /*index*/)
{
    if (dict == olddict)
        setDictionary(newdict);
}

CommandCategories DictionaryWidget::activeCategory() const
{
    if (!isVisibleTo(window()) || dynamic_cast<ZKanjiForm*>(window()) == nullptr)
        return CommandCategories::NoCateg;

    const QWidget *w = parentWidget();
    while (w != nullptr && dynamic_cast<const ZKanjiWidget*>(w) == nullptr)
        w = w->parentWidget();

    if (w == nullptr)
        return CommandCategories::NoCateg;

    ZKanjiWidget *zw = ((ZKanjiWidget*)w);
    if (zw->isActiveWidget())
        return (zw->mode() == ViewModes::WordSearch ? CommandCategories::SearchCateg : CommandCategories::GroupCateg);

    QList<ZKanjiWidget*> wlist = ((ZKanjiForm*)window())->findChildren<ZKanjiWidget*>();
    for (ZKanjiWidget *lw : wlist)
    {
        if (lw == zw || lw->window() != window())
            continue;
        else if (lw->mode() == zw->mode())
            return CommandCategories::NoCateg;
    }

    return (zw->mode() == ViewModes::WordSearch ? CommandCategories::SearchCateg : CommandCategories::GroupCateg);
}

void DictionaryWidget::on_jpBeforeButton_clicked(bool /*checked*/)
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleAnyStart, categ), ui->jpBeforeButton->isChecked());

    updateWords();
}

void DictionaryWidget::on_jpAfterButton_clicked(bool /*checked*/)
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleAnyEnd, categ), ui->jpAfterButton->isChecked());

    updateWords();
}

void DictionaryWidget::on_enAfterButton_clicked(bool /*checked*/)
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleAnyEnd, categ), ui->enAfterButton->isChecked());

    updateWords();
}

void DictionaryWidget::on_jpStrictButton_clicked(bool /*checked*/)
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleStrict, categ), ui->jpStrictButton->isChecked());

    updateWords();
}

void DictionaryWidget::on_enStrictButton_clicked(bool /*checked*/)
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleStrict, categ), ui->enStrictButton->isChecked());

    updateWords();
}

void DictionaryWidget::on_examplesButton_clicked(bool /*checked*/)
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleExamples, categ), ui->examplesButton->isChecked());
    ui->examples->setVisible(ui->examplesButton->isChecked());
}

void DictionaryWidget::on_inflButton_clicked(bool /*checked*/)
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleDeinflect, categ), ui->inflButton->isChecked());

    updateWords();
}

void DictionaryWidget::on_filterButton_clicked(bool /*checked*/)
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleFilter, categ), ui->filterButton->isChecked());

    updateWords();
}

void DictionaryWidget::installCommands()
{
    if (dynamic_cast<ZKanjiForm*>(window()) != nullptr)
        return;

    delete commandmap;
    commandmap = new QSignalMapper(this);
    connect(commandmap, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, &DictionaryWidget::executeCommand);

    addCommandShortcut(Qt::Key_F2, makeCommand(Commands::FromJapanese));
    addCommandShortcut(Qt::Key_F3, makeCommand(Commands::ToJapanese));
    addCommandShortcut(Qt::Key_F4, makeCommand(Commands::BrowseJapanese));
    addCommandShortcut(Qt::Key_F5, makeCommand(Commands::ToggleExamples));
    addCommandShortcut(Qt::Key_F6, makeCommand(Commands::ToggleAnyStart));
    addCommandShortcut(Qt::Key_F7, makeCommand(Commands::ToggleAnyEnd));
    addCommandShortcut(Qt::Key_F8, makeCommand(Commands::ToggleDeinflect));
    addCommandShortcut(Qt::Key_F9, makeCommand(Commands::ToggleStrict));
    addCommandShortcut(QKeySequence(tr("Ctrl+F")), makeCommand(Commands::ToggleFilter));
    addCommandShortcut(QKeySequence(tr("Ctrl+Shift+F")), makeCommand(Commands::EditFilters));
    addCommandShortcut(Qt::Key_F10, makeCommand(Commands::ToggleMultiline));
}

void DictionaryWidget::addCommandShortcut(const QKeySequence &keyseq, int command)
{
    QAction *a = new QAction(this);
    a->setShortcut(keyseq);
    //a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, commandmap, (void (QSignalMapper::*)())&QSignalMapper::map);
    commandmap->setMapping(a, command);
    addAction(a);
}

void DictionaryWidget::updateSearchMode()
{
    CommandCategories categ = activeCategory();

    if (ui->browseButton->isChecked())
    {
#ifdef _DEBUG
        if (listmode != DictSearch)
            throw "Bug in the program. The browse button should not be accessible.";
#endif

        mode = SearchMode::Browse;
        ui->searchStack->setCurrentIndex(2);

        if (searchmodel)
            searchmodel->deleteLater();
        searchmodel.release();

        model = nullptr;

        if (categ != CommandCategories::NoCateg)
        {
            ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::BrowseJapanese, categ));
            ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleAnyStart, categ), false);
            ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleAnyEnd, categ), false);
            ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleStrict, categ), false);
            ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleDeinflect, categ), false);
        }
    }
    else
    {
        if (browsemodel && model == browsemodel.get())
            model = nullptr;

        //if (!searchmodel)
        //    searchmodel.reset(new DictionarySearchResultItemModel(this));
        //model = searchmodel.get();

        //setTableModel(model);

        if (ui->jpButton->isChecked())
        {
            mode = SearchMode::Japanese;
            ui->searchStack->setCurrentIndex(0);

            if (categ != CommandCategories::NoCateg)
            {
                ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::FromJapanese, categ));
                ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleAnyStart, categ), true);
                ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleAnyEnd, categ), true);
                ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleStrict, categ), true);
                ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleDeinflect, categ), true);
            }
        }
        else if (ui->enButton->isChecked())
        {
            mode = SearchMode::Definition;
            ui->searchStack->setCurrentIndex(1);

            if (categ != CommandCategories::NoCateg)
            {
                ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToJapanese, categ));
                ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleAnyStart, categ), false);
                ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleAnyEnd, categ), true);
                ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleStrict, categ), true);
                ((ZKanjiForm*)window())->enableCommand(makeCommand(Commands::ToggleDeinflect, categ), false);
            }
        }
    }

    updateWords();
}

void DictionaryWidget::updateWords()
{
    if (listmode == DictSearch && dict == nullptr)
    {
        if (browsemodel)
            browsemodel->deleteLater();
        browsemodel.release();
        if (searchmodel)
            searchmodel->deleteLater();
        searchmodel.release();
        if (filtermodel)
            filtermodel->deleteLater();
        filtermodel.release();
        sortfunc = nullptr;

        model = nullptr;

        setTableModel(nullptr);

        updatepending = false;
        return;
    }

    if (!isVisible() && !updateforced)
    {
        updatepending = true;
        return;
    }

    updateforced = false;
    updatepending = false;

    SearchWildcards wildcards = 0;
    bool strict = false;
    if (mode == SearchMode::Japanese)
    {
        if (ui->jpBeforeButton->isChecked())
            wildcards |= SearchWildcard::AnyBefore;
        if (ui->jpAfterButton->isChecked())
            wildcards |= SearchWildcard::AnyAfter;
        strict = ui->jpStrictButton->isChecked();
    }
    else if (mode == SearchMode::Definition)
    {
        if (ui->enAfterButton->isChecked())
            wildcards |= SearchWildcard::AnyAfter;
        strict = ui->enStrictButton->isChecked();
    }

    //WordResultList result(dictionary());

    if (listmode == DictSearch)
    {
        switch (mode)
        {
        case SearchMode::Browse:
        {
            //if (model != nullptr && model != browsemodel)
            //    model->deleteLater();

            if (browsemodel && browsemodel->browseOrder() != browseorder)
            {
                browsemodel->deleteLater();
                browsemodel.release();
            }

            if (!browsemodel)
                browsemodel.reset(new DictionaryBrowseItemModel(dictionary(), browseorder, this));

            browsemodel->setFilterConditions(ui->filterButton->isChecked() ? conditions.get() : nullptr);

            model = browsemodel.get();

            setTableModel(model);

            int browserow = browsemodel->browseRow(searchText());
            if (ui->wordsTable->isMultiLine() && browserow != -1)
                browserow = ui->wordsTable->multiModel()->mapFromSourceRow(browserow);
            if (browserow != -1)
            {
                ui->wordsTable->scrollToRow(browserow, QAbstractItemView::PositionAtTop);
                ui->wordsTable->setCurrentRow(browserow);
            }
            break;
        }
        case SearchMode::Japanese:
        case SearchMode::Definition:
            if (!searchmodel)
                searchmodel.reset(new DictionarySearchResultItemModel(this));
            model = searchmodel.get();

#ifdef _DEBUG
            try
            {
#endif
            searchmodel->search(mode, dictionary(), searchText(), wildcards, strict, ui->inflButton->isChecked(), isStudyDefinitionUsed(), ui->filterButton->isChecked() ? conditions.get() : nullptr);
#ifdef _DEBUG
            }
            catch (...)
            {
                // Just for testing.
                QString txt = searchText();

            }
#endif

            setTableModel(model);
            break;
        }
    }
    else //if (listmode == Filtered)
    {
        if (model == nullptr)
        {
            setTableModel(nullptr);
            return;
        }

        if (filtermodel != nullptr)
        {
            filtermodel->deleteLater();
            filtermodel.release();
        }

        if (!searchText().isEmpty() || !conditionsEmpty() || sortfunc)
        {
            filtermodel.reset(new DictionarySearchFilterProxyModel(this));
            filtermodel->setSourceModel(model);

            if (!searchText().isEmpty() || !conditionsEmpty())
                filtermodel->filter(mode, searchText(), wildcards, strict, ui->inflButton->isChecked(), isStudyDefinitionUsed(), ui->filterButton->isChecked() ? conditions.get() : nullptr);
            if (sortfunc)
                filtermodel->sortBy(ui->wordsTable->horizontalHeader()->sortIndicatorSection(), ui->wordsTable->horizontalHeader()->sortIndicatorOrder(), sortfunc);

            setTableModel(filtermodel.get());
        }
        else
            setTableModel(model);
    }
}

void DictionaryWidget::updateMultiline()
{
    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ToggleMultiline, categ), ui->multilineButton->isChecked());

    if (ui->multilineButton->isChecked() == ui->wordsTable->isMultiLine())
        return;

    ui->wordsTable->setMultiLine(ui->multilineButton->isChecked());
}

void DictionaryWidget::setTableModel(ZAbstractTableModel *newmodel)
{
    if (ui->wordsTable->model() == newmodel)
        return;

    ui->wordsTable->setModel(newmodel);
}

bool DictionaryWidget::conditionsEmpty() const
{
    if (!ui->filterButton->isChecked())
        return true;

    return !conditions || !*conditions;
}

void DictionaryWidget::storeLastSearch(ZComboBox *box)
{
    historytimer.stop();

    QString str = box->text();
    if (str.isEmpty())
        return;
    if (box->validator() != nullptr)
    {
        int p = 0;
        box->validator()->validate(str, p);
        box->validator()->fixup(str);
    }

    int pos = box->findData(str, Qt::DisplayRole);
    if (pos == 0)
        return;

    QStringListModel *m = (QStringListModel*)box->model();
    if (pos != -1)
        m->removeRows(pos, 1);
    m->insertRows(0, 1);
    m->setData(m->index(0), str, Qt::DisplayRole);

    if (m->rowCount() > Settings::dictionary.historylimit)
        m->removeRows(Settings::dictionary.historylimit, m->rowCount() - Settings::dictionary.historylimit);

    box->view()->setCurrentIndex(box->view()->model()->index(0, 0));
    box->setCurrentIndex(0);
}


//-------------------------------------------------------------


