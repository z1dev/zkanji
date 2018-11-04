/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "kanareadingpracticeform.h"
#include "ui_kanareadingpracticeform.h"
#include "kanapracticesettingsform.h"
#include "globalui.h"
#include "zui.h"
#include "zkanjimain.h"
#include "dialogs.h"
#include "fontsettings.h"
#include "romajizer.h"
#include "formstates.h"
#include "colorsettings.h"

#include "checked_cast.h"

//-------------------------------------------------------------


KanaReadingPracticeForm::KanaReadingPracticeForm(QWidget *parent) : base(parent, false), ui(new Ui::KanaReadingPracticeForm)
{
    ui->setupUi(this);

    gUI->scaleWidget(this);

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

    restrictWidgetSize(ui->mainLabel, 3.2, AdjustedValue::Min);
    int rk1 = std::max(restrictedWidgetSize(ui->k1Label, 3.2), restrictedWidgetSize(ui->r1Label, 3.2));
    int rk2 = std::max(restrictedWidgetSize(ui->k2Label, 3.2), restrictedWidgetSize(ui->r2Label, 3.2));
    ui->k1Label->setFixedWidth(rk1);
    ui->r1Label->setFixedWidth(rk1);
    ui->k3Label->setFixedWidth(rk1);

    ui->k2Label->setFixedWidth(rk2);
    ui->r2Label->setFixedWidth(rk2);
    ui->k4Label->setFixedWidth(rk2);

    restrictWidgetWiderSize(ui->text1Label, 1.05);
    restrictWidgetWiderSize(ui->text2Label, 1.05);
    restrictWidgetWiderSize(ui->text3Label, 1.05);
    restrictWidgetWiderSize(ui->text4Label, 1.05);

    ui->status->add(tr("Time:"), 0, "99:99", 6, true);
    //ui->status->addWidget(createStatusWidget(ui->status, -1, nullptr, tr("Time:"), 0, timeLabel = new QLabel(this), "99:99", 6));
    //timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->status->add(tr("Remaining:"), 0, "0", 4, true);
    //ui->status->addWidget(createStatusWidget(ui->status, -1, nullptr, tr("Remaining:"), 0, dueLabel = new QLabel(this), "0", 4));
    //dueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->status->add(tr("Mistakes:"), 0, "0", 4, true);
    //ui->status->addWidget(createStatusWidget(ui->status, -1, nullptr, tr("Mistakes:"), 0, wrongLabel = new QLabel(this), "0", 4));
    //wrongLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->frame->setStyleSheet(QString("background: %1").arg(qApp->palette().color(QPalette::Active, QPalette::Base).name()));

    connect(ui->abortButton, &QPushButton::clicked, this, &DialogWindow::closeAbort);

    fixWrapLabelsHeight(this, -1);
    updateWindowGeometry(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->resultLabel->setText(QString());
}
 

KanaReadingPracticeForm::~KanaReadingPracticeForm()
{
    stopTimer(false);
    delete ui;

    QTimer::singleShot(0, []() {
        gUI->showAppWindows();
        setupKanaPractice();
    });
}

void KanaReadingPracticeForm::exec()
{
    reset();

    //QRect r = frameGeometry();
    //QRect sr = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    //move(sr.left() + (sr.width() - r.width()) / 2, sr.top() + (sr.height() - r.height()) / 2);

    gUI->hideAppWindows();
    show();
}

void KanaReadingPracticeForm::on_restartButton_clicked()
{
    reset();
}

bool KanaReadingPracticeForm::event(QEvent *e)
{
    if (e->type() == QEvent::Timer && ((QTimerEvent*)e)->timerId() == timer.timerId() && ui->status->value(0) != "-")
    {

        QDateTime now = QDateTime::currentDateTimeUtc();
        qint64 passed = starttime.secsTo(now);
        if (passed >= 60 * 60)
            stopTimer(true);
        else
            ui->status->setValue(0, DateTimeFunctions::formatPassedTime(passed, false));
    }

    return base::event(e);
}

void KanaReadingPracticeForm::closeEvent(QCloseEvent *e)
{
    stopTimer(false);
    base::closeEvent(e);
}

void KanaReadingPracticeForm::keyPressEvent(QKeyEvent *e)
{
    base::keyPressEvent(e);
    bool backspace = e->key() == Qt::Key_Backspace;
    if (pos == -1 || pos >= tosigned(list.size()) || (e->modifiers() != 0 && e->modifiers() != Qt::ShiftModifier) || (e->text().size() != 1 && !backspace))
        return;
    QChar txt = e->text().isEmpty() ? QChar(0) : e->text().at(0);
    if (!backspace && (txt < QChar('a') || txt > QChar('z')) && (txt < QChar('A') || txt > QChar('Z')))
        return;

    // Updating the entered string and checking for the correct answer.

    if (backspace)
    {
        if (!entered.isEmpty())
            entered.resize(entered.size() - 1);
        setTextLabels();

        return;
    }

    entered += e->text();
    setTextLabels();

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

void KanaReadingPracticeForm::showEvent(QShowEvent *e)
{
    setFixedSize(size());
    base::showEvent(e);
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

    next();
}

void KanaReadingPracticeForm::next()
{
    ++pos;
    if (pos == tosigned(list.size()))
    {
        stopTimer(false);

        ui->resultLabel->setStyleSheet(QString());
        ui->status->setValue(1, "0");
        ui->status->setValue(2, QString::number(mistakes));
        //dueLabel->setText(0);
        //wrongLabel->setText(QString::number(mistakes));

        if (retries == 2)
            ui->r1Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
        else
            ui->r1Label->setStyleSheet(QString());
        setLabelText(pos, ui->mainLabel);
        setLabelText(pos - 1, ui->k1Label, ui->r1Label);
        setLabelText(pos - 2, ui->k2Label, ui->r2Label);
        setLabelText(pos + 1, ui->k3Label);
        setLabelText(pos + 2, ui->k4Label);

        entered = QString();
        setTextLabels();

        ui->resultLabel->setGraphicsEffect(nullptr);
        ui->resultLabel->setText(tr("Finished"));

        return;
    }

    ui->status->setValue(1, QString::number(std::max(0, (int)list.size() - pos)));
    ui->status->setValue(2, QString::number(mistakes));
    //dueLabel->setText(QString::number(std::max(0, (int)list.size() - pos)));
    //wrongLabel->setText(QString::number(mistakes));

    entered = QString();
    setTextLabels();
    
    if (retries == 2)
        ui->r1Label->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
    else
        ui->r1Label->setStyleSheet(QString());

    retries = 0;

    setLabelText(pos, ui->mainLabel);
    setLabelText(pos - 1, ui->k1Label, ui->r1Label);
    setLabelText(pos - 2, ui->k2Label, ui->r2Label);
    setLabelText(pos + 1, ui->k3Label);
    setLabelText(pos + 2, ui->k4Label);
}

void KanaReadingPracticeForm::setLabelText(int p, QLabel *lb, QLabel *rlb)
{
    if (p < 0 || p >= tosigned(list.size()))
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
        if (retries == 2)
        {
            // Too many mistakes made in the same syllable.
            ui->resultLabel->setText(tr("Mistake"));
            ui->resultLabel->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
            hideLabelAnimation();

            ++mistakes;
            if (mistakes == 3)
                stopTimer(true);
            next();
        }
        else
        {
            // Just a mistake, start over entering the same text.
            ui->resultLabel->setText(tr("Try again"));
            ui->resultLabel->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyWrong).name()));
            hideLabelAnimation();

            entered = QString();
            setTextLabels();


            //ui->text1Label->setText("_");
            //ui->text2Label->setText("_");
            //ui->text3Label->setText("_");
            //ui->text4Label->setText("_");
        }
        return;
    }


    ui->resultLabel->setText(tr("Correct"));
    ui->resultLabel->setStyleSheet(QString("color: %1").arg(Settings::uiColor(ColorSettings::StudyCorrect).name()));
    hideLabelAnimation();

    next();
}

void KanaReadingPracticeForm::stopTimer(bool hide)
{
    timer.stop();
    if (hide)
        ui->status->setValue(0, "-");
}

void KanaReadingPracticeForm::startTimer()
{
    timer.start(1000, this);
    starttime = QDateTime::currentDateTimeUtc();
    ui->status->setValue(0, DateTimeFunctions::formatPassedTime(0, false));
}

void KanaReadingPracticeForm::setTextLabels()
{
    QLabel *labels[]{ ui->text1Label, ui->text2Label, ui->text3Label, ui->text4Label };

    for (int ix = 0; ix != 4; ++ix)
    {
        if (entered.size() > ix)
            labels[ix]->setText(entered.at(ix));
        else
            labels[ix]->setText("_");
    }
}

void KanaReadingPracticeForm::hideLabelAnimation()
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
