/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
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
#include "formstates.h"
#include "kanareadingpracticeform.h"
#include "kanawritingpracticeform.h"


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


KanaPracticeSettingsForm::KanaPracticeSettingsForm(QWidget *parent) : base(parent, false), ui(new Ui::KanaPracticeSettingsForm), updating(false), hiracheckcnt(0), katacheckcnt(0)
{
    ui->setupUi(this);

    gUI->scaleWidget(this);

    connect(ui->closeButton, &QPushButton::clicked, this, &DialogWindow::closeCancel);

    updating = true;

    hirause.resize((int)KanaSounds::Count, 0);
    katause.resize((int)KanaSounds::Count, 0);

    ui->status->add(tr("Hiragana: "), 0, "0", 4, true);
    ui->status->add(tr("Katakana: "), 0, "0", 4, true);

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
    // Center the window on parent to make it work on X11.

    QRect gr = frameGeometry();
    QRect fr = frameGeometry();
    QRect dif = QRect(QPoint(gr.left() - fr.left(), gr.top() - fr.top()), QPoint(fr.right() - gr.right(), fr.bottom() - gr.bottom()));

    QRect r = parent() != nullptr ? ((QWidget*)parent())->frameGeometry() : qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());

    int left = r.left() + (r.width() - fr.width()) / 2 + (dif.left() + dif.right()) / 2;
    int top = r.top() + (r.height() - fr.height()) / 2 + (dif.top() + dif.bottom()) / 2;

    setGeometry(QRect(left, top, gr.width(), gr.height()));

    restoreState(FormStates::kanapractice);

    showModal();
}

void KanaPracticeSettingsForm::boxToggled(int index)
{
    if (updating)
        return;

    bool hiragana = sender() == hiramap;
    std::vector<uchar> &vec = hiragana ? hirause : katause;
    std::vector<QCheckBox*> &boxes = hiragana ? hiraboxes : kataboxes;
    int &checkcnt = hiragana ? hiracheckcnt : katacheckcnt;

    if ((vec[index] == 1) != boxes[index]->isChecked())
    {
        checkcnt += boxes[index]->isChecked() ? 1 : -1;
        ui->test1Button->setEnabled(hiracheckcnt + katacheckcnt >= 5);
        ui->test2Button->setEnabled(hiracheckcnt + katacheckcnt >= 5);
    }

    vec[index] = boxes[index]->isChecked() ? 1 : 0;


    if ((qApp->keyboardModifiers() & Qt::ControlModifier) == 0)
    {
        updateStatus();
        return;
    }

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
    updateStatus();
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
    close();

    QTimer::singleShot(0, []() {
        KanaReadingPracticeForm *form = new KanaReadingPracticeForm();
        form->exec();
    });
}

void KanaPracticeSettingsForm::on_test2Button_clicked()
{
    saveState(FormStates::kanapractice);
    close();

    QTimer::singleShot(0, []() {
        KanaWritingPracticeForm *form = new KanaWritingPracticeForm();
        form->exec();
    });
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

    hiracheckcnt = 0;
    katacheckcnt = 0;

    for (int ix = 0, siz = hirause.size(); ix != siz; ++ix)
    {
        if (hirause[ix] == 1)
        {
            ++hiracheckcnt;
            hiraboxes[ix]->setChecked(true);
        }
        else
            hiraboxes[ix]->setChecked(false);
    }
    for (int ix = 0, siz = katause.size(); ix != siz; ++ix)
    {
        if (katause[ix] == 1)
        {
            ++katacheckcnt;
            kataboxes[ix]->setChecked(true);
        }
        else
            kataboxes[ix]->setChecked(false);
    }

    ui->test1Button->setEnabled(hiracheckcnt + katacheckcnt >= 5);
    ui->test2Button->setEnabled(hiracheckcnt + katacheckcnt >= 5);

    updating = false;

    updateStatus();
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

void KanaPracticeSettingsForm::updateStatus()
{
    ui->status->setValue(0, QString::number(hiracheckcnt));
    ui->status->setValue(1, QString::number(katacheckcnt));
}



//-------------------------------------------------------------


void setupKanaPractice()
{
    KanaPracticeSettingsForm *form = new KanaPracticeSettingsForm(gUI->activeMainForm());
    form->exec();
}


//-------------------------------------------------------------
