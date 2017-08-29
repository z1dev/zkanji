/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QDesktopWidget>
#include <QCheckBox>
#include <QSignalMapper>
#include <QtEvents>
#include "kanapracticesettingsform.h"
#include "ui_kanapracticesettingsform.h"
#include "globalui.h"
#include "zui.h"
#include "kanareadingpracticeform.h"
#include "formstate.h"


//-------------------------------------------------------------


namespace FormStates
{
    KanarPracticeData kanapractice;
}


static const int KanaGroupSizes[] = {
    5, 1,
    5, 5, 5, 5,
    5, 5, 3, 5, 2,
    5, 5, 5, 5, 5,
    3, 3, 3, 3, 3,
    3,
    3, 3, 3, 3, 3
};

const QString kanaStrings[]
{
    "a", "i", "u", "e", "o",
    "n'",
    "ka", "ki", "ku", "ke", "ko", "sa", "shi", "su", "se", "so", "ta", "chi", "tsu", "te", "to", "na", "ni", "nu", "ne", "no",
    "ha", "hi", "fu", "he", "ho", "ma", "mi", "mu", "me", "mo", "ya", "yu", "yo", "ra", "ri", "ru", "re", "ro", "wa", "wo",

    "ga", "gi", "gu", "ge", "go", "za", "ji", "zu", "ze", "zo", "da", "di", "du", "de", "do", "ba", "bi", "bu", "be", "bo", "pa", "pi", "pu", "pe", "po",

    "kya", "kyu", "kyo", "sha", "shu", "sho", "cha", "chu", "cho", "nya", "nyu", "nyo", "hya", "hyu", "hyo",
    "rya", "ryu", "ryo",
    "gya", "gyu", "gyo", "ja", "ju", "jo", "dya", "dyu", "dyo", "bya", "byu", "byo", "pya", "pyu", "pyo"
};


KanaPracticeSettingsForm::KanaPracticeSettingsForm(QWidget *parent) : base(parent), ui(new Ui::KanaPracticeSettingsForm), updating(false), checkcnt(0)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags & (~Qt::WindowMinMaxButtonsHint) | Qt::MSWindowsFixedSizeDialogHint);

    connect(ui->closeButton, &QPushButton::clicked, this, &DialogWindow::closeCancel);

    updating = true;

    hirause.resize((int)KanaSounds::Count, 0);
    katause.resize((int)KanaSounds::Count, 0);

    setAttribute(Qt::WA_DontShowOnScreen);
    show();

    int maxw = checkBoxSize().width();

    ui->stack->setCurrentIndex(0);

    hiramap = new QSignalMapper(this);
    connect(hiramap, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, &KanaPracticeSettingsForm::boxToggled);

    setupBoxes(hiramap, ui->hiraLayout, hiraboxes);

    QGridLayout *g = ui->hiraLayout;
    QList<QLabel*> labels;
    for (int ix = 0, siz = g->count(); ix != siz; ++ix)
        if (dynamic_cast<QLabel*>(g->itemAt(ix)->widget()) != nullptr)
            labels.push_back(dynamic_cast<QLabel*>(g->itemAt(ix)->widget()));

    for (QLabel *l : labels)
    {
        l->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
        maxw = std::max(maxw, l->sizeHint().width());
        l->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        l->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        //l->setFrameStyle(QFrame::Panel | QFrame::Raised);
    }

    ui->stack->setCurrentIndex(1);

    katamap = new QSignalMapper(this);
    connect(katamap, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, &KanaPracticeSettingsForm::boxToggled);

    setupBoxes(katamap, ui->kataLayout, kataboxes);

    g = ui->kataLayout;
    labels.clear();
    for (int ix = 0, siz = g->count(); ix != siz; ++ix)
        if (dynamic_cast<QLabel*>(g->itemAt(ix)->widget()) != nullptr)
            labels.push_back(dynamic_cast<QLabel*>(g->itemAt(ix)->widget()));

    for (QLabel *l : labels)
    {
        l->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
        maxw = std::max(maxw, l->sizeHint().width());
        l->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        l->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        //l->setFrameStyle(QFrame::Panel | QFrame::Raised);
    }


    adjustSize();
    setFixedSize(size());
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    setAttribute(Qt::WA_DeleteOnClose, true);

    // The labels list only contains the kata labels currently, but we want to resize every
    // label to be the exact same width. Adding hiragana labels.
    g = ui->hiraLayout;
    for (int ix = 0, siz = g->count(); ix != siz; ++ix)
        if (dynamic_cast<QLabel*>(g->itemAt(ix)->widget()) != nullptr)
            labels.push_back(dynamic_cast<QLabel*>(g->itemAt(ix)->widget()));

    for (QLabel *l : labels)
        l->setMinimumWidth(maxw);

    ui->stack->setCurrentIndex(0);

    QSignalMapper *stackmap = new QSignalMapper(this);
    connect(stackmap, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, ui->stack, &QStackedWidget::setCurrentIndex);
    connect(ui->hiraButton, &QCheckBox::toggled, stackmap, (void (QSignalMapper::*)())&QSignalMapper::map);
    stackmap->setMapping(ui->hiraButton, 0);
    connect(ui->kataButton, &QCheckBox::toggled, stackmap, (void (QSignalMapper::*)())&QSignalMapper::map);
    stackmap->setMapping(ui->kataButton, 1);

    updating = false;
}

KanaPracticeSettingsForm::~KanaPracticeSettingsForm()
{
    delete ui;
}

void KanaPracticeSettingsForm::exec()
{
    QRect r = frameGeometry();
    QRect sr = qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());
    move(sr.left() + (sr.width() - r.width()) / 2, sr.top() + (sr.height() - r.height()) / 2);

    restoreState(FormStates::kanapractice);

    showModal();
}

void KanaPracticeSettingsForm::boxToggled(int index)
{
    if (updating)
        return;

    std::vector<uchar> &vec = sender() == hiramap ? hirause : katause;
    std::vector<QCheckBox*> &boxes = sender() == hiramap ? hiraboxes : kataboxes;

    if ((vec[index] == 1) != boxes[index]->isChecked())
    {
        checkcnt += boxes[index]->isChecked() ? 1 : -1;
        ui->test1Button->setEnabled(checkcnt >= 5);
        ui->test2Button->setEnabled(checkcnt >= 5);
    }

    vec[index] = boxes[index]->isChecked() ? 1 : 0;


    if ((qApp->keyboardModifiers() & Qt::ControlModifier) == 0)
        return;

    QCheckBox *ch = boxes[index];
    int pos = 0;
    int sum = 0;
    for (int ix = 0, siz = sizeof(KanaGroupSizes) / sizeof(int); ix != siz; ++ix)
    {
        int groupsiz = KanaGroupSizes[ix];
        if (index < sum + groupsiz)
        {
            for (int iy = 0; iy != groupsiz; ++iy)
            {
                if (sum + iy != index)
                    boxes[sum + iy]->setChecked(ch->isChecked());
            }
            break;
        }
        sum += groupsiz;
    }
}

void KanaPracticeSettingsForm::on_checkButton_clicked()
{
    std::vector<uchar> &vec = ui->hiraButton->isChecked() ? hirause : katause;
    std::vector<QCheckBox*> &boxes = ui->hiraButton->isChecked() ? hiraboxes : kataboxes;

    bool unchecked = false;
    for (int ix = 0, siz = vec.size(); !unchecked && ix != siz; ++ix)
        if (vec[ix] == 0)
            unchecked = true;

    for (int ix = 0, siz = boxes.size(); ix != siz; ++ix)
    {
        //vec[ix] = unchecked ? 1 : 0;
        boxes[ix]->setChecked(unchecked);
    }
}

void KanaPracticeSettingsForm::on_test1Button_clicked()
{
    saveState(FormStates::kanapractice);

    KanaReadingPracticeForm *form = new KanaReadingPracticeForm();
    form->exec();

    close();
}

void KanaPracticeSettingsForm::on_test2Button_clicked()
{
    saveState(FormStates::kanapractice);
    close();
}

void KanaPracticeSettingsForm::saveState(KanarPracticeData &data)
{
    data.hirause = hirause;
    data.katause = katause;
}

void KanaPracticeSettingsForm::restoreState(const KanarPracticeData &data)
{
    updating = true;
    hirause = data.hirause;
    katause = data.katause;
    hirause.resize((int)KanaSounds::Count, 0);
    katause.resize((int)KanaSounds::Count, 0);

    checkcnt = 0;

    for (int ix = 0, siz = hirause.size(); ix != siz; ++ix)
    {
        if (hirause[ix] == 1)
        {
            ++checkcnt;
            hiraboxes[ix]->setChecked(true);
        }
        else
            hiraboxes[ix]->setChecked(false);
    }
    for (int ix = 0, siz = katause.size(); ix != siz; ++ix)
    {
        if (katause[ix] == 1)
        {
            ++checkcnt;
            kataboxes[ix]->setChecked(true);
        }
        else
            kataboxes[ix]->setChecked(false);
    }

    ui->test1Button->setEnabled(checkcnt >= 5);
    ui->test2Button->setEnabled(checkcnt >= 5);

    updating = false;
}

void KanaPracticeSettingsForm::setupBoxes(QSignalMapper *map, QGridLayout *g, std::vector<QCheckBox*> &boxes)
{
    int maxw = checkBoxSize().width();

    for (int ix = 0; ix != (int)KanaSounds::Count; ++ix)
    {
        QFrame *w = new QFrame(this);
        w->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
        //w->setFrameStyle(QFrame::Panel | QFrame::Raised);
        QHBoxLayout *wl = new QHBoxLayout(w);
        w->setLayout(wl);
        w->setMinimumWidth(maxw);

        QCheckBox *ch = new QCheckBox(this);
        ch->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        ch->setMinimumWidth(maxw);
        ch->setMaximumWidth(maxw);
        ch->setToolTip(map == hiramap ? kanaStrings[ix] : kanaStrings[ix].toUpper());
        wl->addWidget(ch);
        wl->setAlignment(Qt::AlignCenter);
        wl->setContentsMargins(0, 0, 0, 0);

        boxes.push_back(ch);

        connect(ch, &QCheckBox::toggled, map, (void (QSignalMapper::*)())&QSignalMapper::map);
        map->setMapping(ch, ix);

        if (ix <= (int)KanaSounds::o)
            g->addWidget(w, 1 + ix, 1);
        else if (ix == (int)KanaSounds::n)
            g->addWidget(w, 7, 1);
        else if (ix <= (int)KanaSounds::mo)
            g->addWidget(w, 1 + ((ix - (int)KanaSounds::ka) % 5), 4 + (ix - (int)KanaSounds::ka) / 5);
        else if (ix <= (int)KanaSounds::yo)
            g->addWidget(w, 1 + (ix - (int)KanaSounds::ya) * 2, 10);
        else if (ix <= (int)KanaSounds::ro)
            g->addWidget(w, 1 + ((ix - (int)KanaSounds::ra) % 5), 11);
        else if (ix <= (int)KanaSounds::wo)
            g->addWidget(w, 1 + (ix - (int)KanaSounds::wa) * 4, 12);
        else if (ix <= (int)KanaSounds::po)
            g->addWidget(w, 1 + ((ix - (int)KanaSounds::ga) % 5), 14 + (ix - (int)KanaSounds::ga) / 5);
        else if (ix <= (int)KanaSounds::hyo)
            g->addWidget(w, 8 + ((ix - (int)KanaSounds::kya) % 3), 4 + (ix - (int)KanaSounds::kya) / 3);
        else if (ix <= (int)KanaSounds::ryo)
            g->addWidget(w, 8 + ((ix - (int)KanaSounds::rya) % 3), 11);
        else if (ix <= (int)KanaSounds::pyo)
            g->addWidget(w, 8 + ((ix - (int)KanaSounds::gya) % 3), 14 + (ix - (int)KanaSounds::gya) / 3);
    }
}



//-------------------------------------------------------------


void setupKanaPractice()
{
    KanaPracticeSettingsForm *form = new KanaPracticeSettingsForm(gUI->activeMainForm());
    form->exec();
}


//-------------------------------------------------------------
