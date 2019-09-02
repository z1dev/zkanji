/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QWidget>
#include <QToolButton>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "wordgroupwidget.h"
#include "ui_wordgroupwidget.h"
#include "zui.h"
#include "zkanjimain.h"
#include "words.h"
#include "ztreeview.h"
#include "groups.h"
#include "zgrouptreemodel.h"
#include "zdictionarymodel.h"
#include "wordtestsettingsform.h"
#include "groupsettings.h"
#include "wordstudyform.h"
#include "printpreviewform.h"
#include "globalui.h"
#include "ranges.h"

//-------------------------------------------------------------


WordGroupWidget::WordGroupWidget(QWidget *parent) : base(parent), ui(new Ui::WordGroupWidget), group(nullptr)
{
    ui->setupUi(this);

    ui->dictWidget->setListMode(DictionaryWidget::Filtered);
    ui->testButtonsWidget->hide();
    ui->groupStack->setCurrentWidget(ui->dictPage);
    ui->groupWidget->setMultiSelect(true);

    studyButton = new QToolButton(this);
    studyButton->setIconSize(QSize(18, 18));
    testpaperico = QIcon(QStringLiteral(":/testpaper.svg"));
    testsettingsico = QIcon(QStringLiteral(":/testsettings.svg"));
    studyButton->setIcon(testsettingsico);
    studyButton->setAutoRaise(true);
    studyButton->setEnabled(false);
    connect(studyButton, &QToolButton::clicked, this, &WordGroupWidget::studyButtonClicked);

    printButton = new QToolButton(this);
    printButton->setIconSize(QSize(18, 18));
    printButton->setIcon(QIcon(QStringLiteral(":/print.svg")));
    printButton->setAutoRaise(true);
    printButton->setEnabled(false);
    connect(printButton, &QToolButton::clicked, this, &WordGroupWidget::printButtonClicked);

    ui->groupWidget->addButtonWidget(studyButton);
    ui->groupWidget->addButtonWidget(printButton);

    ui->groupWidget->setDictionary(ZKanji::dictionary(0));
    ui->groupWidget->setMode(GroupWidget::Words);

    ui->dictWidget->setExamplesInteraction(false);
    ui->dictWidget->setWordDragEnabled(true);
    connect(ui->dictWidget, &DictionaryWidget::rowSelectionChanged, this, &WordGroupWidget::wordSelectionChanged);

    //delButton = new QToolButton(this);
    //delButton->setIconSize(QSize(18, 18));
    //delButton->setIcon(QIcon(QStringLiteral(":/delmark.svg")));
    //delButton->setAutoRaise(true);
    //delButton->setEnabled(false);
    //ui->dictWidget->addFrontWidget(delButton, true);
    //connect(delButton, &QToolButton::clicked, this, &WordGroupWidget::delButtonClicked);

    //downButton = new QToolButton(this);
    //downButton->setIconSize(QSize(18, 18));
    //downButton->setIcon(QIcon(QStringLiteral(":/downmark.svg")));
    //downButton->setAutoRaise(true);
    //downButton->setEnabled(false);
    //downButton->setAutoRepeat(true);
    //ui->dictWidget->addFrontWidget(downButton);
    //connect(downButton, &QToolButton::clicked, this, &WordGroupWidget::downButtonClicked);

    //upButton = new QToolButton(this);
    //upButton->setIconSize(QSize(18, 18));
    //upButton->setIcon(QIcon(QStringLiteral(":/upmark.svg")));
    //upButton->setAutoRaise(true);
    //upButton->setEnabled(false);
    //upButton->setAutoRepeat(true);
    //ui->dictWidget->addFrontWidget(upButton);
    //connect(upButton, &QToolButton::clicked, this, &WordGroupWidget::upButtonClicked);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    ui->dictWidget->addFrontWidget(line);

    ui->splitter->setSizes({ 150, 250 });
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);

    ui->dictWidget->setGroup(nullptr);

    connect(gUI, &GlobalUI::appWindowsVisibilityChanged, this, &WordGroupWidget::appWindowsVisibilityChanged);

    connect(ui->continueButton2, &QPushButton::clicked, this, &WordGroupWidget::on_continueButton_clicked);
    connect(ui->abortButton2, &QPushButton::clicked, this, &WordGroupWidget::on_abortButton_clicked);

    retranslate();
}

WordGroupWidget::~WordGroupWidget()
{
    delete ui;
}

void WordGroupWidget::saveXMLSettings(QXmlStreamWriter &writer) const
{
    writer.writeAttribute("splitsizes", QString("%1,%2").arg(ui->splitter->sizes()[0]).arg(ui->splitter->sizes()[1]));

    writer.writeStartElement("Dictionary");
    ui->dictWidget->saveXMLSettings(writer);
    writer.writeEndElement();
}

void WordGroupWidget::loadXMLSettings(QXmlStreamReader &reader)
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

    while (reader.readNextStartElement())
    {
        if (reader.name() == "Dictionary")
            ui->dictWidget->loadXMLSettings(reader);
        else
            reader.skipCurrentElement();
    }
}

void WordGroupWidget::assignStatusBar(ZStatusBar *bar)
{
    ui->dictWidget->assignStatusBar(bar);
}

//void WordGroupWidget::makeModeSpace(const QSize &size)
//{
//    ui->groupWidget->makeModeSpace(size);
//}

void WordGroupWidget::setDictionary(Dictionary *d)
{
    ui->dictWidget->setDictionary(d);
    ui->groupWidget->setDictionary(d);
}

bool WordGroupWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        retranslate();
    }

    return base::event(e);
}

//void WordGroupWidget::on_groupWidget_currentItemChanged(GroupBase *current)
//{
//    if (group == current)
//        return;
//
//    group = dynamic_cast<WordGroup*>(current);
//
//    if (current == nullptr || current->isCategory())
//    {
//        studyButton->setEnabled(false);
//        printButton->setEnabled(false);
//        ui->dictWidget->setGroup(nullptr);
//        return;
//    }
//
//    updateStudyDisplay();
//
//    studyButton->setEnabled(true);
//    printButton->setEnabled(true);
//
//    ui->dictWidget->setGroup(group);
//}

void WordGroupWidget::on_groupWidget_selectionChanged()
{
    std::vector<GroupBase*> groups;
    ui->groupWidget->selectedGroups(groups);

    WordGroup *selgroup = groups.size() == 1 ? dynamic_cast<WordGroup*>(groups.back()) : nullptr;

    if (group != nullptr && selgroup == group)
        return;

    //if (selgroup == nullptr)
    //{
    //    studyButton->setEnabled(false);
    //    printButton->setEnabled(false);
    //    //ui->dictWidget->setGroup(nullptr);
    //}

    group = selgroup;
    updateStudyDisplay();

    studyButton->setEnabled(selgroup != nullptr);

    bool haswords = false;
    for (int ix = 0, siz = tosigned(groups.size()); !haswords && ix != siz; ++ix)
    {
        GroupBase *g = groups[ix];
        if (g == nullptr || g->isCategory())
            continue;
        haswords = !g->isEmpty();
    }
            
    printButton->setEnabled(haswords);

    if (selgroup != nullptr)
        ui->dictWidget->setGroup(selgroup);
    else
        ui->dictWidget->setModel(nullptr);
}

void WordGroupWidget::wordSelectionChanged()
{
    ui->delButton->setEnabled(group != nullptr && ui->dictWidget->selectionSize() != 0);
    ui->upButton->setEnabled(group != nullptr && !ui->dictWidget->filtered() && ui->dictWidget->hasSelection() && !ui->dictWidget->rowSelected(0) && ui->dictWidget->continuousSelection());
    ui->downButton->setEnabled(group != nullptr && !ui->dictWidget->filtered() && ui->dictWidget->hasSelection() && !ui->dictWidget->rowSelected(ui->dictWidget->rowCount() - 1) && ui->dictWidget->continuousSelection());
}

void WordGroupWidget::studyButtonClicked()
{
    if (!group->studyData().suspended())
    {
        WordTestSettingsForm *form = new WordTestSettingsForm(group, gUI->activeMainForm());
        form->exec();
        return;
    }
    else
    {
        WordStudyForm *study = new WordStudyForm();
        study->exec(&group->studyData());
    }
}

void WordGroupWidget::printButtonClicked()
{
    PrintPreviewForm *form = new PrintPreviewForm(gUI->activeMainForm());
    if (group != nullptr)
        form->exec(ui->groupWidget->dictionary(), group->getIndexes());
    else
    {
        std::vector<GroupBase*> groups;
        ui->groupWidget->selectedGroups(groups);

        std::vector<int> windexes;

        QSet<int> found;
        for (int ix = 0, siz = tosigned(groups.size()); ix != siz; ++ix)
        {
            if (!groups[ix]->isCategory())
            {
                WordGroup *g = (WordGroup*)groups[ix];
                const std::vector<int> &list = g->getIndexes();
                windexes.reserve(windexes.size() + list.size());
                for (int ind : list)
                {
                    if (found.contains(ind))
                        continue;
                    windexes.push_back(ind);
                }
            }
        }
        form->exec(ui->groupWidget->dictionary(), windexes);
    }
}

void WordGroupWidget::on_delButton_clicked()
{
    std::vector<int> indexes;
    ui->dictWidget->selectedRows(indexes);
    ui->dictWidget->unfilteredRows(indexes, indexes);
    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);
    group->remove(ranges);
}

void WordGroupWidget::on_upButton_clicked()
{
    std::vector<int> indexes;
    ui->dictWidget->selectedRows(indexes);
    ui->dictWidget->unfilteredRows(indexes, indexes);
    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);
    group->move(ranges, ranges[0]->first - 1);
}

void WordGroupWidget::on_downButton_clicked()
{
    std::vector<int> indexes;
    ui->dictWidget->selectedRows(indexes);
    ui->dictWidget->unfilteredRows(indexes, indexes);
    smartvector<Range> ranges;
    _rangeFromIndexes(indexes, ranges);
    group->move(ranges, ranges[0]->last + 2);
}

void WordGroupWidget::appWindowsVisibilityChanged(bool shown)
{
    if (shown)
        updateStudyDisplay();
}

void WordGroupWidget::updateStudyDisplay()
{
    if (group == nullptr || !group->studyData().suspended())
    {
        studyButton->setIcon(testsettingsico);
        ui->groupStack->setCurrentWidget(ui->dictPage);
        ui->testButtonsWidget->hide();
    }
    else
    {
        studyButton->setIcon(testpaperico);
        if (Settings::group.hidesuspended)
            ui->groupStack->setCurrentWidget(ui->buttonPage);
        else
            ui->groupStack->setCurrentWidget(ui->dictPage);
        ui->testButtonsWidget->show();
    }
}

void WordGroupWidget::retranslate()
{
    studyButton->setToolTip(tr("Study with group..."));
    printButton->setToolTip(tr("Print preview..."));
}

void WordGroupWidget::on_showButton_clicked()
{
    ui->groupStack->setCurrentWidget(ui->dictPage);
}

void WordGroupWidget::on_continueButton_clicked()
{
    studyButtonClicked();
}

void WordGroupWidget::on_abortButton_clicked()
{
    if (QMessageBox::question(this, "zkanji", tr("The suspended test will be aborted. Are you sure?"), QMessageBox::Yes, QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    group->studyData().abort();
    updateStudyDisplay();
}

void WordGroupWidget::on_hideButton_clicked()
{
    bool oldhide = Settings::group.hidesuspended;
    Settings::group.hidesuspended = true;
    updateStudyDisplay();
    Settings::group.hidesuspended = oldhide;
}


//-------------------------------------------------------------


