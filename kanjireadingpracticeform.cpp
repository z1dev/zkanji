/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QPainter>
#include <QDesktopWidget>
#include <QStatusBar>
#include "kanjireadingpracticeform.h"
#include "ui_kanjireadingpracticeform.h"
#include "worddeck.h"
#include "kanji.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "zui.h"
#include "globalui.h"
#include "fontsettings.h"
#include "furigana.h"
#include "colorsettings.h"


//-------------------------------------------------------------


KanjiReadingPracticeListDelegate::KanjiReadingPracticeListDelegate(ZDictionaryListView *parent) : base(parent)
{
    ;
}

void KanjiReadingPracticeListDelegate::paintKanji(QPainter *painter, const QModelIndex &index, int left, int top, int basey, QRect r) const
{
    if (owner()->isTextSelecting() && owner()->textSelectionIndex() == index)
    {
        base::paintKanji(painter, index, left, top, basey, r);
        return;
    }

    std::vector<int> *rlist = (std::vector<int>*)index.data((int)KanjiReadingRoles::ReadingsList).value<intptr_t>();
    QString str = index.data(Qt::DisplayRole).toString();
    WordEntry *e = index.data((int)DictRowRoles::WordEntry).value<WordEntry*>();
    QFontMetrics fm = painter->fontMetrics();

    QPen p = painter->pen();

    int from = 0;
    for (int ix = 0, siz = rlist->size() + 1; ix != siz; ++ix)
    {
        int next = ix != rlist->size() ? (*rlist)[ix] : str.size();
        QString part = str.mid(from, next - from);
        drawTextBaseline(painter, left, basey, false, r, part);
        left += fm.width(part);
        if (ix != rlist->size())
        {
            painter->setPen(Settings::uiColor(Settings::colors.KanjiTestPos));
            drawTextBaseline(painter, left, basey, false, r, str.at(next));
            left += fm.width(str.at(next));
            painter->setPen(p);
        }
        from = next + 1;
    }
}


//-------------------------------------------------------------


KanjiReadingPracticeListModel::KanjiReadingPracticeListModel(QObject *parent) : base(parent)
{

}

KanjiReadingPracticeListModel::~KanjiReadingPracticeListModel()
{

}

void KanjiReadingPracticeListModel::setWordList(Dictionary *d, std::vector<int> &&wordlist, std::vector<std::vector<int>> &&rlist)
{
    std::swap(readings, rlist);
    base::setWordList(d, wordlist);
}

QVariant KanjiReadingPracticeListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == (int)KanjiReadingRoles::ReadingsList)
        return QVariant::fromValue((intptr_t)&readings[index.row()]);

    return base::data(index, role);
}

QMap<int, QVariant> KanjiReadingPracticeListModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> result = base::itemData(index);
    if (index.isValid())
        result[(int)KanjiReadingRoles::ReadingsList] = QVariant::fromValue((intptr_t)&readings[index.row()]);

    return result;
}


//-------------------------------------------------------------


KanjiReadingPracticeForm::KanjiReadingPracticeForm(WordDeck *deck, QWidget *parent) : base(parent), ui(new Ui::KanjiReadingPracticeForm),
        deck(deck), tries(0), correct(0), wrong(0), model(nullptr)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    gUI->scaleWidget(this);

    model = new KanjiReadingPracticeListModel(this);
    ui->wordsTable->setModel(model);
    ui->wordsTable->setItemDelegate(new KanjiReadingPracticeListDelegate(ui->wordsTable));

    ui->answerEdit->setValidator(&kanaValidator());
    ui->acceptButton->setDefault(true);
    ui->nextButton->setDefault(true);

    ui->kanjiLabel->setFont(Settings::kanjiFont());

    ui->status->add(tr("Due:"), 0, "0", 5, true);
    //statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Due") + ": ", 0, dueLabel = new QLabel(this), "0", 5));
    //dueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->status->add(tr("Correct:"), 0, "0", 5, true);
    //statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Correct") + ": ", 0, correctLabel = new QLabel(this), "0", 5));
    //correctLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->status->add(tr("Wrong:"), 0, "0", 5, true);
    //statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Wrong") + ": ", 0, wrongLabel = new QLabel(this), "0", 5));
    //wrongLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);


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

    setAttribute(Qt::WA_DontShowOnScreen, true);
    show();
    qApp->processEvents();
    ui->kanjiLabel->setMaximumHeight(ui->kanjiLabel->width());
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    QRect r = frameGeometry();
    QRect sr = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    move(sr.left() + (sr.width() - r.width()) / 2, sr.top() + (sr.height() - r.height()) / 2);

    gUI->hideAppWindows();

    show();
    ui->answerEdit->setFocus();
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

        if (tries == 0)
            ++wrong;

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

            if (tries == 0)
                ++wrong;

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

            if (tries == 0)
                ++correct;
        }
    }

    if (deck->readingsQueued() != 0)
        ui->nextButton->setText(tr("Continue"));
    else
        ui->nextButton->setText(tr("Finish"));
    ui->answerStack->setCurrentIndex(1);
    updateLabels();
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

        if (deck->readingsQueued() == 0)
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

    Dictionary *d = deck->dictionary();

    std::vector<std::vector<int>> readings;
    // Go over the words and check the character position where the current kanji with reading
    // r is to be found.
    for (int wix : words)
    {
        const WordEntry *const w = d->wordEntry(wix);
        std::vector<FuriganaData> furi;
        findFurigana(w->kanji, w->kana, furi);
        
        readings.push_back(std::vector<int>());
        std::vector<int> &rlist = readings.back();

        for (int ix = 0, siz = furi.size(); ix != siz; ++ix)
        {
            if (furi[ix].kanji.len == 1 && w->kanji[furi[ix].kanji.pos] == k->ch && findKanjiReading(w->kanji, w->kana, furi[ix].kanji.pos, k, &furi) == r)
                rlist.push_back(furi[ix].kanji.pos);
        }
    }

    model->setWordList(deck->dictionary(), std::move(words), std::move(readings));
    updateLabels();

    ui->answerEdit->setFocus();
}

void KanjiReadingPracticeForm::updateLabels()
{
    ui->status->setValue(0, QString::number(deck->readingsQueued()));
    ui->status->setValue(1, QString::number(correct));
    ui->status->setValue(2, QString::number(wrong));
    //dueLabel->setText(QString("%1").arg(QString::number(deck->readingsQueued())));
    //correctLabel->setText(QString("%1").arg(QString::number(correct)));
    //wrongLabel->setText(QString("%1").arg(QString::number(wrong)));
}


//-------------------------------------------------------------
