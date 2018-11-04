/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "kanawritingpracticeform.h"
#include "ui_kanawritingpracticeform.h"
#include "kanapracticesettingsform.h"
#include "fontsettings.h"
#include "zui.h"
#include "globalui.h"
#include "dialogs.h"
#include "formstates.h"
#include "zkanjimain.h"
#include "romajizer.h"
#include "colorsettings.h"
#include "generalsettings.h"

#include "checked_cast.h"


//-------------------------------------------------------------


KanaWritingPracticeForm::KanaWritingPracticeForm(QWidget *parent) : base(parent, false), ui(new Ui::KanaWritingPracticeForm), t(nullptr)
{
    ui->setupUi(this);

    gUI->scaleWidget(this);

    stopico = QIcon(QPixmap::fromImage(loadColorSVG(":/playstopbtn.svg", ui->revealButton->iconSize(), qRgb(0, 0, 0), qApp->palette().color(QPalette::Text))));
    playico = QIcon(QPixmap::fromImage(loadColorSVG(":/playbtn.svg", ui->revealButton->iconSize(), qRgb(0, 0, 0), qApp->palette().color(QPalette::Text))));

    QSizePolicy sp = ui->titleLabel->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    ui->titleLabel->setSizePolicy(sp);

    QFont f = ui->kanaLabel->font();
    f.setFamily(Settings::fonts.main);
    ui->kanaLabel->setFont(f);

    f = ui->questionLabel->font();
    f.setFamily(Settings::fonts.main);
    ui->questionLabel->setFont(f);

    f = ui->text1Label->font();
    f.setFamily(Settings::fonts.kana);
    ui->text1Label->setFont(f);
    f = ui->text2Label->font();
    f.setFamily(Settings::fonts.kana);
    ui->text2Label->setFont(f);
    f = ui->text3Label->font();
    f.setFamily(Settings::fonts.kana);
    ui->text3Label->setFont(f);

    restrictWidgetWiderSize(ui->text1Label, 1.05);
    restrictWidgetWiderSize(ui->text2Label, 1.05);
    restrictWidgetWiderSize(ui->text3Label, 1.05);

    ui->prevButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    ui->clearButton->setEnabled(false);

    ui->drawArea->setShowGrid(true);
    ui->drawArea->setCharacterMatch(RecognizerArea::Kana);

    ui->kanjiView->setGrid(true);

    candidates = new CandidateKanjiScrollerModel(this);
    ui->candidateScroller->setModel(candidates);
    ui->candidateScroller->setScrollerType(ZScrollArea::Buttons);

    ui->status->add(tr("Time:"), 0, "99:99", 6, true);
    //statusBar()->addWidget(createStatusWidget(ui->status, -1, nullptr, tr("Time:"), 0, timeLabel = new QLabel(this), "99:99", 6));
    //timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->status->add(tr("Remaining:"), 0, "0", 4, true);
    //statusBar()->addWidget(createStatusWidget(ui->status, -1, nullptr, tr("Remaining:"), 0, dueLabel = new QLabel(this), "0", 4));
    //dueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->status->add(tr("Mistakes:"), 0, "0", 4, true);
    //statusBar()->addWidget(createStatusWidget(ui->status, -1, nullptr, tr("Mistakes:"), 0, wrongLabel = new QLabel(this), "0", 4));
    //wrongLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    connect(ui->abortButton, &QPushButton::clicked, this, &DialogWindow::closeAbort);

    connect(ui->drawArea, &RecognizerArea::changed, this, &KanaWritingPracticeForm::candidatesChanged);
    connect(ui->candidateScroller, &ZItemScroller::itemClicked, this, &KanaWritingPracticeForm::candidateClicked);

    connect(gUI, &GlobalUI::settingsChanged, this, &KanaWritingPracticeForm::settingsChanged);

    //setAttribute(Qt::WA_DontShowOnScreen);
    //show();
    int siz = Settings::scaled(220);
    ui->drawArea->setFixedSize(QSize(siz, siz));
    ui->kanjiView->setFixedSize(QSize(siz, siz));
    fixWrapLabelsHeight(this, -1);
    //adjustSize();
    //setFixedSize(size());

    //hide();
    //setAttribute(Qt::WA_DontShowOnScreen, false);

    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->titleLabel->hide();
    ui->resultLabel->setText(QString());

    ui->kanjiView->installEventFilter(this);
}

KanaWritingPracticeForm::~KanaWritingPracticeForm()
{
    stopTimer(false);
    delete ui;

    delete t;
    t = nullptr;

    QTimer::singleShot(0, []() {
        gUI->showAppWindows();
        setupKanaPractice();
    });
}

void KanaWritingPracticeForm::exec()
{
    reset();

    //QRect r = frameGeometry();
    //QRect sr = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    //move(sr.left() + (sr.width() - r.width()) / 2, sr.top() + (sr.height() - r.height()) / 2);

    gUI->hideAppWindows();
    show();
}

void KanaWritingPracticeForm::on_restartButton_clicked()
{
    reset();
}

void KanaWritingPracticeForm::candidatesChanged(const std::vector<int> &l)
{
    std::vector<int> copy;
    copy.reserve(l.size());

    // The model in the candidate scroller can show both elements and characters. Elements
    // must be passed as a negative number starting from -1, while the result received in l
    // holds the element indexes as a positive number, starting at 0. To make the scroller
    // display our characters from element indexes, l must be converted.
    for (int val : l)
        copy.push_back(-val - 1);

    candidates->setItems(copy);
    ui->prevButton->setEnabled(!ui->drawArea->empty());
    ui->nextButton->setEnabled(!ui->drawArea->hasNext());
    ui->clearButton->setEnabled(!ui->drawArea->empty());

    ui->text1Label->setStyleSheet(QString());
    ui->text2Label->setStyleSheet(QString());
    ui->text3Label->setStyleSheet(QString());

    ui->titleLabel->setVisible(!entered.isEmpty());

    if (entered.size() > 0)
        ui->text1Label->setText(entered.at(0));
    else
        ui->text1Label->setText(" ");
    if (entered.size() > 1)
        ui->text2Label->setText(entered.at(1));
    else
        ui->text2Label->setText(" ");
    if (entered.size() > 2)
        ui->text3Label->setText(entered.at(2));
    else
        ui->text3Label->setText(" ");
}

void KanaWritingPracticeForm::candidateClicked(int index)
{
    if (pos < 0 || pos >= tosigned(list.size()))
        return;

    QChar ch = ZKanji::elements()->itemUnicode(-candidates->items(index) - 1);
    QString answer = toKana(kanaStrings[list[pos] % (int)KanaSounds::Count]);
    if (list[pos] >= (int)KanaSounds::Count)
        answer = toKatakana(answer);

    entered.append(ch);

    ui->text1Label->setStyleSheet(QString());
    ui->text2Label->setStyleSheet(QString());
    ui->text3Label->setStyleSheet(QString());

    ui->text1Label->setText(entered.at(0));
    ui->titleLabel->setVisible(!entered.isEmpty());

    if (entered.size() > 1)
        ui->text2Label->setText(entered.at(1));
    else
        ui->text2Label->setText(" ");
    if (entered.size() > 2)
        ui->text3Label->setText(entered.at(2));
    else
        ui->text3Label->setText(" ");

    if (answer.indexOf(entered) != 0)
        answered(false);
    else if (answer == entered)
        answered(true);
    else
        ui->drawArea->clear();
}

void KanaWritingPracticeForm::on_gridButton_toggled(bool checked)
{
    ui->drawArea->setShowGrid(checked);
    ui->kanjiView->setGrid(checked);
}

void KanaWritingPracticeForm::on_clearButton_clicked()
{
    ui->drawArea->clear();
}

void KanaWritingPracticeForm::on_prevButton_clicked()
{
    ui->drawArea->prev();
}

void KanaWritingPracticeForm::on_nextButton_clicked()
{
    ui->drawArea->next();
}

void KanaWritingPracticeForm::on_revealButton_clicked()
{
    if (pos < 0 || pos >= tosigned(list.size()))
        return;

    if (ui->stack->currentIndex() == 0)
    {
        ui->revealButton->setIcon(stopico);

        ui->drawArea->clear();

        animatepos = -1;
        animateNext();
        connect(ui->kanjiView, &ZKanjiDiagram::strokeChanged, this, &KanaWritingPracticeForm::animateNext);

        ui->stack->setCurrentIndex(1);
    }
    else
    {
        if (t != nullptr)
            t->stop();
        delete t;
        t = nullptr;
        disconnect(ui->kanjiView, &ZKanjiDiagram::strokeChanged, this, &KanaWritingPracticeForm::animateNext);

        ui->revealButton->setIcon(playico);
        ui->stack->setCurrentIndex(0);
        ui->kanjiView->stop();
    }
}

void KanaWritingPracticeForm::settingsChanged()
{
    stopico = QIcon(QPixmap::fromImage(loadColorSVG(":/playstopbtn.svg", ui->revealButton->iconSize(), qRgb(0, 0, 0), qApp->palette().color(QPalette::Text))));
    playico = QIcon(QPixmap::fromImage(loadColorSVG(":/playbtn.svg", ui->revealButton->iconSize(), qRgb(0, 0, 0), qApp->palette().color(QPalette::Text))));

    if (ui->stack->currentIndex() == 1)
        ui->revealButton->setIcon(stopico);
    else
        ui->revealButton->setIcon(playico);
}

bool KanaWritingPracticeForm::event(QEvent *e)
{
    if (e->type() == QEvent::Timer && ((QTimerEvent*)e)->timerId() == timer.timerId() && ui->status->value(0) != "-")
    {

        QDateTime now = QDateTime::currentDateTimeUtc();
        qint64 passed = starttime.secsTo(now);
        if (passed >= 60 * 60)
            stopTimer(true);
        else
            ui->status->setValue(0, DateTimeFunctions::formatPassedTime(passed, false));
        //timeLabel->setText(DateTimeFunctions::formatPassedTime(passed, false));
    }

    return base::event(e);
}

bool KanaWritingPracticeForm::eventFilter(QObject *o, QEvent *e)
{
    if (o == ui->kanjiView)
    {
        if (e->type() == QEvent::MouseButtonPress)
        {
            ui->revealButton->click();
        }
    }

    return base::eventFilter(o, e);
}

void KanaWritingPracticeForm::showEvent(QShowEvent *e)
{
    adjustSize();
    setFixedSize(size());
    base::showEvent(e);
}

void KanaWritingPracticeForm::reset()
{
    list.clear();

    std::vector<int> nums;
    for (int ix = 0, siz = (int)KanaSounds::Count * 2; ix != siz; ++ix)
    {
        int i = ix % (int)KanaSounds::Count;
        std::vector<uchar> &vec = ix < (int)KanaSounds::Count ? FormStates::kanapractice.hirause : FormStates::kanapractice.katause;
        if (vec[i] == 0)
            continue;

        nums.push_back(ix);
    }

    while (list.size() != 40)
    {
        int numpos = rnd(0, tosigned(nums.size()) - 1);
        if (!list.empty() && nums[numpos] == list.back())
            continue;
        list.push_back(nums[numpos]);
        //nums.erase(nums.begin() + numpos);
    }

    pos = -1;
    mistakes = 0;
    starttime = QDateTime::currentDateTimeUtc();

    startTimer();

    ui->text1Label->setStyleSheet(QString());
    ui->text2Label->setStyleSheet(QString());
    ui->text3Label->setStyleSheet(QString());
    ui->text1Label->setText(" ");
    ui->text2Label->setText(" ");
    ui->text3Label->setText(" ");
    ui->titleLabel->hide();

    next();
}

void KanaWritingPracticeForm::next()
{
    ui->drawArea->clear();
    if (t != nullptr)
        t->stop();
    delete t;
    t = nullptr;
    disconnect(ui->kanjiView, &ZKanjiDiagram::strokeChanged, this, &KanaWritingPracticeForm::animateNext);
    ui->revealButton->setIcon(playico);
    ui->stack->setCurrentIndex(0);
    ui->kanjiView->stop();

    ++pos;

    ui->kanjiView->setKanjiIndex(0);

    if (pos == tosigned(list.size()))
    {
        stopTimer(false);

        ui->resultLabel->setStyleSheet(QString());
        ui->status->setValue(1, "0");
        ui->status->setValue(2, QString::number(mistakes));
        //dueLabel->setText(0);
        //wrongLabel->setText(QString::number(mistakes));

        ui->kanaLabel->setText(QString());
        ui->questionLabel->setText(QString());

        ui->resultLabel->setGraphicsEffect(nullptr);
        ui->resultLabel->setText(tr("Finished"));
        return;
    }

    ui->status->setValue(1, QString::number(std::max(0, (int)list.size() - pos)));
    ui->status->setValue(2, QString::number(mistakes));
    //dueLabel->setText(QString::number(std::max(0, (int)list.size() - pos)));
    //wrongLabel->setText(QString::number(mistakes));

    entered = QString();
    retries = 0;

    //ui->text1Label->setText(" ");
    //ui->text2Label->setText(" ");
    //ui->text3Label->setText(" ");

    ui->kanaLabel->setText(list[pos] < (int)KanaSounds::Count ? tr("Hiragana") : tr("Katakana"));
    ui->kanaLabel->setStyleSheet(QString("background-color: %1; color: %2;").arg(list[pos] < (int)KanaSounds::Count ? Settings::uiColor(ColorSettings::HiraBg).name() : Settings::uiColor(ColorSettings::KataBg).name()).arg(Settings::textColor(ColorSettings::Text).name()));
    ui->questionLabel->setText(list[pos] < (int)KanaSounds::Count ? kanaStrings[list[pos]] : kanaStrings[list[pos] - (int)KanaSounds::Count].toUpper());
}

void KanaWritingPracticeForm::answered(bool correct)
{
    if (!correct)
    {
        ++retries;
        if (retries == 2)
        {
            // Too many mistakes made in the same syllable.
            ui->resultLabel->setText(tr("Mistake"));
            ui->resultLabel->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
            hideLabelAnimation();

            ++mistakes;
            if (mistakes == 3)
                stopTimer(true);

            QString answer = toKana(kanaStrings[list[pos] % (int)KanaSounds::Count]);
            if (list[pos] > (int)KanaSounds::Count)
                answer = toKatakana(answer);

            next();

            ui->titleLabel->setVisible(!answer.isEmpty());

            ui->text1Label->setText(answer.at(0));
            if (answer.size() > 1)
                ui->text2Label->setText(answer.at(1));
            else
                ui->text2Label->setText(" ");
            if (answer.size() > 2)
                ui->text3Label->setText(answer.at(2));
            else
                ui->text3Label->setText(" ");

            ui->text1Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
            ui->text2Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
            ui->text3Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
        }
        else
        {
            // Just a mistake, start over entering the same text.
            ui->resultLabel->setText(tr("Try again"));
            ui->resultLabel->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
            hideLabelAnimation();

            entered = QString();

            ui->text1Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
            ui->text2Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
            ui->text3Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
        }
        return;
    }

    ui->resultLabel->setText(tr("Correct"));
    ui->resultLabel->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyCorrect).name()));

    hideLabelAnimation();

    next();

    ui->text1Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyCorrect).name()));
    ui->text2Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyCorrect).name()));
    ui->text3Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyCorrect).name()));
}

void KanaWritingPracticeForm::stopTimer(bool hide)
{
    timer.stop();
    if (hide)
        ui->status->setValue(0, "-");
    //timeLabel->setText("-");
}

void KanaWritingPracticeForm::startTimer()
{
    timer.start(1000, this);
    starttime = QDateTime::currentDateTimeUtc();
    ui->status->setValue(0, DateTimeFunctions::formatPassedTime(0, false));
    //timeLabel->setText(DateTimeFunctions::formatPassedTime(0, false));
}

void KanaWritingPracticeForm::animateNext(int index, bool ended)
{
    if (!ended)
        return;
    t = new QTimer(this);
    t->setSingleShot(true);
    t->start(index == -1 ? 0 : 500);
    connect(t, &QTimer::timeout, [this]() {
        t->deleteLater(); 
        t = nullptr;

        if (ui->stack->currentIndex() == 0)
            return;

        ++animatepos;
        QString answer = toKana(kanaStrings[list[pos] % (int)KanaSounds::Count]);
        if (list[pos] >= (int)KanaSounds::Count)
            answer = toKatakana(answer);
        if (animatepos >= answer.size())
            animatepos = 0;

        ui->kanjiView->setKanjiIndex(-1 - ZKanji::elements()->elementOf(-answer.at(animatepos).unicode()));
        ui->kanjiView->play();

    });
}

void KanaWritingPracticeForm::hideLabelAnimation()
{
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    ui->resultLabel->setGraphicsEffect(eff);
    QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
    a->setDuration(1000);
    a->setStartValue(1);
    a->setEndValue(0);
    a->start(QPropertyAnimation::DeleteWhenStopped);
}


//-------------------------------------------------------------
