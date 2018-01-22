/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QListWidget>
#include <QCheckBox>
#include <QPushButton>
#include "wordeditorform.h"
#include "ui_wordeditorform.h"
#include "grammar_enums.h"
#include "zstrings.h"
#include "zui.h"
#include "recognizerform.h"
#include "words.h"
#include "zdictionarymodel.h"
#include "zflowlayout.h"
#include "romajizer.h"
#include "formstate.h"
#include "fontsettings.h"
#include "globalui.h"
#include "generalsettings.h"

//-------------------------------------------------------------


DictionaryListEditDelegate::DictionaryListEditDelegate(ZDictionaryListView *parent) : base(parent)
{
    ;
}

extern const double defRowSize;

void DictionaryListEditDelegate::paintDefinition(QPainter *painter, QColor textcolor, QRect r, int y, WordEntry *e, std::vector<InfTypes> *inf, int defix, bool selected) const
{
    if (defix != e->defs.size())
    {
        base::paintDefinition(painter, textcolor, r, y, e, inf, defix, selected);
        return;
    }

    // Main definition font.
    QFont f = Settings::mainFont();
    f.setPixelSize(r.height() * defRowSize);
    f.setItalic(true);

    QString str = tr("Edit to create a new definition for this word.");

    QColor pc = textcolor;
    QColor bc = painter->brush().color();

    QColor col{ (pc.red() + bc.red()) / 2, (pc.green() + bc.green()) / 2, (pc.blue() + bc.blue()) / 2 };


    painter->setPen(col);
    painter->setFont(f);
    drawTextBaseline(painter, r.left(), y, false, r, str);
    //painter->drawText(r.left(), y, str);

}


//-------------------------------------------------------------


ZDictionaryEditListView::ZDictionaryEditListView(QWidget *parent) : base(parent)
{
    setItemDelegate(new DictionaryListEditDelegate(this));
}

//ZListViewItemDelegate* ZDictionaryEditListView::createItemDelegate()
//{
//    return new DictionaryListEditDelegate(this);
//}


//-------------------------------------------------------------


//WordEditorSelectionModel::WordEditorSelectionModel(WordEditorForm *owner, QAbstractItemModel *model, QObject *parent) : base(model, parent), owner(owner)
//{
//
//}
//
//WordEditorSelectionModel::~WordEditorSelectionModel()
//{
//
//}
//
//void WordEditorSelectionModel::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
//{
//    if (!index.isValid() || index.row() != currentIndex().row())
//        return;
//
//    base::select(index, command);
//}
//
//void WordEditorSelectionModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
//{
//    if (selection.indexes().first() != selection.indexes().last() || currentIndex().row() != selection.indexes().first().row())
//        return;
//
//    base::select(selection, command);
//}
//
//void WordEditorSelectionModel::setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
//{
//    int newrow = index.row();
//    int oldrow = currentIndex().row();
//    if (newrow == oldrow || oldrow == -1 || owner->allowSelChange())
//        base::setCurrentIndex(index, command);
//}


//-------------------------------------------------------------

WordEditorFormFactory *WordEditorFormFactory::inst = nullptr;

WordEditorFormFactory& WordEditorFormFactory::instance()
{
    if (inst == nullptr)
        inst = new WordEditorFormFactory;
    return *inst;
}

WordEditorFormFactory::WordEditorFormFactory()
{

}

void WordEditorFormFactory::createForm(Dictionary *d, int windex, int defindex, QWidget *parent)
{
    if (windex != -1 && activateEditor(d, windex))
        return;

    WordEditorForm *form = new WordEditorForm(parent);
    addEditor(d, windex, form);
    form->exec(d, windex, defindex);
}

void WordEditorFormFactory::createForm(Dictionary *srcd, int srcwindex, const std::vector<int> &srcdindexes, Dictionary *dest, int destwindex, QWidget *parent)
{
    if (destwindex != -1 && activateEditor(dest, destwindex))
        return;

    WordEditorForm *form = new WordEditorForm(parent);
    addEditor(dest, destwindex, form);
    form->exec(srcd, srcwindex, srcdindexes, dest, destwindex);
}

void WordEditorFormFactory::dictEntryRemoved(int windex) /* slot */
{
    Dictionary *d = dynamic_cast<Dictionary*>(sender());
    if (windex == -1 || d == nullptr)
        return;

    auto it = editors.find(d);
    if (it == editors.end())
        return;

    auto &dictmap = it->second;
    auto it2 = dictmap.find(windex);
    if (it2 != dictmap.end())
    {
        WordEditorForm *f = it2->second;
        dictmap.erase(it2);
        f->uncheckedClose();
    }

    auto tmp = dictmap;
    dictmap.clear();
    for (auto &p : tmp)
    {
        if (p.first < windex)
            dictmap.insert(p);
        else if (p.first > windex)
            dictmap[p.first - 1] = p.second;
    }
}


bool WordEditorFormFactory::activateEditor(Dictionary *d, int windex)
{
    auto it = editors.find(d);
    if (it == editors.end())
        return false;

    auto it2 = it->second.find(windex);
    if (it2 != it->second.end())
    {
        it2->second->raise();
        it2->second->activateWindow();
        return true;
    }

    return false;
}

void WordEditorFormFactory::addEditor(Dictionary *dict, int windex, WordEditorForm *form)
{
    if (windex == -1)
        return;

    if (editors.count(dict) == 0)
        connect(dict, &Dictionary::entryRemoved, this, &WordEditorFormFactory::dictEntryRemoved);

    editors[dict][windex] = form;
}

void WordEditorFormFactory::editorClosed(Dictionary *dict, int windex)
{
    if (windex == -1)
        return;
    auto it = editors.find(dict);
    if (it == editors.end())
        return;

    auto it2 = it->second.find(windex);
    if (it2 == it->second.end())
        return;

    it->second.erase(it2);
    if (it->second.empty())
        editors.erase(it);
}


//-------------------------------------------------------------


WordEditorForm::WordEditorForm(QWidget *parent) : base(parent), ui(new Ui::WordEditorForm), model(nullptr), /*modified(false), origmodified(false),*/ ignoreedits(false)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    gUI->scaleWidget(this);

    resetBtn = ui->buttonBox->button(QDialogButtonBox::Reset);
    okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    cancelBtn = ui->buttonBox->button(QDialogButtonBox::Discard);
    applyBtn = ui->buttonBox->button(QDialogButtonBox::Apply);

    okBtn->setText(tr("Save and close"));

    ui->attribWidget->setShowGlobal(false);

    ZFlowLayout *attribLayout = new ZFlowLayout(ui->wordAttribWidget);
    attribLayout->setContentsMargins(0, 0, 0, 0);
    attribLayout->setVerticalSpacing(2);
    attribLayout->setHorizontalSpacing(4);
    //if (ui->wordAttribWidget->layout() != nullptr)
    //    ui->wordAttribWidget->layout()->deleteLater();

    ui->wordAttribWidget->setLayout(attribLayout);

    //attribLayout->addWidget(ui->wordAttribLabel);

    for (int ix = 0; ix != (int)WordInfo::Count; ++ix)
    {
        QCheckBox *box = new QCheckBox(ui->wordAttribWidget);
        QSizePolicy pol = box->sizePolicy();
        pol.setHorizontalPolicy(QSizePolicy::Fixed);
        box->setSizePolicy(pol);
        box->setText(Strings::wordInfoLong(ix));
        box->setCheckState(Qt::Unchecked);


        connect(box, &QCheckBox::toggled, this, &WordEditorForm::wordChanged);

        attribLayout->addWidget(box);
    }

    installRecognizer(ui->kanjiButton, ui->kanjiEdit);
    ui->kanjiEdit->setValidator(&japaneseValidator());
    ui->kanaEdit->setValidator(&kanaValidator());

    connect(ui->kanjiEdit, &ZLineEdit::textEdited, this, &WordEditorForm::wordChanged);
    connect(ui->kanaEdit, &ZLineEdit::textEdited, this, &WordEditorForm::wordChanged);
    connect(ui->freqEdit, &ZLineEdit::textEdited, this, &WordEditorForm::wordChanged);
    connect(ui->defTextEdit, &QPlainTextEdit::textChanged, this, &WordEditorForm::attribDefChanged);
    connect(ui->attribWidget, &WordAttribWidget::changed, this, &WordEditorForm::attribDefChanged);

    connect(ui->delButton, &QAbstractButton::clicked, this, &WordEditorForm::delDefClicked);
    connect(ui->cloneButton, &QAbstractButton::clicked, this, &WordEditorForm::cloneDefClicked);

    connect(cancelBtn, &QAbstractButton::clicked, this, &WordEditorForm::close);
    connect(resetBtn, &QAbstractButton::clicked, this, &WordEditorForm::resetClicked);
    connect(okBtn, &QAbstractButton::clicked, this, &WordEditorForm::okClicked);
    connect(applyBtn, &QAbstractButton::clicked, this, &WordEditorForm::applyClicked);

    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &WordEditorForm::dictionaryToBeRemoved);

    entry.reset(new WordEntry);
    original.reset(new WordEntry);

    ui->defTable->assignStatusBar(ui->listStatus);
}

WordEditorForm::~WordEditorForm()
{
    delete ui;
}

void WordEditorForm::exec(Dictionary *d, int windex, int defindex)
{
    ignoreedits = true;

    dict = d;
    index = windex;

    connect(dict, &Dictionary::entryChanged, this, &WordEditorForm::dictEntryChanged);
    connect(dict, &Dictionary::dictionaryReset, this, &WordEditorForm::uncheckedClose);

    WordEntry *w = windex < 0 ? nullptr : d->wordEntry(windex);

    installRecognizer(ui->kanjiButton, ui->kanjiEdit);

    if (windex >= 0)
    {
        ZKanji::cloneWordData(entry.get(), w, true);

        ui->kanjiEdit->setText(entry->kanji.toQString());
        ui->kanaEdit->setText(entry->kana.toQString());
        ui->freqEdit->setText(QString::number(entry->freq));

        ui->kanjiEdit->setReadOnly(true);
        ui->kanaEdit->setReadOnly(true);
        ui->kanjiLabel->setEnabled(false);
        ui->kanaLabel->setEnabled(false);

        for (int ix = 0; ix != (int)WordInfo::Count; ++ix)
            ((QCheckBox*)ui->wordAttribWidget->layout()->itemAt(ix)->widget())->setCheckState((entry->inf & (1 << ix)) != 0 ? Qt::Checked : Qt::Unchecked);
    }
    else
        ui->freqEdit->setText("0");

    // Adding an extra empty definition at the end of the entry to allow inserting new definitions.
    //entry->defs.resize(entry->defs.size() + 1);

    model = new DictionaryEntryItemModel(dict, entry.get(), this);

    ui->defTable->setModel(model);

    connect(ui->defTable, &ZDictionaryListView::currentRowChanged, this, &WordEditorForm::defCurrentChanged);
    ui->defTable->setCurrentRow(std::max(0, defindex));

    ZKanji::cloneWordData(original.get(), entry.get(), true);

    checkInput();

    ui->cloneButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());
    ui->delButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());

    FormStates::restoreDialogSplitterState("WordEditor", this, ui->splitter);

    ignoreedits = false;

    updateDictionaryLabels();

    show();
}

void WordEditorForm::exec(Dictionary *srcd, int srcwindex, const std::vector<int> &srcdindexes, Dictionary *dest, int windex)
{
    ignoreedits = true;

    dict = dest;
    index = windex;

    //connect(dict, &Dictionary::entryRemoved, this, &WordEditorForm::dictEntryRemoved);
    connect(dict, &Dictionary::entryChanged, this, &WordEditorForm::dictEntryChanged);
    connect(dict, &Dictionary::dictionaryReset, this, &WordEditorForm::uncheckedClose);

    WordEntry *srcw = srcd->wordEntry(srcwindex);

#ifdef _DEBUG
    for (int ix = 0; ix != srcdindexes.size(); ++ix)
        if (srcdindexes[ix] < 0 || srcdindexes[ix] >= srcw->defs.size())
            throw "Invalid index in srcdindexes list. Should be a valid definition index in srcw.";
#endif

    installRecognizer(ui->kanjiButton, ui->kanjiEdit);

    if (windex >= 0)
    {
        ZKanji::cloneWordData(entry.get(), dest->wordEntry(windex), true);
        ZKanji::cloneWordData(original.get(), entry.get(), true);

        ui->kanjiEdit->setReadOnly(true);
        ui->kanaEdit->setReadOnly(true);
    }
    else
    {
        entry->freq = srcw->freq;
        uint infmask = 0xFFFFFFFF << (int)WordInfo::Count;
        entry->inf = srcw->inf & ~infmask;
        entry->kanji = srcw->kanji;
        entry->kana = srcw->kana;
        entry->romaji = srcw->romaji;
    }

    ui->kanjiEdit->setText(entry->kanji.toQString());
    ui->kanaEdit->setText(entry->kana.toQString());
    ui->freqEdit->setText(QString::number(entry->freq));

    std::vector<int> indordered(srcdindexes);
    std::sort(indordered.begin(), indordered.end());
    indordered.resize(std::unique(indordered.begin(), indordered.end()) - indordered.begin());

    int pos = entry->defs.size();
    entry->defs.resize(pos + indordered.size());
    for (int ix = 0; ix != indordered.size(); ++ix)
        entry->defs[ix + pos] = srcw->defs[indordered[ix]];

    // Adding an extra empty definition at the end of the entry to allow inserting new definitions.
    //entry->defs.resize(entry->defs.size() + 1);

    model = new DictionaryEntryItemModel(dict, entry.get(), this);

    ui->defTable->setModel(model);

    connect(ui->defTable, &ZDictionaryListView::currentRowChanged, this, &WordEditorForm::defCurrentChanged);
    ui->defTable->setCurrentRow(pos);

    for (int ix = 0; ix != (int)WordInfo::Count; ++ix)
        ((QCheckBox*)ui->wordAttribWidget->layout()->itemAt(ix)->widget())->setCheckState((entry->inf & (1 << ix)) != 0 ? Qt::Checked : Qt::Unchecked);

    ZKanji::cloneWordData(original.get(), entry.get(), true);

    checkInput();

    ui->cloneButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());
    ui->delButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());

    FormStates::restoreDialogSplitterState("WordEditor", this, ui->splitter);

    ignoreedits = false;

    updateDictionaryLabels();

    show();
}

void WordEditorForm::uncheckedClose()
{
    entry.reset();
    index = -1;
    close();
}

void WordEditorForm::closeEvent(QCloseEvent *e)
{
    if (canReset())
    {
        checkInput();

        if (!applyBtn->isEnabled())
        {
            // The word is modified but apply is not enabled means the word is in an invalid
            // state. Closing the dialog now can only result in a loss.
            int r = showAndReturn("zkanji", tr("Your current changes to the word are not saved in the dictionary and will be lost."),
                tr("The word is incomplete and cannot be saved. Select \"%1\" to continue editing the word, or \"%2\" to close the window without saving.").arg(tr("Cancel")).arg(tr("Discard Changes")),
                {
                    { tr("Cancel"), QMessageBox::RejectRole },
                    { tr("Discard Changes"), QMessageBox::DestructiveRole }
                }, 0);

            if (r == 0)
            {
                e->ignore();
                base::closeEvent(e);
                return;
            }

        }
        else
        {
            int r = canReset() ? showAndReturn(QStringLiteral("zkanji"), tr("Your current changes to the word are not saved in the dictionary and will be lost."),
                tr("Select \"%1\" to continue editing the word, \"%2\" to update the dictionary and close the window, or \"%3\" to close the window without saving your changes.")
                .arg(tr("Cancel")).arg(tr("Save and Close")).arg(tr("Discard Changes")),
                {
                    { tr("Cancel"), QMessageBox::RejectRole },
                    { tr("Save and Close"), QMessageBox::AcceptRole },
                    { tr("Discard Changes"), QMessageBox::DestructiveRole }
                }, 0) : 2;
            if (r == 0)
            {
                e->ignore();
                base::closeEvent(e);
                return;
            }

            if (r == 1)
                applyClicked();
        }
    }

    e->accept();

    FormStates::saveDialogSplitterState("WordEditor", this, ui->splitter);

    if (index != -1)
        WordEditorFormFactory::instance().editorClosed(dict, index);

    base::closeEvent(e);
}

void WordEditorForm::checkInput()
{
    // Updates the buttons' enabled states when the edited word changes.

    bool convok = false;
    int freqval = 0;
    bool valid = !ui->kanjiEdit->text().isEmpty() && !ui->kanaEdit->text().isEmpty() && !ui->freqEdit->text().isEmpty() && (freqval = ui->freqEdit->text().toInt(&convok)) >= 0 && convok && freqval <= 10000 && entry->defs.size() != 0;
    for (int ix = 0; valid && ix != entry->defs.size(); ++ix)
    {
        if (entry->defs[ix].def.empty())
            valid = false;
    }

    resetBtn->setEnabled(canReset());
    okBtn->setEnabled(valid && canReset());
    okBtn->setDefault(okBtn->isEnabled());
    cancelBtn->setText(!canReset() ? tr("Close") : tr("Discard"));
    if (!okBtn->isEnabled())
        cancelBtn->setDefault(true);
    applyBtn->setEnabled(valid && canReset());
}

void WordEditorForm::dictEntryChanged(int windex, bool studydef)
{
    if (ignoreedits || windex != index || studydef)
        return;

    ZKanji::cloneWordData(original.get(), dict->wordEntry(index), true);
    checkInput();
}

void WordEditorForm::wordChanged()
{
    if (ignoreedits)
        return;

    if (sender() == ui->kanjiEdit)
    {
        QString tmp = ui->kanjiEdit->text();
        int cpos = ui->kanjiEdit->cursorPosition();
        if (ui->kanjiEdit->validator()->validate(tmp, cpos) != QValidator::Acceptable)
            return;
        entry->kanji.copy(ui->kanjiEdit->text().constData());
    }
    else if (sender() == ui->kanaEdit)
    {
        QString tmp = ui->kanaEdit->text();
        int cpos = ui->kanaEdit->cursorPosition();
        if (ui->kanaEdit->validator()->validate(tmp, cpos) != QValidator::Acceptable)
            return;
        entry->kana.copy(ui->kanaEdit->text().constData());
        entry->romaji.copy(romanize(entry->kana).constData());

        //m->dataChanged(m->index(0, 0), m->index(entry->defs.size() - 1, 1));
    }
    else if (sender() == ui->freqEdit)
    {
        bool convok = false;
        int freqval = ui->freqEdit->text().toInt(&convok);
        if (convok && freqval >= 0 && freqval <= 10000)
        {
            if (entry->freq == freqval)
                return;
            entry->freq = freqval;
        }

    }
    else if (dynamic_cast<QWidget*>(sender()) != nullptr && ((QWidget*)sender())->parentWidget() == ui->wordAttribWidget)
    {
        uchar infbits = checkedInfoBits();
        if (infbits == entry->inf)
            return;
        entry->inf = infbits;
    }

    model->entryChanged();

    checkInput();
}

void WordEditorForm::attribDefChanged()
{
    if (ignoreedits || ui->defTable->currentRow() == -1)
        return;

    int ix = ui->defTable->currentRow();

    model->changeDefinition(ix, enteredDefinition(), ui->attribWidget->checkedTypes());

    checkInput();

    ui->cloneButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());
    ui->delButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());
}

void WordEditorForm::defCurrentChanged(int row, int prev)
{
    ignoreedits = true;
    if (row == -1 || row >= entry->defs.size())
    {
        ui->defTextEdit->setPlainText(QString());
        ui->attribWidget->clearChecked();

        ui->cloneButton->setEnabled(false);
        ui->delButton->setEnabled(false);
    }
    else
    {
        ui->defTextEdit->setPlainText(entry->defs[row].def.toQString().split(GLOSS_SEP_CHAR, QString::SkipEmptyParts).join('\n'));
        ui->attribWidget->setChecked(entry->defs[row].attrib);
        checkInput();

        ui->cloneButton->setEnabled(true);
        ui->delButton->setEnabled(true);
    }
    ignoreedits = false;
}

void WordEditorForm::delDefClicked()
{
    if (ui->defTable->currentRow() == -1)
        return;

    int ix = ui->defTable->currentRow();
    if (ix >= entry->defs.size())
        return;

    model->deleteDefinition(ix);

    defCurrentChanged(ui->defTable->currentRow(), -1);

    checkInput();

    ui->cloneButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());
    ui->delButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());
}

void WordEditorForm::cloneDefClicked()
{
    if (ui->defTable->currentRow() == -1)
        return;

    int ix = ui->defTable->currentRow();
    if (ix >= entry->defs.size())
        return;

    model->cloneDefinition(ix);

    ui->defTable->setCurrentRow(entry->defs.size() - 1);

    checkInput();
}

void WordEditorForm::resetClicked()
{
    model->copyFromEntry(original.get());

    ignoreedits = true;
    ui->kanjiEdit->setText(entry->kanji.toQString());
    ui->kanaEdit->setText(entry->kana.toQString());
    ui->freqEdit->setText(QString::number(entry->freq));

    for (int ix = 0; ix != (int)WordInfo::Count; ++ix)
        ((QCheckBox*)ui->wordAttribWidget->layout()->itemAt(ix)->widget())->setCheckState((entry->inf & (1 << ix)) != 0 ? Qt::Checked : Qt::Unchecked);
    ignoreedits = false;

    //m->emitDataChanged();
    //m->emitReset();

    int row = ui->defTable->currentRow();
    if (row == -1)
        ui->defTable->setCurrentRow(0);
    else
    {
        ignoreedits = true;
        ui->attribWidget->setChecked(entry->defs[row].attrib);
        ignoreedits = false;
    }

    defCurrentChanged(ui->defTable->currentRow(), -1);

    checkInput();

    ui->cloneButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());
    ui->delButton->setEnabled(ui->defTable->currentRow() < entry->defs.size());
}

void WordEditorForm::applyClicked()
{
    bool convok = false;
    int freqval = 0;
    bool valid = !ui->kanjiEdit->text().isEmpty() && !ui->kanaEdit->text().isEmpty() && !ui->freqEdit->text().isEmpty() && (freqval = ui->freqEdit->text().toInt(&convok)) >= 0 && convok && freqval <= 10000 && entry->defs.size() != 0;
    for (int ix = 0; valid && ix != entry->defs.size(); ++ix)
    {
        if (entry->defs[ix].def.empty())
            valid = false;
    }

    if (index == -1 && !ui->kanjiEdit->text().isEmpty() && !ui->kanaEdit->text().isEmpty())
    {
        // A new word is being created. If it clashes with an existing word, the user should
        // be notified before any other error to avoid confusion.

        if (dict->findKanjiKanaWord(entry.get()) != -1)
        {
            QMessageBox::warning(this, "zkanji", tr("The word cannot be created, as another word with the entered kanji form and kana reading already exists in the dictionary. "));
            return;
        }
    }

    if (!valid)
    {
        QMessageBox::warning(this, "zkanji", tr("The word has missing or invalid fields. Please make sure you have entered at least the written form, kana reading and one not empty word definition."));
        return;
    }

    WordEntry *e = nullptr;
    if (index == -1)
    {
        index = dict->addWordCopy(entry.get(), true);
        WordEditorFormFactory::instance().addEditor(dict, index, this);
    }
    else
    {
        // Updating the word in the dictionary.

        if (!ZKanji::sameWord(dict->wordEntry(index), entry.get()))
        {
            ignoreedits = true;
            dict->cloneWordData(index, entry.get(), true, true);
            ignoreedits = false;
        }
    }

    ZKanji::cloneWordData(original.get(), entry.get(), true);

    ui->kanjiEdit->setReadOnly(true);
    ui->kanaEdit->setReadOnly(true);

    checkInput();
}

void WordEditorForm::okClicked()
{
    if (!applyBtn->isEnabled())
        return;

    if (canReset())
        applyBtn->click();

    checkInput();

    if (!canReset())
        close();
}

void WordEditorForm::dictionaryToBeRemoved(int index, int orderindex, Dictionary *d)
{
    if (d == dict)
        close();
}

void WordEditorForm::reset()
{
    entry->freq = 0;
    entry->inf = 0;
    entry->defs.clear();
    entry->kanji.clear();
    entry->kana.clear();
    entry->romaji.clear();
    index = -1;

    close();
}

uint WordEditorForm::checkedInfoBits()
{
    uint bits = 0;

    int dif = 0;
    for (int ix = 0; ix != (int)WordInfo::Count + dif; ++ix)
    {
        QCheckBox *cb = dynamic_cast<QCheckBox*>(ui->wordAttribWidget->layout()->itemAt(ix)->widget());
        if (cb == nullptr)
            ++dif;
        else if (cb->checkState() == Qt::Checked)
            bits |= (1 << (ix - dif));
    }
    return bits;
}

QString WordEditorForm::enteredDefinition()
{
    QStringList list = ui->defTextEdit->toPlainText().split('\n');
    for (int ix = 0; ix != list.size(); ++ix)
        list[ix] = list.at(ix).trimmed();

    return list.join(GLOSS_SEP_CHAR);
}

//bool WordEditorForm::wordUnsaved()
//{
//    //if (index == -1)
//        return false;
//
//    //WordEntry *e = dict->wordEntry(index);
//    //return e->inf != checkedInfoBits() || e->freq != ui->freqEdit->text().toInt();
//}
//
//bool WordEditorForm::defUnsaved()
//{
//    //if (index == -1 || ui->defTable->currentRow() == -1)
//        return false;
//
//    //WordEntry *e = dict->wordEntry(index);
//    //WordDefinition &def = e->defs[ui->defTable->currentRow()];
//    //return def.attrib != ui->attribWidget->checkedTypes() || def.def != enteredDefinition();
//}


bool WordEditorForm::canReset() const
{
    if (entry == nullptr || original == nullptr)
        return false;

    //if (index != -1)
    return entry->freq != original->freq || entry->inf != original->inf || entry->defs != original->defs;
    //return entry->freq != 0 || entry->inf != 0 || entry->defs.size() != 0;
}

void WordEditorForm::updateDictionaryLabels()
{
    ui->dictImgLabel->setPixmap(ZKanji::dictionaryFlag(QSize(Settings::scaled(18), Settings::scaled(18)), dict->name(), Flags::Flag));
    ui->dictLabel->setText(dict->name());
}


//-------------------------------------------------------------


void editNewWord(Dictionary *d)
{
    WordEditorFormFactory::instance().createForm(d, -1, -1, gUI->activeMainForm());
}

void editWord(Dictionary *d, int windex, int defindex, QWidget *parent)
{
    WordEditorFormFactory::instance().createForm(d, windex, defindex, parent);
}

void editWord(Dictionary *srcd, int srcwindex, const std::vector<int> &srcdindexes, Dictionary *dest, int destwindex, QWidget *parent)
{
    WordEditorFormFactory::instance().createForm(srcd, srcwindex, srcdindexes, dest, destwindex, parent);
}


//-------------------------------------------------------------

