/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "definitionwidget.h"
#include "ui_definitionwidget.h"
#include "words.h"

#include "checked_cast.h"

//-------------------------------------------------------------


DefinitionWidget::DefinitionWidget(QWidget *parent) : base(parent), ui(new Ui::DefinitionWidget), dict(nullptr), ignorechange(false)
{
    ui->setupUi(this);
}

DefinitionWidget::~DefinitionWidget()
{
    delete ui;
}

void DefinitionWidget::setWords(Dictionary *d, const std::vector<int> &words)
{
    if (dict != nullptr)
    {
        disconnect(dict, &Dictionary::entryRemoved, this, &DefinitionWidget::dictEntryRemoved);
        disconnect(dict, &Dictionary::entryChanged, this, &DefinitionWidget::dictEntryChanged);
    }
    dict = d;
    if (dict != nullptr)
    {
        connect(dict, &Dictionary::entryRemoved, this, &DefinitionWidget::dictEntryRemoved);
        connect(dict, &Dictionary::entryChanged, this, &DefinitionWidget::dictEntryChanged);
    }
    list = words;
    std::sort(list.begin(), list.end());

    // Find out if any word on the list has custom meaning set.
    data.resize(list.size());
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
        data[ix] = dict->studyDefinition(list[ix]) != nullptr ? 1 : 0;

    //if (list.size() != 1)
    //{
    //    ui->defEdit->setText(QString());
    //    ui->defEdit->setPlaceholderText(QString());
    //}
    //else
    //{
    //    auto def = dict->studyDefinition(list.front());
    //    if (def != nullptr)
    //        ui->defEdit->setText(def->toQString());
    //    else
    //        ui->defEdit->setText(QString());
    //}

    update();
}

bool DefinitionWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    return base::event(e);
}

void DefinitionWidget::on_defResetButton_clicked(bool)
{
    ignorechange = true;
    for (int ix = 0, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        data[ix] = 0;
        dict->setWordStudyDefinition(list[ix], QString());
    }
    ui->defResetButton->setEnabled(false);
    if (list.size() == 1)
    {
        if (ui->defEdit->hasFocus())
            ui->defEdit->setText(original);
        else
            ui->defEdit->setText(QString());
    }
    //    ui->defSaveButton->setEnabled(!ui->defEdit->text().isEmpty() && ui->defEdit->text() != original);
    ignorechange = false;
}

void DefinitionWidget::dictEntryRemoved(int windex)
{
    int pos = indexpos(windex);
    if (pos == tosigned(list.size()))
        return;

    if (list.size() == 1 && list[pos] != windex)
    {
        --list.front();
        return;
    }

    if (list[pos] == windex)
    {
        list.erase(list.begin() + pos);
        data.erase(data.begin() + pos);
    }

    while (pos != tosigned(list.size()))
    {
        --list[pos];
        ++pos;
    }

    update();
}

void DefinitionWidget::dictEntryChanged(int windex, bool /*studydef*/)
{
    int pos = indexpos(windex);
    if (pos != tosigned(list.size()) && list[pos] == windex)
        data[pos] = dict->studyDefinition(list[pos]) != nullptr ? 1 : 0;

    if (list.size() != 1)
        update();
    else if (list.front() == windex)
    {
        original = dict->wordDefinitionString(windex, false);
        if (!ui->defEdit->hasFocus())
            ui->defEdit->setPlaceholderText(original);
        const QCharString *def = dict->studyDefinition(windex);

        if (!ui->defEdit->hasFocus() || (def == nullptr && !ui->defEdit->text().isEmpty() && ui->defEdit->text() != original) || (def != nullptr && ui->defEdit->text() != def->toQString()))
            ui->defEdit->setText(def == nullptr ? (!ui->defEdit->hasFocus() ? QString() : original) : def->toQString());
        
        ui->defResetButton->setEnabled(def != nullptr);
    }
}

void DefinitionWidget::on_defEdit_textEdited(const QString &str)
{
    if (list.empty() || list.size() != 1)
        return;

    ui->defSaveButton->setEnabled(!str.isEmpty() && str != original);
}

void DefinitionWidget::on_defEdit_focusChanged(bool activated)
{
    ui->defEdit->setPlaceholderText(activated ? QString() : original);

    if (activated && ui->defEdit->text().isEmpty())
        ui->defEdit->setText(original);
    if (!activated && ui->defEdit->text() == original)
        ui->defEdit->setText(QString());

    ui->defSaveButton->setEnabled(!ui->defEdit->text().isEmpty() && ui->defEdit->text() != original);
}

void DefinitionWidget::on_defSaveButton_clicked(bool /*checked*/)
{
    if (list.empty() || list.size() != 1)
        return;

    dict->setWordStudyDefinition(list.front(), ui->defEdit->text());
}

int DefinitionWidget::indexpos(int windex)
{
    return std::lower_bound(list.begin(), list.end(), windex) - list.begin();
}

void DefinitionWidget::update()
{
    if (ignorechange)
        return;

    ui->defEdit->setEnabled(list.size() == 1);
    ui->defSaveButton->setEnabled(false);

    if (!ui->defEdit->isEnabled())
    {
        ui->defEdit->setText(QString());
        ui->defEdit->setPlaceholderText(QString());
        //ui->defSaveButton->setEnabled(false);

        bool found = false;
        for (int ix = 0, siz = tosigned(data.size()); !found && ix != siz; ++ix)
            found = data[ix] == 1;

        ui->defResetButton->setEnabled(found);
    }
    else
    {
        original = dict->wordDefinitionString(list.front(), false);
        const QCharString *def = dict->studyDefinition(list.front());

        if (!ui->defEdit->hasFocus())
            ui->defEdit->setPlaceholderText(original);

        ui->defEdit->setText(def == nullptr ? (!ui->defEdit->hasFocus() ? QString() : original) : def->toQString());

        ui->defResetButton->setEnabled(def != nullptr);
        //ui->defSaveButton->setEnabled(!ui->defEdit->text().isEmpty() && ui->defEdit->text() != original && (def == nullptr || ui->defEdit->text() != def->toQString()));
    }
}


//-------------------------------------------------------------

