/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "wordtodictionaryform.h"
#include "ui_wordtodictionaryform.h"
#include "dialogs.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "wordeditorform.h"
#include "formstates.h"
#include "globalui.h"
#include "zdictionariesmodel.h"
#include "zui.h"


//-------------------------------------------------------------


WordToDictionaryForm::WordToDictionaryForm(QWidget *parent) : base(parent), ui(new Ui::WordToDictionaryForm), proxy(nullptr), dict(nullptr), dest(nullptr), dictindex(-1), expandsize(-1)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->wordsTable->setSelectionType(ListSelectionType::Toggle);
    ui->meaningsTable->setSelectionType(ListSelectionType::None);

    ui->wordsTable->assignStatusBar(ui->wordsStatus);
    ui->meaningsTable->assignStatusBar(ui->meaningsStatus);

    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &WordToDictionaryForm::dictionaryToBeRemoved);
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &WordToDictionaryForm::dictionaryReplaced);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &WordToDictionaryForm::on_addButton_clicked);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &WordToDictionaryForm::on_cancelButton_clicked);
}

WordToDictionaryForm::~WordToDictionaryForm()
{
    delete ui;
}

void WordToDictionaryForm::exec(Dictionary *d, int windex, Dictionary *initial, bool updatelastdiag)
{
    updateWindowGeometry(this);

    dict = d;
    index = windex;

    updatelast = updatelastdiag;
    if (updatelast)
        gUI->setLastWordToDialog(GlobalUI::LastWordDialog::Dictionary);

    connect(dict, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);

    // Set fixed width to the add button.

    translateTexts();

    // Show the original word on top.

    DictionaryDefinitionListItemModel *model = new DictionaryDefinitionListItemModel(this);
    model->setWord(dict, windex);
    ui->wordsTable->setModel(model);

    //connect(ui->wordsTable, &ZDictionaryListView::rowSelectionChanged, this, &WordToDictionaryForm::wordSelChanged);

    proxy = new DictionariesProxyModel(this);
    proxy->setDictionary(dict);
    ui->dictCBox->setModel(proxy);

    if (!updatelast)
        ui->switchButton->hide();

    if (initial == nullptr)
        ui->dictCBox->setCurrentIndex(0);
    else
    {
        bool found = false;
        for (int ix = 0, siz = proxy->rowCount(); ix != siz && !found; ++ix)
            if (proxy->dictionaryAtRow(ix) == initial)
            {
                ui->dictCBox->setCurrentIndex(ix);
                found = true;
            }

        if (!found)
            ui->dictCBox->setCurrentIndex(0);
    }

    FormStates::restoreDialogSplitterState("WordToDictionary", this, ui->splitter, &expandsize);

    //expandsize = ui->splitter->sizes().at(1);

    show();
}

bool WordToDictionaryForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateTexts();
    }

    return base::event(e);
}

void WordToDictionaryForm::closeEvent(QCloseEvent *e)
{
    FormStates::saveDialogSplitterState("WordToDictionary", this, ui->splitter, &expandsize);

    base::closeEvent(e);
}

void WordToDictionaryForm::on_addButton_clicked()
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dict;
    int windex = index;

    std::vector<int> wdindexes;
    ui->wordsTable->selectedRows(wdindexes);
    std::sort(wdindexes.begin(), wdindexes.end());

    gUI->setLastWordDestination(proxy->dictionaryAtRow(ui->dictCBox->currentIndex()));

    close();

    // Creates and opens the word editor form to add new definition to the destination word.
    editWord(d, windex, wdindexes, dest, dictindex, parentWidget());
}

void WordToDictionaryForm::on_cancelButton_clicked()
{
    close();
}

void WordToDictionaryForm::on_switchButton_clicked()
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dict;
    int windex = index;
    bool modal = windowModality() != Qt::NonModal;

    close();

    wordToGroupSelect(d, windex, modal, updatelast/*, parentWidget()*/);
}

void WordToDictionaryForm::on_dictCBox_currentIndexChanged(int ix)
{
    if (dest != nullptr)
        disconnect(dest, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);
    dest = proxy->dictionaryAtRow(ix);
    if (dest != nullptr)
        connect(dest, &Dictionary::dictionaryReset, this, &WordToDictionaryForm::close);
    dictindex = dest->findKanjiKanaWord(dict->wordEntry(index));

    //int h = ui->meaningsWidget->height() + ui->splitter->handleWidth(); // + ui->centralwidget->layout()->spacing();

    if (dictindex == -1)
    {
        if (ui->meaningsWidget->isVisibleTo(this))
        {
            QSize s = size();
            s.setHeight(s.height() - ui->meaningsWidget->height() - ui->splitter->handleWidth());

            expandsize = ui->meaningsWidget->height();

            ui->meaningsWidget->hide();

            ui->splitter->refresh();
            ui->splitter->updateGeometry();

            updateWindowGeometry(this);

            //ui->widget->layout()->activate();
            //centralWidget()->layout()->activate();
            //layout()->activate();

            if (!windowState().testFlag(Qt::WindowMaximized) && !windowState().testFlag(Qt::WindowFullScreen))
                resize(s);
        }
        translateTexts();
    }
    else
    {
        // Show existing definitions in the bottom table.
        DictionaryWordListItemModel *model = new DictionaryWordListItemModel(this);
        std::vector<int> result;
        result.push_back(dictindex);
        model->setWordList(dest, std::move(result));
        if (ui->meaningsTable->model() != nullptr)
            ui->meaningsTable->model()->deleteLater();
        ui->meaningsTable->setMultiLine(true);
        ui->meaningsTable->setModel(model);

        translateTexts();

        if (!ui->meaningsWidget->isVisibleTo(this))
        {
            int oldexp = expandsize;

            int siz1 = ui->widget->height();

            if (!windowState().testFlag(Qt::WindowMaximized) && !windowState().testFlag(Qt::WindowFullScreen))
            {
                QSize s = size();
                s.setHeight(s.height() + ui->splitter->handleWidth() + expandsize);
                resize(s);
            }

            //updateWindowGeometry(this);
            //centralWidget()->layout()->activate();
            //layout()->activate();

            ui->meaningsWidget->show();

            if (!windowState().testFlag(Qt::WindowMaximized) && !windowState().testFlag(Qt::WindowFullScreen))
                ui->splitter->setSizes({ siz1, oldexp });
            else
                ui->splitter->setSizes({ siz1 - oldexp - ui->splitter->handleWidth(), oldexp });
        }
    }
}

void WordToDictionaryForm::on_meaningsTable_wordDoubleClicked(int windex, int dindex)
{
    // Copy member values in case the form is deleted in close.
    Dictionary *d = dest;
    gUI->setLastWordDestination(proxy->dictionaryAtRow(ui->dictCBox->currentIndex()));

    if (updatelast)
        gUI->setLastWordToDialog(GlobalUI::LastWordDialog::Dictionary);

    close();

    // Create and open the word editor in simple edit mode to edit double clicked definition
    // of word in the destination dictionary. This method ignores the source dictionary.
    editWord(d, windex, dindex, parentWidget());
}

//void WordToDictionaryForm::wordSelChanged(/*const QItemSelection &selected, const QItemSelection &deselected*/)
//{
//    addButton->setEnabled(ui->wordsTable->hasSelection());
//}

void WordToDictionaryForm::dictionaryToBeRemoved(int /*index*/, int /*orderindex*/, Dictionary *d)
{
    if (d == dict || ZKanji::dictionaryCount() == 1)
        close();
}

void WordToDictionaryForm::dictionaryReplaced(Dictionary *old, Dictionary* /*newdict*/, int /*index*/)
{
    if (dict == old || dest == old)
        close();
}

void WordToDictionaryForm::translateTexts()
{
    QPushButton *addButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    addButton->setMinimumWidth(0);
    addButton->setMaximumWidth(QWIDGETSIZE_MAX);

    if (dictindex == -1)
        addButton->setText(tr("Create word"));
    else
        addButton->setText(tr("Add meanings"));

    QFontMetrics mcs = addButton->fontMetrics();
    int dif = std::abs(mcs.boundingRect(tr("Create word")).width() - mcs.boundingRect(tr("Add meanings")).width());

    addButton->setMinimumWidth(addButton->sizeHint().width() + dif);
    addButton->setMaximumWidth(addButton->minimumWidth());

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(qApp->translate("ButtonBox", "Cancel"));
}


//-------------------------------------------------------------


void wordToDictionarySelect(Dictionary *d, int windex, bool showmodal, bool updatelastdiag)
{
    WordToDictionaryForm *form = new WordToDictionaryForm(gUI->activeMainForm());
    if (showmodal)
        form->setWindowModality(Qt::ApplicationModal);
    form->exec(d, windex, gUI->lastWordDestination(), updatelastdiag);
}


//-------------------------------------------------------------
