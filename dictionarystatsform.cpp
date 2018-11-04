/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QSet>
#include <QDesktopServices>
#include <QPushButton>
#include <QScrollBar>
#include <QDesktopWidget>
#include <chrono>
#include <thread>
#include <list>
#include "dictionarystatsform.h"
#include "ui_dictionarystatsform.h"

#include "globalui.h"
#include "words.h"
#include "kanji.h"
#include "zui.h"
#include "sentences.h"
#include "groups.h"
#include "zkanjimain.h"
#include "zui.h"
#include "formstates.h"


#ifndef NDEBUG
#ifndef _DEBUG
#define NDEBUG
#define NDEBUG_ADDED_SEPARATELY
#endif
#endif

#include <cassert>

#ifdef NDEBUG_ADDED_SEPARATELY
#undef NDEBUG_ADDED_SEPARATELY
#undef NDEBUG
#endif


//-------------------------------------------------------------


StatsThread::StatsThread(DictionaryStatsForm *owner, Dictionary *dict) : base(), owner(owner), dict(dict), valentry(0), valdef(0), valkanji(0)
{
    terminate = false;
    done = false;

    setAutoDelete(false);
}

StatsThread::~StatsThread()
{
    assert(done);
}

void StatsThread::run()
{
    if (calculateEntries() && calculateDefinitions() && calculateKanji())
        done = true;
}

int StatsThread::entryResult() const
{
    return valentry;
}
int StatsThread::definitionResult() const
{
    return valdef;
}

int StatsThread::kanjiResult() const
{
    return valkanji;
}

bool StatsThread::checkTerminate()
{
    if (terminate)
    {
        done = true;
        return true;
    }
    return false;
}

bool StatsThread::calculateEntries()
{
    if (dict->entryCount() == 0)
        return true;

    if (checkTerminate())
        return false;

    std::vector<int> list;
    list.resize(dict->entryCount());
    std::iota(list.begin(), list.end(), 0);

    interruptSort(list.begin(), list.end(), [this](int a, int b, bool &localdone) {
        localdone = terminate;
        if (localdone)
            return false;

        WordEntry *ea = dict->wordEntry(a);
        WordEntry *eb = dict->wordEntry(b);

        if (ea->defs.size() != eb->defs.size())
            return ea->defs.size() < eb->defs.size();
        for (int ix = 0, siz = tosigned(ea->defs.size()); ix != siz; ++ix)
        {
            int res = ea->defs[ix].def.compare(eb->defs[ix].def);
            if (res != 0)
                return res < 0;
        }

        return false;
    });
    if (checkTerminate())
        return false;

    valentry = 1;
    for (int ix = 1, siz = tosigned(list.size()); ix != siz; ++ix)
    {
        const auto &defa = dict->wordEntry(list[ix - 1])->defs;
        const auto &defb = dict->wordEntry(list[ix])->defs;

        if (defa.size() != defb.size())
            ++valentry;
        else
        {
            for (int iy = 0, sizy = tosigned(defa.size()); iy != sizy; ++iy)
            {
                if (defa[iy].def != defb[iy].def)
                {
                    ++valentry;
                    break;
                }
            }
        }

        if ((ix % 5 == 0) && checkTerminate())
            return false;
    }

    return true;
}

bool StatsThread::calculateDefinitions()
{
    if (checkTerminate())
        return false;

    // Word index, definition index pairs.
    typedef std::pair<int, int> DefPair;
    std::vector<DefPair> pairs;
    pairs.reserve(dict->entryCount() * 1.2);
    for (int ix = 0, siz = dict->entryCount(); ix != siz; ++ix)
    {
        WordEntry *e = dict->wordEntry(ix);
        for (int iy = 0, sizy = tosigned(e->defs.size()); iy != sizy; ++iy)
            pairs.push_back(std::make_pair(ix, iy));
    }

    if (pairs.empty())
        return true;

    if (checkTerminate())
        return false;

    interruptSort(pairs.begin(), pairs.end(), [this](const DefPair &a, const DefPair &b, bool &localdone) {
        localdone = terminate;
        if (localdone)
            return false;

        WordEntry *ea = dict->wordEntry(a.first);
        WordEntry *eb = dict->wordEntry(b.first);

        return ea->defs[a.second].def < eb->defs[b.second].def;
    });
    if (checkTerminate())
        return false;

    valdef = 1;
    for (int ix = 1, siz = tosigned(pairs.size()); ix != siz; ++ix)
    {
        if (dict->wordEntry(pairs[ix - 1].first)->defs[pairs[ix - 1].second].def != dict->wordEntry(pairs[ix].first)->defs[pairs[ix].second].def)
            ++valdef;

        if ((ix % 10) && checkTerminate())
            return false;
    }

    return true;
}

bool StatsThread::calculateKanji()
{
    if (checkTerminate())
        return false;

    QSet<int> found;
    KanjiGroupCategory *cat = &dict->kanjiGroups();

    std::list<KanjiGroupCategory*> stack;
    stack.push_back(cat);
    while (!stack.empty())
    {
        if (checkTerminate())
            return false;

        cat = stack.front();
        stack.pop_front();
        for (int ix = 0, siz = cat->categoryCount(); ix != siz; ++ix)
            stack.push_back(cat->categories(ix));
        if (checkTerminate())
            return false;

        for (int ix = 0, siz = tosigned(cat->size()); ix != siz; ++ix)
        {
            KanjiGroup *grp = cat->items(ix);
            for (int iy = 0, siy = tosigned(grp->size()); iy != siy; ++iy)
            {
                if ((iy % 10) == 0 && checkTerminate())
                    return false;
                int i = grp->items(iy)->index;
                if (!found.contains(i))
                {
                    found.insert(i);
                    ++valkanji;
                }
            }
        }
    }

    return true;
}



//-------------------------------------------------------------


DictionaryStatsForm::DictionaryStatsForm(int index, QWidget *prnt) : base(prnt), ui(new Ui::DictionaryStatsForm)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    gUI->scaleWidget(this);

    ui->dictCBox->setCurrentIndex(ZKanji::dictionaryOrder(index));
    ui->dictInfoText->viewport()->installEventFilter(this);
    ui->dictInfoText->setMouseTracking(true);

    connect(ui->buttonBox->button(QDialogButtonBox::Close), &QAbstractButton::clicked, this, &DictionaryStatsForm::close);

    //for (int ix = 0; ix != 3; ++ix)
    updateWindowGeometry(this);

    //setAttribute(Qt::WA_DontShowOnScreen);
    //show();

    QLabel *lb[] = { ui->lb1, ui->lb2, ui->lb3, ui->lb4, ui->lb5, ui->lb6,
                     ui->lb7, ui->lb8, ui->lb9, ui->lb10, ui->lb11, ui->lb12,
                     ui->lb13, ui->lb14, ui->lb15 };
    int mm = 0;
    for (int ix = 0, siz = sizeof(lb) / sizeof(QLabel*); ix != siz; ++ix)
    {
        updateWindowGeometry(lb[ix]);
        mm = std::max<int>(mm, lb[ix]->sizeHint().width());
    }
    for (int ix = 0, siz = sizeof(lb) / sizeof(QLabel*); ix != siz; ++ix)
    {
        lb[ix]->setMinimumWidth(mm);
        lb[ix]->setMaximumWidth(mm);
    }

    //hide();
    //setAttribute(Qt::WA_DontShowOnScreen, false);

    restrictWidgetSize(ui->kanjiNumLabel, 8);
    restrictWidgetSize(ui->entryNumLabel, 8);
    restrictWidgetSize(ui->kanjiGrpLabel, 8);
    restrictWidgetSize(ui->entryUniqueLabel, 8);
    restrictWidgetSize(ui->defNumLabel, 8);
    restrictWidgetSize(ui->defUniqueLabel, 8);
    restrictWidgetSize(ui->entryPopLabel, 8);
    restrictWidgetSize(ui->entryMidLabel, 8);
    restrictWidgetSize(ui->entryNofreqLabel, 8);
    restrictWidgetSize(ui->kanjiDefLabel, 8);
    restrictWidgetSize(ui->kanjiJouyouLabel, 8);
    restrictWidgetSize(ui->kanjiJLPTLabel, 8);
    restrictWidgetSize(ui->kanjiGrpNumLabel, 8);
    restrictWidgetSize(ui->wordGrpLabel, 8);
    restrictWidgetSize(ui->wordGrpNumLabel, 8);

    updateData();

    // Center the window on parent to make it work on X11.

    //QRect gr = frameGeometry();
    //QRect fr = frameGeometry();
    //QRect dif = QRect(QPoint(gr.left() - fr.left(), gr.top() - fr.top()), QPoint(fr.right() - gr.right(), fr.bottom() - gr.bottom()));

    //QRect r = parent() != nullptr ? ((QWidget*)parent())->frameGeometry() : qApp->desktop()->screenGeometry((QWidget*)gUI->mainForm());

    //int left = r.left() + (r.width() - fr.width()) / 2 + (dif.left() + dif.right()) / 2;
    //int top = r.top() + (r.height() - fr.height()) / 2 + (dif.top() + dif.bottom()) / 2;

    //setGeometry(QRect(left, top, gr.width(), gr.height()));

    FormStates::restoreDialogSize("DictionaryStats", this, true);
}

DictionaryStatsForm::~DictionaryStatsForm()
{
    stopThreads();
    delete ui;
}

void DictionaryStatsForm::on_dictCBox_currentIndexChanged(int /*newindex*/)
{
    updateData();
}

bool DictionaryStatsForm::event(QEvent *e)
{
    if (e->type() == QEvent::Timer)
    {
        QTimerEvent *te = (QTimerEvent*)e;
        if (te->timerId() == timer.timerId())
        {
            if (thread != nullptr && !thread->terminate && thread->done)
            {
                timer.stop();

                Dictionary *d = ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));
                StatResult &r = results[d];
                r.entries = thread->entryResult();
                r.defs = thread->definitionResult();
                r.kanji = thread->kanjiResult();

                thread.reset();

                updateLabels(r);

            }

            return true;
        }
    }
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateTexts();
    }

    return base::event(e);
}

bool DictionaryStatsForm::eventFilter(QObject *o, QEvent *e)
{
    if (o == ui->dictInfoText->viewport())
    {
        if (e->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *me = (QMouseEvent*)e;

            QString str;
            if (me->button() == Qt::LeftButton && !(str = ui->dictInfoText->anchorAt(me->pos())).isEmpty())
                QDesktopServices::openUrl(QUrl(str));
        }
        else if (e->type() == QEvent::MouseMove)
        {
            QMouseEvent *me = (QMouseEvent*)e;
            QString str = ui->dictInfoText->anchorAt(me->pos());
            ui->dictInfoText->viewport()->setCursor(str.isEmpty() ? Qt::IBeamCursor : Qt::PointingHandCursor);
        }
    }

    return base::eventFilter(o, e);
}

void DictionaryStatsForm::closeEvent(QCloseEvent *e)
{
    stopThreads();

    base::closeEvent(e);
}

void DictionaryStatsForm::showEvent(QShowEvent *e)
{
    ui->dictInfoText->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
    base::showEvent(e);
}

void DictionaryStatsForm::updateData()
{
    stopThreads();

    Dictionary *d = ZKanji::dictionary(0);
    d = ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));
    translateTexts();

    ui->kanjiNumLabel->setText(QString::number(ZKanji::kanjis.size()));
    int cnt = 0;
    for (int ix = 0, siz = tosigned(ZKanji::kanjis.size()); ix != siz; ++ix)
        if (d->hasKanjiMeaning(ix))
            ++cnt;
    ui->kanjiDefLabel->setText(QString::number(cnt));

    ui->entryNumLabel->setText(QString::number(d->entryCount()));
    cnt = 0;
    for (int ix = 0, siz = d->entryCount(); ix != siz; ++ix)
        cnt += tosigned(d->wordEntry(ix)->defs.size());
    ui->defNumLabel->setText(QString::number(cnt));

    cnt = 0;
    int cnt2 = 0;
    for (int ix = 0, siz = d->entryCount(); ix != siz; ++ix)
        if (d->wordEntry(ix)->freq > ZKanji::popularFreqLimit)
            ++cnt;
        else if (d->wordEntry(ix)->freq > ZKanji::mediumFreqLimit)
            ++cnt2;
    ui->entryPopLabel->setText(QString::number(cnt));
    ui->entryMidLabel->setText(QString::number(cnt2));
    ui->entryNofreqLabel->setText(QString::number(d->entryCount() - cnt - cnt2));


    ui->kanjiGrpLabel->setText(QString::number(d->kanjiGroups().groupCount()));
    ui->wordGrpLabel->setText(QString::number(d->wordGroups().groupCount()));
    ui->wordGrpNumLabel->setText(QString::number(d->wordGroups().wordsInGroups()));

    cnt = 0;
    cnt2 = 0;
    for (int ix = 0, siz = tosigned(ZKanji::kanjis.size()); ix != siz; ++ix)
    {
        if (ZKanji::kanjis[ix]->jouyou < 7)
            ++cnt;
        if (ZKanji::kanjis[ix]->jlpt != 0)
            ++cnt2;
    }

    ui->kanjiJouyouLabel->setText(QString::number(cnt));
    ui->kanjiJLPTLabel->setText(QString::number(cnt2));

    if (d == ZKanji::dictionary(0))
        setHtmlInfoText(d->infoText());
    else
        ui->dictInfoText->setPlainText(d->infoText());

    ui->dictInfoText->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);

    auto it = results.find(d);
    if (it != results.end())
    {
        updateLabels(it->second);
        return;
    }

    ui->entryUniqueLabel->setText("...");
    ui->defUniqueLabel->setText("...");
    ui->kanjiGrpNumLabel->setText("...");

    startThreads(d);
}

void DictionaryStatsForm::stopThreads()
{
    if (!timer.isActive())
        return;

    timer.stop();

    if (thread == nullptr)
        return;

    thread->terminate = true;

    auto mil1 = std::chrono::milliseconds(1);
    while (!thread->done)
        std::this_thread::sleep_for(mil1);

    thread.reset();
}

void DictionaryStatsForm::startThreads(Dictionary *d)
{
    thread.reset(new StatsThread(this, d));

    QThreadPool::globalInstance()->start(thread.get());

    timer.start(100, this);
}

void DictionaryStatsForm::translateTexts()
{
    Dictionary *d = ZKanji::dictionary(0);
    ui->baseLabel->setText(tr("The base dictionary was built for zkanji %1 on %2.").arg(d->programVersion()).arg(formatDateTime(d->baseDate())));
    if (ZKanji::sentences.isLoaded())
        ui->exampleLabel->setText(tr("The example sentences database was imported in zkanji %1 on %2.").arg(ZKanji::sentences.programVersion()).arg(formatDateTime(ZKanji::sentences.creationDate())));
    else
        ui->exampleLabel->setText(tr("No example sentences database loaded."));
    d = ZKanji::dictionary(ZKanji::dictionaryPosition(ui->dictCBox->currentIndex()));
    ui->buildLabel->setText(tr("The selected dictionary was last saved in zkanji %1 on %2.").arg(d->programVersion()).arg(formatDateTime(d->lastWriteDate())));

    ui->buttonBox->button(QDialogButtonBox::Close)->setText(qApp->translate("ButtonBox", "Close"));
}

void DictionaryStatsForm::updateLabels(const StatResult &r)
{
    ui->entryUniqueLabel->setText(QString::number(r.entries));
    ui->defUniqueLabel->setText(QString::number(r.defs));
    ui->kanjiGrpNumLabel->setText(QString::number(r.kanji));
}

void DictionaryStatsForm::setHtmlInfoText(const QString &str)
{
    ui->dictInfoText->setPlainText(QString());

    QStringList strings = str.split('\n');

    for (QString s : strings)
        ui->dictInfoText->appendHtml(s);
}


//-------------------------------------------------------------


void showDictionaryStats(int dictindex)
{
    DictionaryStatsForm f(dictindex, gUI->activeMainForm());
    f.showModal();
}


//-------------------------------------------------------------
