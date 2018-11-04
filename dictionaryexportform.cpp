/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include <QFileDialog>
#include <QSet>
#include "dictionaryexportform.h"
#include "ui_dictionaryexportform.h"
#include "zgrouptreemodel.h"
#include "words.h"
#include "groups.h"
#include "globalui.h"
#include "zui.h"
#include "formstates.h"


//-------------------------------------------------------------


DictionaryExportForm::DictionaryExportForm(QWidget *parent) : base(parent), ui(new Ui::DictionaryExportForm), kanjimodel(nullptr), wordsmodel(nullptr)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    exportbutton = ui->buttonBox->button(QDialogButtonBox::Ok);
    exportbutton->setText(tr("Export"));
    exportbutton->setEnabled(false);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &DictionaryExportForm::exportClicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DictionaryExportForm::closeCancel);
    translateTexts();

    FormStates::restoreDialogSize("DictionaryExport", this, true);
}

bool DictionaryExportForm::exec(Dictionary *dict)
{
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(dict)));
    if (kanjimodel == nullptr)
        on_dictCBox_currentIndexChanged(ui->dictCBox->currentIndex());

    showModal();

    return modalResult() == ModalResult::Ok;
}

DictionaryExportForm::~DictionaryExportForm()
{
    delete ui;
}

void DictionaryExportForm::exportClicked()
{
    QString fname = QFileDialog::getSaveFileName(gUI->activeMainForm(), tr("Save export file"), QString(), QString("%1 (*.zkanji.dictionary)").arg(tr("Dictionary export file")));
    if (fname.isEmpty())
        closeCancel();

    std::vector<ushort> klist;
    std::vector<int> wlist;
    if (ui->groupBox->isChecked())
    {
        std::vector<ushort> kanji;
        std::vector<int> words;

        QSet<ushort> kfound;

        QSet<GroupBase*> sel;
        kanjimodel->checkedGroups(sel);

        for (GroupBase *g : sel)
        {
            if (g->isCategory())
                continue;
            KanjiGroup *kg = (KanjiGroup*)g;

            auto &list = kg->getIndexes();
            for (ushort val : list)
                kfound.insert(val);
        }

        klist.reserve(kfound.size());
        for (ushort val : kfound)
            klist.push_back(val);

        QSet<int> wfound;
        wordsmodel->checkedGroups(sel);

        for (GroupBase *g : sel)
        {
            if (g->isCategory())
                continue;
            WordGroup *wg = (WordGroup*)g;

            auto &list = wg->getIndexes();
            for (int val : list)
                wfound.insert(val);
        }

        wlist.reserve(wfound.size());
        for (int val : wfound)
            wlist.push_back(val);
    }

    Dictionary *dict = ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));
    dict->exportDictionary(fname, ui->groupBox->isChecked(), klist, wlist);

    closeOk();
}

void DictionaryExportForm::modelDataChanged()
{
    bool sel = !ui->groupBox->isChecked() || kanjimodel->hasChecked() || wordsmodel->hasChecked();
    exportbutton->setEnabled(sel);
}

void DictionaryExportForm::on_dictCBox_currentIndexChanged(int ix)
{
    if (kanjimodel != nullptr)
        kanjimodel->deleteLater();
    if (wordsmodel != nullptr)
        wordsmodel->deleteLater();

    Dictionary *dict = ZKanji::dictionary(ZKanji::dictionaryPosition(ix));
    kanjimodel = new CheckedGroupTreeModel(&dict->kanjiGroups(), false, false, ui->kanjiTree);
    wordsmodel = new CheckedGroupTreeModel(&dict->wordGroups(), false, false, ui->wordsTree);

    ui->kanjiTree->setModel(kanjimodel);
    connect(kanjimodel, (void(CheckedGroupTreeModel::*)(const TreeItem *, const TreeItem *, const QVector<int> &))&CheckedGroupTreeModel::dataChanged, this, &DictionaryExportForm::modelDataChanged);
    ui->wordsTree->setModel(wordsmodel);
    connect(wordsmodel, (void(CheckedGroupTreeModel::*)(const TreeItem *, const TreeItem *, const QVector<int> &))&CheckedGroupTreeModel::dataChanged, this, &DictionaryExportForm::modelDataChanged);
    ui->kanjiTree->expandAll();
    ui->wordsTree->expandAll();

    modelDataChanged();
}

void DictionaryExportForm::on_groupBox_toggled(bool /*on*/)
{
    modelDataChanged();
}

void DictionaryExportForm::translateTexts()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(qApp->translate("ButtonBox", "OK"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}


//-------------------------------------------------------------

