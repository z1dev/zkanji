/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include "kanjidefform.h"
#include "ui_kanjidefform.h"
#include "globalui.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "zui.h"


//-------------------------------------------------------------


KanjiDefinitionForm::KanjiDefinitionForm(QWidget *parent) : base(parent), ui(new Ui::KanjiDefinitionForm), model(nullptr), nextpos(-1)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    int h = ui->origText->fontMetrics().height() * 10;
    ui->origText->setFixedHeight(h);
    ui->defText->setFixedHeight(h);
    ui->diagramFrame->setFixedSize(QSize(h, h));
    //ui->kanjiDiagram->setRadical(false);

    ui->dictWidget->setExamplesVisible(false);

    model = new DictionaryWordListItemModel(this);
    ui->dictWidget->setModel(model);

    connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &KanjiDefinitionForm::close);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &KanjiDefinitionForm::ok);

    connect(gUI, &GlobalUI::dictionaryRemoved, this, &KanjiDefinitionForm::dictionaryRemoved);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(false);
    ui->acceptButton->setDefault(true);
}

KanjiDefinitionForm::~KanjiDefinitionForm()
{
    delete ui;
}

void KanjiDefinitionForm::exec(Dictionary *d, ushort kindex)
{
    dict = d;
    list = { kindex };

    connect(dict, &Dictionary::dictionaryReset, this, &KanjiDefinitionForm::close);

    pos = 0;
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(dict) == 0 ? 1 : 0));
    ui->dictWidget->setDictionary(dict);

    updateKanji();

    show();
}

void KanjiDefinitionForm::exec(Dictionary *d, const std::vector<ushort> &l)
{
    dict = d;
    list = l;

    connect(dict, &Dictionary::dictionaryReset, this, &KanjiDefinitionForm::close);

    pos = 0;
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(dict) == 0 ? 1 : 0));
    ui->dictWidget->setDictionary(dict);

    updateKanji();

    show();
}

void KanjiDefinitionForm::on_discardButton_clicked(bool checked)
{
    pos = prevkanji.back();
    prevkanji.pop_back();
    updateKanji();
}

void KanjiDefinitionForm::on_acceptButton_clicked(bool checked)
{
    prevkanji.push_back(pos);
    dict->setKanjiMeaning(list[pos], ui->defText->toPlainText());
    if (ui->skipBox->isChecked())
        pos = nextpos;
    else
        ++pos;
    updateKanji();
}

void KanjiDefinitionForm::on_dictCBox_currentIndexChanged(int index)
{
	updateDictionary();
}

void KanjiDefinitionForm::ok()
{
    dict->setKanjiMeaning(list[pos], ui->defText->toPlainText());
    close();
}

void KanjiDefinitionForm::dictionaryRemoved(int index, int orderindex, void *oldaddress)
{
	if (dict == oldaddress)
		close();
}

void KanjiDefinitionForm::updateKanji()
{
    if (model == nullptr)
    {
        ui->discardButton->setEnabled(false);
        ui->acceptButton->setEnabled(false);
        return;
    }

    nextpos = pos + 1;
    while (nextpos != list.size() && dict->hasKanjiMeaning(list[nextpos]))
        ++nextpos;
    if (nextpos == list.size())
        nextpos = -1;

    ui->discardButton->setEnabled(!prevkanji.empty());
    ui->acceptButton->setEnabled(ui->skipBox->isChecked() ? nextpos != -1 : pos < list.size() - 1);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(!ui->acceptButton->isEnabled());
    ui->acceptButton->setDefault(ui->acceptButton->isEnabled());


    ui->kanjiDiagram->setKanjiIndex(list[pos]);
    updateDictionary();

    if (pos < list.size())
        ui->defText->setText(dict->kanjiMeaning(list[pos], "\n"));
    else
        ui->defText->clear();

    ui->defText->setFocus();
}

void KanjiDefinitionForm::updateDictionary()
{
    ui->dictWidget->setDictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));

    if (model == nullptr)
        return;

    if (model->dictionary() != nullptr)
        disconnect(model->dictionary(), &Dictionary::dictionaryReset, this, &KanjiDefinitionForm::updateDictionary);

    std::vector<int> words;
    Dictionary *d = ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));

    if (pos < list.size())
        d->getKanjiWords(list[pos], words);

    if (pos < list.size())
        ui->origText->setText(d->kanjiMeaning(list[pos], "\n"));
    else
        ui->origText->clear();

    std::sort(words.begin(), words.end(), [d](int a, int b) {
        return Dictionary::jpSortFunc(std::pair<WordEntry*, const std::vector<InfTypes>*>(d->wordEntry(a), nullptr), std::pair<WordEntry*, const std::vector<InfTypes>*>(d->wordEntry(b), nullptr));
    });

    model->setWordList(d, std::move(words));
    if (d != nullptr)
        connect(d, &Dictionary::dictionaryReset, this, &KanjiDefinitionForm::updateDictionary);
}


//-------------------------------------------------------------


void editKanjiDefinition(Dictionary *d, const std::vector<ushort> &list)
{
    KanjiDefinitionForm *f = new KanjiDefinitionForm(gUI->activeMainForm());
    f->exec(d, list);
}

void editKanjiDefinition(Dictionary *d, ushort kindex)
{
    KanjiDefinitionForm *f = new KanjiDefinitionForm(gUI->activeMainForm());
    f->exec(d, kindex);
}


//-------------------------------------------------------------
