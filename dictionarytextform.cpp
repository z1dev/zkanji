/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include "dictionarytextform.h"
#include "ui_dictionarytextform.h"
#include "words.h"
#include "globalui.h"
#include "zui.h"
#include "generalsettings.h"
#include "formstates.h"

//-------------------------------------------------------------


DictionaryTextForm::DictionaryTextForm(QWidget *parent) : base(parent), ui(new Ui::DictionaryTextForm)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &DictionaryTextForm::accept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DictionaryTextForm::close);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &DictionaryTextForm::checkRemoved);

    FormStates::restoreDialogSize("DictionaryText", this, true);
}

DictionaryTextForm::~DictionaryTextForm()
{
    delete ui;
}

void DictionaryTextForm::exec(Dictionary *d)
{
    dict = d;
    ui->dictImgLabel->setPixmap(ZKanji::dictionaryFlag(QSize(Settings::scaled(18), Settings::scaled(18)), dict->name(), Flags::Flag));
    ui->dictLabel->setText(dict->name());
    ui->infoText->setPlainText(dict->infoText());

    connect(d, &Dictionary::dictionaryReset, this, &DictionaryTextForm::dictionaryReset);

    show();
}

void DictionaryTextForm::accept()
{
    dict->setInfoText(ui->infoText->toPlainText());
    close();
}

void DictionaryTextForm::checkRemoved(int index, int orderindex, void *oldaddress)
{
    if (oldaddress == dict)
        close();
}

void DictionaryTextForm::dictionaryReset()
{
    ui->infoText->setPlainText(dict->infoText());
}


//-------------------------------------------------------------


void editDictionaryText(Dictionary *d)
{
    DictionaryTextForm *f = new DictionaryTextForm(gUI->activeMainForm());
    f->exec(d);
}


//-------------------------------------------------------------
