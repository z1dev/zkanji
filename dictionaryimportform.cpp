/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "dictionaryimportform.h"
#include "ui_dictionaryimportform.h"
#include "words.h"
#include "globalui.h"
#include "formstates.h"

//-------------------------------------------------------------


DictionaryImportForm::DictionaryImportForm(QWidget *parent) : base(parent), ui(new Ui::DictionaryImportForm)
{
    ui->setupUi(this);

    connect(ui->fullOption, &QRadioButton::toggled, this, &DictionaryImportForm::optionChanged);
    connect(ui->partOption, &QRadioButton::toggled, this, &DictionaryImportForm::optionChanged);

    gUI->scaleWidget(this);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &DictionaryImportForm::closeOk);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DictionaryImportForm::closeCancel);

    //connect(ui->dictCBox, &QComboBox::currentTextChanged, this, &DictionaryImportForm::dictTextChanged);
    connect(ui->kanjiBox, &QCheckBox::toggled, this, &DictionaryImportForm::optionChanged);
    connect(ui->wordBox, &QCheckBox::toggled, this, &DictionaryImportForm::optionChanged);

    ui->kanjiGroups->setMode(GroupWidget::Kanji);
    ui->wordGroups->setMode(GroupWidget::Words);

    ui->kanjiGroups->setMultiSelect(false);
    ui->wordGroups->setMultiSelect(false);

    ui->kanjiBox->setEnabled(false);
    ui->wordBox->setEnabled(false);
    ui->kanjiGroups->setEnabled(false);
    ui->wordGroups->setEnabled(false);

    FormStates::restoreDialogSize("DictionaryImport", this, true);
}

DictionaryImportForm::~DictionaryImportForm()
{
    delete ui;
}

bool DictionaryImportForm::exec(Dictionary *dict)
{
    dictmodel = new DictionariesProxyModel(this);
    dictmodel->setDictionary(ZKanji::dictionary(0));

    ui->dictCBox->setModel(dictmodel);
    ui->dictCBox->setCurrentIndex(dict != ZKanji::dictionary(0) ? ZKanji::dictionaryOrderIndex(dict) - 1 : 0);
    on_dictCBox_currentTextChanged(ui->dictCBox->currentText());

    showModal();
    return modalResult() == ModalResult::Ok;
}

QString DictionaryImportForm::dictionaryName() const
{
    return ui->dictCBox->currentText();
}

bool DictionaryImportForm::fullImport() const
{
    return ui->fullOption->isChecked();
}

WordGroup* DictionaryImportForm::selectedWordGroup() const
{
    if (ui->fullOption->isChecked() || !ui->wordBox->isChecked())
        return nullptr;

    return (WordGroup*)ui->wordGroups->singleSelected();
}

KanjiGroup* DictionaryImportForm::selectedKanjiGroup() const
{
    if (ui->fullOption->isChecked() || !ui->kanjiBox->isChecked())
        return nullptr;

    return (KanjiGroup*)ui->kanjiGroups->singleSelected();
}

void DictionaryImportForm::optionChanged()
{
    int index = ui->dictCBox->currentIndex();
    if (index != -1 && ui->dictCBox->currentText() != ui->dictCBox->itemText(index))
        index = -1;

    ui->kanjiBox->setEnabled(index != -1 && ui->partOption->isChecked());
    ui->wordBox->setEnabled(index != -1 && ui->partOption->isChecked());
    ui->kanjiGroups->setEnabled(index != -1 && ui->partOption->isChecked() && ui->kanjiBox->isChecked());
    ui->wordGroups->setEnabled(index != -1 && ui->partOption->isChecked() && ui->wordBox->isChecked());
}

void DictionaryImportForm::on_dictCBox_currentTextChanged(const QString &text)
{
    int index = ui->dictCBox->currentIndex();
    if (index != -1 && text != ui->dictCBox->itemText(index))
        index = -1;

    Dictionary *dict = index >= 0 ? dictmodel->dictionaryAtRow(index) : nullptr;

    ui->kanjiGroups->setDictionary(dict);
    ui->wordGroups->setDictionary(dict);

    ui->partOption->setEnabled(dict != nullptr);
    optionChanged();
    if (dict == nullptr)
        ui->fullOption->setChecked(true);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ZKanji::dictionaryNameValid(text, true) && text.toLower() != ZKanji::dictionary(0)->name().toLower());
}

void DictionaryImportForm::translateTexts()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(qApp->translate("ButtonBox", "Next"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}


//-------------------------------------------------------------

