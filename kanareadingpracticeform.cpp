/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QApplication>
#include <QDesktopWidget>
#include "kanareadingpracticeform.h"
#include "ui_kanareadingpracticeform.h"
#include "kanapracticesettingsform.h"
#include "globalui.h"
#include "zui.h"
#include "zkanjimain.h"
#include "dialogs.h"
#include "fontsettings.h"
#include "romajizer.h"
#include "formstate.h"


//-------------------------------------------------------------


KanaReadingPracticeForm::KanaReadingPracticeForm(QWidget *parent) : base(parent), ui(new Ui::KanaReadingPracticeForm)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

    QFont f = ui->mainLabel->font();
    f.setFamily(Settings::fonts.kana);
    ui->mainLabel->setFont(f);

    f = ui->k1Label->font();
    f.setFamily(Settings::fonts.kana);
    ui->k1Label->setFont(f);
    f = ui->k2Label->font();
    f.setFamily(Settings::fonts.kana);
    ui->k2Label->setFont(f);
    f = ui->k3Label->font();
    f.setFamily(Settings::fonts.kana);
    ui->k3Label->setFont(f);
    f = ui->k4Label->font();
    f.setFamily(Settings::fonts.kana);
    ui->k4Label->setFont(f);

    f = ui->r1Label->font();
    f.setFamily(Settings::fonts.main);
    ui->r1Label->setFont(f);
    f = ui->r2Label->font();
    f.setFamily(Settings::fonts.main);
    ui->r2Label->setFont(f);

    f = ui->text1Label->font();
    f.setFamily(Settings::fonts.main);
    ui->text1Label->setFont(f);
    f = ui->text2Label->font();
    f.setFamily(Settings::fonts.main);
    ui->text2Label->setFont(f);
    f = ui->text3Label->font();
    f.setFamily(Settings::fonts.main);
    ui->text3Label->setFont(f);
    f = ui->text4Label->font();
    f.setFamily(Settings::fonts.main);
    ui->text4Label->setFont(f);

    restrictWidgetSize(ui->mainLabel, 3.5, AdjustedValue::Min);
    int rk1 = std::max(restrictedWidgetSize(ui->k1Label, 3.5), restrictedWidgetSize(ui->r1Label, 3.5));
    int rk2 = std::max(restrictedWidgetSize(ui->k2Label, 3.5), restrictedWidgetSize(ui->r2Label, 3.5));
    ui->k1Label->setFixedWidth(rk1);
    ui->r1Label->setFixedWidth(rk1);
    ui->k3Label->setFixedWidth(rk1);

    ui->k2Label->setFixedWidth(rk2);
    ui->r2Label->setFixedWidth(rk2);
    ui->k4Label->setFixedWidth(rk2);

    restrictWidgetSize(ui->text1Label, 1.1);
    restrictWidgetSize(ui->text2Label, 1.1);
    restrictWidgetSize(ui->text3Label, 1.1);
    restrictWidgetSize(ui->text4Label, 1.1);

    connect(ui->abortButton, &QPushButton::clicked, this, &DialogWindow::closeAbort);
    

    statusBar()->addWidget(createStatusWidget(ui->status, -1, nullptr, tr("Remaining:"), 0, dueLabel = new QLabel(this), "0", 4));
    dueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    statusBar()->addWidget(createStatusWidget(ui->status, -1, nullptr, tr("Mistakes:"), 0, wrongLabel = new QLabel(this), "0", 4));
    wrongLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->frame->setStyleSheet(QString("background: %1").arg(qApp->palette().color(QPalette::Active, QPalette::Base).name()));

    setAttribute(Qt::WA_DontShowOnScreen);
    show();
    setFixedSize(size());
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);
}
 

KanaReadingPracticeForm::~KanaReadingPracticeForm()
{
    delete ui;
}

void KanaReadingPracticeForm::exec()
{
    reset();

    QRect r = frameGeometry();
    QRect sr = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    move(sr.left() + (sr.width() - r.width()) / 2, sr.top() + (sr.height() - r.height()) / 2);

    gUI->hideAppWindows();
    show();
}

void KanaReadingPracticeForm::on_restartButton_clicked()
{
    reset();
}

bool KanaReadingPracticeForm::event(QEvent *e)
{
    if (e->type() == QEvent::Timer && ((QTimerEvent*)e)->timerId() == timer.timerId() && ui->timeLabel->isVisibleTo(this))
    {

        QDateTime now = QDateTime::currentDateTimeUtc();
        qint64 passed = starttime.secsTo(now);
        if (passed >= 60 * 60)
            stopTimer(true);
        else
            ui->timeLabel->setText(DateTimeFunctions::formatPassedTime(passed, false));
    }

    return base::event(e);
}

void KanaReadingPracticeForm::closeEvent(QCloseEvent *e)
{
    base::closeEvent(e);

    gUI->showAppWindows();
    setupKanaPractice();
}

void KanaReadingPracticeForm::keyPressEvent(QKeyEvent *e)
{
    base::keyPressEvent(e);
    bool backspace = e->key() == Qt::Key_Backspace;
    if (pos == -1 || pos >= list.size() || (e->modifiers() != 0 && e->modifiers() != Qt::ShiftModifier) || (e->text().size() != 1 && !backspace))
        return;
    QChar txt = e->text().isEmpty() ? QChar(0) : e->text().at(0);
    if (!backspace && (txt < QChar('a') || txt > QChar('z')) && (txt < QChar('A') || txt > QChar('Z')))
        return;

    // Updating the entered string and checking for the correct answer.

    QLabel *labels[]{ ui->text1Label, ui->text2Label, ui->text3Label, ui->text4Label };

    if (backspace)
    {
        if (entered.isEmpty())
            return;
        entered.resize(entered.size() - 1);
        labels[entered.size()]->setText("_");

        return;
    }

    entered += e->text();
    QLabel *lb = labels[entered.size() - 1];

    lb->setText(e->text());

    int strpos = list[pos];
    bool kata = false;
    if (strpos >= (int)KanaSounds::Count)
    {
        strpos -= (int)KanaSounds::Count;
        kata = true;
    }

    QString current = toKana(kanaStrings[strpos]);
    if ((entered == "n" || entered == "N") && current == QChar(0x3093))
    {
        answered(true);
        return;
    }

    JapaneseValidator &v = kanaValidator();
    QString kana = entered;
    int kanapos = entered.size();
    QValidator::State state = v.validate(kana, kanapos);

    if (state == QValidator::Intermediate)
    {
        // Too many characters already. Adding another won't fix this.
        if (entered.size() == 4)
            answered(false);
        return;
    }

    for (int ix = kana.size() - 1; ix != -1; --ix)
    {
        if ((kana.at(ix) >= 'a' && kana.at(ix) <= 'z') || (kana.at(ix) >= 'A' && kana.at(ix) <= 'Z'))
            kana.remove(ix, 1);
    }

    if (hiraganize(current) == hiraganize(kana))
        answered(true);
    else
        answered(false);
}

void KanaReadingPracticeForm::reset()
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
        int numpos = rnd(0, nums.size() - 1);
        if (!list.empty() && nums[numpos] == list.back())
            continue;
        list.push_back(nums[numpos]);
        //nums.erase(nums.begin() + numpos);
    }

    pos = -1;
    mistakes = 0;
    starttime = QDateTime::currentDateTimeUtc();

    startTimer();

    next();
}

void KanaReadingPracticeForm::next()
{
    ++pos;
    if (pos == list.size())
    {
        stopTimer(false);
        return;
    }

    dueLabel->setText(QString::number(std::max(0, (int)list.size() - pos)));
    wrongLabel->setText(QString::number(mistakes));

    retries = 0;

    entered = QString();
    ui->text1Label->setText("_");
    ui->text2Label->setText("_");
    ui->text3Label->setText("_");
    ui->text4Label->setText("_");

    setLabelText(pos, ui->mainLabel);
    setLabelText(pos - 1, ui->k1Label, ui->r1Label);
    setLabelText(pos - 2, ui->k2Label, ui->r2Label);
    setLabelText(pos + 1, ui->k3Label);
    setLabelText(pos + 2, ui->k4Label);
}

void KanaReadingPracticeForm::setLabelText(int p, QLabel *lb, QLabel *rlb)
{
    if (p < 0 || p >= list.size())
    {
        lb->setText("");
        if (rlb != nullptr)
            rlb->setText("");
        return;
    }

    int strpos = list[p];
    bool kata = false;
    if (strpos >= (int)KanaSounds::Count)
    {
        strpos -= (int)KanaSounds::Count;
        kata = true;
    }
    QString str = toKana(kanaStrings[strpos]);
    lb->setText(kata ? toKatakana(str) : str);

    if (rlb != nullptr)
        rlb->setText(kata ? kanaStrings[strpos].toUpper() : kanaStrings[strpos]);
}

void KanaReadingPracticeForm::answered(bool correct)
{
    if (correct == false)
    {
        ++retries;
        if (retries == 3)
        {
            // Too many mistakes made in the same syllable.

            ++mistakes;
            if (mistakes == 3)
                stopTimer(true);
            next();
        }
        else
        {
            // Just a mistake, start over entering the same text.

            entered = QString();
            ui->text1Label->setText("_");
            ui->text2Label->setText("_");
            ui->text3Label->setText("_");
            ui->text4Label->setText("_");
        }
        return;
    }

    next();
}

void KanaReadingPracticeForm::stopTimer(bool hide)
{
    timer.stop();
    if (hide)
    {
        ui->timeCaptionLabel->hide();
        ui->timeLabel->hide();
    }
}

void KanaReadingPracticeForm::startTimer()
{
    timer.start(1000, this);
    starttime = QDateTime::currentDateTimeUtc();
    ui->timeLabel->setText(DateTimeFunctions::formatPassedTime(0, false));
    ui->timeCaptionLabel->show();
    ui->timeLabel->show();
}


//-------------------------------------------------------------
