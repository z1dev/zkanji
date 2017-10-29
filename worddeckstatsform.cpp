/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QAbstractButton>
#include <QPushButton>
#include <QSignalMapper>
#include <QtEvents>
#include "worddeckstatsform.h"
#include <QtCharts/QBarSeries> 
#include <QtCharts/QBarSet> 
#include <QtCharts/QAreaSeries> 
#include <QtCharts/QLineSeries> 
#include <QtCharts/QValueAxis> 
#include <QtCharts/QDateTimeAxis> 
#include <map>
#include "ui_worddeckstatsform.h"
#include "worddeck.h"
#include "globalui.h"
#include "words.h"
#include "zui.h"


//-------------------------------------------------------------

WordDeckStatsForm::WordDeckStatsForm(QWidget *parent) : base(parent), ui(new Ui::WordDeckStatsForm)/*, data(nullptr)*/, viewed((DeckStatPage) - 1)
{
    ui->setupUi(this);
    connect(ui->buttons->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &WordDeckStatsForm::close);
    setAttribute(Qt::WA_DeleteOnClose);
}

WordDeckStatsForm::~WordDeckStatsForm()
{
    delete ui;
}

void WordDeckStatsForm::exec(WordDeck *d)
{
    Dictionary *dict = d->dictionary();

    deck = d;

    ui->dueLabel->setText(QString::number(deck->dueSize()));
    ui->queueLabel->setText(QString::number(deck->queueSize()));

    ui->studiedLabel->setText(QString::number(deck->studySize()));

    // Calculate unique words and kanji count by checking every word data.
    int wordcnt = 0;
    QSet<ushort> kanjis;
    for (int ix = 0, siz = deck->wordDataSize(); ix != siz; ++ix)
    {
        if (!deck->wordDataStudied(ix))
            continue;
        ++wordcnt;
        WordEntry *e = dict->wordEntry(deck->wordData(ix)->index);
        for (int iy = 0, siy = e->kanji.size(); iy != siy; ++iy)
            if (KANJI(e->kanji[iy].unicode()))
                kanjis.insert(e->kanji[iy].unicode());
    }

    ui->wordsLabel->setText(QString::number(wordcnt));
    ui->kanjiLabel->setText(QString::number(kanjis.size()));

    ui->firstLabel->setText(DateTimeFunctions::formatPastDay(deck->firstDay()));
    ui->lastLabel->setText(DateTimeFunctions::formatPastDay(deck->lastDay()));
    ui->testDaysLabel->setText(QString::number(deck->testDayCount()));
    ui->skippedDaysLabel->setText(QString::number(deck->skippedDayCount()));

    ui->studyTimeLabel->setText(DateTimeFunctions::formatLength((int)((deck->totalTime() + 5) / 10)));
    ui->studyTimeAvgLabel->setText(DateTimeFunctions::formatLength((int)((deck->studyAverage() + 5) / 10)));
    ui->answerTimeAvgLabel->setText(DateTimeFunctions::formatLength((int)((deck->answerAverage() + 5) / 10)));

    showStat(DeckStatPage::Items);

    ui->statView->installEventFilter(this);


    QSignalMapper *map = new QSignalMapper(this);
    map->setMapping(ui->itemsButton, (int)DeckStatPage::Items);
    map->setMapping(ui->testsButton, (int)DeckStatPage::Tests);
    map->setMapping(ui->decksButton, (int)DeckStatPage::Decks);
    connect(ui->itemsButton, &QToolButton::clicked, map, (void (QSignalMapper::*)())&QSignalMapper::map);
    connect(ui->testsButton, &QToolButton::clicked, map, (void (QSignalMapper::*)())&QSignalMapper::map);
    connect(ui->decksButton, &QToolButton::clicked, map, (void (QSignalMapper::*)())&QSignalMapper::map);
    connect(map, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), [this](int val) { showStat((DeckStatPage)val);  });

    showModal();
}

bool WordDeckStatsForm::eventFilter(QObject *o, QEvent *e)
{
    if (o == ui->statView && e->type() == QEvent::Resize && ui->statView->chart() != nullptr && ui->statView->chart()->axisY() != nullptr)
    {
        ((QValueAxis*)ui->statView->chart()->axisY())->setTickCount(std::max(2, ui->statView->height() / 70));
        if (viewed == DeckStatPage::Items)
            ((QDateTimeAxis*)ui->statView->chart()->axisX())->setTickCount(std::max(2, ui->statView->width() / 70));
    }
    return base::eventFilter(o, e);
}

void WordDeckStatsForm::showStat(DeckStatPage page)
{
    if (viewed == page)
        return;

    viewed = page;
    if (ui->statView->chart() != nullptr)
        ui->statView->chart()->deleteLater();
    QChart *chart = new QChart();

    switch (viewed)
    {
    case DeckStatPage::Decks:
    {
        QBarSeries *bars = new QBarSeries(chart);
        QBarSet *dataset = new QBarSet("Levels", chart);

        std::vector<int> levels;
        for (int ix = 0, siz = deck->studySize(); ix != siz; ++ix)
        {
            int lv = deck->studyLevel(ix);
            if (levels.size() <= lv)
                levels.resize(lv + 1);
            ++levels[lv];
        }
        for (int ix = 0, siz = levels.size(); ix != siz; ++ix)
            (*dataset) << levels[ix];

        for (int ix = levels.size(), siz = 12; ix < siz; ++ix)
            (*dataset) << 0;


        bars->append(dataset);
        bars->setLabelsVisible(true);
        bars->setLabelsPosition(QBarSeries::LabelsOutsideEnd);
        chart->addSeries(bars);
        chart->setTitle(tr("Number of cards at each level"));
        chart->legend()->hide();
        chart->createDefaultAxes();
        ((QValueAxis*)chart->axisY())->setLabelFormat("%i");
        ((QValueAxis*)chart->axisY())->setTickCount(std::max(2, ui->statView->height() / 70));
        break;
    }
    case DeckStatPage::Items:
    {
        QLineSeries *l1 = new QLineSeries(chart);
        QLineSeries *l2 = new QLineSeries(chart);
        QLineSeries *l3 = new QLineSeries(chart);

        const StudyDeck *study = deck->getStudyDeck();

        qreal hi = 0;

        QDate last;
        for (int ix = 0, siz = study->dayStatSize(); ix != siz; ++ix)
        {
            const DeckDayStat &stat = study->dayStat(ix);
            last = stat.day;
            qint64 timesince = QDateTime(stat.day, QTime()).toMSecsSinceEpoch();

            hi = std::max(hi, (qreal)stat.itemcount);
            l1->append(timesince, stat.itemcount);
            l2->append(timesince, stat.itemcount - stat.itemlearned);
            l3->append(timesince, stat.testcount);
        }
        QDateTime now = QDateTime(ltDay(QDateTime::currentDateTimeUtc()), QTime());
        qint64 timesince = now.toMSecsSinceEpoch();
        if (last != now.date())
        {
            l1->append(timesince, static_cast<const QVector<QPointF>>(l1->pointsVector()).last().y());
            l2->append(timesince, static_cast<const QVector<QPointF>>(l2->pointsVector()).last().y());
            l3->append(timesince, static_cast<const QVector<QPointF>>(l3->pointsVector()).last().y());
        }

        QDateTimeAxis *xaxis = new QDateTimeAxis(chart);
        xaxis->setTickCount(10);
        xaxis->setFormat("MMM yyyy");
        xaxis->setTitleText("Study days");

        QValueAxis *yaxis = new QValueAxis(chart);
        yaxis->setLabelFormat("%i");
        yaxis->setTitleText("Items");
        yaxis->setRange(0, hi + 100);

        QAreaSeries *area1 = new QAreaSeries(chart);
        area1->setUpperSeries(l1);
        area1->setLowerSeries(l2);
        chart->addSeries(area1);

        QAreaSeries *area2 = new QAreaSeries(chart);
        area2->setUpperSeries(l2);
        area2->setLowerSeries(l3);
        chart->addSeries(area2);

        QAreaSeries *area3 = new QAreaSeries(chart);
        area3->setUpperSeries(l3);
        chart->addSeries(area3);

        xaxis->setTickCount(std::max(2, ui->statView->width() / 70));
        yaxis->setTickCount(std::max(2, ui->statView->height() / 70));

        chart->setAxisX(xaxis);
        chart->setAxisY(yaxis);

        chart->setTitle(tr("Number of items on each day"));
        chart->legend()->hide();

        area1->attachAxis(xaxis);
        area1->attachAxis(yaxis);
        area2->attachAxis(xaxis);
        area2->attachAxis(yaxis);
        area3->attachAxis(xaxis);
        area3->attachAxis(yaxis);

        break;
    }
    case DeckStatPage::Tests:
        break;
    }

    ui->statView->setChart(chart);
}


//-------------------------------------------------------------


void showDeckStats(WordDeck *deck)
{
    WordDeckStatsForm *form = new WordDeckStatsForm(gUI->activeMainForm());
    form->exec(deck);
}


//-------------------------------------------------------------

