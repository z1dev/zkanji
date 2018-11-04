/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
//#include "zui.h"
#include "ui_grouppickerform.h"
#include "grouppickerform.h"
#include "words.h"
#include "groups.h"
#include "zabstracttreemodel.h"
#include "globalui.h"
#include "formstates.h"

//-------------------------------------------------------------


/* static */
GroupBase* GroupPickerForm::select(GroupWidget::Modes mode, const QString &instructions, Dictionary *dict, bool onlycategories, bool showdicts, QWidget *parent)
{
    GroupPickerForm *f = new GroupPickerForm(mode, onlycategories, parent);
    if (!showdicts)
        f->hideDictionarySelect();
    f->setDictionary(dict);
    f->setInstructions(instructions);

    FormStates::restoreDialogSize("GroupPicker", f, true);

    GroupBase *result = nullptr;
    connect(f, &GroupPickerForm::groupSelected, [&result](Dictionary * /*dict*/, GroupBase *group) {
        result = group;
    });

    f->showModal();

    return result;
}

GroupPickerForm::GroupPickerForm(bool onlycategories, QWidget *parent) : base(parent), ui(new Ui::GroupPickerForm), selectButton(nullptr)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->instructionLabel->hide();
    ui->groupWidget->setDictionary(ZKanji::dictionary(0));
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(0));
    ui->groupWidget->setMode(GroupWidget::Words);
    if (onlycategories)
        ui->groupWidget->showOnlyCategories();

    selectButton = ui->buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    translateTexts();
    connect(selectButton, &QPushButton::clicked, this, &GroupPickerForm::selectClicked);

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &GroupPickerForm::close);
}

GroupPickerForm::GroupPickerForm(GroupWidget::Modes mode, bool onlycategories, QWidget *parent) : base(parent), ui(new Ui::GroupPickerForm), selectButton(nullptr)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    gUI->scaleWidget(this);

    ui->instructionLabel->hide();
    ui->groupWidget->setDictionary(ZKanji::dictionary(0));
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(0));
    ui->groupWidget->setMode(mode);
    if (onlycategories)
        ui->groupWidget->showOnlyCategories();

    selectButton = ui->buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    translateTexts();
    connect(selectButton, &QPushButton::clicked, this, &GroupPickerForm::selectClicked);

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &GroupPickerForm::close);
}

GroupPickerForm::~GroupPickerForm()
{
    delete ui;
}

void GroupPickerForm::hideDictionarySelect()
{
    ui->dictWidget->hide();
}

void GroupPickerForm::setDictionary(Dictionary *dict)
{
    if (ui->dictWidget->isVisibleTo(this))
        ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(ZKanji::dictionaryIndex(dict)));
    ui->groupWidget->setDictionary(dict);
}

void GroupPickerForm::setMode(GroupWidget::Modes mode)
{
    ui->groupWidget->setMode(mode);
}

void GroupPickerForm::setInstructions(const QString &instructions)
{
    if (instructions.isEmpty())
        ui->instructionLabel->hide();
    else
    {
        ui->instructionLabel->show();
        ui->instructionLabel->setText(instructions);
    }
}

bool GroupPickerForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateTexts();
    }

    return base::event(e);
}

void GroupPickerForm::translateTexts()
{
    if (selectButton != nullptr)
        selectButton->setText(tr("Select group"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}

void GroupPickerForm::selectClicked()
{
    emit groupSelected(ui->groupWidget->dictionary(), ui->groupWidget->current());
    modalClose(ModalResult::Ok);
}

void GroupPickerForm::on_groupWidget_currentItemChanged(GroupBase *current)
{
    if (selectButton != nullptr)
        selectButton->setEnabled(current != nullptr && (ui->groupWidget->onlyCategories() || !current->isCategory()));
}

void GroupPickerForm::on_dictCBox_currentIndexChanged(int index)
{
    if (!ui->dictCBox->isVisibleTo(this))
        return;
    ui->groupWidget->setDictionary(ZKanji::dictionary(ZKanji::dictionaryPosition(index)));
}


//-------------------------------------------------------------
