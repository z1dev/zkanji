/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFileDialog>

#include "ui_groupexportform.h"
#include "groupexportform.h"
#include "zgrouptreemodel.h"
#include "words.h"
#include "groups.h"
#include "globalui.h"
#include "formstates.h"


//-------------------------------------------------------------


GroupExportForm::GroupExportForm(QWidget *parent) : base(parent), ui(new Ui::GroupExportForm),  kanjimodel(nullptr), wordsmodel(nullptr)
{
    ui->setupUi(this);

    gUI->scaleWidget(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &GroupExportForm::exportClicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &GroupExportForm::closeCancel);

    translateTexts();

    FormStates::restoreDialogSize("GroupExport", this, true);
}

GroupExportForm::~GroupExportForm()
{
    delete ui;
}

bool GroupExportForm::exec(Dictionary *dict)
{
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(dict)));
    if (kanjimodel == nullptr)
        on_dictCBox_currentIndexChanged(ui->dictCBox->currentIndex());

    showModal();

    return modalResult() == ModalResult::Ok;
}

void GroupExportForm::exportClicked()
{
    QString fname = QFileDialog::getSaveFileName(nullptr, tr("Save export file"), QString(), QString("%1 (*.zkanji.export)").arg(tr("Export file")));
    if (fname.isEmpty())
        closeCancel();

    std::vector<KanjiGroup*> kanjilist;
    std::vector<WordGroup*> wordslist;

    QSet<GroupBase*> sel;
    kanjimodel->checkedGroups(sel);
    kanjilist.reserve(sel.size());
    for (auto it = sel.begin(); it != sel.end(); ++it)
    {
        if ((*it)->isCategory())
            continue;
        kanjilist.push_back((KanjiGroup*)*it);
    }

    wordsmodel->checkedGroups(sel);
    wordslist.reserve(sel.size());
    for (auto it = sel.begin(); it != sel.end(); ++it)
    {
        if ((*it)->isCategory())
            continue;
        wordslist.push_back((WordGroup*)*it);
    }

    Dictionary *dict = ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));
    dict->exportUserData(fname, kanjilist, ui->kanjiBox->isChecked(), wordslist, ui->wordsBox->isChecked());

    closeOk();
}

void GroupExportForm::modelDataChanged()
{
    if (kanjimodel == nullptr)
        return;

    bool sel = kanjimodel->hasChecked() || wordsmodel->hasChecked();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(sel);
}

void GroupExportForm::on_dictCBox_currentIndexChanged(int ix)
{
    if (kanjimodel != nullptr)
        kanjimodel->deleteLater();
    if (wordsmodel != nullptr)
        wordsmodel->deleteLater();

    Dictionary *dict = ZKanji::dictionary(ZKanji::dictionaryPosition(ix));
    kanjimodel = new CheckedGroupTreeModel(&dict->kanjiGroups(), false, false, ui->kanjiTree);
    wordsmodel = new CheckedGroupTreeModel(&dict->wordGroups(), false, false, ui->wordsTree);

    ui->kanjiTree->setModel(kanjimodel);
    connect(kanjimodel, (void(CheckedGroupTreeModel::*)(const TreeItem *, const TreeItem *, const QVector<int> &))&CheckedGroupTreeModel::dataChanged, this, &GroupExportForm::modelDataChanged);
    ui->wordsTree->setModel(wordsmodel);
    connect(wordsmodel, (void(CheckedGroupTreeModel::*)(const TreeItem *, const TreeItem *, const QVector<int> &))&CheckedGroupTreeModel::dataChanged, this, &GroupExportForm::modelDataChanged);
    ui->kanjiTree->expandAll();
    ui->wordsTree->expandAll();

    modelDataChanged();
}

void GroupExportForm::translateTexts()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(qApp->translate("ButtonBox", "Export"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}


//-------------------------------------------------------------

