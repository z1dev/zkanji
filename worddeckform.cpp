/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QWindow>
#include <QMessageBox>
#include <QMimeData>
#include "worddeckform.h"
#include "ui_worddeckform.h"
#include "worddeck.h"
#include "zkanjimain.h"
#include "zui.h"
#include "zevents.h"
#include "words.h"
#include "wordstudylistform.h"
#include "wordstudyform.h"
#include "zdictionariesmodel.h"
#include "wordtodeckform.h"
#include "globalui.h"
#include "ranges.h"
#include "dialogs.h"


//-------------------------------------------------------------

DeckListModel::DeckListModel(WordDeckList *decks, QObject *parent) : base(parent), decks(decks), tempitem(false)
{
    //for (int ix = 0; ix != dict->wordDeckCount(); ++ix)
    //    ui->deckBox->addItem(decks->items(ix)->getName());

    //connect(dict, &Dictionary::beginDeckMove, this, &DeckListModel::beginMove);
    connect(decks, &WordDeckList::decksMoved, this, &DeckListModel::decksMoved);
    connect(decks, &WordDeckList::deckRemoved, this, &DeckListModel::deckRemoved);
}

void DeckListModel::addTempItem()
{
    if (tempitem)
        return;
    tempitem = true;
    signalRowsInserted({ { decks->size(), 1 } });
}

void DeckListModel::removeTempItem()
{
    if (!tempitem)
        return;
    tempitem = false;
    signalRowsRemoved({ { decks->size(), decks->size() } });
}

bool DeckListModel::hasTempItem() const
{
    return tempitem;
}

int DeckListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || decks == nullptr)
        return 0;

    return decks->size() + (tempitem ? 1 : 0);
}

int DeckListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid() || decks == nullptr)
        return 0;
    return 5;
}

QModelIndex DeckListModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

QVariant DeckListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return base::headerData(section, orientation, role);

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0:
            return tr("Deck name");
        case 1:
            return tr("Due");
        case 2:
            return tr("Last study");
        case 3:
            return tr("Queued");
        case 4:
            return tr("Studied");
        }
    }

    if (role == (int)ColumnRoles::AutoSized)
    {
        if (section == 0)
            return (int)ColumnAutoSize::Stretched;
        return (int)ColumnAutoSize::HeaderAuto;
    }

    if (role == Qt::TextAlignmentRole)
        return Qt::AlignLeft;

    if (role == (int)ColumnRoles::FreeSized)
        return false;
    if (role == (int)ColumnRoles::DefaultWidth)
        return 0;

    return base::headerData(section, orientation, role);
}

bool DeckListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() != 0)
        return false;

    if (tempitem && index.row() == decks->size())
    {
        QString str = value.toString().left(255);
        if (decks->nameTakenOrInvalid(str))
        {
            removeTempItem();
            return false;
        }
        tempitem = false;
        return decks->add(str);
    }
    else
        return decks->rename(index.row(), value.toString());
}

QVariant DeckListModel::data(const QModelIndex &index, int role) const
{
    int col = index.column();
    int row = index.row();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (tempitem && row == decks->size())
            return QString();

        if (col == 1)
            return decks->items(row)->dueSize();
        else if (col == 2)
        {
            QDate date = decks->items(row)->lastDay();
            if (!date.isValid())
                return QString("Never");
            return formatDate(date);
        }
        else if (col == 3)
            return decks->items(row)->queueSize();
        else if (col == 4)
            return decks->items(row)->studySize();

        return decks->items(row)->getName();

    }

    return QVariant();
}

Qt::ItemFlags DeckListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 0)
        return base::flags(index);
    return base::flags(index) | Qt::ItemIsEditable;
}

bool DeckListModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    return ((row != -1 && data->hasFormat("zkanji/decks")) ||
        (row == -1 && column == -1 && parent.isValid() && data->hasFormat("zkanji/words")) && *(intptr_t*)data->data("zkanji/words").constData() == (intptr_t)decks->dictionary());
}

Qt::DropActions DeckListModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (!samesource && mime->hasFormat(QStringLiteral("zkanji/words")))
        return Qt::CopyAction;
    if (samesource && mime->hasFormat(QStringLiteral("zkanji/decks")))
        return Qt::MoveAction;
    return 0;
}

//QStringList DeckListModel::mimeTypes() const
//{
//    QStringList types;
//    types << QStringLiteral("zkanji/words");
//    types << QStringLiteral("zkanji/decks");
//    return types;
//}

bool DeckListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (data->hasFormat("zkanji/words"))
    {
        auto arr = data->data("zkanji/words");

        if (arr.size() <= sizeof(intptr_t) * 2 && decks->dictionary() != (void*)*(intptr_t*)arr.constData())
            return false;

        const int *dat = (const int*)(arr.constData() + sizeof(intptr_t) * 2);

        int cnt = (arr.size() - sizeof(intptr_t) * 2) / sizeof(int);
        if (cnt * sizeof(int) + sizeof(intptr_t) * 2 != arr.size())
            return false;

        std::vector<int> windexes;
        windexes.reserve(cnt);
        for (int ix = 0; ix != cnt; ++ix, ++dat)
            windexes.push_back(*dat);

        addWordsToDeck(decks->items(parent.row())->dictionary(), decks->items(parent.row()), windexes, WordDeckForm::Instance(false));

        return true;
    }
    else if (data->hasFormat("zkanji/decks"))
    {
        auto arr = data->data("zkanji/decks");

        const int *dat = (const int*)(arr.constData());

        int cnt = arr.size() / sizeof(int);
        if (cnt * sizeof(int) != arr.size())
            return false;

        std::vector<int> windexes;
        windexes.reserve(cnt);
        for (int ix = 0; ix != cnt; ++ix, ++dat)
            windexes.push_back(*dat);

        smartvector<Range> ranges;
        _rangeFromIndexes(windexes, ranges);

        decks->move(ranges, row);

        return true;
    }

    return false;
}

Qt::DropActions DeckListModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

QMimeData* DeckListModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty())
        return nullptr;

    QMimeData *data = new QMimeData();

    QByteArray arr;
    arr.resize(indexes.size() * sizeof(int));

    int *dat = (int*)arr.data();
    for (auto it = indexes.cbegin(); it != indexes.cend(); ++it, ++dat)
        *dat = it->row();

    data->setData("zkanji/decks", arr);
    return data;
}

//void DeckListModel::beginMove(int first, int last, int pos)
//{
//    emit beginMoveRows(QModelIndex(), first, last, QModelIndex(), pos);
//}

void DeckListModel::decksMoved(const smartvector<Range> &ranges, int pos)
{
    signalRowsMoved(ranges, pos);
}

void DeckListModel::deckRemoved(int index)
{
    signalRowsRemoved({ { index, index } });
}

//-------------------------------------------------------------


WordDeckListView::WordDeckListView(QWidget *parent) : base(parent)
{

}

void WordDeckListView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    if (hint == QAbstractItemDelegate::EditNextItem || hint == QAbstractItemDelegate::EditPreviousItem)
        hint = QAbstractItemDelegate::SubmitModelCache;

    base::closeEditor(editor, hint);

    emit editorClosed();
}


//-------------------------------------------------------------


// Single instance of WordDeckForm 
WordDeckForm *WordDeckForm::inst = nullptr;

WordDeckForm* WordDeckForm::Instance(bool createshow, int dict)
{
    if (inst == nullptr && createshow)
    {
        inst = new WordDeckForm(gUI->activeMainForm());
        //i->connect(worddeckform, &QMainWindow::destroyed, i, &GlobalUI::formDestroyed);

        if (dict != -1)
            inst->setDictionary(dict);
        inst->show();
    }

    if (createshow)
    {
        inst->raise();
        inst->activateWindow();
    }

    return inst;
}

WordDeckForm::WordDeckForm(QWidget *parent) : base(parent), ui(new Ui::WordDeckForm), decks(ZKanji::dictionary(0)->wordDecks()), model(nullptr)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    connect(ui->deckTable, &WordDeckListView::editorClosed, this, &WordDeckForm::editorClosed);
    ui->deckTable->setEditColumn(0);
    ui->deckTable->setAcceptDrops(true);
    ui->deckTable->setDragEnabled(true);

    connect(ui->viewButton, &QPushButton::clicked, this, &WordDeckForm::viewClicked);
    connect(ui->studyButton, &QPushButton::clicked, this, &WordDeckForm::studyClicked);
    //connect(ui->dictCBox, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &WordDeckForm::setDictionary);
    connect(ui->deckTable, &ZListView::currentRowChanged, this, &WordDeckForm::currentDeckChanged);
    connect(ui->deckTable, &ZListView::rowDoubleClicked, this, &WordDeckForm::deckDoubleClicked);

    fillDeckList();

    ui->dictCBox->setModel(ZKanji::dictionariesModel());

    QHeaderView *h = ui->deckTable->horizontalHeader();
    h->setSectionResizeMode(QHeaderView::Fixed);

    QFont f = h->font();
    QFontMetrics fm(f);
    int col1w = fm.boundingRect(ui->deckTable->model()->headerData(1, Qt::Horizontal, Qt::DisplayRole).toString()).width() + 8;
    int col2w = fm.boundingRect(ui->deckTable->model()->headerData(2, Qt::Horizontal, Qt::DisplayRole).toString()).width() + 8;

}

WordDeckForm::~WordDeckForm()
{
    delete ui;
    inst = nullptr;
}

void WordDeckForm::setDictionary(int index)
{
    if (decks->dictionary() == ZKanji::dictionary(index))
        return;

    decks = ZKanji::dictionary(index)->wordDecks();
    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(index));

    fillDeckList();
}

//void WordDeckForm::showEvent(QShowEvent *event)
//{
//    base::showEvent(event);
//    if (maximumSize() == QSize(16777215, 16777215))
//    {
//        setMinimumSize(size());
//        setMaximumSize(size());
//    }
//}

void WordDeckForm::on_delButton_clicked(bool checked)
{
    int row = ui->deckTable->selectedRow(0);
    if (!decks->items(row)->empty() && QMessageBox::question(this, "zkanji", tr("Once deleted, the study data in the selected deck will be lost and cannot be restored.\n\nDo you want to delete the deck?")) != QMessageBox::Yes)
        return;

    decks->remove(row);
    ui->addButton->setEnabled(ui->deckTable->model()->rowCount() < 255);
}

void WordDeckForm::on_addButton_clicked(bool checked)
{
    cacherow = ui->deckTable->selectedRow(0);

    model->addTempItem();
    int row = model->rowCount() - 1;
    ui->deckTable->setCurrentRow(row);
    ui->deckTable->edit(model->index(row, 0));
}

void WordDeckForm::on_statsButton_clicked(bool checked)
{

}

void WordDeckForm::on_deckTable_rowSelectionChanged()
{
    bool hassel = ui->deckTable->hasSelection() && !model->hasTempItem();
    ui->addButton->setEnabled(!model->hasTempItem());
    ui->delButton->setEnabled(hassel);
    ui->viewButton->setEnabled(hassel);
    ui->studyButton->setEnabled(hassel);
    ui->statsButton->setEnabled(hassel);
}

void WordDeckForm::on_dictCBox_currentIndexChanged(int index)
{
    setDictionary(ZKanji::dictionaryPosition(index));
}

void WordDeckForm::editorClosed()
{
    if (model != nullptr && model->hasTempItem())
        qApp->postEvent(this, new EditorClosedEvent);
}

bool WordDeckForm::event(QEvent *e)
{
    if (e->type() == SetDictEvent::Type())
    {
        // Changes active dictionary to the dictionary specified by the event.
        int index = ((SetDictEvent*)e)->index();
        setDictionary(index);
        return true;
    }
    else if (e->type() == GetDictEvent::Type())
    {
        GetDictEvent *de = (GetDictEvent*)e;
        de->setResult(ZKanji::dictionaryIndex(decks->dictionary()));
        return true;
    }
    else if (e->type() == EditorClosedEvent::Type())
    {
        if (model->hasTempItem())
        {
            model->removeTempItem();
            ui->deckTable->setCurrentRow(cacherow);
        }
        return true;
    }

    return base::event(e);
}

void WordDeckForm::focusInEvent(QFocusEvent *e)
{
    base::focusInEvent(e);
    emit activated(this);
}

void WordDeckForm::keyPressEvent(QKeyEvent *e)
{
    if (model->hasTempItem() || ui->deckTable->editing())
    {
        e->accept();
        return;
    }
    
    if (e->modifiers().testFlag(Qt::AltModifier) && e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9)
    {
        SetDictEvent *se = new SetDictEvent(e->key() - Qt::Key_1);
        qApp->postEvent(this, se);
    }


    base::keyPressEvent(e);
}

void WordDeckForm::fillDeckList()
{
    if (ui->deckTable->model() != nullptr)
        ui->deckTable->model()->deleteLater();
    ui->deckTable->setModel(model = new DeckListModel(decks, this));

    if (ui->deckTable->model()->rowCount() != 0)
    {
        ui->deckTable->setCurrentRow(0);
        ui->addButton->setEnabled(ui->deckTable->model()->rowCount() < 255);
    }
    else
    {
        ui->delButton->setEnabled(false);
        ui->viewButton->setEnabled(false);
        ui->studyButton->setEnabled(false);
        ui->statsButton->setEnabled(false);
    }
}

void WordDeckForm::viewClicked()
{
    WordStudyListForm::Instance(decks->items(ui->deckTable->currentRow()), true);
}

void WordDeckForm::studyClicked()
{
    WordStudyForm::studyDeck(decks->items(ui->deckTable->currentRow()));
}

void WordDeckForm::currentDeckChanged(int current, int previous)
{
    ui->addButton->setEnabled(!model->hasTempItem());
    //ui->viewButton->setEnabled(current != -1);
    //ui->studyButton->setEnabled(current != -1);
    //ui->delButton->setEnabled(current != -1);
    //ui->statsButton->setEnabled(current != -1);
}

void WordDeckForm::deckDoubleClicked(int index)
{
    if (index == -1)
        return;
    WordStudyListForm::Instance(decks->items(index), true);
}


//-------------------------------------------------------------

