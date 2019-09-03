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
#include "formstates.h"

#include "checked_cast.h"

//-------------------------------------------------------------


KanjiDefinitionForm::KanjiDefinitionForm(QWidget *parent) : base(parent), ui(new Ui::KanjiDefinitionForm), nextpos(-1), model(nullptr)
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
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &KanjiDefinitionForm::dictionaryReplaced);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(false);
    ui->acceptButton->setDefault(true);

    translateTexts();

    FormStates::restoreDialogSize("KanjiDefinition", this, true);
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

void KanjiDefinitionForm::on_discardButton_clicked(bool /*checked*/)
{
    pos = prevkanji.back();
    prevkanji.pop_back();
    updateKanji();
}

void KanjiDefinitionForm::on_acceptButton_clicked(bool /*checked*/)
{
    prevkanji.push_back(pos);
    dict->setKanjiMeaning(list[pos], ui->defText->toPlainText());
    if (ui->skipBox->isChecked())
        pos = nextpos;
    else
        ++pos;
    updateKanji();
}

void KanjiDefinitionForm::on_dictCBox_currentIndexChanged(int /*index*/)
{
    updateDictionary();
}

void KanjiDefinitionForm::ok()
{
    dict->setKanjiMeaning(list[pos], ui->defText->toPlainText());
    close();
}

void KanjiDefinitionForm::dictionaryRemoved(int /*index*/, int /*orderindex*/, void *oldaddress)
{
    if (dict == oldaddress)
        close();
}

void KanjiDefinitionForm::dictionaryReplaced(Dictionary *olddict, Dictionary* /*newdict*/, int /*index*/)
{
    if (dict == olddict)
        close();
}

bool KanjiDefinitionForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateTexts();
    }

    return base::event(e);
}

void KanjiDefinitionForm::translateTexts()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(qApp->translate("ButtonBox", "OK"));
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(qApp->translate("ButtonBox", "Close"));
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

    int lsiz = tosigned(list.size());
    while (nextpos != lsiz && dict->hasKanjiMeaning(list[nextpos]))
        ++nextpos;
    if (nextpos == lsiz)
        nextpos = -1;

    ui->discardButton->setEnabled(!prevkanji.empty());
    ui->acceptButton->setEnabled(ui->skipBox->isChecked() ? nextpos != -1 : pos < lsiz - 1);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(!ui->acceptButton->isEnabled());
    ui->acceptButton->setDefault(ui->acceptButton->isEnabled());


    ui->kanjiDiagram->setKanjiIndex(list[pos]);
    updateDictionary();

    if (pos < lsiz)
        ui->defText->setText(dict->kanjiMeaning(list[pos], "\n"));
    else
        ui->defText->clear();

    ui->defText->setFocus();
}

void KanjiDefinitionForm::updateDictionary()
{
    ui->dictWidget->setDictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));
    ui->dictWidget->resetScrollPosition();

    if (model == nullptr)
        return;

    if (model->dictionary() != nullptr)
        disconnect(model->dictionary(), &Dictionary::dictionaryReset, this, &KanjiDefinitionForm::updateDictionary);

    std::vector<int> words;
    Dictionary *d = ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));

    if (pos < tosigned(list.size()))
        d->getKanjiWords(list[pos], words);

    if (pos < tosigned(list.size()))
        ui->origText->setText(d->kanjiMeaning(list[pos], "\n"));
    else
        ui->origText->clear();

    model->setWordList(d, std::move(words), true);
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
