/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include "examplewidget.h"
#include "ui_examplewidget.h"
#include "zevents.h"
#include "globalui.h"
#include "words.h"

//-------------------------------------------------------------


ExampleWidget::ExampleWidget(QWidget *parent) : base(parent), ui(new Ui::ExampleWidget), locked(false), repeatbutton(nullptr)
{
    ui->setupUi(this);

    QFontMetrics fm(ui->cntLabel->font(), this);
    int w = fm.averageCharWidth(); // boundingRect(QStringLiteral("9999")
    //int h = fm.height();

    //ui->indexEdit->setMaximumSize(QSize(w + 15, h + 6));
    //ui->indexEdit->setMinimumSize(ui->indexEdit->maximumSize());

    ui->indexEdit->setMaximumWidth(w * 5 + 15);
    ui->indexEdit->setMinimumWidth(w * 5 + 15);
    ui->cntLabel->setMaximumWidth(w * 8 + 15);
    ui->cntLabel->setMinimumWidth(w * 8 + 15);

    ui->prevButton->setMinimumHeight(ui->indexEdit->sizeHint().height());
    ui->prevButton->setMaximumHeight(ui->indexEdit->sizeHint().height());
    ui->nextButton->setMinimumHeight(ui->indexEdit->sizeHint().height());
    ui->nextButton->setMaximumHeight(ui->indexEdit->sizeHint().height());

    ui->prevButton->setMaximumWidth(ui->prevButton->sizeHint().height());
    ui->nextButton->setMaximumWidth(ui->prevButton->sizeHint().height());

    //ui->jptrButton->setFixedSize(QSize(ui->indexEdit->sizeHint().height(), ui->indexEdit->sizeHint().height()));
    //ui->linkButton->setFixedSize(QSize(ui->indexEdit->sizeHint().height(), ui->indexEdit->sizeHint().height()));

    //ui->prevButton->setMaximumSize(QSize(h + 3, h + 6));
    //ui->nextButton->setMaximumSize(QSize(h + 3, h + 6));
    adjustSize();

    ui->cntLabel->setText(QStringLiteral("-"));
    ui->indexEdit->setText(QStringLiteral("1"));

    ui->prevButton->setAutoRepeatInterval(40);
    ui->nextButton->setAutoRepeatInterval(40);

    connect(gUI, &GlobalUI::sentencesReset, this, &ExampleWidget::onReset);
    connect(ui->strip, &ZExampleStrip::sentenceChanged, this, &ExampleWidget::stripChanged);
    connect(ui->strip, &ZExampleStrip::wordSelected, this, &ExampleWidget::wordSelected);

    ui->prevButton->installEventFilter(this);
    ui->nextButton->installEventFilter(this);
}

ExampleWidget::~ExampleWidget()
{
    delete ui;
}

//void ExampleWidget::saveXMLSettings(QXmlStreamWriter &writer) const
//{
//    switch (ui->strip->displayed())
//    {
//    case ExampleDisplay::Both:
//        writer.writeAttribute("mode", "both");
//        break;
//    case ExampleDisplay::Japanese:
//        writer.writeAttribute("mode", "from");
//        break;
//    case ExampleDisplay::Translated:
//        writer.writeAttribute("mode", "to");
//        break;
//    }
//}
//
//void ExampleWidget::loadXMLSettings(QXmlStreamReader &reader)
//{
//    QStringRef ref = reader.attributes().value("mode");
//    if (ref == "both")
//        ui->strip->setDisplayed(ExampleDisplay::Both);
//    else if (ref == "from")
//        ui->strip->setDisplayed(ExampleDisplay::Japanese);
//    else if (ref == "to")
//        ui->strip->setDisplayed(ExampleDisplay::Translated);
//    reader.skipCurrentElement();
//}

void ExampleWidget::setItem(Dictionary *d, int windex)
{
    if (locked)
        return;
    ui->strip->setItem(d, windex);
}

void ExampleWidget::lock()
{
    locked = true;
}

ExampleDisplay ExampleWidget::displayed() const
{
    return ui->strip->displayed();
}

void ExampleWidget::setDisplayed(ExampleDisplay disp)
{
    ui->strip->setDisplayed(disp);
}

bool ExampleWidget::hasInteraction() const
{
    return ui->strip->hasInteraction();
}

void ExampleWidget::setInteraction(bool allow)
{
    ui->strip->setInteraction(allow);
}

void ExampleWidget::unlock(Dictionary *d, int windex, int wordpos, int wordform)
{
    locked = false;
    if (d == nullptr)
        return;

    ui->strip->setItem(d, windex, wordpos, wordform);
}

bool ExampleWidget::eventFilter(QObject *o, QEvent *e)
{
    if ((o == ui->prevButton || o == ui->nextButton ) && (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease || e->type() == QEvent::MouseButtonDblClick))
    {
        QMouseEvent *me = (QMouseEvent*)e;
        if (me->button() == Qt::RightButton)
        {
            if (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick)
            {
                repeatbutton = (QToolButton*)o;
                if (repeatbutton == ui->prevButton)
                    ui->strip->showPreviousLinkedSentence();
                else
                    ui->strip->showNextLinkedSentence();

                repeattimer.start(repeatbutton->autoRepeatDelay(), this);
            }
            else
            {
                repeattimer.stop();
                repeatbutton = nullptr;
            }
            return true;
        }
    }

    if ((o == ui->prevButton || o == ui->nextButton) && e->type() == QEvent::MouseMove && repeatbutton != nullptr && !repeattimer.isActive() && repeatbutton->rect().contains(repeatbutton->mapFromGlobal(QCursor::pos())))
    {
        repeattimer.start(repeatbutton->autoRepeatInterval(), this);
        QTimerEvent te(repeattimer.timerId());
        //qApp->sendEvent(this, &te);
        event(&te);
    }


    return base::eventFilter(o, e);
}

bool ExampleWidget::event(QEvent *e)
{
    if (e->type() == QEvent::Timer)
    {
        QTimerEvent *te = (QTimerEvent*)e;
        if (te->timerId() == repeattimer.timerId())
        {
            repeattimer.stop();
            if (repeatbutton != nullptr && repeatbutton->rect().contains(repeatbutton->mapFromGlobal(QCursor::pos())))
            {
                if (repeatbutton == ui->prevButton)
                    ui->strip->showPreviousLinkedSentence();
                else
                    ui->strip->showNextLinkedSentence();
                repeattimer.start(repeatbutton->autoRepeatInterval(), this);
            }
        }
    }
    else if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    return base::event(e);
}

void ExampleWidget::onReset()
{
    ui->strip->setItem(nullptr, -1);
}

void ExampleWidget::stripChanged()
{
    if (locked)
        return;

    lock();
    if (ui->strip->sentenceCount() == 0)
        ui->cntLabel->setText("-");
    else
        ui->cntLabel->setText(QString("1 - %1").arg(ui->strip->sentenceCount()));
    ui->indexEdit->setText(QString::number(ui->strip->currentSentence() + 1));
    ui->indexEdit->setEnabled(ui->strip->sentenceCount() != 0);
    ui->prevButton->setEnabled(ui->strip->currentSentence() > 0);
    ui->nextButton->setEnabled(ui->strip->currentSentence() < ui->strip->sentenceCount() - 1);
    ui->linkButton->setEnabled(ui->strip->sentenceCount() != 0);

    const WordCommonsExample *ex = ui->strip->currentExample();
    ui->linkButton->setChecked(ex != nullptr && ui->strip->sentenceCount() != 0 && ZKanji::wordexamples.isExample(ui->strip->dictionary()->wordEntry(ui->strip->wordIndex())->kanji.data(), ui->strip->dictionary()->wordEntry(ui->strip->wordIndex())->kana.data(), ex->block * 100 + ex->line));
    unlock();
}

void ExampleWidget::on_jptrButton_clicked(bool /*checked*/)
{
    //QToolButton *btn = (QToolButton*)sender();
    //if (!btn->isChecked())
    //    return;
    //if (btn == ui->jptrButton)
    //    ui->strip->setDisplayed(ExampleDisplay::Both);
    //else if (btn == ui->jpButton)
    //    ui->strip->setDisplayed(ExampleDisplay::Japanese);
    //else if (btn == ui->trButton)
    //    ui->strip->setDisplayed(ExampleDisplay::Translated);

    switch (ui->strip->displayed())
    {
    case ExampleDisplay::Both:
        ui->strip->setDisplayed(ExampleDisplay::Japanese);
        break;
    case ExampleDisplay::Japanese:
        ui->strip->setDisplayed(ExampleDisplay::Translated);
        break;
    case ExampleDisplay::Translated:
        ui->strip->setDisplayed(ExampleDisplay::Both);
        break;
    }
}

void ExampleWidget::on_prevButton_clicked()
{
    ui->strip->setCurrentSentence(ui->indexEdit->text().toInt() - 2);
}

void ExampleWidget::on_nextButton_clicked()
{
    ui->strip->setCurrentSentence(ui->indexEdit->text().toInt());
}

void ExampleWidget::on_indexEdit_textEdited(const QString &text)
{
    bool ok = false;
    int ix = text.toInt(&ok);
    if (ok)
        ui->strip->setCurrentSentence(ix - 1);
}

void ExampleWidget::on_linkButton_clicked(bool checked)
{
    const WordCommonsExample *ex = ui->strip->currentExample();
    if (ex == nullptr)
        return;
    ZKanji::wordexamples.linkExample(ui->strip->dictionary()->wordEntry(ui->strip->wordIndex())->kanji.data(), ui->strip->dictionary()->wordEntry(ui->strip->wordIndex())->kana.data(), ex->block * 100 + ex->line, checked);
}


//-------------------------------------------------------------
