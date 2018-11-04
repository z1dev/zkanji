/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QMessageBox>
#include <QtEvents>
#include "jlptreplaceform.h"
#include "ui_jlptreplaceform.h"
#include "zui.h"
#include "words.h"
#include "kanji.h"
#include "globalui.h"


#include "checked_cast.h"

//-------------------------------------------------------------


JLPTReplaceForm::JLPTReplaceForm(QWidget *parent) : base(parent), ui(new Ui::JLPTReplaceForm()), pos(-1), skipped(0)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    connect(ui->abortButton, &QPushButton::clicked, this, &JLPTReplaceForm::close);
    connect(ui->skipButton, &QPushButton::clicked, this, &JLPTReplaceForm::skip);
    connect(ui->useButton, &QPushButton::clicked, this, &JLPTReplaceForm::replace);
    connect(ui->undoButton, &QPushButton::clicked, this, &JLPTReplaceForm::undo);

    ui->status->add(tr("Words remaining:"), 0, "0", 10);
    ui->status->add(tr("Words replaced:"), 0, "0", 10);
    ui->status->add(tr("Words skipped:"), 0, "0", 10);

    //restrictWidgetSize(ui->replacedLabel, 10);
    //restrictWidgetSize(ui->skippedLabel, 10);
    //restrictWidgetSize(ui->wordsLabel, 10);

    //ui->status->addWidget(ui->wordsWidget);
    //ui->status->addWidget(ui->replacedWidget);
    //ui->status->addWidget(ui->skippedWidget);
}

JLPTReplaceForm::~JLPTReplaceForm()
{
    ZKanji::wordfilters().clear();

    delete ui;
}

void JLPTReplaceForm::exec(QString filepath, Dictionary *dict, const std::vector<std::tuple<QString, QString, int>> &words)
{
    list = words;
    path = filepath;

    ui->dict->setDictionary(dict);

    ZKanji::wordfilters().add(tr("JLPT"), WordDefAttrib(), 0, 31, FilterMatchType::AnyCanMatch);
    ui->dict->turnOnWordFilter(tosigned(ZKanji::wordfilters().size()) - 1, Inclusion::Exclude);

    pos = 0;
    updateButtons();

    showModal();
}

void JLPTReplaceForm::skip()
{
    commonslist.push_back(-1);
    ++pos;
    ++skipped;
    updateButtons();
}

void JLPTReplaceForm::replace()
{
    WordEntry *e = ui->dict->currentWord();
    WordCommons *wc = ZKanji::commons.findWord(e->kanji.data(), e->kana.data());
    if (wc != nullptr && wc->jlptn != 0)
    {
        QMessageBox::information(this, "zkanji", tr("Please select a word that doesn't have a JLPT level specified."));
        return;
    }

    commonslist.push_back(ZKanji::commons.addJLPTN(e->kanji.data(), e->kana.data(), std::get<2>(list[pos]), true));
    ++pos;
    updateButtons();
}

void JLPTReplaceForm::undo()
{
    int val = commonslist.back();
    if (val != -1)
    {
        ZKanji::commons.removeJLPTN(val);
        //if (ZKanji::commons.removeJLPTN(val))
        //{
        //    for (int &v : commonslist)
        //        if (v > val)
        //            --v;
        //}
    }
    else
        --skipped;

    commonslist.pop_back();

    --pos;
    updateButtons();
}

void JLPTReplaceForm::closeEvent(QCloseEvent *e)
{
    if (pos != tosigned(list.size()))
    {
        if (QMessageBox::warning(this, "zkanji", tr("Words without a replacement will be missing their JLPT data. Do you wish to skip them and continue the dictionary import?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        {
            e->ignore();
            base::closeEvent(e);
            return;
        }
    }

    if (pos != skipped)
    {
        if (QMessageBox::question(this, "zkanji", tr("Changes have been made to the JLPT data that can be saved to JLPTNData.txt. If you decide to save the changes, a backup file will be created from the original file.\n\nWould you like to save the updated JLPTNData.txt?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
            saveJLPTData();
    }

    base::closeEvent(e);
}

void JLPTReplaceForm::updateButtons()
{
    if (pos != tosigned(list.size()))
    {
        ui->wordEdit->setText(QString("%1(%2) N %3").arg(std::get<0>(list[pos])).arg(std::get<1>(list[pos])).arg(std::get<2>(list[pos])));
        if (ui->dict->searchMode() == SearchMode::Definition)
            ui->dict->setSearchMode(SearchMode::Browse);
        ui->dict->setSearchText(std::get<1>(list[pos]));
    }
    else
    {
        ui->wordEdit->setText(tr("No more words. Press \"%1\" to continue.").arg(tr("Finish")));
        ui->dict->setSearchText(QString());
    }

    ui->status->setValue(0, QString::number(list.size() - pos));
    ui->status->setValue(1, QString::number(pos - skipped));
    ui->status->setValue(2, QString::number(skipped));

    //ui->replacedLabel->setText(QString::number(pos - skipped));
    //ui->skippedLabel->setText(QString::number(skipped));
    //ui->wordsLabel->setText(QString::number(list.size() - pos));

    ui->undoButton->setEnabled(pos > 0);
    ui->skipButton->setEnabled(pos != tosigned(list.size()));
    ui->useButton->setEnabled(pos != tosigned(list.size()));

    ui->abortButton->setText(pos != tosigned(list.size()) ? tr("Abort") : tr("Finish"));
}

void JLPTReplaceForm::saveJLPTData()
{
    NTFSPermissionGuard pguard;

    QFile f(path + "/JLPTNData.txt");

    int baknum = 1;
    while (QFile::exists(QString(path + "/JLPTNData.txt.bak%1").arg(baknum, 3, 10, (QLatin1Char)('0'))))
        ++baknum;

    while (!f.copy(QString(path + "/JLPTNData.txt.bak%1").arg(baknum, 3, 10, (QLatin1Char)('0'))))
    {
        QMessageBox::StandardButton r = QMessageBox::warning(this, "zkanji", tr("Couldn't backup JLPTNData.txt. Please check file permissions at the import path. Press \"%1\" to try again.\n\nIf you select \"%2,\" no backup will be created, and the file will be overwritten. Press \"%3\" leave the original JLPTNData.txt intact.").arg(tr("Retry")).arg(tr("Cancel")).arg(tr("Abort")), QMessageBox::Retry | QMessageBox::Cancel | QMessageBox::Abort, QMessageBox::Retry);
        if (r == QMessageBox::Abort)
            return;
        if (r == QMessageBox::Cancel)
            break;
    }

    while (!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (QMessageBox::warning(this, "zkanji", tr("Couldn't open JLPTNData.txt for writing. Please check file permissions at the import path.\n\nWould you like to \"%1\" or \"%2\" saving the file?").arg(tr("Retry")).arg(tr("Cancel")), QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
            return;
    }

    QTextStream stream(&f);
    stream.setCodec("UTF-8");

    stream << "# JLPT N Data file for zkanji.\n"
            "#\n"
            "# The data was compiled from official JLPT lists(prior 2010).There are no\n"
            "# official lists since 2010, but the levels are around the same difficulty.\n"
            "# A new level N3 was introduced in 2010 as a level between the old 2kyuu and\n"
            "# 3kyuu. N1 matching 1kyuu, N2 is around 2kyuu, N4 is around 3kyuu, and N5 is\n"
            "# like 4kyuu. The data for the missing N3 comes from a part of 2kyuu, and is\n"
            "# based entirely on early textbooks from 2010 and my assumptions, which were\n"
            "# based on word frequency, official example questions for N3 and words about\n"
            "# topics that come up in the JLPT.\n"
            "#\n"
            "# DISCLAIMER: this data is provided AS IS. It contains errors. There is no\n"
            "# guarantee that the new N levels use anything from the old JLPT. You might\n"
            "# fail the exam even if you learn everything from this list. Use this data\n"
            "# at your own risk. I take no responsibility if you don't pass the test.\n"
            "#\n"
            "# Format:\n"
            "#   Lines starting with # are comments.\n"
            "#   JLPT N data for words lasts till the first empty line.\n"
            "#   Each word data contains three fields separated by TAB.\n"
            "#   The fields are:\n"
            "#     kanji form\n"
            "#     kana reading\n"
            "#     N level\n"
            "#   JLPT N data for kanji starts after the first empty line till the\n"
            "#   end of file.\n"
            "#   The kanji N data contains 2 fields with no space between them:\n"
            "#     kanji character.\n"
            "#     N level\n#\n";

    auto &commons = ZKanji::commons.getItems();
    for (int ix = 0, siz = tosigned(commons.size()); ix != siz; ++ix)
    {
        if (commons[ix]->jlptn == 0)
            continue;
        stream << commons[ix]->kanji.toQStringRaw() << "\t" << commons[ix]->kana.toQStringRaw() << "\t" << QString::number((int)commons[ix]->jlptn) << "\n";
    }

    // The skipped words are saved here
    for (int ix = 0, siz = tosigned(commonslist.size()); ix != siz; ++ix)
    {
        if (commonslist[ix] != -1)
            continue;
        stream << std::get<0>(list[ix]) << "\t" << std::get<1>(list[ix]) << "\t" << QString::number(std::get<2>(list[ix])) << "\n";
    }

    stream << "\n";

    // Writing kanji.

    for (int ix = 0, siz = ZKanji::kanjicount; ix != siz; ++ix)
        stream << ZKanji::kanjis[ix]->ch << QString::number((int)ZKanji::kanjis[ix]->jlpt) << "\n";

    f.close();
}


//-------------------------------------------------------------
