/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include <QItemSelectionModel>

#include "groupwidget.h"
#include "zkanjimain.h"
#include "groups.h"
#include "ui_groupwidget.h"
#include "zgrouptreemodel.h"
#include "words.h"
#include "zkanjiwidget.h"

#include "checked_cast.h"

//-------------------------------------------------------------


GroupWidget::GroupWidget(QWidget *parent) : base(parent), ui(new Ui::GroupWidget), dict(nullptr), mode(Words), groupmodel(nullptr), multisel(false), onlycateg(false)
{
    ui->setupUi(this);

    ui->btnDelGroup->setEnabled(false);
}

GroupWidget::~GroupWidget()
{
    delete ui;
}

bool GroupWidget::multiSelect() const
{
    return multisel;
}

void GroupWidget::setMultiSelect(bool mul)
{
    if (multisel == mul)
        return;
    multisel = mul;
    if (ui->groupsTree->model() != nullptr /*&& ui->groupsTree->model()->isFiltered()*/)
        return;
    ui->groupsTree->setSelectionMode(multisel ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
}

void GroupWidget::showOnlyCategories()
{
    if (onlycateg)
        return;
    ui->btnAddGroup->hide();
    ui->filterWidget->hide();
    onlycateg = true;

    updateGroups();
    ui->groupsTree->selectTop();
}

bool GroupWidget::onlyCategories() const
{
    return onlycateg;
}

//void GroupWidget::makeModeSpace(const QSize &size)
//{
//    ui->buttonsLayout->setContentsMargins(size.width(), 0, 0, 0);
//}

void GroupWidget::setDictionary(Dictionary *d)
{
    if (dict == d)
        return;
    dict = d;

    updateGroups();
    ui->groupsTree->selectTop();

    //emit currentItemChanged(nullptr);
}

Dictionary* GroupWidget::dictionary()
{
    return dict;
}

void GroupWidget::setMode(Modes newmode)
{
    if (mode == newmode)
        return;
    mode = newmode;

    updateGroups();
    ui->groupsTree->selectTop();
}

GroupBase* GroupWidget::current()
{
    TreeItem *item = ui->groupsTree->currentItem();
    if (item == nullptr)
        return nullptr;
    return (GroupBase*)item->userData();
}

GroupBase* GroupWidget::singleSelected(bool allowcateg)
{
    std::vector<GroupBase*> groups;
    ui->groupsTree->selectedGroups(groups);
    if (groups.size() != 1 || (!allowcateg && groups.front()->isCategory()))
        return nullptr;
    return groups.front();
}

void GroupWidget::selectGroup(GroupBase *group)
{
    ui->groupsTree->select(group);
}

void GroupWidget::selectGroups(const std::vector<GroupBase*> &groups)
{
    ui->groupsTree->selectGroups(groups);
}

void GroupWidget::selectedGroups(std::vector<GroupBase*> &result)
{
    ui->groupsTree->selectedGroups(result);
}

//QToolButton* GroupWidget::addButton(const QString &label)
//{
//    QToolButton *button = new QToolButton(this);
//    if (label.left(2) == ":/")
//        button->setIcon(QIcon(label));
//    else
//        button->setText(label);
//    addWidget(button);
//    return button;
//}

void GroupWidget::addButtonWidget(QAbstractButton *widget)
{
    ui->buttonLayout->insertWidget(ui->buttonLayout->count(), widget);
}

void GroupWidget::addControlWidget(QWidget *widget, bool rightAlign)
{
    if (rightAlign)
        ui->controlsLayout->addWidget(widget);
    else
    {
        int pos = 1;
        // Find the spacer item to insert widgets in front of it
        for (int siz = ui->controlsLayout->count(); pos != siz; ++pos)
            if (ui->controlsLayout->itemAt(siz - pos)->spacerItem() != nullptr)
                break;
        ui->controlsLayout->insertWidget(ui->controlsLayout->count() - pos, widget);
    }
}

bool GroupWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    return base::event(e);
}

void GroupWidget::keyPressEvent(QKeyEvent *e)
{
    if (ui->groupsTree->currentItem() != nullptr && !ui->groupsTree->currentCompleted(false))
    {
        e->accept();
        return;
    }
    base::keyPressEvent(e);
}

void GroupWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu popup;

    QAction *a;
    a = popup.addAction(ui->btnAddCateg->toolTip());
    connect(a, &QAction::triggered, this, &GroupWidget::on_btnAddCateg_clicked);
    a->setEnabled(ui->btnAddCateg->isEnabled());

    a = popup.addAction(ui->btnAddGroup->toolTip());
    connect(a, &QAction::triggered, this, &GroupWidget::on_btnAddGroup_clicked);
    a->setEnabled(ui->btnAddGroup->isEnabled());

    a = popup.addAction(ui->btnDelGroup->toolTip());
    connect(a, &QAction::triggered, this, &GroupWidget::on_btnDelGroup_clicked);
    a->setEnabled(ui->btnDelGroup->isEnabled());

    if (ui->buttonLayout->count() > 3)
    {
        popup.addSeparator();

        for (int ix = 3, siz = ui->buttonLayout->count(); ix != siz; ++ix)
        {
            QAbstractButton *btn = dynamic_cast<QAbstractButton*>(ui->buttonLayout->itemAt(ix)->widget());
            if (btn == nullptr)
                continue;

            a = popup.addAction(btn->toolTip());
            connect(a, &QAction::triggered, btn, &QAbstractButton::clicked);
            a->setEnabled(btn->isEnabled());

        }
    }

    ZKanjiWidget *w = ZKanjiWidget::getZParent(this);
    if (w != nullptr)
        w->addDockAction(&popup);

    popup.exec(e->globalPos());
    e->accept();
}

void GroupWidget::updateGroups()
{
    if (dict == nullptr)
    {
        if (groupmodel != nullptr)
            groupmodel->deleteLater();
        ui->groupsTree->setModel(nullptr);
        groupmodel = nullptr;
        return;
    }

    if (mode == Words)
    {
        if (!onlycateg)
            groupmodel = new GroupTreeModel(&dict->wordGroups(), ui->filterEdit->text().trimmed().toLower(), ui->groupsTree);
        else
            groupmodel = new GroupTreeModel(&dict->wordGroups(), true, true, ui->groupsTree);
    }
    else
    {
        if (!onlycateg)
            groupmodel = new GroupTreeModel(&dict->kanjiGroups(), ui->filterEdit->text().trimmed().toLower(), ui->groupsTree);
        else
            groupmodel = new GroupTreeModel(&dict->kanjiGroups(), true, true, ui->groupsTree);
    }

    if (ui->groupsTree->model() != nullptr)
        ui->groupsTree->model()->deleteLater();

    ui->groupsTree->setModel(groupmodel);

    ui->groupsTree->setSelectionMode(/*!groupmodel->isFiltered() &&*/ multisel ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);

    TreeItem *tcurr = ui->groupsTree->currentItem();
    emit currentItemChanged(tcurr != nullptr ? (GroupBase*)tcurr->userData() : nullptr);
    emit selectionChanged();

    if (onlycateg)
        ui->groupsTree->expandAll();
}

void GroupWidget::on_groupsTree_currentItemChanged(const TreeItem *tcurr, const TreeItem * /*tprev*/)
{
    // Only enable any kind of interaction if the current item is a fully created category or
    // group. When the editor is open for a new item, we should report that no item is
    // selected.
    if (!ui->groupsTree->currentCompleted(false))
        tcurr = nullptr;

    ui->btnAddCateg->setEnabled((!onlycateg || (tcurr != nullptr && tcurr != ui->groupsTree->model()->items(0))) && ui->filterEdit->text().trimmed().isEmpty());

    emit currentItemChanged(tcurr != nullptr ? (GroupBase*)tcurr->userData() : nullptr);
}

void GroupWidget::on_groupsTree_itemSelectionChanged()
{
    ui->btnDelGroup->setEnabled(ui->groupsTree->model() != nullptr && (!onlycateg || !ui->groupsTree->isSelected(ui->groupsTree->model()->items(0)) ) && ui->groupsTree->currentCompleted(true) && ui->groupsTree->selectionModel()->hasSelection());
    ui->btnAddGroup->setEnabled(ui->groupsTree->model() != nullptr && !onlycateg && ui->groupsTree->currentCompleted(false) && ui->filterEdit->text().trimmed().isEmpty());
    ui->btnAddCateg->setEnabled(ui->groupsTree->model() != nullptr && (!onlycateg || (ui->groupsTree->currentItem() != nullptr && ui->groupsTree->currentItem() != ui->groupsTree->model()->items(0))) && ui->groupsTree->currentCompleted(false) && ui->filterEdit->text().trimmed().isEmpty());

    emit selectionChanged();
}

void GroupWidget::on_groupsTree_itemCreated()
{
    ui->btnDelGroup->setEnabled(true);
    ui->btnAddGroup->setEnabled(!onlycateg && ui->filterEdit->text().trimmed().isEmpty());
    ui->btnAddCateg->setEnabled(ui->filterEdit->text().trimmed().isEmpty());
    TreeItem *item = ui->groupsTree->currentItem();
    emit currentItemChanged(item != nullptr ? (GroupBase*)item->userData() : nullptr);
}

//void GroupWidget::on_groupsTree_collapsed(const TreeItem *item)
//{
//    QItemSelectionModel *sel = ui->groupsTree->selectionModel();
//    QModelIndexList slist = sel->selectedIndexes();
//
//    QItemSelection items;
//    for (QModelIndex &i : slist)
//    {
//        TreeItem *cur = ((TreeItem*)i.internalPointer())->parent();
//        while (cur)
//        {
//            if (cur == item)
//            {
//                items.select(i, i);
//                break;
//            }
//            cur = cur->parent();
//        }
//    }
//    if (!items.empty())
//        sel->select(items, QItemSelectionModel::Deselect);
//}

void GroupWidget::on_btnAddCateg_clicked()
{
    TreeItem *item;
    item = ui->groupsTree->currentParent();
    ui->groupsTree->createCategory(item);
}

void GroupWidget::on_btnAddGroup_clicked()
{
    if (onlycateg)
        return;

    TreeItem *item;
    item = ui->groupsTree->currentParent();
    ui->groupsTree->createGroup(item);
}

void GroupWidget::on_btnDelGroup_clicked()
{
    std::vector<GroupBase*> groups;
    ui->groupsTree->selectedGroups(groups);

    bool haswords = false;
    for (int ix = 0, siz = tosigned(groups.size()); !haswords && ix != siz; ++ix)
    {
        GroupBase *g = groups[ix];
        if (g == nullptr)
            continue;
        if (g->isCategory())
        {
            GroupCategoryBase *cat = (GroupCategoryBase*)groups[ix];
            for (int iy = 0, sizy = cat->categoryCount(); iy != sizy; ++iy)
            {
                groups.push_back(cat->categories(iy));
                ++siz;
            }
            for (int iy = 0, sizy = tosigned(cat->size()); iy != sizy; ++iy)
                haswords = !cat->items(iy)->isEmpty();
        }
        else
            haswords = !g->isEmpty();
    }

    //TreeItem *item = ui->groupsTree->currentItem();


    if (haswords)
    {
        QString str = !onlycateg ? tr("The selected groups or categories are not empty and will be deleted.\n\nAre you sure?") : tr("The selected categories are not empty and will be deleted.\n\nAre you sure?");
        if (QMessageBox::question(window(), "zkanji", str) != QMessageBox::Yes)
            return;
    }

    //ui->groupsTree->remove(groups);

    if (mode == Words)
        dict->wordGroups().remove(groups);
    else
        dict->kanjiGroups().remove(groups);
}

void GroupWidget::on_filterEdit_textChanged(const QString &text)
{
    if (onlycateg)
        return;

    ui->btnAddCateg->setEnabled(text.trimmed().isEmpty());
    ui->btnAddGroup->setEnabled(text.trimmed().isEmpty());

    std::vector<GroupBase*> groups;
    ui->groupsTree->selectedGroups(groups);

    updateGroups();

    ui->groupsTree->selectGroups(groups);
}


//-------------------------------------------------------------
