/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include <QFileDialog>
#include <QSvgRenderer>
#include <QMessageBox>
#include <QPainter>
#include "dictionarytextform.h"
#include "ui_dictionarytextform.h"
#include "words.h"
#include "globalui.h"
#include "zui.h"
#include "generalsettings.h"
#include "formstates.h"


//-------------------------------------------------------------

static std::map<Dictionary*, DictionaryTextForm*> _dicttextforms;


DictionaryTextForm::DictionaryTextForm(QWidget *parent) : base(parent), ui(new Ui::DictionaryTextForm), dict(nullptr), flagerased(false)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &DictionaryTextForm::accept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DictionaryTextForm::close);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &DictionaryTextForm::dictionaryRemoved);
    connect(gUI, &GlobalUI::dictionaryRenamed, this, &DictionaryTextForm::dictionaryRenamed);

    FormStates::restoreDialogSize("DictionaryText", this, true);
}

DictionaryTextForm::~DictionaryTextForm()
{
    if (dict != nullptr)
        _dicttextforms.erase(dict);
    delete ui;
}

void DictionaryTextForm::exec(Dictionary *d)
{
    auto it = _dicttextforms.find(d);
    if (it != _dicttextforms.end())
    {
        it->second->raise();
        it->second->activateWindow();

        deleteLater();
        return;
    }

    _dicttextforms[d] = this;

    dict = d;
    ui->dictImgLabel->setPixmap(ZKanji::dictionaryFlag(QSize(Settings::scaled(18), Settings::scaled(18)), dict->name(), Flags::Flag));
    translateTexts();
    ui->infoText->setPlainText(dict->infoText());

    connect(d, &Dictionary::dictionaryReset, this, &DictionaryTextForm::dictionaryReset);
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &DictionaryTextForm::dictionaryReplaced);

    ui->removeButton->setEnabled(ZKanji::dictionaryHasCustomFlag(dict->name()));
    show();
}

void DictionaryTextForm::accept()
{
    dict->setInfoText(ui->infoText->toPlainText());

    if (!flagdata.isEmpty())
    {
        ZKanji::assignDictionaryFlag(flagdata, dict->name());
        dict->setToModified();
    }
    else if (flagerased)
    {
        ZKanji::unassignDictionaryFlag(dict->name());
        dict->setToModified();
    }
    close();
}

void DictionaryTextForm::dictionaryRemoved(int /*index*/, int /*order*/, void *oldaddress)
{
    if (oldaddress == dict)
        close();
}

void DictionaryTextForm::dictionaryRenamed(const QString &/*oldname*/, int index, int /*order*/)
{
    if (ZKanji::dictionary(index) == dict)
        setWindowTitle(tr("zkanji - Edit information: %1").arg(dict->name()));
}

void DictionaryTextForm::dictionaryReset()
{
    ui->infoText->setPlainText(dict->infoText());
}

void DictionaryTextForm::dictionaryReplaced(Dictionary *olddict, Dictionary * /*newdict*/, int /*index*/)
{
    if (dict == olddict)
        close();
}

void DictionaryTextForm::on_browseButton_clicked()
{
    QString fname;

    fname = QFileDialog::getOpenFileName(this, tr("Select dictionary flag image"), QString(), QString("%1 (*.svg)").arg(tr("SVG file")));
    if (fname.isEmpty())
        return;

    QFile f(fname);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "zkanji", tr("Couldn't open the selected file."));
        return;
    }

    QByteArray arr = f.readAll();

    QSvgRenderer sr(arr);

    if (!sr.isValid())
    {
        QMessageBox::warning(this, "zkanji", tr("Couldn't read the image file."));
        return;
    }

    flagdata = arr;
    flagerased = false;

    ui->removeButton->setEnabled(true);

    int s = Settings::scaled(18);
    QPixmap img(QSize(s, s));
    img.fill(QColor(0, 0, 0, 0));
    QPainter p(&img);
    sr.render(&p, QRectF(0, s * 0.0625, s, s));
    p.end();

    ui->dictImgLabel->setPixmap(img /*ZKanji::dictionaryFlag(, dict->name(), Flags::Flag)*/);
}

void DictionaryTextForm::on_removeButton_clicked()
{
    //ZKanji::unassignDictionaryFlag(dict->name());
    ui->removeButton->setEnabled(false);

    flagdata.clear();
    flagerased = true;

    ui->dictImgLabel->setPixmap(ZKanji::dictionaryFlag(QSize(Settings::scaled(18), Settings::scaled(18)), "", Flags::Flag));
}

bool DictionaryTextForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        translateTexts();

    return base::event(e);
}

void DictionaryTextForm::translateTexts()
{
    ui->retranslateUi(this);
    setWindowTitle(tr("zkanji - Edit information: %1").arg(dict->name()));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(qApp->translate("ButtonBox", "OK"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}


//-------------------------------------------------------------


void editDictionaryText(Dictionary *d)
{
    DictionaryTextForm *f = new DictionaryTextForm(gUI->activeMainForm());
    f->exec(d);
}


//-------------------------------------------------------------
