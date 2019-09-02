/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
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
#include "formstates.h"
#include "zui.h"
#include "zdictionarycombobox.h"


//-------------------------------------------------------------


KanjiToGroupForm::KanjiToGroupForm(QWidget *parent) : base(parent), ui(new Ui::KanjiToGroupForm)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->groupWidget->setMultiSelect(true);
    connect(ui->groupWidget, &GroupWidget::selectionChanged, this, &KanjiToGroupForm::selChanged);
    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &KanjiToGroupForm::dictionaryToBeRemoved);
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &KanjiToGroupForm::dictionaryReplaced);

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &KanjiToGroupForm::close);
    acceptButton = ui->buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    acceptButton->setDefault(true);
    acceptButton->setEnabled(false);
    connect(acceptButton, &QPushButton::clicked, this, &KanjiToGroupForm::accept);

    translateTexts();

    //dictCBox = new ZDictionaryComboBox(this);
    //ui->groupWidget->addControlWidget(dictCBox, true);

    //connect(ui->dictCBox, (void(QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &KanjiToGroupForm::setDictionary);

    ui->kanjiGrid->assignStatusBar(ui->kanjiStatus);
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

    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(d)));

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

    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(d)));

    ui->kanjiGrid->setModel(model);

    if (initialSelection != nullptr)
        ui->groupWidget->selectGroup(initialSelection);

    FormStates::restoreDialogSplitterState("KanjiToGroup", this, ui->splitter);

    show();
}

bool KanjiToGroupForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateTexts();
    }

    return base::event(e);
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
    dict->kanjiGroups().setLastSelected(g);
    close();
}

void KanjiToGroupForm::selChanged()
{
    acceptButton->setEnabled(ui->groupWidget->singleSelected(false) != nullptr);
}

void KanjiToGroupForm::on_dictCBox_currentIndexChanged(int index)
{
    ui->groupWidget->setDictionary(ZKanji::dictionary(ZKanji::dictionaryPosition(index)));
}

void KanjiToGroupForm::dictionaryToBeRemoved(int /*index*/, int /*orderindex*/, Dictionary *d)
{
    if (dict == d)
        close();
}

void KanjiToGroupForm::dictionaryReplaced(Dictionary *olddict, Dictionary* /*newdict*/, int /*index*/)
{
    if (dict == olddict)
        close();
}

void KanjiToGroupForm::translateTexts()
{
    acceptButton->setText(tr("Add to group"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}


//-------------------------------------------------------------


void kanjiToGroupSelect(Dictionary *d, ushort kindex, bool showmodal/*, QWidget *dialogParent*/)
{
    KanjiToGroupForm *form = new KanjiToGroupForm(gUI->activeMainForm() /*dialogParent*/);
    if (showmodal)
        form->setWindowModality(Qt::ApplicationModal);
    form->exec(d, kindex, d->kanjiGroups().lastSelected());
}

void kanjiToGroupSelect(Dictionary *d, const std::vector<ushort> kindexes, bool showmodal/*, QWidget *dialogParent*/)
{
    KanjiToGroupForm *form = new KanjiToGroupForm(gUI->activeMainForm() /*dialogParent*/);
    if (showmodal)
        form->setWindowModality(Qt::ApplicationModal);
    form->exec(d, kindexes, d->kanjiGroups().lastSelected());
}


//-------------------------------------------------------------
