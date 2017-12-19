/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include "dictionaryeditorform.h"
#include "ui_dictionaryeditorform.h"
#include "globalui.h"
#include "words.h"
#include "dialogs.h"
#include "zkanjimain.h"
#include "zui.h"
#include "settings.h"


//-------------------------------------------------------------


DictionaryEditorModel::DictionaryEditorModel(ZListView *view, QWidget *parent) : base(parent), view(view)
{
    connect(gUI, &GlobalUI::dictionaryAdded, this, &DictionaryEditorModel::handleAdded);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &DictionaryEditorModel::handleRemoved);
    connect(gUI, &GlobalUI::dictionaryMoved, this, &DictionaryEditorModel::handleMoved);
    connect(gUI, &GlobalUI::dictionaryRenamed, this, &DictionaryEditorModel::handleRenamed);
}

DictionaryEditorModel::~DictionaryEditorModel()
{

}

bool DictionaryEditorModel::orderChanged() const
{
    return orderchanged;
}

int DictionaryEditorModel::rowCount(const QModelIndex &index) const
{
    return ZKanji::dictionaryCount();
}

QVariant DictionaryEditorModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole)
    {
        int h = view->rowHeight(0) * 0.8;
        return ZKanji::dictionaryFlag(QSize(h, h), ZKanji::dictionary(ZKanji::dictionaryPosition(index.row()))->name(), Flags::Flag);
    }
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    return ZKanji::dictionary(ZKanji::dictionaryPosition(index.row()))->name();
}

bool DictionaryEditorModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    int row = index.row();
    QString str = value.toString().trimmed();
    if (str.isEmpty())
        return false;
    if (ZKanji::dictionaryOrder(0) == row || ZKanji::dictionary(ZKanji::dictionaryPosition(row))->name().toLower() == str.toLower())
        return false;

    if (ZKanji::dictionaryNameTaken(str))
    {
        QMessageBox::warning(qApp->activeWindow(), "zkanji", tr("The name entered for the dictionary is already in use. Please try again with a different name."));
        return false;
    }
    if (!ZKanji::dictionaryNameValid(str))
    {
        QMessageBox::warning(qApp->activeWindow(), "zkanji", tr("The name entered for the dictionary is invalid. Make sure the name is a valid file name under your system, and such file does not exist in your data folder already."));
        return false;
    }

    ZKanji::renameDictionary(ZKanji::dictionaryPosition(row), str);

    return true;
}

Qt::ItemFlags DictionaryEditorModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    if (index.row() == ZKanji::dictionaryOrder(0))
        return base::flags(index);
    return base::flags(index) | Qt::ItemIsEditable;
}

Qt::DropActions DictionaryEditorModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions DictionaryEditorModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (samesource)
        return Qt::MoveAction;
    return 0;
}

bool DictionaryEditorModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && row != -1 && action == Qt::MoveAction && data->hasFormat(QStringLiteral("zkanji/dictionaryordering")) && data->data(QStringLiteral("zkanji/dictionaryordering")).size() == sizeof(int);
}

bool DictionaryEditorModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    int orig = *((int*)data->data(QStringLiteral("zkanji/dictionaryordering")).constData());
    if (orig == row || orig + 1 == row)
        return false;

    ZKanji::moveDictionaryOrder(orig, row);
    return true;
}

QMimeData* DictionaryEditorModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mime = new QMimeData();
    QByteArray arr;
    arr.resize(sizeof(int));
    int *dat = (int*)arr.data();
    *dat = indexes.front().row();

    mime->setData(QStringLiteral("zkanji/dictionaryordering"), arr);
    return mime;
}

void DictionaryEditorModel::handleAdded()
{
    signalRowsInserted({ { (int)ZKanji::dictionaryCount() - 1, 1 } });
}

void DictionaryEditorModel::handleRemoved(int index, int order)
{
    signalRowsRemoved({ { order, order } });
}

void DictionaryEditorModel::handleMoved(int from, int to)
{
    orderchanged = true;
    signalRowsMoved({ { from, from } }, to);
}

void DictionaryEditorModel::handleRenamed(int index, int order)
{
    emit dataChanged(base::index(order, 0), base::index(order, columnCount() - 1));
}


//-------------------------------------------------------------


DictionaryEditorForm::DictionaryEditorForm(QWidget *parent) : base(parent), ui(new Ui::DictionaryEditorForm)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    scaleWidget(this);

    model = new DictionaryEditorModel(ui->dictView, this);
    ui->dictView->setModel(model);
    ui->dictView->setAcceptDrops(true);
    ui->dictView->setDragEnabled(true);
    ui->dictView->setCurrentRow(0);

    connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &DictionaryEditorForm::close);
}

DictionaryEditorForm::~DictionaryEditorForm()
{
    delete ui;
}

void DictionaryEditorForm::on_upButton_clicked(bool checked)
{
    ZKanji::moveDictionaryOrder(ui->dictView->currentRow(), ui->dictView->currentRow() - 1);
}

void DictionaryEditorForm::on_downButton_clicked(bool checked)
{
    ZKanji::moveDictionaryOrder(ui->dictView->currentRow(), ui->dictView->currentRow() + 2);
}

void DictionaryEditorForm::on_createButton_clicked(bool checked)
{
    bool ok;
    QString str = QInputDialog::getText(this, "zkanji", tr("Enter name for the new dictionary:"), QLineEdit::Normal, QString(), &ok);
    if (ok == false)
        return;
    if (ZKanji::dictionaryNameTaken(str))
    {
        QMessageBox::warning(this, "zkanji", tr("The name entered for the new dictionary is already in use. Please try again with a different name."));
        return;
    }
    if (!ZKanji::dictionaryNameValid(str))
    {
        QMessageBox::warning(this, "zkanji", tr("The name entered for the new dictionary is invalid. Make sure the name is a valid file name under your system, and such file does not exist in your data folder already."));
        return;
    }

    Dictionary *dict = new Dictionary();
    dict->setName(str);
    dict->setToModified();

    ZKanji::addDictionary(dict);
}

void DictionaryEditorForm::on_editButton_clicked(bool checked)
{
    editDictionaryText(ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictView->currentRow())));
}

void DictionaryEditorForm::on_delButton_clicked(bool checked)
{
    if (QMessageBox::warning(this, "zkanji", tr("The data stored for the selected dictionary, including words, study statistics and groups will be deleted. The file holding the dictionary's data will be deleted as well. This operation cannot be undone.\n\nDo you want to delete the dictionary?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        return;

    QString name = ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictView->currentRow()))->name();
    QFile f(ZKanji::userFolder() + "/data/" + name + ".zkdict");
    f.remove();
    f.setFileName(ZKanji::userFolder() + "/data/" + name + ".zkuser");
    f.remove();

    ZKanji::deleteDictionary(ZKanji::dictionaryPosition(ui->dictView->currentRow()));
}

void DictionaryEditorForm::on_dictView_currentRowChanged(int curr, int prev)
{
    ui->upButton->setEnabled(curr > 0);
    ui->downButton->setEnabled(curr != -1 && curr != ZKanji::dictionaryCount() - 1);
    ui->editButton->setEnabled(curr != -1 && ZKanji::dictionaryPosition(curr) != 0);
    ui->delButton->setEnabled(curr != -1 && ZKanji::dictionaryPosition(curr) != 0);
}

void DictionaryEditorForm::closeEvent(QCloseEvent *e)
{
    if (model->orderChanged())
        Settings::saveIniFile();

    base::closeEvent(e);
}


//-------------------------------------------------------------


void editDictionaries()
{
    (new DictionaryEditorForm(gUI->activeMainForm()))->show();
}


//-------------------------------------------------------------
