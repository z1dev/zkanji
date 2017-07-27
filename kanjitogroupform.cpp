/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "kanjitogroupform.h"
#include "ui_kanjitogroupform.h"
#include "groups.h"
#include "dialogsettings.h"
#include "zkanjigridmodel.h"
#include "words.h"
#include "globalui.h"
#include "zdictionariesmodel.h"
#include "formstate.h"

//-------------------------------------------------------------


KanjiToGroupForm::KanjiToGroupForm(QWidget *parent) : base(parent), ui(new Ui::KanjiToGroupForm)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->groupWidget->setMultiSelect(true);
    connect(ui->groupWidget, &GroupWidget::selectionChanged, this, &KanjiToGroupForm::selChanged);
    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &KanjiToGroupForm::dictionaryToBeRemoved);

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &KanjiToGroupForm::close);
    acceptButton = ui->buttonBox->addButton(tr("Add to group"), QDialogButtonBox::AcceptRole);
    acceptButton->setDefault(true);
    acceptButton->setEnabled(false);
    connect(acceptButton, &QPushButton::clicked, this, &KanjiToGroupForm::accept);

    //ui->groupWidget->addControlWidget(new QLabel(tr("Destination:"), this));
    dictCBox = new QComboBox(this);
    ui->groupWidget->addControlWidget(dictCBox);

    dictCBox->setModel(ZKanji::dictionariesModel());
    connect(dictCBox, (void(QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &KanjiToGroupForm::setDictionary);
}

KanjiToGroupForm::~KanjiToGroupForm()
{
    delete ui;
}

void KanjiToGroupForm::exec(Dictionary *d, ushort kindex, GroupBase *initialSelection)
{
    dict = d;

    ui->groupWidget->setMode(GroupWidget::Kanji);
    ui->groupWidget->setDictionary(d);
    model = new KanjiListModel(this);
    model->setList({ kindex });

    dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(d)));

    ui->kanjiGrid->setModel(model);

    if (initialSelection != nullptr)
        ui->groupWidget->selectGroup(initialSelection);

    FormStates::restoreDialogSplitterState("KanjiToGroup", this, ui->splitter);

    show();
}

void KanjiToGroupForm::exec(Dictionary *d, const std::vector<ushort> kindexes, GroupBase *initialSelection)
{
    dict = d;

    ui->groupWidget->setMode(GroupWidget::Kanji);
    ui->groupWidget->setDictionary(d);
    model = new KanjiListModel(this);
    model->setList(kindexes);

    dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(d)));

    ui->kanjiGrid->setModel(model);

    if (initialSelection != nullptr)
        ui->groupWidget->selectGroup(initialSelection);

    FormStates::restoreDialogSplitterState("KanjiToGroup", this, ui->splitter);

    show();
}

void KanjiToGroupForm::closeEvent(QCloseEvent *e)
{
    FormStates::saveDialogSplitterState("KanjiToGroup", this, ui->splitter);

    base::closeEvent(e);
}

void KanjiToGroupForm::accept()
{
    KanjiGroup *g = ((KanjiGroup*)ui->groupWidget->singleSelected(false));
    g->add(model->getList());
    Settings::dialog.defkanjigroup = g != nullptr ? g->fullEncodedName() : QString();
    close();
}

void KanjiToGroupForm::selChanged()
{
    acceptButton->setEnabled(ui->groupWidget->singleSelected(false) != nullptr);
}

void KanjiToGroupForm::setDictionary(int index)
{
    ui->groupWidget->setDictionary(ZKanji::dictionary(ZKanji::dictionaryPosition(index)));
}

void KanjiToGroupForm::dictionaryToBeRemoved(int index, int orderindex, Dictionary *d)
{
    if (d == dict)
        close();
}


//-------------------------------------------------------------


void kanjiToGroupSelect(Dictionary *d, ushort kindex, QWidget *dialogParent)
{
    KanjiToGroupForm *form = new KanjiToGroupForm(dialogParent);
    form->exec(d, kindex, d->kanjiGroups().groupFromEncodedName(Settings::dialog.defkanjigroup));
}

void kanjiToGroupSelect(Dictionary *d, const std::vector<ushort> kindexes, QWidget *dialogParent)
{
    KanjiToGroupForm *form = new KanjiToGroupForm(dialogParent);
    form->exec(d, kindexes, d->kanjiGroups().groupFromEncodedName(Settings::dialog.defkanjigroup));
}


//-------------------------------------------------------------
