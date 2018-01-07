/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "wordtogroupform.h"
#include "ui_wordtogroupform.h"
#include "groups.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "wordtodictionaryform.h"
#include "dialogsettings.h"
#include "formstate.h"
#include "zlistview.h"
#include "globalui.h"
#include "zui.h"


//-------------------------------------------------------------


WordToGroupForm::WordToGroupForm(QWidget *parent) : base(parent), ui(new Ui::WordToGroupForm)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->wordsTable->setSelectionType(ListSelectionType::None);
    ui->groupWidget->setMultiSelect(true);

    connect(ui->groupWidget, &GroupWidget::selectionChanged, this, &WordToGroupForm::selChanged);
    connect(gUI, &GlobalUI::dictionaryAdded, this, &WordToGroupForm::dictionaryAdded);
    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &WordToGroupForm::dictionaryToBeRemoved);

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &WordToGroupForm::close);
    acceptButton = ui->buttonBox->addButton(tr("Add to group"), QDialogButtonBox::AcceptRole);
    acceptButton->setDefault(true);
    acceptButton->setEnabled(false);
    connect(acceptButton, &QPushButton::clicked, this, &WordToGroupForm::accept);
}

WordToGroupForm::~WordToGroupForm()
{
    delete ui;
}

void WordToGroupForm::exec(Dictionary *d, int windex, GroupBase *initialSelection)
{
    ui->switchButton->setVisible(ZKanji::dictionaryCount() > 1);

    dict = d;
    connect(dict, &Dictionary::dictionaryReset, this, &WordToGroupForm::close);
    list.push_back(windex);

    ui->groupWidget->setDictionary(d);

    DictionaryWordListItemModel *model = new DictionaryWordListItemModel(this);

    model->setWordList(dict, list);

    if (ui->wordsTable->model() != nullptr)
        ui->wordsTable->model()->deleteLater();

    ui->wordsTable->setMultiLine(true);
    ui->wordsTable->setModel(model);

    if (ZKanji::dictionaryCount() == 1)
        ui->switchButton->hide();

    if (initialSelection != nullptr)
        ui->groupWidget->selectGroup(initialSelection);

    FormStates::restoreDialogSplitterState("WordToGroup", this, ui->splitter);

    show();
}

void WordToGroupForm::exec(Dictionary *d, const std::vector<int> &indexes, GroupBase *initialSelection)
{
    if (indexes.empty())
        return;

    dict = d;
    connect(dict, &Dictionary::dictionaryReset, this, &WordToGroupForm::close);
    list = indexes;

    ui->switchButton->setVisible(ZKanji::dictionaryCount() > 1 && list.size() == 1);

    ui->groupWidget->setDictionary(d);

    DictionaryWordListItemModel *model = new DictionaryWordListItemModel(this);

    model->setWordList(dict, list);

    if (ui->wordsTable->model() != nullptr)
        ui->wordsTable->model()->deleteLater();

    ui->wordsTable->setMultiLine(list.size() == 1);
    ui->wordsTable->setModel(model);

    if (ZKanji::dictionaryCount() == 1)
        ui->switchButton->hide();

    if (initialSelection != nullptr)
        ui->groupWidget->selectGroup(initialSelection);

    FormStates::restoreDialogSplitterState("WordToGroup", this, ui->splitter);

    show();
}

void WordToGroupForm::closeEvent(QCloseEvent *e)
{
    FormStates::saveDialogSplitterState("WordToGroup", this, ui->splitter);

    base::closeEvent(e);
}

void WordToGroupForm::accept()
{
    WordGroup *g = ((WordGroup*)ui->groupWidget->singleSelected(false));
    g->add(list);
    dict->wordGroups().setLastSelected(g);

    close();
}

void WordToGroupForm::on_switchButton_clicked()
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dict;

    close();

    WordToDictionaryForm *form = new WordToDictionaryForm(parentWidget());
    form->exec(d, list.front());
}

void WordToGroupForm::selChanged()
{
    acceptButton->setEnabled(ui->groupWidget->singleSelected(false) != nullptr);
}

void WordToGroupForm::dictionaryAdded()
{
    if (list.size() == 1)
        ui->switchButton->setVisible(true);
}

void WordToGroupForm::dictionaryToBeRemoved(int index, int orderindex, Dictionary *d)
{
    if (d == dict)
        close();
    if (ZKanji::dictionaryCount() < 3)
        ui->switchButton->setVisible(false);
}


//-------------------------------------------------------------


// Declared in "dialogs.h"
void wordToGroupSelect(Dictionary *d, int windex, bool showmodal/*, QWidget *dialogParent*/)
{
    WordToGroupForm *form = new WordToGroupForm(gUI->activeMainForm() /*dialogParent*/);
    if (showmodal)
        form->setWindowModality(Qt::ApplicationModal);
    form->exec(d, windex, d->wordGroups().lastSelected());
}

void wordToGroupSelect(Dictionary *d, const std::vector<int> &indexes, bool showmodal/*, QWidget *dialogParent*/)
{
    WordToGroupForm *form = new WordToGroupForm(gUI->activeMainForm() /*dialogParent*/);
    if (showmodal)
        form->setWindowModality(Qt::ApplicationModal);
    form->exec(d, indexes, d->wordGroups().lastSelected());
}


//-------------------------------------------------------------
