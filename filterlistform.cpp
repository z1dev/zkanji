/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include <QDesktopWidget>
#include <QPainter>
#include "filterlistform.h"
#include "ui_filterlistform.h"
#include "zlistview.h"
#include "words.h"
#include "zui.h"
#include "dictionarywidget.h"
#include "ranges.h"
#include "dialogs.h"
#include "globalui.h"


//-------------------------------------------------------------

//QImage *FilterListModel::plusimg = nullptr;
//QImage *FilterListModel::minusimg = nullptr;
//QImage *FilterListModel::magnimg = nullptr;
//QImage *FilterListModel::delimg = nullptr;


FilterListModel::FilterListModel(WordFilterConditions *conditions, ZListView *parent) : base(parent), conditions(conditions)
{
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterErased, this, &FilterListModel::filterErased);
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterCreated, this, &FilterListModel::filterCreated);
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterRenamed, this, &FilterListModel::filterRenamed);
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterMoved, this, &FilterListModel::filterMoved);
}

FilterListModel::~FilterListModel()
{

}

int FilterListModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

int FilterListModel::rowCount(const QModelIndex &parent) const
{
    return ZKanji::wordfilters().size() + 2;
}

QVariant FilterListModel::data(const QModelIndex &index, int role) const
{
    int col = index.column();
    int row = index.row();

    int fsiz = ZKanji::wordfilters().size();

    enum RowSource { Filters, InExample, InGroup };
    RowSource src = row < fsiz ? Filters : row == fsiz ? InExample : InGroup;

    if (role == Qt::CheckStateRole)
    {
        if (col == 0)
            return QVariant();
        if (src == Filters)
        {
            if (row < conditions->inclusions.size() && conditions->inclusions[row] == (col == 1 ? Inclusion::Include : Inclusion::Exclude) ? Qt::Checked : Qt::Unchecked)
                return Qt::Checked;
            return Qt::Unchecked;
        }

        if (src == InExample)
            return conditions->examples == (col == 1 ? Inclusion::Include : Inclusion::Exclude) ? Qt::Checked : Qt::Unchecked;
        else
            return conditions->groups == (col == 1 ? Inclusion::Include : Inclusion::Exclude) ? Qt::Checked : Qt::Unchecked;

    }

    if (role == Qt::DisplayRole)
    {
        if (col == 0)
        {
            if (src == Filters)
                return ZKanji::wordfilters().items(index.row()).name;
            if (src == InExample)
                return tr("In example sentence");
            return tr("In word group");

        }

        return QString();

        //if (col > 2 && src != Filters)
        //    return 0;

        //const int imgw = 18;
        //const int imgh = 18;

        //ZListView *view = (ZListView*)parent();
        //bool mouseover = (view->checkBoxMouseState(index) & ZListView::Hover) == ZListView::Hover;

        //QImage *img = nullptr;
        //if (col == 3)
        //    img = imageFromSvg(":/magnifier.svg", imgw, imgh);
        //else if (col == 4)
        //    img = imageFromSvg(":/xmark.svg", imgw, imgh);
        //else
        //{
        //    Inclusion inc = Inclusion::Ignore;
        //    if (src == Filters)
        //    {
        //        if (row < conditions->inclusions.size())
        //            inc = conditions->inclusions[row];
        //    }
        //    else if (src == InExample)
        //        inc = conditions->examples;
        //    else
        //        inc = onditions->groups;

        //    if (col == 1 && (inc == Inclusion::Include || mouseover))
        //        img = imageFromSvg(":/plusmark.svg", imgw, imgh);
        //    else if (col == 2 && (inc == Inclusion::Exclude || mouseover))
        //        img = imageFromSvg(":/minusmark.svg", imgw, imgh);
        //}

        //return (int)img;
    }

    return QVariant(); //base::data(index, role);
}

bool FilterListModel::setData(const QModelIndex &index, const QVariant &val, int role)
{
    int col = index.column();
    int row = index.row();

    int fsiz = ZKanji::wordfilters().size();

    enum RowSource { Filters, InExample, InGroup };
    RowSource src = row < fsiz ? Filters : row == fsiz ? InExample : InGroup;

    if (col != 0 && role == Qt::CheckStateRole)
    {
        if (src == Filters)
        {
            while (conditions->inclusions.size() <= row)
                conditions->inclusions.push_back(Inclusion::Ignore);
        }

        QModelIndex six = index;
        QModelIndex eix = index;

        // Old inclusion value is saved so the other cell can be redrawn.
        Inclusion inc = src == Filters ? conditions->inclusions[row] : src == InExample ? conditions->examples : conditions->groups;

        if (inc != (col == 1 ? Inclusion::Include : Inclusion::Exclude))
        {
            if (src == Filters)
                conditions->inclusions[row] = (col == 1 ? Inclusion::Include : Inclusion::Exclude);
            else if (src == InExample)
                conditions->examples = (col == 1 ? Inclusion::Include : Inclusion::Exclude);
            else
                conditions->groups = (col == 1 ? Inclusion::Include : Inclusion::Exclude);

            if (inc == Inclusion::Include)
                six = createIndex(index.row(), index.column() - 1);
            else if (inc == Inclusion::Exclude)
                eix = createIndex(index.row(), index.column() + 1);
        }
        else
        {
            if (src == Filters)
                conditions->inclusions[row] = Inclusion::Ignore;
            else if (src == InExample)
                conditions->examples = Inclusion::Ignore;
            else
                conditions->groups = Inclusion::Ignore;
        }

        emit dataChanged(six, eix, { Qt::DisplayRole, Qt::CheckStateRole });

        if (src == Filters)
            emit includeChanged(row, inc);
        else
            emit conditionChanged();

        return true;
    }

    return setData(index, val, role);
}

QVariant FilterListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || (role != (int)ColumnRoles::DefaultWidth && role != (int)ColumnRoles::AutoSized && role != (int)ColumnRoles::FreeSized && role != Qt::DisplayRole))
        return base::headerData(section, orientation, role);

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0:
            return tr("Filter name");
        case 1:
            return tr("+");
        case 2:
            return tr("-");
        default:
            return QString();
        }
    }

    if (role == (int)ColumnRoles::DefaultWidth)
        return -1;

    if (role == (int)ColumnRoles::FreeSized)
        return false;

    // Only stretch the first column.
    return section == 0 ? (int)ColumnAutoSize::Stretched : (int)ColumnAutoSize::HeaderAuto;
}

Qt::ItemFlags FilterListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    int col = index.column();

    Qt::ItemFlags result = base::flags(index);

    if (col != 0)
        result |= Qt::ItemIsUserCheckable;

    return result;
}

Qt::DropActions FilterListModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions FilterListModel::supportedDropActions(bool samesource, const QMimeData *mime) const
{
    if (!samesource || !mime->hasFormat("zkanji/wordfilter"))
        return 0;
    return Qt::MoveAction;
}

QMimeData* FilterListModel::mimeData(const QModelIndexList &indexes) const
{
    // The last 2 rows shouldn't be moved. If indexes contains one of those rows, return.
    if (indexes.isEmpty() || !indexes.front().isValid() || indexes.front().row() >= ZKanji::wordfilters().size())
        return nullptr;

    QMimeData *mime = new QMimeData();
    QByteArray arr;
    arr.resize(sizeof(int));
    int *dat = (int*)arr.data();
    *dat = indexes.front().row();

    mime->setData("zkanji/wordfilter", arr);

    return mime;
}

bool FilterListModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row == -1 || row > ZKanji::wordfilters().size() || !data->hasFormat("zkanji/wordfilter") || data->data("zkanji/wordfilter").size() != sizeof(int) || action != Qt::MoveAction ||
        *((int*)data->data("zkanji/wordfilter").data()) >= ZKanji::wordfilters().size())
        return false;

    return true;
}

bool FilterListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    int index = *((int*)data->data("zkanji/wordfilter").data());
    ZKanji::wordfilters().move(index, row);
    return true;
}

void FilterListModel::filterErased(int index)
{
    //beginRemoveRows(QModelIndex(), index, index);
    //endRemoveRows();
    signalRowsRemoved({ { index, index } });
}

void FilterListModel::filterCreated()
{
    signalRowsInserted({ { ZKanji::wordfilters().size() - 1, 1 } });
    //beginInsertRows(QModelIndex(), ZKanji::wordfilters().size() - 1, ZKanji::wordfilters().size() - 1);
    //endInsertRows();
}

void FilterListModel::filterRenamed(int row)
{
    dataChanged(index(row, 0), index(row, 0), { Qt::DisplayRole });
}

void FilterListModel::filterMoved(int index, int to)
{
    signalRowsMoved({ { index, index } }, to);
}


//-------------------------------------------------------------


//FilterListItemDelegate::FilterListItemDelegate(ZListView *parent) : base(parent)
//{
//
//}
//
//FilterListItemDelegate::~FilterListItemDelegate()
//{
//
//}
//
//QRect FilterListItemDelegate::checkBoxRect(const QModelIndex &index) const
//{
//    QRect cellrect = owner()->visualRect(index);
//
//    // Check box image sizes.
//    int siz = cellrect.height() - 4;
//
//    return QRect(cellrect.left() + 4, cellrect.top() + 2, siz, siz);
//
//    //ZListView::CheckBoxMouseState mstate = owner()->checkBoxMouseState(index);
//    //ZListView *view = (ZListView*)parent();
//    //bool mouseover = (view->checkBoxMouseState(index) & ZListView::Hover) == ZListView::Hover;
//
//    //QImage *img = nullptr;
//    //if (col == 3)
//    //    img = imageFromSvg(":/magnifier.svg", imgw, imgh);
//    //else if (col == 4)
//    //    img = imageFromSvg(":/xmark.svg", imgw, imgh);
//    //else
//    //{
//    //    Inclusion inc = Inclusion::Ignore;
//    //    if (src == Filters)
//    //    {
//    //        if (row < conditions->inclusions.size())
//    //            inc = conditions->inclusions[row];
//    //    }
//    //    else if (src == InExample)
//    //        inc = conditions->examples;
//    //    else
//    //        inc = conditions->groups;
//
//    //    if (col == 1 && (inc == Inclusion::Include || mouseover))
//    //        img = imageFromSvg(":/plusmark.svg", imgw, imgh);
//    //    else if (col == 2 && (inc == Inclusion::Exclude || mouseover))
//    //        img = imageFromSvg(":/minusmark.svg", imgw, imgh);
//    //}
//}
//
//QRect FilterListItemDelegate::checkBoxHitRect(const QModelIndex &index) const
//{
//    return checkBoxRect(index);
//}
//
//QRect FilterListItemDelegate::drawCheckBox(QPainter *p, const QModelIndex &index) const
//{
//    QRect r = owner()->visualRect(index);
//
//    // Check box image sizes.
//    int siz = r.height() - 4;
//
//    r = QRect(r.left() + 4, r.top() + 2, siz, siz);
//
//    int col = index.column();
//    int row = index.row();
//    int fsiz = ZKanji::wordfilters().size();
//
//    //enum RowSource { Filters, InExample, InGroup };
//    //RowSource src = row < fsiz ? Filters : row == fsiz ? InExample : InGroup;
//
//    QImage img;
//    if (col == 3)
//        img = imageFromSvg(":/magnifier.svg", siz, siz);
//    else if (col == 4)
//        img = imageFromSvg(":/xmark.svg", siz, siz);
//    else
//    {
//        //Inclusion inc = (Inclusion)index.data((int)CellRoles::Custom).toInt();
//
//        auto mstate = owner()->checkBoxMouseState(index);
//        bool mouseover = mstate == CheckBoxMouseState::Hover || mstate == CheckBoxMouseState::DownHover;
//
//        Qt::CheckState chkstate = (Qt::CheckState)index.data(Qt::CheckStateRole).toInt();
//
//        if (col == 1 && (chkstate == Qt::Checked || mouseover))
//            img = imageFromSvg(":/plusmark.svg", siz, siz);
//        else if (col == 2 && (chkstate == Qt::Checked || mouseover))
//            img = imageFromSvg(":/minusmark.svg", siz, siz);
//    }
//
//    if (!img.isNull())
//        p->drawImage(r.topLeft(), img);
//
//    return r;
//}


//-------------------------------------------------------------


FilterListForm::FilterListForm(WordFilterConditions *conditions, QWidget *parent) : base(parent), ui(new Ui::FilterListForm), conditions(conditions)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    scaleWidget(this);

    ui->filterTable->setAutoSizeColumns(true);
    ui->filterTable->setSelectionType(ListSelectionType::Single);
    ui->filterTable->setAcceptDrops(true);
    ui->filterTable->setDragEnabled(true);

    ui->filterTable->setModel(new FilterListModel(conditions, ui->filterTable));
    //ui->filterTable->setItemDelegate(new FilterListItemDelegate(ui->filterTable));

    FilterListModel *model = (FilterListModel*)ui->filterTable->model();
    connect(model, &FilterListModel::includeChanged, this, &FilterListForm::includeChanged);
    connect(model, &FilterListModel::conditionChanged, this, &FilterListForm::conditionChanged);
    connect(model, &FilterListModel::editInitiated, this, &FilterListForm::editInitiated);
    connect(model, &FilterListModel::deleteInitiated, this, &FilterListForm::deleteInitiated);
    connect(ui->filterTable, &ZListView::rowDoubleClicked, this, &FilterListForm::editInitiated);
    connect(ui->filterTable, &ZListView::currentRowChanged, this, &FilterListForm::currentRowChanged);
}

FilterListForm::~FilterListForm()
{
    delete ui;
}

void FilterListForm::updatePosition(const QRect &r)
{
    setAttribute(Qt::WA_DontShowOnScreen);
    show();
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    QRect desk = qApp->desktop()->availableGeometry(QPoint((r.right() + r.left()) / 2, (r.bottom() + r.top()) / 2));

    QRect geom = geometry();
    QRect fr = frameGeometry();
    // QMainWindow::setGeometry ignores the frame so the window will be larger. Check the
    // difference here which will be used when updating the popup position. Each member of the
    // rect will hold the distance between the geometry and the frame.
    QRect dif = QRect(QPoint(geom.left() - fr.left(), geom.top() - fr.top()), QPoint(fr.right() - geom.right(), fr.bottom() - geom.bottom()));

    int top;
    if (r.top() - 8 - geom.height() - dif.top() - dif.bottom() < desk.top())
        top = r.bottom() + 8 + dif.top();
    else
        top = r.top() - 8 - geom.height() - dif.bottom();
    int left;
    if (r.left() + geom.width() + dif.left() + dif.right() >= desk.right())
        left = r.right() - geom.width() - dif.right();
    else
        left = r.left() + dif.left();

    setGeometry(QRect(left, top, geom.width(), geom.height()));
}

void FilterListForm::on_addButton_clicked()
{
    if (ZKanji::wordfilters().size() == 255)
    {
        QMessageBox::warning(this, QStringLiteral("zkanji"), tr("The list of filters is full. Can't add more than 255 filters."));
        return;
    }

    editInitiated(-1);
}

void FilterListForm::on_delButton_clicked()
{
    deleteInitiated(ui->filterTable->currentRow());
}

void FilterListForm::on_editButton_clicked()
{
    editInitiated(ui->filterTable->currentRow());
}

void FilterListForm::editInitiated(int row)
{
    if (row >= ZKanji::wordfilters().size())
        return;

    ui->addButton->setEnabled(false);
    editWordFilter(row, this);
}

void FilterListForm::deleteInitiated(int row)
{
    // Delete filter.
    int btn = showAndReturn(this, QStringLiteral("zkanji"), tr("Do you want to delete the filter?"),
        tr("If you select \"%1\", the filter will be removed and any listing using it will be updated. Select \"%2\" if you changed your mind.").arg(tr("Delete")).arg(tr("Cancel")), {
            { tr("Delete"), QMessageBox::AcceptRole },
            { tr("Cancel"), QMessageBox::RejectRole }
        });
    if (btn == 1)
        return;

    ZKanji::wordfilters().erase(row);
    currentRowChanged();
}

void FilterListForm::currentRowChanged()
{
    int curr = ui->filterTable->currentRow();
    ui->delButton->setEnabled(curr >= 0 && curr < ZKanji::wordfilters().size());
    ui->editButton->setEnabled(curr >= 0 && curr < ZKanji::wordfilters().size());
}

void FilterListForm::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::ActivationChange)
    {
        if (qApp->activeWindow() != this && (qApp->activeWindow() == nullptr || qApp->activeWindow()->parent() != this))
            deleteLater();
        else if (qApp->activeWindow() == this)
            ui->addButton->setEnabled(true);
    }

    base::changeEvent(e);
}


//-------------------------------------------------------------

