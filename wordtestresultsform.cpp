/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QImage>
#include <QDesktopWidget>
#include "wordtestresultsform.h"
#include "ui_wordtestresultsform.h"
#include "groupstudy.h"
#include "zui.h"
#include "wordstudyform.h"
#include "globalui.h"
#include "groups.h"

//-------------------------------------------------------------


WordStudyItemModel::WordStudyItemModel(WordStudy *study, QObject *parent) : base(parent), study(study)
{
    setColumn(0, { (int)DictColumnTypes::Default, Qt::AlignHCenter, ColumnAutoSize::NoAuto, false, -1, QString() });
    if (study->studySettings().method == WordStudyMethod::Single)
        insertColumn(1, { (int)WordStudyColumnTypes::Score, Qt::AlignHCenter, ColumnAutoSize::NoAuto, true, 40, tr("Score") });
}

WordStudyItemModel::~WordStudyItemModel()
{

}

Dictionary* WordStudyItemModel::dictionary() const
{
    return study->dictionary();
}

int WordStudyItemModel::indexes(int pos) const
{
    return study->testedIndex(pos);
}

int WordStudyItemModel::rowCount(const QModelIndex &parent) const
{
    return study->testedCount();
}

QVariant WordStudyItemModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == 0)
    {
        if (role == Qt::CheckStateRole)
        {
            if (study->testedCorrect(index.row()))
                return Qt::Checked;
            return Qt::Unchecked;
        }
    }

    if (role == Qt::DisplayRole && index.data((int)DictColumnRoles::Type).toInt() == (int)WordStudyColumnTypes::Score)
    {
        int row = index.row();
        const WordStudyItem &item = study->testedItems(row);
        int correct = item.correct + (study->testedCorrect(row) ? 1 : 0);
        int incorrect = item.incorrect + (!study->testedCorrect(row) ? 1 : 0);
        return QString("+%1/-%2 (%3%4)").arg(correct).arg(incorrect).arg(correct > incorrect ? "+" : "").arg(correct - incorrect);
    }

    return base::data(index, role);
}

bool WordStudyItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole && index.column() == 0)
    {
        study->setTestedCorrect(index.row(), value == Qt::Checked);
        emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), columnCount() - 1), { Qt::CheckStateRole });
        return true;
    }
    return base::setData(index, value, role);
}

Qt::ItemFlags WordStudyItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags r = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == 0)
        r |= Qt::ItemIsUserCheckable;

    return r;
}

//void WordStudyItemModel::entryAboutToBeRemoved(int windex)
//{
//    // The model is not used in windows that are shown while the dictionary can be
//    // manipulated.
//    ;
//}

void WordStudyItemModel::entryRemoved(int windex, int abcdeindex, int aiueoindex)
{
    // The model is not used in windows that are shown while the dictionary can be
    // manipulated.
    ;
}

void WordStudyItemModel::entryChanged(int windex, bool studydef)
{
    // The model is not used in windows that are shown while the dictionary can be
    // manipulated.
    ;
}

void WordStudyItemModel::entryAdded(int windex)
{
    // The model is not used in windows that are shown while the dictionary can be
    // manipulated.
    ;
}



//-------------------------------------------------------------


WordTestResultsForm::WordTestResultsForm(QWidget *parent) : base(parent), ui(new Ui::WordTestResultsForm), study(nullptr), nextround(false)
{
    ui->setupUi(this);
    ui->wordsTable->setStudyDefinitionUsed(true);
}

WordTestResultsForm::~WordTestResultsForm()
{
    delete ui;
}

void WordTestResultsForm::exec(WordStudy *s)
{
    setWindowTitle(tr("Word test results") + QString(" - %1").arg(s->getOwner()->name()));
    study = s;
    if (!study->finished())
    {
        ui->buttonBox->setStandardButtons(0);
        ui->buttonBox->addButton(tr("Continue"), QDialogButtonBox::AcceptRole);
        ui->buttonBox->addButton(tr("Suspend"), QDialogButtonBox::RejectRole);
    }

    setAttribute(Qt::WA_DontShowOnScreen, true);
    show();
    qApp->processEvents();
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    QRect r = frameGeometry();
    QRect sr = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    move(sr.left() + (sr.width() - r.width()) / 2, sr.top() + (sr.height() - r.height()) / 2);

    WordStudyItemModel *m = new WordStudyItemModel(s, this);
    ui->wordsTable->setModel(m);

    show();
}

void WordTestResultsForm::closeEvent(QCloseEvent *e)
{
    e->accept();

    if (!study->finished() && nextround)
    {
        WordStudyForm *f = new WordStudyForm;
        f->exec(study);
    }
    else if (study->studySettings().method == WordStudyMethod::Single)
        study->applyScore();

    gUI->showAppWindows();
    deleteLater();

    base::closeEvent(e);
}

void WordTestResultsForm::on_buttonBox_clicked(QAbstractButton *button)
{
    nextround = ui->buttonBox->buttonRole(button) != QDialogButtonBox::RejectRole;
    close();
}


//-------------------------------------------------------------

