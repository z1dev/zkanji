/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include "languageform.h"
#include "ui_languageform.h"
#include "languages.h"
#include "languagesettings.h"
#include "globalui.h"
#include "zui.h"


//-------------------------------------------------------------


LanguageForm::LanguageForm(QWidget *parent) : base(parent), ui(new Ui::LanguageForm)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    gUI->scaleWidget(this);

    ui->languageCBox->clear();
    for (int ix = 0, siz = zLang->count(); ix != siz; ++ix)
        ui->languageCBox->addItem(zLang->names(ix));

    connect(ui->buttons->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &LanguageForm::okClicked);
    connect(ui->buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DialogWindow::closeCancel);

    translateTexts();

    updateWindowGeometry(this);
    adjustSize();
    setFixedSize(minimumSize());
}

LanguageForm::~LanguageForm()
{
    delete ui;
}

void LanguageForm::showEvent(QShowEvent *e)
{
    base::showEvent(e);
}

void LanguageForm::translateTexts()
{
    ui->buttons->button(QDialogButtonBox::Ok)->setText(qApp->translate("ButtonBox", "OK"));
    ui->buttons->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Quit"));
}

void LanguageForm::okClicked()
{
    Settings::language = zLang->IDs(ui->languageCBox->currentIndex());
    zLang->setLanguage(Settings::language);
    modalClose(ModalResult::Ok);
}


//-------------------------------------------------------------


bool languageSelect()
{
    if (zLang->count() < 2)
    {
        Settings::language = QString();
        return true;
    }


    LanguageForm *frm = new LanguageForm;
    if (frm->showModal() == ModalResult::Cancel)
        return false;

    return true;
}


//-------------------------------------------------------------
