/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include "kanjireadingpracticeform.h"
#include "ui_kanjireadingpracticeform.h"
#include "worddeck.h"
#include "kanji.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "zui.h"
#include "globalui.h"


//-------------------------------------------------------------


KanjiReadingPracticeForm::KanjiReadingPracticeForm(WordDeck *deck, QWidget *parent) : base(parent), ui(new Ui::KanjiReadingPracticeForm),
        deck(deck), tries(0), model(nullptr)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    model = new DictionaryWordListItemModel(this);
    ui->wordsTable->setModel(model);

    ui->answerEdit->setValidator(&kanaValidator());

    connect(ui->acceptButton, &QPushButton::clicked, this, &KanjiReadingPracticeForm::readingAccepted);
    connect(ui->answerEdit, &ZLineEdit::textChanged, this, &KanjiReadingPracticeForm::answerChanged);
    connect(ui->nextButton, &QPushButton::clicked, this, &KanjiReadingPracticeForm::nextClicked);
}

KanjiReadingPracticeForm::~KanjiReadingPracticeForm()
{
    delete ui;
    gUI->showAppWindows();
}

void KanjiReadingPracticeForm::exec()
{
    initNextRound();

    show();
}

void KanjiReadingPracticeForm::answerChanged()
{
    if (ui->answerEdit->text().trimmed().isEmpty())
        ui->acceptButton->setText(tr("Reveal"));
    else
        ui->acceptButton->setText(tr("Accept"));
}

void KanjiReadingPracticeForm::readingAccepted()
{
    if (ui->answerEdit->text().trimmed().isEmpty())
    {
        ui->solvedLabel->setText(tr("Incorrect"));

        // Answer string was empty, just show the correct answer.
        ui->label1->hide();
        ui->label2->hide();
        ui->triesLabel->setText(deck->correctReading());
    }
    else
    {
        if (!deck->practiceReadingMatches(ui->answerEdit->text().trimmed()))
        {
            // Incorrect answer.

            ui->solvedLabel->setText(tr("Incorrect"));

            ++tries;
            // The answer was incorrect. Show correct answer if this is the
            // 5th time.
            if (tries == 5)
            {
                ui->label1->hide();
                ui->label2->hide();
                ui->triesLabel->setText(deck->correctReading());
            }
            else
                ui->triesLabel->setText(ui->triesLabel->text() + (tries == 1 ? "" : ", ") + ui->answerEdit->text().trimmed());
        }
        else
        {
            // Answer was correct.
            ui->label1->hide();
            ui->label2->hide();
            ui->triesLabel->setText(deck->correctReading());

            ui->solvedLabel->setText(tr("Correct"));

        }
    }

    if (deck->readingsQueued())
        ui->nextButton->setText(tr("Continue"));
    else
        ui->nextButton->setText(tr("Finish"));
    ui->answerStack->setCurrentIndex(1);
}

void KanjiReadingPracticeForm::nextClicked()
{
    if (!ui->answerEdit->text().trimmed().isEmpty() && tries != 5 && !deck->practiceReadingMatches(ui->answerEdit->text().trimmed()))
    {
        // The answer was incorrect. Show everything again to retry.
        ui->label1->show();
        ui->label2->show();

        ui->answerEdit->setText(QString());
        ui->acceptButton->setText(tr("Reveal"));

        ui->answerStack->setCurrentIndex(0);

        ui->answerEdit->setFocus();
    }
    else
    {
        // Answer was correct or had to be revealed.

        deck->practiceReadingAnswered();

        if (!deck->readingsQueued())
        {
            // Last item was already answered.
            close();
            return;
        }

        initNextRound();
    }
}

void KanjiReadingPracticeForm::initNextRound()
{
    tries = 0;

    int kix = deck->nextPracticeKanji();
    uchar r = deck->nextPracticeReading();

    KanjiEntry *k = ZKanji::kanjis[kix];
    ui->kanjiLabel->setText(k->ch);

    ui->label1->show();
    ui->label2->show();
    ui->triesLabel->setText(QString());
    ui->answerEdit->setText(QString());
    ui->acceptButton->setText(tr("Reveal"));

    ui->answerStack->setCurrentIndex(0);

    std::vector<int> words;
    deck->nextPracticeReadingWords(words);
    model->setWordList(deck->dictionary(), std::move(words));

    ui->answerEdit->setFocus();
}


//-------------------------------------------------------------
