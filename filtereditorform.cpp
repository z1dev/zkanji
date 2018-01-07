/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include "filtereditorform.h"
#include "ui_filtereditorform.h"
#include "words.h"
#include "globalui.h"


//static std::map<int, FilterEditorForm*> filterforms;

//-------------------------------------------------------------


FilterEditorForm::FilterEditorForm(int filterindex, QWidget *parent) : base(parent), ui(new Ui::FilterEditorForm), filterindex(filterindex)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    gUI->scaleWidget(this);

    //// Making sure only one connection is added.
    //if (filterforms.find(-1) == filterforms.end())
    //{
    //    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterErased, &FilterEditorForm::filterErased);
    //    filterforms[-1] = nullptr;
    //}

    ui->applyButton->setEnabled(false);

    connect(ui->nameEdit, &ZLineEdit::textEdited, this, &FilterEditorForm::allowApply);
    connect(ui->anyButton, &QRadioButton::toggled, this, &FilterEditorForm::allowApply);
    connect(ui->allButton, &QRadioButton::toggled, this, &FilterEditorForm::allowApply);
    connect(ui->attribWidget, &WordAttribWidget::changed, this, &FilterEditorForm::allowApply);
    connect(&ZKanji::wordfilters(), &WordAttributeFilterList::filterChanged, this, &FilterEditorForm::filterChanged);

    if (filterindex != -1)
    {
        const WordAttributeFilter &filter = ZKanji::wordfilters().items(filterindex);
        ui->attribWidget->setChecked(filter.attrib, filter.inf, filter.jlpt);
        if (filter.matchtype == FilterMatchType::AllMustMatch)
            ui->allButton->setChecked(true);
        else
            ui->anyButton->setChecked(true);
        ui->nameEdit->setText(filter.name);
    }
    allowApply();
}

FilterEditorForm::~FilterEditorForm()
{
    delete ui;
}

void FilterEditorForm::closeEvent(QCloseEvent *e)
{
    if (qApp->activeWindow() == this)
    {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }

    //if (filterindex != -1)
    //{
    //    auto at = filterforms.find(filterindex);
    //    if (at != filterforms.end())
    //        filterforms.erase(at);
    //}

    base::closeEvent(e);
}

void FilterEditorForm::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::ActivationChange)
    {
        //qApp->processEvents();
        if (qApp->activeWindow() != this && (qApp->activeWindow() == nullptr || (qApp->activeWindow()->parentWidget() != this)))
            deleteLater();
    }

    base::changeEvent(e);
}

///* static */ void FilterEditorForm::filterErased(int index)
//{
//    auto it = filterforms.find(index);
//    if (it != filterforms.end())
//    {
//        FilterEditorForm *f = it->second;
//        f->filterindex = -1;
//        filterforms.erase(it);
//        f->close();
//    }
//
//    auto tmp = filterforms;
//    filterforms.clear();
//    for (auto &p : tmp)
//    {
//        if (p.first < index)
//            filterforms.insert(p);
//        else if (p.first > index)
//        {
//            filterforms[p.first - 1] = p.second;
//            --p.second->filterindex;
//        }
//    }
//}

void FilterEditorForm::on_applyButton_clicked()
{
    if (!checkName())
        return;

    const WordAttributeFilter &filter = ZKanji::wordfilters().items(filterindex);

    ZKanji::wordfilters().rename(filterindex, ui->nameEdit->text().trimmed());
    ZKanji::wordfilters().update(filterindex, ui->attribWidget->checkedTypes(), ui->attribWidget->checkedInfo(), ui->attribWidget->checkedJLPT(), ui->allButton->isChecked() ? FilterMatchType::AllMustMatch : FilterMatchType::AnyCanMatch);

    ui->applyButton->setEnabled(false);
}

void FilterEditorForm::on_acceptButton_clicked()
{
    if (!checkName())
        return;

    if (filterindex == -1)
        ZKanji::wordfilters().add(ui->nameEdit->text().trimmed(), ui->attribWidget->checkedTypes(), ui->attribWidget->checkedInfo(), ui->attribWidget->checkedJLPT(), ui->allButton->isChecked() ? FilterMatchType::AllMustMatch : FilterMatchType::AnyCanMatch);
    else
    {
        const WordAttributeFilter &filter = ZKanji::wordfilters().items(filterindex);

        ZKanji::wordfilters().rename(filterindex, ui->nameEdit->text().trimmed());
        ZKanji::wordfilters().update(filterindex, ui->attribWidget->checkedTypes(), ui->attribWidget->checkedInfo(), ui->attribWidget->checkedJLPT(), ui->allButton->isChecked() ? FilterMatchType::AllMustMatch : FilterMatchType::AnyCanMatch);
    }
    close();
}

void FilterEditorForm::on_cancelButton_clicked()
{
    close();
}

void FilterEditorForm::filterChanged(int index)
{
    if (index != filterindex)
        return;

    ui->nameEdit->setText(ZKanji::wordfilters().items(filterindex).name);
    allowApply();
}

bool FilterEditorForm::checkName(bool msgbox)
{
    if (ui->nameEdit->text().trimmed().isEmpty())
    {
        if (msgbox)
            QMessageBox::warning(this, QStringLiteral("zkanji"), tr("Please specify a name for the filter!"), QMessageBox::Ok);
        return false;
    }

    if ((filterindex == -1 || ZKanji::wordfilters().items(filterindex).name != ui->nameEdit->text().trimmed()) && ZKanji::wordfilters().itemIndex(ui->nameEdit->text().trimmed()) != -1)
    {
        if (msgbox)
            QMessageBox::warning(this, QStringLiteral("zkanji"), tr("Another filter exists with the same name. Please change the name and try again!"), QMessageBox::Ok);
        return false;
    }

    return true;
}

void FilterEditorForm::allowApply()
{
    if (filterindex != -1)
    {
        const WordAttributeFilter &filter = ZKanji::wordfilters().items(filterindex);
        bool unchanged = (ui->attribWidget->checkedTypes() == filter.attrib &&
            ui->attribWidget->checkedInfo() == filter.inf &&
            ui->attribWidget->checkedJLPT() == filter.jlpt) &&
            ((filter.matchtype == FilterMatchType::AllMustMatch) == ui->allButton->isChecked()) &&
            ((filter.matchtype == FilterMatchType::AnyCanMatch) == ui->anyButton->isChecked());

        if (unchanged)
        {
            ui->applyButton->setEnabled(false);
            ui->acceptButton->setEnabled(true);
            return;
        }
    }

    ui->applyButton->setEnabled(filterindex != -1 && checkName(false));
    ui->acceptButton->setEnabled((filterindex == -1 && checkName(false)) || ui->applyButton->isEnabled());
}


//-------------------------------------------------------------


void editWordFilter(int filter, QWidget *parent)
{
    //if (filter != -1)
    //{
    //    auto it = filterforms.find(filter);
    //    if (it != filterforms.end())
    //    {
    //        it->second->raise();
    //        it->second->activateWindow();
    //        return;
    //    }
    //}

    auto form = new FilterEditorForm(filter, parent);
    //if (filter != -1)
    //    filterforms[filter] = form;
    form->show();
}


//-------------------------------------------------------------
