/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QWidget>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "zkanjimain.h"
#include "kanjigroupwidget.h"
#include "ui_kanjigroupwidget.h"
#include "zgrouptreemodel.h"
#include "groups.h"
#include "words.h"
#include "zui.h"
#include "zkanjigridmodel.h"
#include "ranges.h"
#include "generalsettings.h"
#include "zkanjiform.h"


//-------------------------------------------------------------


KanjiGroupWidget::KanjiGroupWidget(QWidget *parent) : base(parent), ui(new Ui::KanjiGroupWidget)
{
    ui->setupUi(this);

    //connect(ui->groupWidget, &GroupWidget::currentItemChanged, this, &KanjiGroupWidget::currChanged);

    ui->groupWidget->setDictionary(ZKanji::dictionary(0));
    ui->groupWidget->setMode(GroupWidget::Kanji);
    ui->groupWidget->setMultiSelect(true);

    connect(ui->kanjiGrid, &ZKanjiGridView::selectionChanged, this, &KanjiGroupWidget::kanjiSelectionChanged);

    ui->splitter->setSizes({ Settings::scaled(150), Settings::scaled(250) });

    if (dynamic_cast<ZKanjiForm*>(window()) != nullptr)
        ui->kanjiGrid->assignStatusBar(ui->kanjiStatus);
    else
        ui->kanjiStatus->hide();
}

KanjiGroupWidget::~KanjiGroupWidget()
{
    delete ui;
}

void KanjiGroupWidget::saveXMLSettings(QXmlStreamWriter &writer) const
{
    writer.writeAttribute("splitsizes", QString("%1,%2").arg(ui->splitter->sizes()[0]).arg(ui->splitter->sizes()[1]));
    writer.writeCharacters(QString());
}

void KanjiGroupWidget::loadXMLSettings(QXmlStreamReader &reader)
{
    bool ok = false;
    auto sizestr = reader.attributes().value("splitsizes").split(',');;
    if (sizestr.size() == 2)
    {
        QList<int> sizes;
        sizes << sizestr.at(0).toInt(&ok);
        sizes << (ok ? sizestr.at(1).toInt(&ok) : -1);
        if (ok)
            ui->splitter->setSizes(sizes);
    }
    reader.skipCurrentElement();
}

void KanjiGroupWidget::assignStatusBar(ZStatusBar *bar)
{
    ui->kanjiGrid->assignStatusBar(bar);
}

//void KanjiGroupWidget::makeModeSpace(const QSize &size)
//{
//    ui->groupWidget->makeModeSpace(size);
//}

void KanjiGroupWidget::setDictionary(Dictionary *d)
{
    ui->kanjiGrid->setDictionary(d);
    ui->groupWidget->setDictionary(d);
}

bool KanjiGroupWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    return base::event(e);
}

void KanjiGroupWidget::kanjiSelectionChanged()
{
    bool hassel = ui->kanjiGrid->selCount() != 0;
    ui->delButton->setEnabled(hassel);
    ui->upButton->setEnabled(hassel && !ui->kanjiGrid->selected(0) && ui->kanjiGrid->continuousSelection());
    ui->downButton->setEnabled(hassel && !ui->kanjiGrid->selected(tosigned(ui->kanjiGrid->model()->size()) - 1) && ui->kanjiGrid->continuousSelection());

}

//void KanjiGroupWidget::on_groupWidget_currentItemChanged(GroupBase *current)
//{
//    if (current == nullptr || current->isCategory())
//    {
//        ui->kanjiGrid->setModel(nullptr);
//        return;
//    }
//
//    ui->kanjiGrid->setModel(&kanjiGroupModel((KanjiGroup*)current));
//}

void KanjiGroupWidget::on_groupWidget_selectionChanged()
{
    std::vector<GroupBase*> groups;
    ui->groupWidget->selectedGroups(groups);

    KanjiGroup *selgroup = groups.size() == 1 ? dynamic_cast<KanjiGroup*>(groups.back()) : nullptr;

    if (selgroup != nullptr)
        ui->kanjiGrid->setModel(selgroup->groupModel());
    else
        ui->kanjiGrid->setModel(nullptr);

}

void KanjiGroupWidget::on_delButton_clicked(bool /*checked*/)
{
    std::vector<int> indexes;
    ui->kanjiGrid->selectedCells(indexes);
    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);
    ui->kanjiGrid->model()->kanjiGroup()->remove(ranges);
}

void KanjiGroupWidget::on_upButton_clicked(bool /*checked*/)
{
    std::vector<int> indexes;
    ui->kanjiGrid->selectedCells(indexes);
    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);
    ui->kanjiGrid->model()->kanjiGroup()->move(ranges, ranges[0]->first - 1);
}

void KanjiGroupWidget::on_downButton_clicked(bool /*checked*/)
{
    std::vector<int> indexes;
    ui->kanjiGrid->selectedCells(indexes);
    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);
    ui->kanjiGrid->model()->kanjiGroup()->move(ranges, ranges[0]->last + 2);
}


//-------------------------------------------------------------
