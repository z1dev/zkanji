/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
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
#include "formstates.h"
#include "zlistview.h"
#include "globalui.h"
#include "zui.h"
#include "dialogs.h"


//-------------------------------------------------------------


WordToGroupForm::WordToGroupForm(QWidget *parent) : base(parent), ui(new Ui::WordToGroupForm), updatelast(false)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->buttonBox->addButton(ui->switchButton, QDialogButtonBox::ResetRole);

    gUI->scaleWidget(this);

    ui->wordsTable->setSelectionType(ListSelectionType::None);
    ui->groupWidget->setMultiSelect(true);

    connect(ui->groupWidget, &GroupWidget::selectionChanged, this, &WordToGroupForm::selChanged);
    connect(gUI, &GlobalUI::dictionaryAdded, this, &WordToGroupForm::dictionaryAdded);
    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &WordToGroupForm::dictionaryToBeRemoved);
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &WordToGroupForm::dictionaryReplaced);

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &WordToGroupForm::close);
    QPushButton *acceptButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    acceptButton->setDefault(true);
    acceptButton->setEnabled(false);
    connect(acceptButton, &QPushButton::clicked, this, &WordToGroupForm::accept);

    translateTexts();

    ui->wordsTable->assignStatusBar(ui->listStatus);
}

WordToGroupForm::~WordToGroupForm()
{
    delete ui;
}

void WordToGroupForm::exec(Dictionary *d, int windex, GroupBase *initialSelection, bool updatelastdiag)
{
    ui->switchButton->setVisible(ZKanji::dictionaryCount() > 1);

    dict = d;
    connect(dict, &Dictionary::dictionaryReset, this, &WordToGroupForm::close);
    list.push_back(windex);

    updatelast = updatelastdiag;
    if (updatelast)
        gUI->setLastWordToDialog(GlobalUI::LastWordDialog::Group);

    ui->groupWidget->setDictionary(d);

    DictionaryWordListItemModel *model = new DictionaryWordListItemModel(this);

    model->setWordList(dict, list);

    if (ui->wordsTable->model() != nullptr)
        ui->wordsTable->model()->deleteLater();

    ui->wordsTable->setMultiLine(true);
    ui->wordsTable->setModel(model);

    if (ZKanji::dictionaryCount() == 1 || !updatelast)
        ui->switchButton->hide();

    if (initialSelection != nullptr)
        ui->groupWidget->selectGroup(initialSelection);

    FormStates::restoreDialogSplitterState("WordToGroup", this, ui->splitter);

    show();
}

void WordToGroupForm::exec(Dictionary *d, const std::vector<int> &indexes, GroupBase *initialSelection, bool updatelastdiag)
{
    if (indexes.empty())
        return;

    dict = d;
    connect(dict, &Dictionary::dictionaryReset, this, &WordToGroupForm::close);
    list = indexes;

    updatelast = updatelastdiag;
    if (updatelast)
        gUI->setLastWordToDialog(GlobalUI::LastWordDialog::Group);

    ui->switchButton->setVisible(ZKanji::dictionaryCount() > 1 && list.size() == 1);

    ui->groupWidget->setDictionary(d);

    DictionaryWordListItemModel *model = new DictionaryWordListItemModel(this);

    model->setWordList(dict, list);

    if (ui->wordsTable->model() != nullptr)
        ui->wordsTable->model()->deleteLater();

    ui->wordsTable->setMultiLine(list.size() == 1);
    ui->wordsTable->setModel(model);

    if (ZKanji::dictionaryCount() == 1 || !updatelast)
        ui->switchButton->hide();

    if (initialSelection != nullptr)
        ui->groupWidget->selectGroup(initialSelection);

    FormStates::restoreDialogSplitterState("WordToGroup", this, ui->splitter);

    show();
}

bool WordToGroupForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateTexts();
    }

    return base::event(e);
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
    int windex = list.front();
    bool modal = windowModality() != Qt::NonModal;

    close();

    wordToDictionarySelect(d, windex, modal, updatelast);
}

void WordToGroupForm::selChanged()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ui->groupWidget->singleSelected(false) != nullptr);
}

void WordToGroupForm::dictionaryAdded()
{
    if (list.size() == 1 && updatelast)
        ui->switchButton->setVisible(true);
}

void WordToGroupForm::dictionaryToBeRemoved(int /*index*/, int /*orderindex*/, Dictionary *d)
{
    if (dict == d)
        close();
    if (ZKanji::dictionaryCount() < 3)
        ui->switchButton->setVisible(false);
}

void WordToGroupForm::dictionaryReplaced(Dictionary *olddict, Dictionary* /*newdict*/, int /*index*/)
{
    if (dict == olddict)
        close();
}

void WordToGroupForm::translateTexts()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Add to group"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}


//-------------------------------------------------------------


// Declared in "dialogs.h"
void wordToGroupSelect(Dictionary *d, int windex, bool showmodal, bool updatelastdiag/*, QWidget *dialogParent*/)
{
    WordToGroupForm *form = new WordToGroupForm(gUI->activeMainForm() /*dialogParent*/);
    if (showmodal)
        form->setWindowModality(Qt::ApplicationModal);
    form->exec(d, windex, d->wordGroups().lastSelected(), updatelastdiag);
}

void wordToGroupSelect(Dictionary *d, const std::vector<int> &indexes, bool showmodal, bool updatelastdiag/*, QWidget *dialogParent*/)
{
    WordToGroupForm *form = new WordToGroupForm(gUI->activeMainForm() /*dialogParent*/);
    if (showmodal)
        form->setWindowModality(Qt::ApplicationModal);
    form->exec(d, indexes, d->wordGroups().lastSelected(), updatelastdiag);
}


//-------------------------------------------------------------
