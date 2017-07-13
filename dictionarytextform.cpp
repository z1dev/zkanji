#include <QPushButton>
#include "dictionarytextform.h"
#include "ui_dictionarytextform.h"
#include "words.h"
#include "globalui.h"
#include "zui.h"

//-------------------------------------------------------------


DictionaryTextForm::DictionaryTextForm(QWidget *parent) : base(parent), ui(new Ui::DictionaryTextForm)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &DictionaryTextForm::accept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DictionaryTextForm::close);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &DictionaryTextForm::checkRemoved);
}

DictionaryTextForm::~DictionaryTextForm()
{
    delete ui;
}

void DictionaryTextForm::exec(Dictionary *d)
{
    dict = d;
    ui->dictImgLabel->setPixmap(ZKanji::dictionaryFlag(QSize(18, 18), dict->name(), Flags::Flag));
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
