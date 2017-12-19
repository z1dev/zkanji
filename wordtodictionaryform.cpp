/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "wordtodictionaryform.h"
#include "ui_wordtodictionaryform.h"
#include "dialogs.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "wordeditorform.h"
#include "formstate.h"
#include "globalui.h"
#include "zdictionariesmodel.h"
#include "zui.h"


//-------------------------------------------------------------


WordToDictionaryForm::WordToDictionaryForm(QWidget *parent) : base(parent), ui(new Ui::WordToDictionaryForm), proxy(nullptr), dict(nullptr), dest(nullptr)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose);

    scaleWidget(this);

    ui->wordsTable->setSelectionType(ListSelectionType::Toggle);
    ui->meaningsTable->setSelectionType(ListSelectionType::None);

    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &WordToDictionaryForm::dictionaryToBeRemoved);
}

WordToDictionaryForm::~WordToDictionaryForm()
{
    delete ui;
}

void WordToDictionaryForm::exec(Dictionary *d, int windex)
{
    // Pre-compute window sizes.
    setAttribute(Qt::WA_DontShowOnScreen);
    show();
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    dict = d;
    index = windex;

    connect(dict, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);

    // Set fixed width to the add button.

    QFontMetrics mcs = ui->addButton->fontMetrics();
    int dif = std::abs(mcs.boundingRect(tr("Create word")).width() - mcs.boundingRect(tr("Add meanings")).width());
    ui->addButton->setMinimumWidth(ui->addButton->width() + dif);
    ui->addButton->setMaximumWidth(ui->addButton->minimumWidth());

    // Show the original word on top.

    DictionaryDefinitionListItemModel *model = new DictionaryDefinitionListItemModel(this);
    model->setWord(dict, windex);
    ui->wordsTable->setModel(model);

    connect(ui->wordsTable, &ZDictionaryListView::rowSelectionChanged, this, &WordToDictionaryForm::wordSelChanged);

    proxy = new DictionariesProxyModel(this);
    proxy->setDictionary(dict);
    ui->dictCBox->setModel(proxy);
    ui->dictCBox->setCurrentIndex(0);

    FormStates::restoreDialogSplitterState("WordToDictionary", this, ui->splitter);

    show();
}

void WordToDictionaryForm::closeEvent(QCloseEvent *e)
{
    FormStates::saveDialogSplitterState("WordToDictionary", this, ui->splitter);

    base::closeEvent(e);
}

void WordToDictionaryForm::on_addButton_clicked()
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dict;
    int windex = index;

    std::vector<int> wdindexes;
    ui->wordsTable->selectedRows(wdindexes);
    std::sort(wdindexes.begin(), wdindexes.end());

    close();

    // Creates and opens the word editor form to add new definition to the destination word.
    editWord(d, windex, wdindexes, dest, dindex, parentWidget());
}

void WordToDictionaryForm::on_cancelButton_clicked()
{
    close();
}

void WordToDictionaryForm::on_switchButton_clicked()
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dict;
    int windex = index;

    close();

    wordToGroupSelect(d, windex/*, parentWidget()*/);
}

void WordToDictionaryForm::on_dictCBox_currentIndexChanged(int ix)
{
    if (dest != nullptr)
        disconnect(dest, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);
    dest = proxy->dictionaryAtRow(ix);
    if (dest != nullptr)
        connect(dest, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);
    dindex = dest->findKanjiKanaWord(dict->wordEntry(index));

    int h = ui->meaningsWidget->frameGeometry().height() + centralWidget()->layout()->spacing();

    if (dindex == -1)
    {
        if (ui->meaningsWidget->isVisibleTo(this))
        {
            QSize s = geometry().size();
            ui->meaningsWidget->hide();

            centralWidget()->layout()->activate();
            layout()->activate();

            s.setHeight(s.height() - h);
            resize(s);
        }
        ui->addButton->setText(tr("Create word"));
    }
    else
    {
        // Show existing definitions in the bottom table.
        //MultiLineDictionaryItemModel *mmodel = new MultiLineDictionaryItemModel(ui->meaningsTable);
        DictionaryWordListItemModel *model = new DictionaryWordListItemModel(this);
        std::vector<int> result;
        result.push_back(dindex);
        model->setWordList(dest, std::move(result));
        //mmodel->setSourceModel(model);
        if (ui->meaningsTable->model() != nullptr)
            ui->meaningsTable->model()->deleteLater();
        ui->meaningsTable->setMultiLine(true);
        ui->meaningsTable->setModel(/*m*/model);

        ui->addButton->setText(tr("Add meanings"));

        if (!ui->meaningsWidget->isVisibleTo(this))
        {
            QSize s = geometry().size();
            s.setHeight(s.height() + h);
            resize(s);
            ui->meaningsWidget->show();
        }
    }

}

void WordToDictionaryForm::on_meaningsTable_wordDoubleClicked(int windex, int dindex)
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dest;

    close();

    // Create and open the word editor in simple edit mode to edit double clicked definition
    // of word in the destination dictionary. This method ignores the source dictionary.
    editWord(d, windex, dindex, parentWidget());
}

void WordToDictionaryForm::wordSelChanged(/*const QItemSelection &selected, const QItemSelection &deselected*/)
{
    ui->addButton->setEnabled(ui->wordsTable->hasSelection());
}

void WordToDictionaryForm::dictionaryToBeRemoved(int index, int orderindex, Dictionary *d)
{
    if (d == dict || ZKanji::dictionaryCount() == 1)
        close();
}


//-------------------------------------------------------------

