/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QSignalMapper>
#include <QtEvents>
#include "zkanjimain.h"
#include "romajizer.h"
#include "radform.h"
#include "ui_radform.h"
#include "zui.h"

RadicalForm::RadicalForm(QWidget *parent) : base(parent), ui(new Ui::RadicalForm), sizeinited(false), model(nullptr), btnresult(Cancel)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_DontShowOnScreen);

    setDisplayMode(RadicalFilterModes::Parts);
    ui->searchEdit->setValidator(&kanaValidator());
    ui->strokeEdit->setValidator(new IntRangeValidator(1, 17, true, this));

    connect(ui->mode1Button, &QToolButton::toggled, this, &RadicalForm::updateMode);
    connect(ui->mode2Button, &QToolButton::toggled, this, &RadicalForm::updateMode);
    connect(ui->mode3Button, &QToolButton::toggled, this, &RadicalForm::updateMode);

    connect(ui->strokeEdit, &QLineEdit::textChanged, this, &RadicalForm::updateSettings);
    connect(ui->searchEdit, &ZLineEdit::textChanged, this, &RadicalForm::updateSettings);
    connect(ui->groupButton, &QToolButton::toggled, this, &RadicalForm::updateSettings);
    connect(ui->namesButton, &QToolButton::toggled, this, &RadicalForm::updateSettings);
    connect(ui->exactButton, &QToolButton::toggled, this, &RadicalForm::updateSettings);

    connect(ui->clearButton, &QPushButton::clicked, this, &RadicalForm::clearSelection);

    connect(ui->prevSelButton, &QToolButton::clicked, this, &RadicalForm::showHidePrevSel);

    connect(ui->radicalGrid, &ZRadicalGrid::selectionTextChanged, this, &RadicalForm::updateSelectionText);
    connect(ui->radicalGrid, &ZRadicalGrid::selectionChanged, this, &RadicalForm::selectionChanged);
    connect(ui->radicalGrid, &ZRadicalGrid::groupingChanged, this, &RadicalForm::groupingChanged);

    connect(ui->dialogButtons, &QDialogButtonBox::clicked, this, &RadicalForm::handleButtons);

    RadicalFiltersModel &m = radicalFiltersModel();
    connect(&m, &RadicalFiltersModel::filterAdded, this, &RadicalForm::radsAdded);
    connect(&m, &RadicalFiltersModel::filterRemoved, this, &RadicalForm::radsRemoved);
    connect(&m, &RadicalFiltersModel::filterMoved, this, &RadicalForm::radsMoved);
    connect(&m, &RadicalFiltersModel::cleared, this, &RadicalForm::radsCleared);

    for (int ix = 0; ix != m.size(); ++ix)
        new QListWidgetItem(m.filterText(m.filters(ix)), ui->radsList);

    connect(ui->radsUpButton, &QToolButton::clicked, this, &RadicalForm::radsMoveUp);
    connect(ui->radsDownButton, &QToolButton::clicked, this, &RadicalForm::radsMoveDown);
    connect(ui->radsRemoveButton, &QToolButton::clicked, this, &RadicalForm::radsRemove);
    connect(ui->radsClearButton, &QToolButton::clicked, this, &RadicalForm::radsClear);
    ui->radsUpButton->setEnabled(false);
    ui->radsDownButton->setEnabled(false);
    ui->radsRemoveButton->setEnabled(false);
    ui->radsClearButton->setEnabled(!m.empty());
    ui->dialogButtons->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->radsList, &QListWidget::currentRowChanged, this, &RadicalForm::radsRowChanged);
    connect(ui->radsList, &QListWidget::itemDoubleClicked, this, &RadicalForm::radsDoubleClicked);
}

RadicalForm::~RadicalForm()
{
    delete ui;
}

void RadicalForm::updateMode()
{
    RadicalFilterModes newmode;

    if (sender() == ui->mode1Button)
        newmode = RadicalFilterModes::Parts;
    else if (sender() == ui->mode2Button)
        newmode = RadicalFilterModes::Radicals;
    else
        newmode = RadicalFilterModes::NamedRadicals;
    setDisplayMode(newmode);
}

void RadicalForm::setDisplayMode(RadicalFilterModes newmode)
{
    if (dynamic_cast<QToolButton*>(sender()) != nullptr && !((QToolButton*)sender())->isChecked())
        return;

    ui->radicalGrid->setDisplayMode((RadicalFilterModes)newmode);
    ui->groupButton->setEnabled(newmode == RadicalFilterModes::NamedRadicals);
    ui->namesButton->setEnabled(newmode == RadicalFilterModes::NamedRadicals);
    ui->searchEdit->setEnabled(newmode == RadicalFilterModes::NamedRadicals);
    ui->exactButton->setEnabled(newmode == RadicalFilterModes::NamedRadicals);
    ui->addButton->setEnabled(newmode != RadicalFilterModes::Radicals);
    //ui->clearButton->setEnabled(newmode != RadicalFilterModes::Radicals);
}

void RadicalForm::setFilter(int index)
{
    ui->radsList->setCurrentRow(index);
    RadicalFiltersModel &m = radicalFiltersModel();
    const RadicalFilter &filter = m.filters(index);
    switch (filter.mode)
    {
    case RadicalFilterModes::Parts:
        ui->mode1Button->setChecked(true);
        break;
    case RadicalFilterModes::Radicals:
        ui->mode2Button->setChecked(true);
        break;
    default:
        ui->groupButton->setChecked(filter.grouped);
        ui->mode3Button->setChecked(true);
        break;
    }
    ui->radicalGrid->setActiveFilter(filter);
}

void RadicalForm::setFromModel(KanjiGridModel *model)
{
    ui->radicalGrid->setFromModel(model);
}

void RadicalForm::setFromList(const std::vector<ushort> &list)
{
    ui->radicalGrid->setFromList(list);
}

//void RadicalForm::clearKanjiList()
//{
//    ui->radicalGrid->clearKanjiList();
//}

void RadicalForm::activeFilter(RadicalFilter &f)
{
    if (btnresult != Ok)
        ui->radicalGrid->activeFilter(f);
    else
        f = *finalfilter.get();
}

std::vector<ushort> RadicalForm::selectionToFilter()
{
    return ui->radicalGrid->selectionToFilter();
}

RadicalForm::Results RadicalForm::result()
{
    return btnresult;
}

void RadicalForm::radsAdded()
{
    RadicalFiltersModel &m = radicalFiltersModel();
    new QListWidgetItem(m.filterText(m.filters(m.size() - 1)), ui->radsList);

    if (ui->radsList->count() == 1)
        ui->radsList->setCurrentRow(0);
    ui->radsUpButton->setEnabled(ui->radsList->currentRow() != 0);
    ui->radsDownButton->setEnabled(ui->radsList->currentRow() != ui->radsList->count() - 1);
    ui->radsRemoveButton->setEnabled(true);
    ui->radsClearButton->setEnabled(true);
}

void RadicalForm::radsRemoved(int index)
{
    delete ui->radsList->takeItem(index);
    ui->radsUpButton->setEnabled(ui->radsList->count() != 0 && ui->radsList->currentRow() != 0);
    ui->radsDownButton->setEnabled(ui->radsList->count() != 0 && ui->radsList->currentRow() != ui->radsList->count() - 1);
    ui->radsRemoveButton->setEnabled(ui->radsList->count() != 0);
    ui->radsClearButton->setEnabled(ui->radsList->count() != 0);
}

void RadicalForm::radsMoved(int index, bool up)
{
    ui->radsList->setUpdatesEnabled(false);
    bool current = ui->radsList->currentRow() == index;
    QListWidgetItem *item = ui->radsList->takeItem(index);
    ui->radsList->insertItem(index + (up ? -1 : 1), item);
    if (current)
        ui->radsList->setCurrentItem(item);
    ui->radsList->setUpdatesEnabled(true);
}

void RadicalForm::radsCleared()
{
    ui->radsList->setUpdatesEnabled(false);
    while (ui->radsList->count())
        delete ui->radsList->takeItem(0);
    ui->radsList->setUpdatesEnabled(true);
    ui->radsUpButton->setEnabled(ui->radsList->count() != 0 && ui->radsList->currentRow() != 0);
    ui->radsDownButton->setEnabled(ui->radsList->count() != 0 && ui->radsList->currentRow() != ui->radsList->count() - 1);
    ui->radsRemoveButton->setEnabled(ui->radsList->count() != 0);
    ui->radsClearButton->setEnabled(ui->radsList->count() != 0);
}

void RadicalForm::radsMoveUp()
{
    RadicalFiltersModel &m = radicalFiltersModel();
    m.move(ui->radsList->currentRow(), true);
}

void RadicalForm::radsMoveDown()
{
    RadicalFiltersModel &m = radicalFiltersModel();
    m.move(ui->radsList->currentRow(), false);
}

void RadicalForm::radsRemove()
{
    RadicalFiltersModel &m = radicalFiltersModel();
    m.remove(ui->radsList->currentRow());
}

void RadicalForm::radsClear()
{
    RadicalFiltersModel &m = radicalFiltersModel();
    m.clear();
}

void RadicalForm::radsRowChanged(int row)
{
    ui->radsUpButton->setEnabled(ui->radsList->count() != 0 && row != 0);
    ui->radsDownButton->setEnabled(ui->radsList->count() != 0 && row != ui->radsList->count() - 1);
    ui->radsRemoveButton->setEnabled(row >= 0 && row < ui->radsList->count());
}

void RadicalForm::radsDoubleClicked(QListWidgetItem *item)
{
    setFilter(ui->radsList->currentRow());
}

void RadicalForm::on_addButton_clicked()
{
    ui->radicalGrid->finalizeSelection();
    ui->searchEdit->setText(QString());
}

void RadicalForm::showEvent(QShowEvent *event)
{
    if (ui->prevSelButton->text().isEmpty())
    {
        if (sizeinited)
        {
            hide();
            showHidePrevSel();
            setAttribute(Qt::WA_DontShowOnScreen, false);
            show();
            return;
        }
        sizeinited = true;
        QShowEvent *e = new QShowEvent();
        qApp->postEvent(this, e);
    }
    base::showEvent(event);
}

void RadicalForm::updateSettings()
{
    if (sender() == ui->strokeEdit)
    {

        int smin;
        int smax;
        findStrIntMinMax(ui->strokeEdit->text().trimmed(), 1, 17, smin, smax);
        if (ui->strokeEdit->text().trimmed().isEmpty() || (smin == 0 && smax == 0))
            ui->radicalGrid->setStrokeLimit(0);
        else
        {
            smin = clamp(smin, 1, 17);
            smax = clamp(smax, 1, 17);
            ui->radicalGrid->setStrokeLimits(smin, smax);
        }
        return;
    }

    if (sender() == ui->groupButton)
    {
        ui->radicalGrid->setGroupDisplay(ui->groupButton->isChecked());
        return;
    }
    if (sender() == ui->namesButton)
    {
        ui->radicalGrid->setNamesDisplay(ui->namesButton->isChecked());
        return;
    }
    if (sender() == ui->exactButton)
    {
        ui->radicalGrid->setExactMatch(ui->exactButton->isChecked());
        return;
    }
    if (sender() == ui->searchEdit)
    {
        ui->radicalGrid->setSearchName(hiraganize(ui->searchEdit->text()));
        return;
    }
}

void RadicalForm::clearSelection()
{
    ui->searchEdit->setText(QString());
    ui->strokeEdit->setText(QString());
    ui->radicalGrid->setUpdatesEnabled(false);
    ui->radicalGrid->setStrokeLimit(0);
    ui->radicalGrid->setSearchName(QString());
    ui->radicalGrid->clearSelection();
    ui->radicalGrid->setUpdatesEnabled(true);
}

void RadicalForm::showHidePrevSel()
{
    int h = ui->expandWidget->frameGeometry().height() + centralWidget()->layout()->spacing();

    QSize s = geometry().size();
    if (ui->expandWidget->isVisibleTo(this))
    {
        ui->prevSelButton->setText(tr("Previous selections") + QStringLiteral(" >>"));
        ui->expandWidget->hide();

        centralWidget()->layout()->activate();
        layout()->activate();
        ui->radsButtons->layout()->activate();

        s.setHeight(s.height() - h);
        resize(s);
    }
    else
    {
        ui->prevSelButton->setText(tr("Previous selections") + QStringLiteral(" <<"));
        s.setHeight(s.height() + h);
        resize(s);
        ui->expandWidget->show();
    }
}

void RadicalForm::updateSelectionText(const QString &str)
{
    ui->selectionLabel->setText(str);
    ui->dialogButtons->button(QDialogButtonBox::Ok)->setEnabled(!str.isEmpty());
}

void RadicalForm::handleButtons(QAbstractButton *button)
{
    if (ui->dialogButtons->buttonRole(button) == QDialogButtonBox::AcceptRole)
    {
        ui->radicalGrid->finalizeSelection();
        RadicalFilter *f = new RadicalFilter();
        ui->radicalGrid->activeFilter(*f);
        finalfilter.reset(f);
        btnresult = Ok;
        emit resultIsOk();
    }
    close();
}
