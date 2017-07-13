#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include "examplewidget.h"
#include "ui_examplewidget.h"
#include "zevents.h"
#include "globalui.h"

//-------------------------------------------------------------


ExampleWidget::ExampleWidget(QWidget *parent) : base(parent), ui(new Ui::ExampleWidget), locked(false)
{
    ui->setupUi(this);

    QFontMetrics fm(ui->cntLabel->font(), this);
    int w = fm.averageCharWidth() * 8; // boundingRect(QStringLiteral("9999")
    int h = fm.height();
    ui->indexEdit->setMaximumSize(QSize(w + 15, h + 6));
    ui->indexEdit->setMinimumSize(ui->indexEdit->maximumSize());
    ui->prevButton->setMaximumSize(QSize(h + 3, h + 6));
    ui->nextButton->setMaximumSize(QSize(h + 3, h + 6));
    //ui->jptrButton->setMaximumHeight(h + ui->indexEdit->maximumHeight() + ui->indexWidget->layout()->spacing() + 6);
    //ui->jptrButton->setMinimumHeight(ui->jptrButton->minimumHeight());
    //ui->jpButton->setMaximumHeight((ui->jptrButton->maximumHeight() - ui->gridLayout->verticalSpacing()) / 2);
    //ui->trButton->setMaximumHeight(ui->jpButton->maximumHeight());
    //int btnh = h * 2 /*(ui->prevButton->maximumHeight() + h)*/ + 6;
    //ui->jptrButton->setMinimumHeight(btnh);
    //ui->jpButton->setMinimumHeight(btnh);
    //ui->trButton->setMinimumHeight(btnh);
    adjustSize();

    ui->cntLabel->setText(QStringLiteral("-"));
    ui->indexEdit->setText(QStringLiteral("1"));

    ui->prevButton->setAutoRepeatInterval(40);
    ui->nextButton->setAutoRepeatInterval(40);

    connect(gUI, &GlobalUI::sentencesReset, this, &ExampleWidget::onReset);
    connect(ui->strip, &ZExampleStrip::sentenceChanged, this, &ExampleWidget::stripChanged);
    connect(ui->strip, &ZExampleStrip::wordSelected, this, &ExampleWidget::wordSelected);
    //connect(ui->jptrButton, &QToolButton::clicked, this, &ExampleWidget::jptrToggled);
    //connect(ui->jpButton, &QToolButton::clicked, this, &ExampleWidget::on_jptrButton_toggled);
    //connect(ui->trButton, &QToolButton::clicked, this, &ExampleWidget::on_jptrButton_toggled);
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

void ExampleWidget::unlock(Dictionary *d, int windex, int wordpos, int wordform)
{
    locked = false;
    if (d == nullptr)
        return;

    ui->strip->setItem(d, windex, wordpos, wordform);
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
    unlock();
}

void ExampleWidget::on_jptrButton_clicked(bool checked)
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


//-------------------------------------------------------------
