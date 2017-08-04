/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QDesktopWidget>
#include "wordstudyform.h"
#include "ui_wordstudyform.h"
#include "words.h"
#include "worddeck.h"
#include "groupstudy.h"
#include "zui.h"
#include "kanjireadingpracticeform.h"
#include "zevents.h"
#include "zkanjimain.h"
#include "romajizer.h"
#include "wordtestresultsform.h"
#include "recognizerform.h"
#include "dialogwindow.h"
#include "studysettings.h"
#include "fontsettings.h"
#include "globalui.h"
#include "zstrings.h"

// Color of border and "New" text during long term study.
static const QColor newcolor(80, 140, 200);
// Color for invalid or retried answer.
static const QColor retrycolor(245, 0, 0);
// Color when answer was correctly entered.
static const QColor correctcolor(30, 160, 70);
// Color of answer when the answer wasn't entered correctly.
static const QColor answercolor(30, 70, 160);

//-------------------------------------------------------------


ChoiceLabel::ChoiceLabel(int index, QWidget *parent) : base(parent), index(index), active(false)
{
    setAttribute(Qt::WA_MouseTracking);

    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QFont f = Settings::defFont();
    f.setPointSize(f.pointSize() * 1.3);
    setFont(f);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    //setWordWrap(true);
}

void ChoiceLabel::setHeight(int height)
{
    h = height;
    setMinimumHeight(h);
    setMaximumHeight(h);
}

QSize ChoiceLabel::sizeHint() const
{
    QSize s;// = base::sizeHint();
    s.setHeight(h);
    s.setWidth(1);
    return s;
}

QSize ChoiceLabel::minimumSizeHint() const
{
    QSize s = base::minimumSizeHint();
    s.setHeight(h);
    return s;
}

bool ChoiceLabel::hasHeightForWidth() const
{
    return false;
}

int ChoiceLabel::heightForWidth(int h) const
{
    return -1;
}

void ChoiceLabel::setText(QString str)
{
    str.replace('&', "&&");
    base::setText(str);
}

bool ChoiceLabel::isActive()
{
    return active;
}

void ChoiceLabel::setActive(bool a)
{
    active = a;
    setStyleSheet(QStringLiteral(""));
}

void ChoiceLabel::enterEvent(QEvent *e)
{
    if (active)
        setStyleSheet(QStringLiteral("background-color: %1; color: %2").arg(palette().color(QPalette::Active, QPalette::Highlight).name()).arg(palette().color(QPalette::Active, QPalette::HighlightedText).name()));
    base::enterEvent(e);
}

void ChoiceLabel::leaveEvent(QEvent *e)
{
    if (active)
        setStyleSheet(QStringLiteral(""));
    base::leaveEvent(e);
}

void ChoiceLabel::mousePressEvent(QMouseEvent *e)
{
    base::mousePressEvent(e);

    if (active && e->button() == Qt::LeftButton)
        emit clicked(index);
}


//-------------------------------------------------------------


DefinitionLabel::DefinitionLabel(QWidget *parent) : base(parent), ptsize(-1), lineh(0)
{

}

QSize DefinitionLabel::sizeHint() const
{
    return minimumSizeHint();
}

QSize DefinitionLabel::minimumSizeHint() const
{
    QSize s = base::minimumSizeHint();
    s.setHeight(lineh);
    return s;
}

bool DefinitionLabel::hasHeightForWidth() const
{
    return false;
}

int DefinitionLabel::heightForWidth(int h) const
{
    return -1;
}

void DefinitionLabel::updated()
{
    if (ptsize == -1)
    {
        ptsize = font().pointSize();
        lineh = fontMetrics().height();
    }

    QFont f = font();
    QFontMetrics fm(f);

    f.setPointSize(ptsize);
    int newptsize = ptsize;

    int h0 = height();
    int h = h0 + 1;

    f.setPointSize(newptsize);
    fm = QFontMetrics(f);

    h = fm.boundingRect(QRect(0, 0, width(), 9999999), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text()).height();
    if (h > h0)
    {
        int maxptsize = newptsize;
        int minptsize = newptsize * (double(h0) / h);

        while (minptsize < maxptsize)
        {
            newptsize = (maxptsize + minptsize) / 2;

            f.setPointSize(newptsize);
            fm = QFontMetrics(f);

            h = fm.boundingRect(QRect(0, 0, width(), 9999999), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text()).height();
            if (h > h0)
                maxptsize = newptsize - 1;
            else
                minptsize = newptsize + 1;
        }

        if (h > h0)
            f.setPointSize(f.pointSize() - 1);
        else
        {
            f.setPointSize(f.pointSize() + 1);
            fm = QFontMetrics(f);
            h = fm.boundingRect(QRect(0, 0, width(), 9999999), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text()).height();
            if (h > h0)
                f.setPointSize(f.pointSize() - 1);
        }
    }

    setFont(f);
}

void DefinitionLabel::resizeEvent(QResizeEvent *e)
{
    updated();

    base::resizeEvent(e);
}


//-------------------------------------------------------------


void WordStudyForm::studyDeck(WordDeck *deck)
{
    bool ok = true;
    int r = 0;

    HideAppWindowsGuard hideguard;

    int due = deck->dueSize();
    int queue = deck->queueSize();

    if (deck->firstTest() || (due == 0 && queue != 0 && Settings::study.includecount == 0))
    {
        // One, Two, Three, Five, Seven
        int includedays[] = { 1, 2, 3, 5, 7 };
        int sincedays = deck->daysSinceLastInclude();
        if (sincedays == -1 || (Settings::study.includecount == 0 && queue != 0) || (Settings::study.includecount != 0 && sincedays >= includedays[(int)Settings::study.includedays]))
        {
            if (queue != 0)
            {
                if (Settings::study.includecount == 0)
                {
                    r = QInputDialog::getInt(nullptr, tr("Inclusion of new items"),
                        (deck->firstTest() ? QString() : tr("You have already finished studying for the day.\nCancel or close the window if you started the test by mistake.\n\n") ) +
                        tr("Select the number of new items to study.\nDue words from previous days: %1\n\nValid range: 0 - %2 (Unique items: %3)").arg(due).arg(queue).arg(deck->queueUniqueSize()), 0, 0, queue, 1, &ok);
                }
                else
                {
                    r = std::min(Settings::study.onlyunique ? deck->queueUniqueSize() : queue, Settings::study.includecount);
                    if (Settings::study.limitnew)
                        r = std::min(r, std::max(0, Settings::study.limitcount - due));
                }
            }
            else
            {
                ok = due == 0 || QMessageBox::question(nullptr, tr("No new items"), tr("There are no new items to include in the test for today. Would you like to study anyway?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
            }
        }
    }

    if (ok == false || (r == 0 && due == 0))
    {
        if (ok)
        {
            QMessageBox::information(nullptr, "zkanji",
                queue == 0 ?
                tr("No words to study for today.\n\nIf you would like to study, add more words to your study queue and start a new test.")
                :
                tr("No words to study for today.\n\nIf you would like to study new words, start a new test and allow new words to be included."), QMessageBox::Ok);
        }

        if (deck->readingsQueued() && QMessageBox::question(nullptr, tr("Readings available"), tr("Would you like to practice readings of kanji from words tested today?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            KanjiReadingPracticeForm *form = new KanjiReadingPracticeForm(deck);
            form->exec();
        }

        return;
    }

    deck->initNewStudy(r);

    WordStudyForm *form = new WordStudyForm();
    form->exec(deck);
}

WordStudyForm::WordStudyForm(QWidget *parent) :
        base(parent), ui(new Ui::WordStudyForm), result(ModalResult::Suspend), deck(nullptr), study(nullptr),
        testedcount(0), passedcount(0), passedtime(0), answered(false), first(true)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_QuitOnClose, false);

    ui->statusLabel->hide();
    ui->optionsButton->hide();

    connect(ui->acceptButton, &QPushButton::clicked, this, &WordStudyForm::answerEntered);
    connect(ui->showButton, &QPushButton::clicked, this, &WordStudyForm::answerEntered);
    connect(ui->hintButton, &QPushButton::clicked, this, &WordStudyForm::showWordHint);

    connect(ui->retryButton, &QPushButton::clicked, this, &WordStudyForm::resultSelected);
    connect(ui->wrongButton, &QPushButton::clicked, this, &WordStudyForm::resultSelected);
    connect(ui->correctButton, &QPushButton::clicked, this, &WordStudyForm::resultSelected);
    connect(ui->easyButton, &QPushButton::clicked, this, &WordStudyForm::resultSelected);

    connect(ui->decideCorrectButton, &QPushButton::clicked, this, &WordStudyForm::answerDecided);
    connect(ui->decideWrongButton, &QPushButton::clicked, this, &WordStudyForm::answerDecided);
    connect(ui->continueButton, &QPushButton::clicked, this, &WordStudyForm::next);

    connect(ui->quitButton, &QPushButton::clicked, this, &WordStudyForm::close);
    connect(ui->pauseButton, &QPushButton::clicked, this, &WordStudyForm::suspend);
    connect(ui->resumeButton, &QPushButton::clicked, this, &WordStudyForm::resume);
}

WordStudyForm::~WordStudyForm()
{
    delete ui;
}

Dictionary* WordStudyForm::dictionary()
{
    if (deck)
        return deck->dictionary();
    return study->dictionary();
}

void WordStudyForm::exec(WordDeck *d)
{
    deck = d;

    ui->timerBar->hide();

    ui->undoCBox->addItem(tr("Correct"));
    ui->undoCBox->addItem(tr("Incorrect"));
    ui->undoCBox->addItem(tr("Try again"));
    ui->undoCBox->addItem(tr("Easy"));

    deck->startTest();

    statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Due") + ": ", 0, dueLabel1 = new QLabel(this), "0", 5, dueLabel2 = new QLabel(this), "(0)", 7));
    dueLabel1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Tested") + ": ", 0, testedLabel1 = new QLabel(this), "0", 5, testedLabel2 = new QLabel(this), "(0)", 7));
    testedLabel1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addWidget(createStatusWidget(this, -1, nullptr, tr("ETA") + ": ", 0, etaLabel = new QLabel(this), "00:00:00", 0));
    statusBar()->addWidget(createStatusWidget(this, -1, nullptr, tr("Time passed") + ": ", 0, timeLabel = new QLabel(this), "00:00:00", 0));

    ui->testStack->setCurrentWidget(ui->inputPage);
    setAttribute(Qt::WA_DontShowOnScreen, true);
    show();
    qApp->processEvents();
    if (ui->hintButton->isVisible())
        ui->hintWidget->setMinimumWidth(ui->hintButton->width());
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    if (!showNext())
    {
        deleteLater();
        return;
    }

    ui->correctLabel->setVisible(Settings::study.showestimate);
    ui->easyLabel->setVisible(Settings::study.showestimate);
    ui->retryLabel->setVisible(Settings::study.showestimate);
    ui->wrongLabel->setVisible(Settings::study.showestimate);

    QRect r = frameGeometry();
    QRect sr = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    move(sr.left() + (sr.width() - r.width()) / 2, sr.top() + (sr.height() - r.height()) / 2);

    gUI->hideAppWindows();

    lasttime = idletime = starttime = QDateTime::currentDateTimeUtc();
    qApp->installEventFilter(this);
    timer.start(50, this);
    show();
}


void WordStudyForm::exec(WordStudy *s)
{
    study = s;

    if (!s->studySettings().usetimer)
        ui->timerBar->hide();

    ui->undoCBox->addItem(tr("Correct"));
    ui->undoCBox->addItem(tr("Wrong"));

    // Manually add labels of selectable choices to their parent widget.

    if (study->studySettings().answer == WordStudyAnswering::Choices5 || study->studySettings().answer == WordStudyAnswering::Choices8)
    {
        ui->label->setText(tr("Choose the answer:"));

        ui->choiceWidget->setStyleSheet(QString("background-color: %1").arg(palette().color(QPalette::Active, QPalette::Base).name()));
        QVBoxLayout *vLayout = new QVBoxLayout(ui->choiceWidget);
        int avg = fontMetrics().averageCharWidth() * 0.8;
        vLayout->setContentsMargins(avg, avg, avg, avg);
        vLayout->setSpacing(3);

        // Choice label heights are always 2 lines tall.
        int lbh = -1;

        for (int ix = 0, siz = study->studySettings().answer == WordStudyAnswering::Choices5 ? 5 : 8; ix != siz; ++ix)
        {
            ChoiceLabel *l = new ChoiceLabel(ix, ui->choiceWidget);
            if (lbh == -1)
                lbh = l->fontMetrics().height() * 1.1;

            l->setHeight(lbh);
            vLayout->addWidget(l);
            connect(l, &ChoiceLabel::clicked, this, &WordStudyForm::choiceClicked);
        }

        //int lbcnt = ui->choiceWidget->layout()->count();
        //int ch = lbcnt * lbh + vLayout->spacing() * (lbcnt - 1) + avg * 2;
        //ui->choiceWidget->setMinimumHeight(ch);
        //ui->choiceWidget->setMaximumHeight(ch);

        //ui->answerStack->setMinimumHeight(ch + ui->choicePage->layout()->spacing() + ui->choiceButton->height());
        //ui->answerStack->setMaximumHeight(ui->answerStack->minimumHeight());

        //ui->testStack->setMinimumHeight(ui->answerStack->minimumHeight() + ui->inputPage->layout()->spacing() + ui->horizontalLayout->sizeHint().height());
    }

    study->initTest();

    // Using the gradual inclusion method, numbers in brackets are the new words included in
    // this round.
    if (s->studySettings().method == WordStudyMethod::Gradual)
    {
        QLabel *roundLabel;
        statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Round") + ": ", 0, roundLabel = new QLabel(this), QString::number(s->currentRound() + 1), 4));
        roundLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Due") + ": ", 0, dueLabel1 = new QLabel(this), "0", 5, dueLabel2 = new QLabel(this), "(0)", 7));
    }
    else
        statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Due") + ": ", 0, dueLabel1 = new QLabel(this), "0", 5));
    dueLabel1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Correct") + ": ", 0, testedLabel1 = new QLabel(this), "0", 5));
    testedLabel1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Wrong") + ": ", 0, testedLabel2 = new QLabel(this), "0", 5));
    testedLabel2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    statusBar()->addWidget(createStatusWidget(this, 2, nullptr, tr("Time passed") + ": ", 0, timeLabel = new QLabel(this), "00:00:00", 0));

    showNext();

    setAttribute(Qt::WA_DontShowOnScreen, true);
    show();
    qApp->processEvents();
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    QRect r = frameGeometry();
    QRect sr = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    move(sr.left() + (sr.width() - r.width()) / 2, sr.top() + (sr.height() - r.height()) / 2);

    gUI->hideAppWindows();

    lasttime = idletime = starttime = QDateTime::currentDateTimeUtc();
    qApp->installEventFilter(this);
    timer.start(50, this);
    show();
}

void WordStudyForm::closeEvent(QCloseEvent *e)
{
    e->accept();

    if (deck || result != ModalResult::Suspend)
        qApp->postEvent(this, new EndEvent());
    else
    {
        study->finish();

        // Study aborted.
        gUI->showAppWindows();
        deleteLater();
    }

    base::closeEvent(e);
}

bool WordStudyForm::event(QEvent *e)
{
    QDateTime currenttime;
    if (e->type() == QEvent::Timer && ui->mainStack->currentWidget() != ui->suspendPage && ((QTimerEvent*)e)->timerId() == timer.timerId())
    {
        // The value of currenttime is used when testing with a normal group below too. When
        // this is changed, another value must be used there to save the current date time.
        currenttime = QDateTime::currentDateTimeUtc();

        int waittimes[] = { 1000 * 60, 1000 * 90, 1000 * 120, 1000 * 180, 1000 * 300 };

        if (deck && Settings::study.idlesuspend && waittimes[Settings::study.idletime] <= idletime.msecsTo(currenttime))
            suspend();
        else
        {
            passedtime += lasttime.msecsTo(currenttime);
            lasttime = currenttime;
            int hours = passedtime / 1000 / 60 / 60;
            int minutes = (passedtime - hours * 60 * 60 * 1000) / 1000 / 60;
            int seconds = (passedtime - hours * 60 * 60 * 1000 - minutes * 60 * 1000) / 1000;
            timeLabel->setText(QString("%1:%2:%3").arg(QString::number(hours), 2, QChar('0')).arg(QString::number(minutes), 2, QChar('0')).arg(QString::number(seconds), 2, QChar('0')));

        }
    }
    

    if (deck)
    {
        if (e->type() == EndEvent::Type())
        {
            if (deck->readingsQueued() && QMessageBox::question(nullptr, tr("Readings available"), tr("Would you like to practice readings of kanji from words tested today?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            {
                KanjiReadingPracticeForm *form = new KanjiReadingPracticeForm(deck);
                form->exec();
            }

            gUI->showAppWindows();
            deleteLater();
            return true;
        }
    }
    else
    {
        if (e->type() == QEvent::Timer && ui->mainStack->currentWidget() != ui->suspendPage && ((QTimerEvent*)e)->timerId() == timer.timerId() && study->studySettings().usetimer)
        {
            if (ui->testStack->currentWidget() != ui->decisionPage && ui->testStack->currentWidget() != ui->continuePage && !answered)
            {
                ui->timerBar->setValue(std::min<int>(100, itemtime.msecsTo(currenttime) / study->studySettings().timer / 10));
                if (ui->timerBar->value() == 100)
                {
                    ui->kanaAnswerEdit->setText(QString());
                    answerEntered();
                }
            }

        }

        if (e->type() == EndEvent::Type())
        {
            study->finish();

            // Test or test round ended. Show the results window.
            WordTestResultsForm *form = new WordTestResultsForm;
            form->exec(study);
            deleteLater();

            return true;
        }
    }

    return base::event(e);
}

bool WordStudyForm::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::KeyPress || e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick || e->type() == QEvent::NonClientAreaMouseButtonPress || e->type() == QEvent::NonClientAreaMouseButtonDblClick)
    {
        if (ui->mainStack->currentWidget() != ui->suspendPage)
            idletime = QDateTime::currentDateTimeUtc();
    }

    return base::eventFilter(obj, e);
}

void WordStudyForm::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    if (ui->mainStack->currentWidget() == ui->suspendPage)
        return;

    bool newcard = false;
    bool failedcard = false;

    if (deck)
    {
        newcard = deck->nextNewCard();
        failedcard = !newcard && deck->nextFailedCard();
    }
    else
    {
        newcard = study->nextNew();
        failedcard = !newcard && study->nextFailed();
    }
    if (!newcard && !failedcard)
        return;

    QPainter p(this);

    QColor c = newcard ? newcolor : retrycolor;
    p.fillRect(rect(), mixColors(c, palette().color(QPalette::Background), 0.03));

    p.setPen(QPen(c, 4));
    p.setBrush(Qt::transparent);

    p.drawRect(rect());
}

void WordStudyForm::answerEntered()
{
    // Color the word part depending on whether the answer was correct or not.
    // 0 - wrong
    // 1 - correct
    // -1 - undetermined (the student must pick whether they got it or not.)
    int correct = -1;

    WordEntry *w = dictionary()->wordEntry(windex);

    ui->kanjiLabel->setText(w->kanji.toQStringRaw());
    ui->kanjiLabel->show();
    ui->kanaLabel->setText(w->kana.toQStringRaw());
    ui->kanaLabel->show();
    ui->meaningLabel->setText(dictionary()->displayedStudyDefinition(windex));
    ui->meaningLabel->show();
    ui->meaningLabel->updated();

    QWidget *enterWidget = deck ? ui->enteredWidget : ui->enteredWidget2;
    QSpacerItem *enterSpacer = deck ? ui->enteredSpacer : ui->enteredSpacer2;
    QLabel *enterLabel = deck ? ui->enteredLabel : ui->enteredLabel2;

    if (ui->answerStack->currentWidget() == ui->textPage)
    {
        // The question can only have one bit set if the answer was typed in.
        if (wquestion == (int)WordPartBits::Kanji || wquestion == (int)WordPartBits::Kana)
        {
            QString entered = ui->kanaAnswerEdit->text();
            QString solution;
            if (wquestion == (int)WordPartBits::Kanji)
                solution = w->kanji.toQStringRaw();
            else if (wquestion == (int)WordPartBits::Kana)
                solution = w->kana.toQStringRaw();

            if ((deck && Settings::study.matchkana) || (!deck && !study->studySettings().matchkana))
            {
                entered = hiraganize(entered);
                solution = hiraganize(solution);
            }
            correct = entered == solution;

            QColor c = correct ? correctcolor : retrycolor;
            enterLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); // red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
        }
        else
        {
            enterLabel = ui->enteredLabel3;
            enterSpacer = ui->enteredSpacer3;
            enterWidget = ui->enteredWidget3;

            enterLabel->setStyleSheet(QStringLiteral(""));
        }

        ZLineEdit *edit;
        if (wquestion == (int)WordPartBits::Definition)
            edit = ui->defAnswerEdit;
        else
            edit = ui->kanaAnswerEdit;

        enterLabel->setText(edit->text());
        enterWidget->show();
        enterSpacer->changeSize(1, 1, QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
    else if (ui->answerStack->currentWidget() == ui->buttonPage)
        ;
    else if (ui->answerStack->currentWidget() == ui->choicePage)
    {
        activateChoices(false);

        correct = wchoice == picked;

        if (correct == 1)
        {
            QColor c = correctcolor;
            ui->choiceWidget->layout()->itemAt(wchoice)->widget()->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
        }
        else if (correct == 0)
        {
            QColor c = answercolor;
            ui->choiceWidget->layout()->itemAt(wchoice)->widget()->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
            if (picked != -1)
            {
                QFont tmpfont = ui->choiceWidget->layout()->itemAt(picked)->widget()->font();
                c = retrycolor;
                ui->choiceWidget->layout()->itemAt(picked)->widget()->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
                ui->choiceWidget->layout()->itemAt(picked)->widget()->setFont(tmpfont);
            }
        }
        picked = -1;
    }

    if (ui->answerStack->currentWidget() != ui->textPage)
        enterWidget->hide();
    enterSpacer->changeSize(1, 1, QSizePolicy::Ignored, QSizePolicy::Expanding);
    enterWidget->parentWidget()->layout()->invalidate();

    if (correct == 0)
    {
        QFont tmpfont = ui->kanjiLabel->font();

        QColor c = retrycolor;
        if ((wquestion & (int)WordPartBits::Kanji) != 0)
        {
            ui->kanjiLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
            ui->kanjiLabel->setFont(tmpfont);
        }
        if ((wquestion & (int)WordPartBits::Kana) != 0)
        {
            ui->kanaLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
            ui->kanaLabel->setFont(tmpfont);
        }
        if ((wquestion & (int)WordPartBits::Definition) != 0)
        {
            ui->meaningLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
            ui->meaningLabel->setFont(tmpfont);
            ui->meaningLabel->updated();
        }
    }
    else if (correct == 1)
    {
        QColor c = correctcolor;
        if ((wquestion & (int)WordPartBits::Kanji) != 0)
            ui->kanjiLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
        if ((wquestion & (int)WordPartBits::Kana) != 0)
            ui->kanaLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
        if ((wquestion & (int)WordPartBits::Definition) != 0)
            ui->meaningLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); //red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));
    }

    if (deck)
        ui->testStack->setCurrentWidget(ui->scorePage);
    else
    {
        answered = true;
        if (correct == -1)
        {
            ui->testStack->setCurrentWidget(ui->decisionPage);
            return;
        }

        study->answer(correct == 1);
        updateStudyLabels();

        if (ui->answerStack->currentWidget() == ui->choicePage)
        {
            ui->previousWidget->setVisible(false);
            ui->choiceButton->setText(tr("Continue"));
            disconnect(ui->choiceButton, &QPushButton::clicked, this, nullptr);
            connect(ui->choiceButton, &QPushButton::clicked, this, &WordStudyForm::next);
            return;
        }
        ui->continueButton->setText(study->nextIndex() != -1 ? tr("Continue") : tr("Finish"));

        ui->testStack->setCurrentWidget(ui->continuePage);
    }
}

void WordStudyForm::showWordHint()
{
    WordEntry *w = dictionary()->wordEntry(windex);
    if (whint == WordParts::Kanji)
        ui->kanjiLabel->setText(w->kanji.toQStringRaw());
    if (whint == WordParts::Kana)
        ui->kanaLabel->setText(w->kana.toQStringRaw());
    if (whint == WordParts::Definition)
        ui->meaningLabel->setText(dictionary()->displayedStudyDefinition(windex));

    ui->hintButton->hide();
}

void WordStudyForm::resultSelected()
{
    QPushButton *s = static_cast<QPushButton*>(sender());
    deck->answer(s == ui->retryButton ? StudyCard::Retry : s == ui->wrongButton ? StudyCard::Wrong : s == ui->correctButton ? StudyCard::Correct : StudyCard::Easy, itemtime.msecsTo(QDateTime::currentDateTimeUtc()));

    ++testedcount;
    if (s == ui->correctButton || s == ui->easyButton)
        ++passedcount;

    if ((Settings::study.itemsuspend && Settings::study.itemsuspendnum <= testedcount) || (Settings::study.timesuspend && Settings::study.timesuspendnum <= (starttime.msecsTo(QDateTime::currentDateTimeUtc())) / (1000 * 60)))
        suspend();

    if (!showNext())
        close();
}

void WordStudyForm::answerDecided()
{
    study->answer(sender() == ui->decideCorrectButton);
    next();
}

void WordStudyForm::on_undoCBox_currentIndexChanged(int index)
{
    if (index == -1 || first)
        return;

    if (deck)
    {
        StudyCard::AnswerType a = deck->lastAnswer();
        deck->changeLastAnswer(index == 0 ? StudyCard::Correct : index == 1 ? StudyCard::Wrong : index == 2 ? StudyCard::Retry : StudyCard::Easy);
        if ((a == StudyCard::Correct || a == StudyCard::Easy) != (index != 1 && index != 2))
        {
            if (index != 1 && index != 2)
                ++passedcount;
            else
                --passedcount;
        }
        updateDeckLabels();
    }
    else
    {
        study->changeAnswer(index == 0);
        updateStudyLabels();
    }


    QMessageBox::information(this, "zkanji", tr("Study data for the previous item has been updated."), QMessageBox::Ok);
}

void WordStudyForm::choiceClicked(int index)
{
    picked = index;
    answerEntered();
}

void WordStudyForm::suspend()
{
    if (ui->mainStack->currentWidget() == ui->suspendPage)
        return;

    timer.stop();
    if (study && study->studySettings().usetimer)
        ui->timerBar->hide();

    ui->mainStack->setCurrentWidget(ui->suspendPage);
    update();
    idletime = QDateTime::currentDateTimeUtc();
}

void WordStudyForm::resume()
{
    if (ui->mainStack->currentWidget() != ui->suspendPage)
        return;

    ui->mainStack->setCurrentWidget(ui->testPage);
    starttime = QDateTime::currentDateTimeUtc();
    testedcount = 0;
    itemtime = itemtime.addMSecs(idletime.msecsTo(QDateTime::currentDateTimeUtc()));
    lasttime = lasttime.addMSecs(idletime.msecsTo(QDateTime::currentDateTimeUtc()));
    timer.start(50, this);

    if (study && study->studySettings().usetimer)
        ui->timerBar->show();

    update();
}

bool WordStudyForm::showNext()
{
    if (deck)
    {
        windex = deck->nextIndex();
        if (windex == -1)
            return false;

        ui->optionsButton->hide();

        updateDeckLabels();

        wquestion = (int)deck->nextQuestion();
        whint = deck->nextMainHint();

        quint32 correctint = deck->nextCorrectSpacing();
        quint32 easyint = deck->nextEasySpacing();
        //bool retrypostpone = deck->nextRetryPostponed();
        //bool wrongpostpone = deck->nextWrongPostponed();

        deck->generateNextItem();

        ui->correctLabel->setText(formatSpacing(correctint));
        ui->easyLabel->setText(formatSpacing(easyint));
        //ui->retryLabel->setText(retrypostpone ? tr("Postpone") : tr("Few minutes"));
        //ui->wrongLabel->setText(wrongpostpone ? tr("Postpone") : tr("Few minutes"));
    }
    else
    {
        if (!study->initNext())
        {
            result = ModalResult::Stop;
            return false;
        }
        windex = study->nextIndex();

        whint = WordParts::Default;
        wquestion = study->nextQuestion();

        if (ui->timerBar->isVisibleTo(this))
            ui->timerBar->setValue(0);

        answered = false;
    }


    WordEntry *w = dictionary()->wordEntry(windex);

    // Remove color and strikeout
    QFont tmpfont = ui->kanjiLabel->font();
    ui->kanjiLabel->setStyleSheet(QStringLiteral(""));
    ui->kanjiLabel->setFont(tmpfont);
    ui->kanaLabel->setStyleSheet(QStringLiteral(""));
    ui->kanaLabel->setFont(tmpfont);
    ui->meaningLabel->setStyleSheet(QStringLiteral(""));
    ui->meaningLabel->setFont(tmpfont);
    ui->meaningLabel->updated();

    // Kanji label.
    if ((wquestion & (int)WordPartBits::Kanji) != 0)
        ui->kanjiLabel->setText(tr("?") % QChar(0x3000));
    else if (whint != WordParts::Kanji)
    {
        QString str = w->kanji.toQStringRaw();
        if ((deck && Settings::study.hidekanjikana) || (!deck && study->studySettings().hidekana))
        {
            QChar subst[] = { QChar(0x25b3), QChar(0x25ce), QChar(0x25c7), QChar(0x2606) };
            for (int ix = 0; ix != str.size(); ++ix)
            {
                if (KANA(str.at(ix).unicode()) || DASH(str.at(ix).unicode()))
                    str[ix] = subst[rnd(0, 3)];
            }
        }
        ui->kanjiLabel->setText(str);
    }
    else
        ui->kanjiLabel->setText(QChar(0x3000));

    // Kana label.
    if ((wquestion & (int)WordPartBits::Kana) != 0)
        ui->kanaLabel->setText(tr("?") % QChar(0x3000));
    else if (whint != WordParts::Kana)
        ui->kanaLabel->setText(w->kana.toQStringRaw());
    else
        ui->kanaLabel->setText(QChar(0x3000));

    // Meaning label.
    if ((wquestion & (int)WordPartBits::Definition) != 0)
        ui->meaningLabel->setText(tr("?") % QChar(' '));
    else if (whint != WordParts::Definition)
        ui->meaningLabel->setText(dictionary()->displayedStudyDefinition(windex));
    else
        ui->meaningLabel->setText(" ");
    ui->meaningLabel->updated();

    ui->hintButton->setVisible(whint != WordParts::Default);

    int wtypes = 0;
    for (const auto &d : w->defs)
        wtypes |= d.attrib.types;
    ui->wtypeLabel->setText(Strings::wordTypesTextLong(wtypes));

    if (deck)
    {
        if (!first && deck->canUndo())
        {
            // TODO: save the times for each answer button so it can be shown in the undo box.
            ui->previousWidget->setVisible(true);

            // Prevent firing the undoCBox's currentIndexChanged.
            first = true;

            ui->undoCBox->setCurrentIndex(deck->lastAnswer() == StudyCard::Correct ? 0 :
                deck->lastAnswer() == StudyCard::Wrong ? 1 :
                deck->lastAnswer() == StudyCard::Retry ? 2 : 3);
            
        }
        else
            ui->previousWidget->setVisible(false);


        if (((wquestion & (int)WordPartBits::Definition) && Settings::study.writedef) ||
            ((wquestion & (int)WordPartBits::Kanji) && Settings::study.writekanji) ||
            ((wquestion & (int)WordPartBits::Kana) && Settings::study.writekana))
        {
            ZLineEdit *edit;
            ui->defAnswerEdit->setVisible((wquestion & (int)WordPartBits::Definition) != 0);
            ui->kanaAnswerEdit->setVisible((wquestion & (int)WordPartBits::Definition) == 0);
            if (wquestion & (int)WordPartBits::Definition)
                edit = ui->defAnswerEdit;
            else
                edit = ui->kanaAnswerEdit;
            edit->clear();
            ui->recognizeButton->setVisible((wquestion & (int)WordPartBits::Kanji) != 0);
            if ((wquestion & (int)WordPartBits::Kanji) != 0)
                installRecognizer(ui->recognizeButton, ui->kanaAnswerEdit, RecognizerPosition::StartBelow);
            else
                uninstallRecognizer(ui->recognizeButton, ui->kanaAnswerEdit);

            ui->kanaAnswerEdit->setAttribute(Qt::WA_InputMethodEnabled, false);
            ui->kanaAnswerEdit->setValidator((wquestion & (int)WordPartBits::Kanji) != 0 ? &japaneseValidator() : &kanaValidator());
            ui->answerStack->setCurrentWidget(ui->textPage);
        }
        else
            ui->answerStack->setCurrentWidget(ui->buttonPage);
        ui->testStack->setCurrentWidget(ui->inputPage);
    }
    else
    {
        // TODO: disable undo if previous item was same as current
        if (!first && study->canUndo())
        {
            ui->previousWidget->setVisible(true);

            // Setting this prevents firing the undoCBox's currentIndexChanged.
            first = true;

            ui->undoCBox->setCurrentIndex(study->previousCorrect() ? 0 : 1);
        }
        else
            ui->previousWidget->setVisible(false);

        if (study->studySettings().answer == WordStudyAnswering::Choices5 || study->studySettings().answer == WordStudyAnswering::Choices8)
        {
            picked = -1;
            QFont tmpfont = ui->choiceWidget->layout()->itemAt(0)->widget()->font();

            ui->choiceButton->setText(tr("Reveal answer"));
            disconnect(ui->choiceButton, &QPushButton::clicked, this, nullptr);
            connect(ui->choiceButton, &QPushButton::clicked, this, &WordStudyForm::answerEntered);

            // Fill the choices widget's labels with text.
            int siz = study->studySettings().answer == WordStudyAnswering::Choices5 ? 5 : 8;
            std::vector<int> choices(siz);
            study->randomIndexes(choices.data(), siz);
            for (int ix = 0; ix != siz; ++ix)
            {
                w = dictionary()->wordEntry(choices[ix]);
                if (choices[ix] == windex)
                    wchoice = ix;

                QString str;
                if ((wquestion & (int)WordPartBits::Kanji) != 0)
                    str = w->kanji.toQStringRaw();
                if ((wquestion & (int)WordPartBits::Kana) != 0)
                {
                    if (!str.isEmpty())
                        str += tr(" / ");
                    str += w->kana.toQStringRaw();
                }
                if ((wquestion & (int)WordPartBits::Definition) != 0)
                {
                    if (!str.isEmpty())
                        str += tr(" / ");
                    str += dictionary()->displayedStudyDefinition(choices[ix]);
                }
                ((QLabel*)((QVBoxLayout*)ui->choiceWidget->layout())->itemAt(ix)->widget())->setStyleSheet(QStringLiteral(""));
                ((QLabel*)((QVBoxLayout*)ui->choiceWidget->layout())->itemAt(ix)->widget())->setFont(tmpfont);
                ((QLabel*)((QVBoxLayout*)ui->choiceWidget->layout())->itemAt(ix)->widget())->setText(tr("   %1)  %2").arg(ix + 1).arg(str));

            }

            activateChoices(true);
            ui->answerStack->setCurrentWidget(ui->choicePage);
        }
        else
        {
            if ((wquestion == (int)WordPartBits::Kanji || wquestion == (int)WordPartBits::Kana || wquestion == (int)WordPartBits::Definition) && (study->studySettings().typed & wquestion) != 0)
            {
                ZLineEdit *edit;
                ui->defAnswerEdit->setVisible(wquestion == (int)WordPartBits::Definition);
                ui->kanaAnswerEdit->setVisible(wquestion != (int)WordPartBits::Definition);
                if (wquestion == (int)WordPartBits::Definition)
                    edit = ui->defAnswerEdit;
                else
                    edit = ui->kanaAnswerEdit;

                edit->clear();
                ui->kanaAnswerEdit->setAttribute(Qt::WA_InputMethodEnabled, false);
                ui->recognizeButton->setVisible(wquestion == (int)WordPartBits::Kanji);
                if (wquestion == (int)WordPartBits::Kanji)
                    installRecognizer(ui->recognizeButton, ui->kanaAnswerEdit, RecognizerPosition::StartBelow);
                else
                    uninstallRecognizer(ui->recognizeButton, ui->kanaAnswerEdit);

                ui->kanaAnswerEdit->setValidator((wquestion == (int)WordPartBits::Kanji) ? &japaneseValidator() : (wquestion == (int)WordPartBits::Kana) ? &kanaValidator() : nullptr);
                ui->answerStack->setCurrentWidget(ui->textPage);
            }
            else
                ui->answerStack->setCurrentWidget(ui->buttonPage);
        }
        ui->testStack->setCurrentWidget(ui->inputPage);

        updateStudyLabels();
    }

    itemtime = QDateTime::currentDateTimeUtc();

    // Set this to false to show the undo button the next time this function
    // is called.
    first = false;

    return true;
}

void WordStudyForm::next()
{
    if (study->studySettings().uselimit && study->studySettings().limit * 1000 * 60 <= starttime.msecsTo(QDateTime::currentDateTimeUtc()))
        suspend();

    if (showNext())
        return;

    result = ModalResult::Ok;
    close();
}

void WordStudyForm::activateChoices(bool activate)
{
    const QLayout *l = ui->choiceWidget->layout();
    for (int ix = 0; ix != l->count(); ++ix)
        ((ChoiceLabel*)l->itemAt(ix)->widget())->setActive(activate);
}

void WordStudyForm::updateDeckLabels()
{
    quint32 eta = deck->dueEta();
    int hours = eta / 10 / 60 / 60;
    int minutes = (eta - hours * 60 * 60 * 10) / 10 / 60;
    int seconds = (eta - hours * 60 * 60 * 10 - minutes * 60 * 10) / 10;
    etaLabel->setText(QString("%1:%2:%3").arg(QString::number(hours), 2, QChar('0')).arg(QString::number(minutes), 2, QChar('0')).arg(QString::number(seconds), 2, QChar('0')));
    dueLabel1->setText(QString("%1").arg(QString::number(deck->dueSize())));
    dueLabel2->setText(QString("(%1)").arg(QString::number(deck->newSize())));
    testedLabel1->setText(QString("%1").arg(QString::number(passedcount)));
    testedLabel2->setText(QString("(%1)").arg(QString::number(deck->failedSize())));

    bool newcard = deck->nextNewCard();
    bool failedcard = !newcard && deck->nextFailedCard();

    if (newcard || failedcard)
    {
        ui->statusLabel->setText(newcard ? tr("New") : tr("Retry"));
        QColor c = newcard ? newcolor : retrycolor;
        ui->statusLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name())); // red(), 2, 16, QLatin1Char('0')).arg(c.green(), 2, 16, QLatin1Char('0')).arg(c.blue(), 2, 16, QLatin1Char('0')));

        ui->statusLabel->show();
    }
    else
    {
        ui->statusLabel->hide();
    }
    update();
}

void WordStudyForm::updateStudyLabels()
{
    dueLabel1->setText(QString("%1").arg(QString::number(study->dueCount())));
    if (study->studySettings().method == WordStudyMethod::Gradual)
        dueLabel2->setText(QString("(%1)").arg(QString::number(study->newCount())));
    testedLabel1->setText(QString("%1").arg(QString::number(study->correctCount())));
    testedLabel2->setText(QString("%1").arg(QString::number(study->wrongCount())));

    bool newcard = study->nextNew();
    bool failedcard = !newcard && study->nextFailed();

    if (newcard || failedcard)
    {
        ui->statusLabel->setText(newcard ? tr("New") : tr("Retry"));
        QColor c = newcard ? newcolor : retrycolor;
        ui->statusLabel->setStyleSheet(QStringLiteral("color: %1").arg(c.name()));
        ui->statusLabel->show();
    }
    else
    {
        ui->statusLabel->hide();
    }
    update();
}

//-------------------------------------------------------------


