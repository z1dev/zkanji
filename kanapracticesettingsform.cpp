/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QCheckBox>
#include <QSignalMapper>
#include <QtEvents>
#include "kanapracticesettingsform.h"
#include "ui_kanapracticesettingsform.h"
#include "globalui.h"
#include "zui.h"


//-------------------------------------------------------------


enum class KanaSounds
{
    a, i, u, e, o,
    n, 
    ka, ki, ku, ke, ko,  sa, si, su, se, so,  ta, ti, tu, te, to,  na, ni, nu, ne, no,
    ha, hi, hu, he, ho,  ma, mi, my, me, mo,  ya, yu, yo,  ra, ri, ru, re, ro,  wa, wo,

    ga, gi, gu, ge, go,  za, zi, zu, ze, zo,  da, di, du, de, ddo,  ba, bi, bu, be, bo,  pa, pi, pu, pe, po,

    kya, kyu, kyo,  sha, shu, sho,  cha, chu, cho, nya, nyu, nyo, hya, hyu, hyo,
    rya, ryu, ryo,
    gya, gyu, gyo,  ja, ju, jo,  dya, dyu, dyo,  bya, byu, byo,  pya, pyu, pyo ,

    count
};

static const int KanaGroupSizes[] = {
    5, 1,
    5, 5, 5, 5,
    5, 5, 3, 5, 2,
    5, 5, 5, 5, 5,
    3, 3, 3, 3, 3,
    3,
    3, 3, 3, 3, 3
};

KanaPracticeSettingsForm::KanaPracticeSettingsForm(QWidget *parent) : base(parent), ui(new Ui::KanaPracticeSettingsForm), updating(false)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags & (~Qt::WindowMinMaxButtonsHint) | Qt::MSWindowsFixedSizeDialogHint);

    connect(ui->closeButton, &QPushButton::clicked, this, &DialogWindow::closeCancel);

    QSignalMapper *map = new QSignalMapper(this);
    connect(map, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, &KanaPracticeSettingsForm::boxToggled);

    QGridLayout *g = ui->gridLayout;
    int maxw = checkBoxSize().width();

    for (int ix = 0; ix != (int)KanaSounds::count; ++ix)
    {
        hirause.push_back(0);
        katause.push_back(0);

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
            g->addWidget(w, 1 + ((ix - (int)KanaSounds::ka) % 5), 4 + (ix - (int)KanaSounds::ka ) / 5);
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
    setAttribute(Qt::WA_DontShowOnScreen);
    show();

    QList<QLabel*> labels;
    for (int ix = 0, siz = g->count(); ix != siz; ++ix)
        if (dynamic_cast<QLabel*>(g->itemAt(ix)->widget()) != nullptr)
            labels.push_back(dynamic_cast<QLabel*>(g->itemAt(ix)->widget()));

    for (QLabel *l : labels)
    {
        l->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        maxw = std::max(maxw, l->width());
        //l->setFrameStyle(QFrame::Panel | QFrame::Raised);
    }
    for (QLabel *l : labels)
        l->setMinimumWidth(maxw);

    adjustSize();
    setFixedSize(size());
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    setAttribute(Qt::WA_DeleteOnClose, true);
}

KanaPracticeSettingsForm::~KanaPracticeSettingsForm()
{
    delete ui;
}

void KanaPracticeSettingsForm::exec()
{
    showModal();
}

void KanaPracticeSettingsForm::boxToggled(int index)
{
    if (updating)
        return;

    std::vector<uchar> &vec = ui->kanaCBox->currentIndex() == 0 ? hirause : katause;
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

void KanaPracticeSettingsForm::on_kanaCBox_currentIndexChanged(int ind)
{
    updating = true;

    std::vector<uchar> &vec = ind == 0 ? hirause : katause;

    for (int ix = 0; ix != (int)KanaSounds::count; ++ix)
        boxes[ix]->setChecked(vec[ix] == 1);

    QGridLayout *g = ui->gridLayout;
    QList<QLabel*> labels;
    for (int ix = 0, siz = g->count(); ix != siz; ++ix)
        if (dynamic_cast<QLabel*>(g->itemAt(ix)->widget()) != nullptr)
            labels.push_back(dynamic_cast<QLabel*>(g->itemAt(ix)->widget()));
    for (QLabel *l : labels)
        l->setText(ind == 0 ? l->text().toLower() : l->text().toUpper());

    updating = false;
}



//-------------------------------------------------------------


void setupKanaPractice()
{
    KanaPracticeSettingsForm *form = new KanaPracticeSettingsForm(gUI->activeMainForm());
    form->exec();
}


//-------------------------------------------------------------
