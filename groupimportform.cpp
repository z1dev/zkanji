/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "groupimportform.h"
#include "ui_groupimportform.h"
#include "words.h"
#include "groups.h"
#include "zdictionariesmodel.h"
#include "zgrouptreemodel.h"


//-------------------------------------------------------------


GroupImportForm::GroupImportForm(QWidget *parent) : base(parent), ui(new Ui::GroupImportForm), importbutton(nullptr)
{
    ui->setupUi(this);

    importbutton = ui->buttonBox->button(QDialogButtonBox::Ok);
    importbutton->setText(tr("Import"));
    importbutton->setEnabled(false);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &GroupImportForm::closeOk);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &GroupImportForm::closeCancel);

    ui->kanjiWidget->setMode(GroupWidget::Kanji);
    ui->wordsWidget->setMode(GroupWidget::Words);

    ui->kanjiWidget->showOnlyCategories();
    ui->wordsWidget->showOnlyCategories();
    ui->kanjiWidget->setMultiSelect(false);
    ui->wordsWidget->setMultiSelect(false);
}

GroupImportForm::~GroupImportForm()
{
    delete ui;
}

bool GroupImportForm::exec(Dictionary *dict)
{
    ui->dictCBox->setModel(ZKanji::dictionariesModel());
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(dict)));
    on_dictCBox_currentIndexChanged(ui->dictCBox->currentIndex());

    showModal();

    return modalResult() == ModalResult::Ok;
}

Dictionary* GroupImportForm::dictionary() const
{
    return ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));
}

KanjiGroupCategory* GroupImportForm::kanjiCategory() const
{
    return ui->kanjiGroupsBox->isChecked() ? (KanjiGroupCategory*)ui->kanjiWidget->current() : nullptr;
}

WordGroupCategory* GroupImportForm::wordsCategory() const
{
    return ui->wordGroupsBox->isChecked() ? (WordGroupCategory*)ui->wordsWidget->current() : nullptr;
}

bool GroupImportForm::importKanjiExamples() const
{
    return ui->kanjiBox->isChecked();
}

bool GroupImportForm::importWordMeanings() const
{
    return ui->wordsBox->isChecked();
}

void GroupImportForm::selectionChanged()
{
    bool sel = ui->kanjiWidget->current() != nullptr && ui->wordsWidget->current() != nullptr;
    importbutton->setEnabled(sel);
}

void GroupImportForm::on_dictCBox_currentIndexChanged(int ix)
{
    ui->kanjiWidget->setDictionary(ZKanji::dictionary(ZKanji::dictionaryPosition(ix)));
    ui->wordsWidget->setDictionary(ZKanji::dictionary(ZKanji::dictionaryPosition(ix)));

    selectionChanged();
}

void GroupImportForm::on_kanjiGroupsBox_toggled(bool checked)
{
    ui->kanjiLabel->setEnabled(checked);
    ui->kanjiWidget->setEnabled(checked);
}

void GroupImportForm::on_wordGroupsBox_toggled(bool checked)
{
    ui->wordsLabel->setEnabled(checked);
    ui->wordsWidget->setEnabled(checked);
}



//-------------------------------------------------------------

