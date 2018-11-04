/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include "importreplaceform.h"
#include "ui_importreplaceform.h"
#include "zui.h"
#include "zevents.h"
#include "words.h"
#include "zkanjimain.h"
#include "zdictionarymodel.h"
#include "globalui.h"


//-------------------------------------------------------------


LabelDialog::LabelDialog(QWidget *parent) : base(parent)
{
    setWindowTitle("zkanji");
    QHBoxLayout *ly = new QHBoxLayout(this);
    lb = new QLabel(this);
    lb->setText(tr("Looking for differences in the dictionaries. Please wait..."));
    ly->addWidget(lb);
    setAttribute(Qt::WA_QuitOnClose, false);

    gUI->scaleWidget(this);
}

LabelDialog::~LabelDialog()
{

}


//-------------------------------------------------------------


ImportReplaceForm::ImportReplaceForm(Dictionary *olddir, Dictionary *newdir, QWidget *parent) : base(parent), ui(new Ui::ImportReplaceForm), model(nullptr), olddir(olddir), owndir(newdir == nullptr), newdir(newdir)
{
    ui->setupUi(this);

    gUI->scaleWidget(this);

    setAttribute(Qt::WA_QuitOnClose, false);

    model = new DictionaryWordListItemModel(ui->missingTable);
    ui->missingTable->setModel(model);
    connect(ui->missingTable, &ZDictionaryListView::currentRowChanged, this, &ImportReplaceForm::missingRowChanged);

    connect(ui->finishButton, &QPushButton::clicked, this, &ImportReplaceForm::close);
    connect(ui->dict, &DictionaryWidget::rowChanged, this, &ImportReplaceForm::dictRowChanged);
    connect(ui->useButton, &QPushButton::clicked, this, &ImportReplaceForm::useClicked);
    connect(ui->skipButton, &QPushButton::clicked, this, &ImportReplaceForm::skipClicked);

    ui->missingTable->assignStatusBar(ui->status);
}

ImportReplaceForm::~ImportReplaceForm()
{
    //delete orig;
    delete ui;
}

bool ImportReplaceForm::exec()
{
    dlg.show();

    qApp->postEvent(this, new StartEvent());

    showModal();
    //loop.exec();

    // Operation cancelled.
    if (ui->missingTable->currentRow() != -1)
    {
        if (owndir)
            delete newdir;
        newdir = nullptr;
        return false;
    }

    return true;
}

std::map<int, int>& ImportReplaceForm::changes()
{
    return list;
}

Dictionary* ImportReplaceForm::dictionary()
{
    return newdir;
}

OriginalWordsList& ImportReplaceForm::originals()
{
    return orig;
}

void ImportReplaceForm::closeEvent(QCloseEvent *e)
{
    if (ui->missingTable->currentRow() != -1)
    {
        int r = showAndReturn("zkanji", tr("Replacement was not selected for every word."), tr("Those words will be removed from groups and study data. Are you sure?\nSelect \"%1\" to keep the old dictionary and update your data later.").arg(tr("Abort update")),
                { { tr("Yes"), QMessageBox::YesRole },
                  { tr("Don't close"), QMessageBox::NoRole },
                  { tr("Abort update"), QMessageBox::RejectRole } });

        if (r == 1)
        {
            e->ignore();
            base::closeEvent(e);
            return;
        }

        if (r == 0)
            ui->missingTable->setCurrentRow(-1);
    }

    // Use event->ignore() to prevent closing.
    e->accept();

    base::closeEvent(e);
    //if (!e->isAccepted())
    //    return;

    //// Notifies the event loop that makes this window behave like a dialog.
    //loop.quit();
}

bool ImportReplaceForm::event(QEvent *e)
{
    if (e->type() == StartEvent::Type())
    {
        if (!init())
        {
            QMessageBox::information(nullptr, "zkanji", tr("Every used word were found in the new dictionary. No need for user input."));

            closeCancel();
        }
        //    loop.exit();

        return true;
    }
    return base::event(e);
}

void ImportReplaceForm::dictRowChanged(int row, int /*oldrow*/)
{
    ui->useButton->setEnabled(row != -1);
}

void ImportReplaceForm::useClicked()
{
    list[model->indexes(ui->missingTable->currentRow())] = ui->dict->currentIndex();

    if (ui->missingTable->currentRow() == model->rowCount() - 1)
    {
        ui->missingTable->setCurrentRow(-1);
        return;
    }

    ui->missingTable->setCurrentRow(ui->missingTable->currentRow() + 1);
}

void ImportReplaceForm::skipClicked()
{
    if (ui->missingTable->currentRow() == model->rowCount() - 1)
    {
        ui->missingTable->setCurrentRow(-1);
        return;
    }

    ui->missingTable->setCurrentRow(ui->missingTable->currentRow() + 1);
}

void ImportReplaceForm::missingRowChanged(int row, int /*prev*/)
{
    if (row == -1)
    {
        ui->finishButton->setText(tr("Finish"));
        ui->dict->setSearchText(QString());
        ui->useButton->setEnabled(false);
        ui->skipButton->setEnabled(false);
    }
    else
    {
        ui->finishButton->setText(tr("Cancel Update"));
        ui->dict->setSearchText(model->items(row)->kana.toQStringRaw());
        ui->useButton->setEnabled(ui->dict->currentRow() != -1);
        ui->skipButton->setEnabled(true);
    }
}

bool ImportReplaceForm::init()
{
    if (owndir)
    {
        if (olddir != ZKanji::dictionary(0))
            throw "Import replace without a passed old dictionary. Only the base dictionary can be updated this way.";

        newdir = new Dictionary;
        newdir->loadFile(ZKanji::appFolder() + "/data/English.zkj", true, true);

        // This is only true if the user replaces the data file while the program is loading,
        // after it found a correct version. Let it throw to quit.
        if (newdir->pre2015())
            throw "Cannot import old file format";
    }

    // Bring over the originals list to the new dictionary, updating the dictionary as
    // necessary. User added words not in the new dictionary are added to it. Added or
    // modified words with corresponding word in new dictionary are added as modifying words.
    // Modified words not in the new dictionary are ignored. When bringing over originals
    // from old version, their inf and freq are taken from the new dictionary.

    for (int ix = 0, siz = tosigned(ZKanji::originals.size()); owndir && ix != siz; ++ix)
    {
        const OriginalWord *o = ZKanji::originals.items(ix);
        if (o->change != OriginalWord::Added && o->change != OriginalWord::Modified)
            continue;

        int wix = newdir->findKanjiKanaWord(o->kanji, o->kana);

        // Ignore if it was just modified and the new dict does not have it.
        if (wix == -1 && o->change == OriginalWord::Modified)
            continue;

        WordEntry *w = olddir->wordEntry(o->index);

        // Add word if necessary.
        if (wix == -1)
        {
            wix = newdir->addWordCopy(w, false);
            orig.createAdded(wix, o->kanji.data(), o->kana.data());
            continue;
        }

        WordEntry *nw = newdir->wordEntry(wix);
        uint oldinf = w->inf;
        ushort oldfreq = w->freq;

        // The inf and freq of the old word can differ, but it should have no consequence when
        // checking whether it matches the new word, or when updating the dictionary. It's
        // restored below.
        if (olddir->pre2015())
        {
            w->inf = nw->inf;
            w->freq = nw->freq;
        }
        if (ZKanji::sameWord(w, nw))
            continue;

        orig.createModified(wix, nw);
        newdir->cloneWordData(wix, w, false, false);

        if (olddir->pre2015())
        {
            w->inf = oldinf;
            w->freq = oldfreq;
        }
    }

    ui->dict->setDictionary(newdir);

    std::vector<int> l;

    // Creating word index mapping of words (that were added to groups, study data etc.) of
    // the original dictionary to indexes in the new dictionary. Result in list is:
    // [old word index, new word index]. The new word index is set to -1 if the same word is
    // not found.
    olddir->listUsedWords(l);
    list = newdir->mapWords(olddir, l);
    dlg.hide();

    // Fill l to contain words in need of a replacement.
    std::vector<int>().swap(l);
    int ix = 0;
    for (auto p : list)
    {
        if (p.second == -1)
            l.push_back(p.first);
        ++ix;
    }
    if (l.empty())
        return false;

    std::vector<int> results;
    std::swap(results, l);
    model->setWordList(olddir, std::move(results));
    ui->missingTable->setCurrentRow(0);

    //show();

    return true;
}


//-------------------------------------------------------------

