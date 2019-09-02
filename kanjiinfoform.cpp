/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QScrollBar>
#include <QTextStream>
#include <QtSvg>
#include <QSvgRenderer>
#include <QByteArray>
#include <QDesktopWidget>
#include <QApplication>

#include "kanjiinfoform.h"
#include "ui_kanjiinfoform.h"

#include "kanji.h"
#include "zflowlayout.h"
#include "zevents.h"
#include "kanjistrokes.h"
#include "zkanjimain.h"
#include "zdictionarymodel.h"
#include "words.h"
#include "zlistview.h"
#include "zdictionarylistview.h"
#include "globalui.h"
#include "fontsettings.h"
#include "kanjisettings.h"
#include "colorsettings.h"
#include "zui.h"
#include "formstates.h"
#include "generalsettings.h"
#include "dialogs.h"

// Event posted when deferring the resize of controls in the info form.
ZEVENT(InfoResizeEvent)


//-------------------------------------------------------------


KanjiInfoForm::KanjiInfoForm(QWidget *parent) : base(parent), ui(new Ui::KanjiInfoForm), dict(nullptr), ignoreresize(false),
        dicth(-1), simmodel(new SimilarKanjiScrollerModel), partsmodel(new KanjiScrollerModel), partofmodel(new KanjiScrollerModel)
{
    ui->setupUi(this);
    ui->dictWidget->showStatusBar();

    windowInit();

    gUI->preventWidgetScale(ui->countLabel);

    if (parent != nullptr && !parent->windowFlags().testFlag(Qt::WindowStaysOnTopHint))
        setStayOnTop(false);

    gUI->scaleWidget(this);

    setAttribute(Qt::WA_DeleteOnClose);

    connect(ui->kanjiView, &ZKanjiDiagram::strokeChanged, this, &KanjiInfoForm::strokeChanged);

    readingFilterButton = new QToolButton(this);
    //readingFilterButton->setText("ReF");
    readingFilterButton->setCheckable(true);
    readingFilterButton->setIconSize(QSize(Settings::scaled(18), Settings::scaled(18)));
    readingFilterButton->setIcon(QIcon(":/wordselect.svg"));
    readingFilterButton->setAutoRaise(true);
    ui->dictWidget->addBackWidget(readingFilterButton);

    connect(readingFilterButton, &QToolButton::clicked, this, &KanjiInfoForm::readingFilterClicked);

    exampleSelButton = new QToolButton(this);
    //exampleSelButton->setText("Sel");
    exampleSelButton->setEnabled(false);
    exampleSelButton->setCheckable(true);
    exampleSelButton->setIconSize(QSize(Settings::scaled(18), Settings::scaled(18)));
    exampleSelButton->setIcon(QIcon(":/paperclip.svg"));
    exampleSelButton->setAutoRaise(true);
    ui->dictWidget->addBackWidget(exampleSelButton);

    connect(exampleSelButton, &QToolButton::clicked, this, &KanjiInfoForm::exampleSelButtonClicked);

    ui->dictWidget->setSelectionType(ListSelectionType::Extended);
    ui->dictWidget->view()->setGroupDisplay(true);

    readingCBox = new QComboBox(this);
    readingCBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    readingCBox->setMinimumContentsLength(6);
    ui->dictWidget->addBackWidget(readingCBox);

    translateUI();
    
    connect(readingCBox, SIGNAL(currentIndexChanged(int)), this, SLOT(readingBoxChanged(int)));

    updateWindowGeometry(this);

    restrictWidgetSize(ui->speedSlider, 16);

    ui->speedSlider->setMaximumHeight(ui->dirButton->height());
    //ui->speedSlider->setMaximumWidth(ui->speedLabel->width());
    //ui->kanjiView->setMinimumHeight(ui->playbackLayout->sizeHint().width());
    ui->kanjiView->setMinimumHeight(restrictedWidgetSize(ui->infoText, 32));

    //Settings::updatePalette(ui->infoText);

    QFont radf = Settings::radicalFont();
    radf.setPointSize(10);
    ui->radSymLabel->setFont(radf);

    int labelw = mmax(ui->similarLabel->width(), ui->partsLabel->width(), ui->partofLabel->width());
    ui->similarLabel->setMinimumWidth(labelw);
    ui->partsLabel->setMinimumWidth(labelw);
    ui->partofLabel->setMinimumWidth(labelw);

    ui->similarWidget->hide();
    ui->partsWidget->hide();
    ui->partofWidget->hide();
    ui->dictWidget->hide();

    ui->dictWidget->setExamplesVisible(false);
    ui->dictWidget->setInflButtonVisible(false);
    ui->dictWidget->setListMode(DictionaryWidget::Filtered);

    ui->similarScroller->setScrollerType(ZScrollArea::Buttons);
    ui->partsScroller->setScrollerType(ZScrollArea::Buttons);
    ui->partofScroller->setScrollerType(ZScrollArea::Buttons);

    //createRefLabels();

    ui->similarScroller->setModel(simmodel.get());
    //ui->similarScroller->setCellSize(ui->similarScroller->cellSize() * 0.8);
    ui->partsScroller->setModel(partsmodel.get());
    //ui->partsScroller->setCellSize(ui->partsScroller->cellSize() * 0.8);
    ui->partofScroller->setModel(partofmodel.get());
    //ui->partofScroller->setCellSize(ui->partofScroller->cellSize() * 0.8);

    ui->similarScroller->setFrameShape(QFrame::NoFrame);
    ui->partsScroller->setFrameShape(QFrame::NoFrame);
    ui->partofScroller->setFrameShape(QFrame::NoFrame);

    ui->infoText->setCursorWidth(0);

    installGrabWidget(ui->kanjiView);
    installGrabWidget(ui->toolbarWidget);

    addCloseButton(ui->captionLayout);

    connect(gUI, &GlobalUI::settingsChanged, this, &KanjiInfoForm::settingsChanged);
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &KanjiInfoForm::dictionaryReplaced);

    connect(ui->similarScroller, &ZItemScroller::itemClicked, this, &KanjiInfoForm::scrollerClicked);
    connect(ui->partsScroller, &ZItemScroller::itemClicked, this, &KanjiInfoForm::scrollerClicked);
    connect(ui->partofScroller, &ZItemScroller::itemClicked, this, &KanjiInfoForm::scrollerClicked);

    connect(ui->dictWidget, &DictionaryWidget::rowSelectionChanged, this, &KanjiInfoForm::wordSelChanged);

    //ui->playButton->installEventFilter(this);

    settingsChanged();
}

KanjiInfoForm::~KanjiInfoForm()
{
    delete ui;
}

void KanjiInfoForm::saveXMLSettings(QXmlStreamWriter &writer)
{
    KanjiInfoFormData dat;
    saveState(dat);
    FormStates::saveXMLSettings(dat, writer);
}

bool KanjiInfoForm::loadXMLSettings(QXmlStreamReader &reader)
{
    KanjiInfoFormData dat;
    FormStates::loadXMLSettings(dat, reader);
    return restoreState(dat);
}

void KanjiInfoForm::saveState(KanjiInfoData &dat) const
{
    QRect g = geometry();
    dat.siz = isMaximized() ? normalGeometry().size() : g.size();
    dat.pos = g.topLeft();
    dat.screen = qApp->desktop()->screenGeometry(screenNumber(QRect(dat.pos, dat.siz)));

    int h = dat.siz.height();
    if (ui->sodWidget->isVisibleTo(this))
        h -= ui->sodWidget->height() + ui->sodWidget->parentWidget()->layout()->spacing();
    if (ui->similarWidget->isVisibleTo(this))
        h -= ui->similarWidget->height() + ui->similarWidget->parentWidget()->layout()->spacing();
    if (ui->partsWidget->isVisibleTo(this))
        h -= ui->partsWidget->height() + ui->partsWidget->parentWidget()->layout()->spacing();
    if (ui->partofWidget->isVisibleTo(this))
        h -= ui->partofWidget->height() + ui->partofWidget->parentWidget()->layout()->spacing();

    int dh = dicth;
    if (ui->dictWidget->isVisibleTo(this))
    {
        dh = ui->dictWidget->height();
        h -= ui->dictWidget->height() + ui->splitter->handleWidth();
    }
    dat.siz.setHeight(h);

    dat.sod = ui->sodButton->isChecked();
    dat.grid = ui->gridButton->isChecked();
    dat.loop = ui->loopButton->isChecked();
    dat.shadow = ui->shadowButton->isChecked();
    dat.numbers = ui->numberButton->isChecked();
    dat.dir = ui->dirButton->isChecked();
    dat.speed = ui->speedSlider->value();
    dat.similar = ui->simButton->isChecked();
    dat.parts = ui->partsButton->isChecked();
    dat.partof = ui->partofButton->isChecked();
    dat.words = ui->wordsButton->isChecked();
    ui->dictWidget->saveState(dat.dict);
    dat.refdict = readingFilterButton->isChecked();

    dat.toph = ui->dictWidget->isVisibleTo(ui->dictWidget->parentWidget()) ? ui->splitter->sizes().at(0) : ui->dataWidget->height();
    dat.dicth = dh;
}

void KanjiInfoForm::restoreState(const KanjiInfoData &dat)
{
    ignoreresize = true;

    int kindex = ui->kanjiView->kanjiIndex();

    ui->sodButton->setChecked(ZKanji::elements()->size() != 0 && dat.sod);
    ui->sodButton->setEnabled(ZKanji::elements()->size() != 0 && kindex >= 0);
    ui->gridButton->setChecked(dat.grid);
    ui->loopButton->setChecked(dat.loop);
    on_loopButton_clicked(dat.loop);

    ui->shadowButton->setChecked(dat.shadow);
    ui->numberButton->setChecked(dat.numbers);
    ui->dirButton->setChecked(dat.dir);
    ui->speedSlider->setValue(dat.speed);
    ui->simButton->setChecked(dat.similar);
    ui->partsButton->setChecked(ZKanji::elements()->size() != 0 && dat.parts);
    ui->partofButton->setChecked(ZKanji::elements()->size() != 0 && dat.partof);
    ui->simButton->setEnabled(kindex < 0 || !dat.words);
    ui->partsButton->setEnabled(ZKanji::elements()->size() != 0 && (kindex < 0 || !dat.words));
    ui->partofButton->setEnabled(ZKanji::elements()->size() != 0 && (kindex < 0 || !dat.words));
    ui->wordsButton->setChecked(dat.words);
    ui->wordsButton->setEnabled(kindex >= 0);
    readingFilterButton->setChecked(dat.refdict);
    ui->dictWidget->restoreState(dat.dict);

    ui->kanjiView->setDiagram(kindex < 0 || dat.sod);
    ui->kanjiView->setGrid(dat.grid);
    ui->kanjiView->setShadow(dat.shadow);
    ui->kanjiView->setNumbers(dat.numbers);
    ui->kanjiView->setDirection(dat.dir);
    ui->sodWidget->setVisible(kindex < 0 || dat.sod);
    ui->similarWidget->setVisible((kindex < 0 || !dat.words) && dat.similar);
    ui->partsWidget->setVisible(ZKanji::elements()->size() != 0 && (kindex < 0 || !dat.words) && dat.parts);
    ui->partofWidget->setVisible(ZKanji::elements()->size() != 0 && (kindex < 0 || !dat.words) && dat.partof);
    ui->dictWidget->setVisible(kindex >= 0 && dat.words);

    ui->kanjiView->setSpeed(dat.speed);

    if (kindex >= 0 && dat.words)
    {
        disconnect(ui->splitter, &QSplitter::splitterMoved, this, &KanjiInfoForm::restrictKanjiViewSize);
        if (!wordsmodel)
            wordsmodel.reset(new KanjiWordsItemModel(this));
        wordsmodel->setKanji(dict, kindex, readingCBox->currentIndex() - 1);
        exampleSelButton->setEnabled(wordsmodel->rowCount() != 0);
        ui->dictWidget->setModel(wordsmodel.get());
        connect(ui->splitter, &QSplitter::splitterMoved, this, &KanjiInfoForm::restrictKanjiViewSize);
    }

    if (wordsmodel != nullptr)
        wordsmodel->setShowOnlyExamples(readingFilterButton->isChecked());

    updateWindowGeometry(this);

    //ui->centralwidget->layout()->activate();
    //layout()->activate();

    //ui->centralwidget->layout()->invalidate();
    //layout()->invalidate();
    //ui->centralwidget->layout()->update();
    //layout()->update();


    if (dat.dicth == -1)
        dicth = ui->dataWidget->minimumSizeHint().height();
    else
        dicth = dat.dicth;

    ui->splitter->setSizes({ dat.toph, dicth });

    //QRect geom = QRect(dat.pos, dat.siz);
    int geomh = 0;
    if (dat.siz.isValid())
    {
        geomh = dat.siz.height();
        if (ui->sodWidget->isVisibleTo(this))
        {
            ui->sodWidget->adjustSize();
            geomh += ui->sodWidget->height() + ui->sodWidget->parentWidget()->layout()->spacing();
        }
        if (ui->similarWidget->isVisibleTo(this))
        {
            ui->similarWidget->adjustSize();
            geomh += ui->similarWidget->height() + ui->similarWidget->parentWidget()->layout()->spacing();
        }
        if (ui->partsWidget->isVisibleTo(this))
        {
            ui->partsWidget->adjustSize();
            geomh += ui->partsWidget->height() + ui->partsWidget->parentWidget()->layout()->spacing();
        }
        if (ui->partofWidget->isVisibleTo(this))
        {
            ui->partofWidget->adjustSize();
            geomh += ui->partofWidget->height() + ui->partofWidget->parentWidget()->layout()->spacing();
        }
        if (ui->dictWidget->isVisibleTo(this))
        {
            ui->dictWidget->adjustSize();
            geomh += dicth + ui->splitter->handleWidth();
        }
        //geom.setHeight(h);
        //resize(siz);
    }
    else
        geomh = geometry().height(); // .setSize(geometry().size());

    //int screennum = !dat.siz.isValid() ? -1 : screenNumber(QRect(dat.pos, geom.size()));
    //QRect sg;
    //if (screennum == -1)
    //{
    //    screennum = qApp->desktop()->screenNumber((QWidget*)gUI->mainForm());
    //    sg = qApp->desktop()->screenGeometry(screennum);
    //    if (dat.siz.isValid())
    //        geom.moveTo(sg.topLeft() + (dat.pos - dat.screenpos));
    //    else
    //        geom.moveTo(sg.center() - QPoint(geom.width() / 2, geom.height() / 2));
    //}
    //else
    //    sg = qApp->desktop()->screenGeometry(screennum);

    //// Geometry might be out of bounds. Move window within bounds.

    //if (geom.left() < sg.left())
    //    geom.moveLeft(sg.left());
    //else if (geom.left() + geom.width() > sg.left() + sg.width())
    //    geom.moveLeft(sg.left() + sg.width() - geom.width());
    //if (geom.top() < sg.top())
    //    geom.moveTop(sg.top());
    //else if (geom.top() + geom.height() > sg.top() + sg.height())
    //    geom.moveTop(sg.top() + sg.height() - geom.height());
    //if (geom.width() > std::min(sg.width(), sg.height()))
    //    geom.setWidth(std::min(sg.width(), sg.height()));
    //if (geom.height() > std::min(sg.height(), sg.height()))
    //    geom.setHeight(std::min(sg.height(), sg.height()));

    //move(geom.topLeft());

    resize(QSize(dat.siz.width(), geomh)/*geom.size()*/);

    //updateWindowGeometry(this);

    if (Settings::kanji.showpos == KanjiSettings::RestoreLast)
    {
        QRect sg = qApp->desktop()->screenGeometry(screenNumber(QRect(dat.pos, dat.siz)));
        if (dat.screen == sg)
            move(std::max(sg.left(), std::min(dat.pos.x(), sg.left() + sg.width() - dat.siz.width())), std::max(sg.top(), std::min(dat.pos.y(), sg.top() + sg.height() - geomh)));
    }
    else if (Settings::kanji.showpos == KanjiSettings::NearCursor)
    {
        QPoint p = QCursor::pos();
        QRect sg = qApp->desktop()->screenGeometry(p);

        // Place window to the left of the cursor if there is more screen space there.
        bool toleft = p.x() - sg.left() > sg.width() + sg.left() - p.x();
        // Place window above the cursor if there is more screen space there.
        bool toabove = p.y() - sg.top() > sg.height() + sg.top() - p.y();

        QPoint pos;
        if (toleft)
            pos.setX(std::max(sg.left(), p.x() - dat.siz.width() - Settings::scaled(32)));
        else
            pos.setX(std::min(sg.left() + sg.width() - dat.siz.width(), p.x() + Settings::scaled(64)));
        if (toabove)
            pos.setY(std::max(sg.top(), p.y() - geomh - Settings::scaled(32)));
        else
            pos.setY(std::min(sg.top() + sg.height() - geomh, p.y() + Settings::scaled(48)));

        move(pos);
    }

    ignoreresize = false;
    _resized(true);
}

void KanjiInfoForm::saveState(KanjiInfoFormData &dat) const
{
    dat.kindex = ui->kanjiView->kanjiIndex();
    dat.dictname = dict == nullptr ? QString() : dict->name();
    dat.locked = ui->lockButton->isChecked();
    saveState(dat.data);
}

bool KanjiInfoForm::restoreState(const KanjiInfoFormData &dat)
{
    ignoreresize = true;
    int dix = ZKanji::dictionaryIndex(dat.dictname);
    if (dix != -1)
        setKanji(ZKanji::dictionary(dix), dat.kindex);
    ui->lockButton->setChecked(dat.locked);
    ui->lockButton->setEnabled(!ui->lockButton->isChecked());
    ignoreresize = false;

    restoreState(dat.data);

    return dix != -1;
}

namespace ZKanji
{
    // Same as the radsymbols in ZRadicalGrid, without the added stroke count.
    extern QChar radsymbols[214];
}

void KanjiInfoForm::setKanji(Dictionary *d, int kindex)
{
    if (dict != nullptr)
        disconnect(dict, &Dictionary::dictionaryReset, this, &KanjiInfoForm::dictionaryReset);
    dict = d;
    if (dict != nullptr)
        connect(dict, &Dictionary::dictionaryReset, this, &KanjiInfoForm::dictionaryReset);

    KanjiEntry *k = kindex >= 0 ? ZKanji::kanjis[kindex] : nullptr;

    ui->kanjiView->setKanjiIndex(kindex);
    ui->infoText->setPlainText(QString());
    ui->infoText->document()->setDefaultStyleSheet(QString("body { font-size: %1pt; color: %2; }").arg(Settings::scaled(10)).arg(Settings::textColor(this, ColorSettings::Text).name()));
    ui->infoText->document()->setHtml(ZKanji::kanjiInfoText(d, kindex));
    ui->infoText->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);

    ui->similarScroller->setDictionary(dict);
    ui->partsScroller->setDictionary(dict);
    ui->partofScroller->setDictionary(dict);

    ui->countLabel->setText(QStringLiteral("<span style=\"font-weight: bold; font-size: %2pt;\">%1</span>").arg(ZKanji::elements()->size() != 0 ? ZKanji::elements()->strokeCount(kindex < 0 ? (-1 - kindex) : k->element, 0) : k->strokes, 2, 10, QChar('0')).arg(Settings::scaled(ui->countLabel->font().pointSize())));

    std::vector<int> l;
    if (k != nullptr)
    {
        auto simit = ZKanji::similarkanji.find(k->ch.unicode());
        if (simit != ZKanji::similarkanji.end())
        {
            auto &arr = simit->second.second;
            l.reserve(arr.size());
            for (int ix = 0, siz = tosigned(arr.size()); ix != siz; ++ix)
                l.push_back(arr[ix]);
            simmodel->setItems(l, simit->second.first);
            l.clear();
        }

        ui->strokesLabel->setText(QString::number(k->strokes));
        ui->radLabel->setText(QString::number(k->rad));
        ui->radSymLabel->setText(ZKanji::radsymbols[k->rad - 1]);
    }
    else
    {
        ui->strokesLabel->setText("-");
        ui->radLabel->setText("-");
        ui->radSymLabel->setText(" ");
    }

    if (kindex < 0 || k->element >= 0)
        ZKanji::elements()->elementParts(kindex < 0 ? (-1 - kindex) : k->element, !Settings::kanji.listparts, true, l);
    partsmodel->setItems(l);
    l.clear();
    if (kindex < 0 || k->element >= 0)
        ZKanji::elements()->elementParents(kindex < 0 ? (-1 - kindex) : k->element, !Settings::kanji.listparts, true, l);
    partofmodel->setItems(l);
    l.clear();

    readingCBox->clear();

    if (k != nullptr)
    {
        // Readings of the kanji.
        QStringList rl;
        rl.reserve(ZKanji::kanjiReadingCount(k, true) + 1);
        //: No reading is selected
        rl.push_back("-");
        //: Irregular readings
        rl.push_back(tr("Irreg."));
        for (int ix = 0, siz = tosigned(k->on.size()); ix != siz; ++ix)
            rl.push_back(k->on[ix].toQString());

        if (!k->kun.empty())
        {
            const QChar *w = k->kun[0].data();
            const QChar *p = qcharchr(w, '.');

            if (!p)
                p = qcharchr(w, 0);

            rl.push_back(k->kun[0].toQString(0, p - w));

            for (int ix = 1, siz = tosigned(k->kun.size()); ix != siz; ++ix)
            {
                const QChar *kun = k->kun[ix].data();
                const QChar *kp = qcharchr(kun, '.');
                if (!kp)
                    kp = qcharchr(kun, 0);

                if (p - w != kp - kun || qcharncmp(w, kun, p - w))
                {
                    p = kp;
                    w = kun;
                    rl.push_back(k->kun[ix].toQString(0, p - w));
                }
            }
        }

        readingCBox->addItems(rl);
    }

    if (kindex < 0 && ui->wordsButton->isEnabled())
    {
        if (ui->wordsButton->isChecked())
            on_wordsButton_clicked(false);
        ui->wordsButton->setEnabled(false);
        if (ZKanji::elements()->size() != 0)
        {
            if (!ui->sodButton->isChecked())
                on_sodButton_clicked(true);
            ui->sodButton->setEnabled(false);
        }
    }
    else if (!ui->wordsButton->isEnabled())
    {
        ui->wordsButton->setEnabled(true);
        if (ui->wordsButton->isChecked())
            on_wordsButton_clicked(true);
        if (ZKanji::elements()->size() != 0)
        {
            ui->sodButton->setEnabled(true);
            if (!ui->sodButton->isChecked())
                on_sodButton_clicked(false);
        }
    }

    if (dict != nullptr)
        ui->dictWidget->setDictionary(dict);

    //_resized(/*true*/);
}

void KanjiInfoForm::popup(Dictionary *d, int kindex)
{
    setKanji(d, kindex);
    if (!isVisible())
    {
        //qApp->processEvents();
        restoreState(FormStates::kanjiinfo);

        //setAttribute(Qt::WA_DontShowOnScreen);
        //show();
        //hide();
        //setAttribute(Qt::WA_DontShowOnScreen, false);

        updateWindowGeometry(this);
        _resized();
    }
    show();
}

QWidget* KanjiInfoForm::captionWidget() const
{
    return ui->captionWidget;
}

bool KanjiInfoForm::locked() const
{
    return ui->lockButton->isChecked();
}

void KanjiInfoForm::copy() const
{
    if (ui->kanjiView->kanjiIndex() < 0)
        return;
    gUI->clipCopy(ZKanji::kanjis[ui->kanjiView->kanjiIndex()]->ch);
}

void KanjiInfoForm::append() const
{
    if (ui->kanjiView->kanjiIndex() < 0)
        return;
    gUI->clipAppend(ZKanji::kanjis[ui->kanjiView->kanjiIndex()]->ch);
}

void KanjiInfoForm::copyData() const
{
    if (ui->kanjiView->kanjiIndex() < 0)
        return;
    gUI->clipCopy(QString("%1\n\n").arg(ZKanji::kanjis[ui->kanjiView->kanjiIndex()]->ch) + ui->infoText->toPlainText());
}

void KanjiInfoForm::addToGroup() const
{
    if (ui->kanjiView->kanjiIndex() < 0)
        return;
    kanjiToGroupSelect(dict, { (ushort)ui->kanjiView->kanjiIndex() });
}

//void KanjiInfoForm::appendData() const
//{
//    if (ui->kanjiView->kanjiIndex() < 0)
//        return;
//    gUI->clipAppend(ui->infoText->toPlainText());
//}

QRect KanjiInfoForm::resizing(int side, QRect r)
{
    if (ui->dictWidget->isVisibleTo(this) || (side != (int)GrabSide::Top && side != (int)GrabSide::Bottom))
        return base::resizing(side, r);

    int mh = minimumSizeHint().height();
    r.setHeight(std::max(mh, r.height()));
    QSize gs = geometry().size();
    r.setWidth(r.height() - gs.height() + gs.width());

    return r;
}

bool KanjiInfoForm::event(QEvent *e)
{
    if (e->type() == InfoResizeEvent::Type())
    {
        _resized();
        return true;
    }
    if (e->type() == QEvent::Show)
        ui->infoText->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
    else if (e->type() == QEvent::WindowActivate || e->type() == QEvent::WindowDeactivate)
    {
        int oldpos = ui->infoText->verticalScrollBar()->value();
        ui->infoText->setPlainText(QString());
        ui->infoText->document()->setDefaultStyleSheet(QString());
        Settings::updatePalette(ui->infoText);
        ui->infoText->document()->setDefaultStyleSheet(QString("body { font-size: %1pt; color: %2; }").arg(Settings::scaled(10)).arg(Settings::textColor(this, ColorSettings::Text).name()));
        ui->infoText->document()->setHtml(ZKanji::kanjiInfoText(dict, ui->kanjiView->kanjiIndex()));
        ui->infoText->verticalScrollBar()->setValue(oldpos);
    }
    else if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        translateUI();
    }

    return base::event(e);
}

bool KanjiInfoForm::eventFilter(QObject *o, QEvent *e)
{
    //if (o == ui->playButton && ui->playButton->isEnabled() && ui->playButton->isVisibleTo(this) && e->type() == QEvent::MouseButtonDblClick && ui->kanjiView->playing())
    //{
    //    QMouseEvent *me = (QMouseEvent*)e;
    //    if (me->button() == Qt::LeftButton)
    //    {
    //        if (ui->kanjiView->looping())
    //        {
    //            ui->kanjiView->setLooping(false);
    //            ui->playButton->setIcon(playico);
    //        }
    //        else
    //        {
    //            ui->kanjiView->setLooping(true);
    //            ui->playButton->setIcon(repeatico);
    //        }
    //    }
    //}

    return base::eventFilter(o, e);
}

void KanjiInfoForm::contextMenuEvent(QContextMenuEvent *e)
{
    showContextMenu(e->globalPos());
}

void KanjiInfoForm::resizeEvent(QResizeEvent *e)
{
    base::resizeEvent(e);

    if (ignoreresize)
        return;

    //if (!ui->refWidget->isVisibleTo(this))
    //{
    //    QRect r = ui->refWidget->geometry();

    //    int left, top, right, bottom;
    //    ui->refWidget->getContentsMargins(&left, &top, &right, &bottom);
    //    r.setWidth(ui->centralwidget->width() - left - right);

    //    ui->refWidget->layout()->setGeometry(r);
    //}
    
    _resized();

    //setMinimumHeight(rect().width() / 2);
}

void KanjiInfoForm::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
    {
        e->accept();
        close();
    }
    else
        base::keyPressEvent(e);
}

void KanjiInfoForm::hideEvent(QHideEvent *e)
{
    ui->kanjiView->stop();

    if (!ignoreresize && !ui->lockButton->isChecked())
        saveState(FormStates::kanjiinfo);

    base::hideEvent(e);
}

void KanjiInfoForm::translateUI()
{
    readingFilterButton->setToolTip(tr("Only show marked words"));
    exampleSelButton->setToolTip(tr("Mark as example word of kanji"));
    readingCBox->setToolTip(tr("Show words that match selected kanji reading"));

    if (readingCBox->count() > 1)
        readingCBox->setItemText(1, tr("Irreg."));
}

void KanjiInfoForm::_resized(bool post)
{
    if (post)
    {
        qApp->postEvent(this, new InfoResizeEvent());
        return;
    }

    restrictKanjiViewSize();
}

void KanjiInfoForm::restrictKanjiViewSize()
{
    ui->kanjiView->setMinimumWidth(ui->kanjiView->height());
    ui->kanjiView->setMaximumWidth(ui->kanjiView->height());
}

void KanjiInfoForm::on_loopButton_clicked(bool checked)
{
    ui->kanjiView->setLooping(checked);
}

void KanjiInfoForm::on_playButton_clicked(bool checked)
{
    if (!checked)
        return;

    ui->kanjiView->play();
}

void KanjiInfoForm::on_pauseButton_clicked(bool checked)
{
    if (!checked)
        return;

    ui->kanjiView->pause();
}

void KanjiInfoForm::on_stopButton_clicked(bool checked)
{
    if (!checked)
        return;

    ui->kanjiView->stop();
    //ui->kanjiView->setLooping(false);
    //ui->playButton->setIcon(playico);
}

void KanjiInfoForm::on_rewindButton_clicked()
{
    ui->kanjiView->rewind();
}

void KanjiInfoForm::on_backButton_clicked()
{
    ui->kanjiView->back();
}

void KanjiInfoForm::on_foreButton_clicked()
{
    ui->kanjiView->next();
}

//void KanjiInfoForm::on_endButton_clicked()
//{
//    ui->kanjiView->stop();
//}

void KanjiInfoForm::on_sodButton_clicked(bool checked)
{
    if (ZKanji::elements()->size() == 0)
        return;

    ui->kanjiView->setDiagram(checked);

    ignoreresize = true;
    int h = ui->sodWidget->height() + ui->sodWidget->parentWidget()->layout()->spacing();

    QSize s = geometry().size();
    if (!checked)
    {
        ui->sodWidget->setVisible(false);

        ui->centralwidget->layout()->activate();
        layout()->activate();

        if (!ui->dictWidget->isVisibleTo(this))
        {
            s.setHeight(s.height() - h);
            resize(s);
        }
    }
    else
    {
        if (!ui->dictWidget->isVisibleTo(this))
        {
            s.setHeight(s.height() + h);
            resize(s);
        }

        ui->sodWidget->setVisible(true);
    }

    ignoreresize = false;
    _resized(true);
}

void KanjiInfoForm::on_gridButton_clicked(bool checked)
{
    ui->kanjiView->setGrid(checked);
}

void KanjiInfoForm::on_shadowButton_clicked(bool checked)
{
    ui->kanjiView->setShadow(checked);
}

void KanjiInfoForm::on_numberButton_clicked(bool checked)
{
    ui->kanjiView->setNumbers(checked);
}

void KanjiInfoForm::on_dirButton_clicked(bool checked)
{
    ui->kanjiView->setDirection(checked);
}

void KanjiInfoForm::on_speedSlider_valueChanged(int value)
{
    ui->kanjiView->setSpeed(value);
}


//void KanjiInfoForm::on_refButton_clicked(bool checked)
//{
//    if (!ui->refWidget->isVisibleTo(this) && checked)
//    {
//        QRect r = ui->refWidget->geometry();
//
//        int left, top, right, bottom;
//        ui->refWidget->getContentsMargins(&left, &top, &right, &bottom);
//        r.setWidth(ui->centralwidget->width() - left - right);
//
//        ui->refWidget->layout()->setGeometry(r);
//    }
//
//    ignoreresize = true;
//    int h = ui->refWidget->height() + ui->refWidget->parentWidget()->layout()->spacing();
//
//    QSize s = geometry().size();
//    if (!checked)
//    {
//        ui->refWidget->setVisible(false);
//        ui->centralwidget->layout()->activate();
//        layout()->activate();
//        s.setHeight(s.height() - h);
//        resize(s);
//    }
//    else
//    {
//        s.setHeight(s.height() + h);
//        resize(s);
//        ui->refWidget->setVisible(true);
//    }
//
//    ignoreresize = false;
//    _resized(true);
//}

void KanjiInfoForm::on_simButton_clicked(bool checked)
{
    ignoreresize = true;
    int h = ui->similarWidget->sizeHint().height() /*std::max(ui->similarLabel->height(), ui->similarScroller->sizeHint().height()) + ui->similarLine->height()*/ + ui->similarWidget->parentWidget()->layout()->spacing();

    QSize s = geometry().size();
    if (!checked)
    {
        ui->similarWidget->setVisible(false);

        ui->centralwidget->layout()->activate();
        layout()->activate();

        s.setHeight(s.height() - h);
        resize(s);
    }
    else
    {
        s.setHeight(s.height() + h);
        resize(s);
        ui->similarWidget->setVisible(true);
    }

    ignoreresize = false;
    _resized(true);
}

void KanjiInfoForm::on_partsButton_clicked(bool checked)
{
    ignoreresize = true;
    int h = ui->partsWidget->sizeHint().height() /*std::max(ui->partsLabel->height(), ui->partsScroller->sizeHint().height()) + ui->partsLine->height()*/ + ui->partsWidget->parentWidget()->layout()->spacing();

    //int h = ui->partsWidget->height() + ui->partsWidget->parentWidget()->layout()->spacing();

    QSize s = geometry().size();
    if (!checked)
    {
        ui->partsWidget->setVisible(false);

        ui->centralwidget->layout()->activate();
        layout()->activate();

        s.setHeight(s.height() - h);
        resize(s);
    }
    else
    {
        s.setHeight(s.height() + h);
        resize(s);
        ui->partsWidget->setVisible(true);
    }

    ignoreresize = false;
    _resized(true);
}

void KanjiInfoForm::on_partofButton_clicked(bool checked)
{
    ignoreresize = true;
    int h = ui->partofWidget->sizeHint().height() /*std::max(ui->partofLabel->height(), ui->partofScroller->sizeHint().height()) + ui->partofLine->height()*/ + ui->partofWidget->parentWidget()->layout()->spacing();
    //int h = ui->partofWidget->height() + ui->partofWidget->parentWidget()->layout()->spacing();

    QSize s = geometry().size();
    if (!checked)
    {
        ui->partofWidget->setVisible(false);

        ui->centralwidget->layout()->activate();
        layout()->activate();

        s.setHeight(s.height() - h);
        resize(s);
    }
    else
    {
        s.setHeight(s.height() + h);
        resize(s);
        ui->partofWidget->setVisible(true);
    }

    ignoreresize = false;
    _resized(true);
}

void KanjiInfoForm::on_wordsButton_clicked(bool checked)
{
    //int h = ui->dictWidget->height() + ui->dictWidget->parentWidget()->parentWidget()->layout()->spacing();
    ignoreresize = true;

    ui->simButton->setEnabled(!checked);
    ui->partsButton->setEnabled(ZKanji::elements()->size() != 0 && !checked);
    ui->partofButton->setEnabled(ZKanji::elements()->size() != 0 && !checked);

    int h = (!checked && ui->simButton->isChecked()) || ui->similarWidget->isVisible() ? (ui->similarWidget->sizeHint().height() + ui->similarWidget->parentWidget()->layout()->spacing()) : 0;
    h += (!checked && ui->partsButton->isChecked()) || ui->partsWidget->isVisible() ? (ui->partsWidget->sizeHint().height() + ui->partsWidget->parentWidget()->layout()->spacing()) : 0;
    h += (!checked && ui->partofButton->isChecked()) || ui->partofWidget->isVisible() ? (ui->partofWidget->sizeHint().height() + ui->partofWidget->parentWidget()->layout()->spacing()) : 0;

    if (dicth == -1 || ui->dictWidget->isVisibleTo(ui->dictWidget->parentWidget()))
        dicth = ui->dictWidget->isVisibleTo(ui->dictWidget->parentWidget()) ? ui->splitter->sizes().at(1) : ui->dataWidget->minimumSizeHint().height();
    h -= dicth + ui->splitter->handleWidth();

    QSize s = geometry().size();

    if (!checked)
    {
        disconnect(ui->splitter, &QSplitter::splitterMoved, this, &KanjiInfoForm::restrictKanjiViewSize);

        dicth = ui->splitter->sizes().at(1);

        ui->dictWidget->setVisible(false);

        ui->similarWidget->setVisible(ui->simButton->isChecked());
        ui->partsWidget->setVisible(ui->partsButton->isEnabled() && ui->partsButton->isChecked());
        ui->partofWidget->setVisible(ui->partofButton->isEnabled() && ui->partofButton->isChecked());

        ui->splitter->refresh();
        ui->centralwidget->layout()->activate();
        layout()->activate();

        s.setHeight(s.height() + h);
        resize(s);
    }
    else
    {
        if (!wordsmodel)
            wordsmodel.reset(new KanjiWordsItemModel(this));
        wordsmodel->setKanji(dict, ui->kanjiView->kanjiIndex(), readingCBox->currentIndex() - 1);
        ui->dictWidget->setModel(wordsmodel.get());
        exampleSelButton->setEnabled(wordsmodel->rowCount() != 0);

        int toph = ui->dataWidget->height();

        ui->similarWidget->setVisible(false);
        ui->partsWidget->setVisible(false);
        ui->partofWidget->setVisible(false);

        s.setHeight(s.height() - h);
        resize(s);

        ui->splitter->setSizes({ toph, dicth });
        ui->dictWidget->setDictionary(dict);
        ui->dictWidget->setVisible(true);
        ui->splitter->setSizes({ toph, dicth });


        ui->centralwidget->layout()->activate();
        layout()->activate();

        connect(ui->splitter, &QSplitter::splitterMoved, this, &KanjiInfoForm::restrictKanjiViewSize);
    }

    ignoreresize = false;
    _resized(true);
}

void KanjiInfoForm::on_lockButton_clicked(bool checked)
{
    if (checked)
    {
        saveState(FormStates::kanjiinfo);
        ui->lockButton->setIcon(QIcon(":/lock.svg"));
        emit formLock(true);
        connect(gUI, &GlobalUI::kanjiInfoShown, this, &KanjiInfoForm::disableUnlock);
    }
    else
    {
        ui->lockButton->setIcon(QIcon(":/lockopen.svg"));
        emit formLock(false);
        disconnect(gUI, &GlobalUI::kanjiInfoShown, this, &KanjiInfoForm::disableUnlock);
    }
}

void KanjiInfoForm::disableUnlock()
{
    disconnect(gUI, &GlobalUI::kanjiInfoShown, this, &KanjiInfoForm::disableUnlock);
    disconnect(this, nullptr, gUI, nullptr);
    ui->lockButton->setEnabled(false);
}

void KanjiInfoForm::strokeChanged(int index, bool ended)
{
    ui->countLabel->setText(QStringLiteral("<span style=\"font-weight: bold; font-size: %2pt;\">%1</span>").arg(index, 2, 10, QChar('0')).arg(Settings::scaled(ui->countLabel->font().pointSize())));

    if (ended)
    {
        ui->stopButton->setChecked(true);
        //ui->kanjiView->setLooping(false);
        //ui->playButton->setIcon(playico);
    }
    else if (!ui->kanjiView->playing())
        ui->pauseButton->setChecked(true);
}


//void testFuriganaReadingTest();
void KanjiInfoForm::readingFilterClicked(bool checked)
{
    //testFuriganaReadingTest();

    if (wordsmodel != nullptr)
    {
        wordsmodel->setShowOnlyExamples(checked);
        exampleSelButton->setEnabled(wordsmodel->rowCount() != 0);
    }
}

void KanjiInfoForm::readingBoxChanged(int /*index*/)
{
    if (wordsmodel != nullptr && ui->kanjiView->kanjiIndex() >= 0)
    {
        wordsmodel->setKanji(dict, ui->kanjiView->kanjiIndex(), readingCBox->currentIndex() - 1);
        exampleSelButton->setEnabled(wordsmodel->rowCount() != 0);
    }
}

void KanjiInfoForm::exampleSelButtonClicked(bool /*checked*/)
{
    //int ix = ui->dictWidget->currentIndex();
    int kindex = ui->kanjiView->kanjiIndex();

    //if (ix != -1 && !dict->isKanjiExample(kindex, ix))
    //    ix = -1;

    std::vector<int> sel;
    ui->dictWidget->selectedIndexes(sel);

    bool select = false;
    for (int val : sel)
        if (!dict->isKanjiExample(kindex, val))
        {
            select = true;
            break;
        }

    if (select)
        for (int val : sel)
            dict->addKanjiExample(kindex, val);
    else
        for (int val : sel)
            dict->removeKanjiExample(kindex, val);

}

void KanjiInfoForm::scrollerClicked(int index)
{
    ZItemScroller *s = (ZItemScroller*)sender();
    KanjiScrollerModel *m = s->model();
    int item = m->items(index);
    setKanji(dict, item);
}

void KanjiInfoForm::settingsChanged()
{
    ui->infoText->setPlainText(QString());
    ui->infoText->document()->setDefaultStyleSheet(QString("body { font-size: %1pt; color: %2; }").arg(Settings::scaled(10)).arg(Settings::textColor(this, ColorSettings::Text).name()));
    ui->infoText->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
    Settings::updatePalette(ui->infoText);
    ui->infoText->document()->setHtml(ZKanji::kanjiInfoText(dict, ui->kanjiView->kanjiIndex()));

    int kindex = ui->kanjiView->kanjiIndex();
    if (kindex != INT_MIN)
    {
        std::vector<int> l;
        int kelem = kindex < 0 ? -1 - kindex : ZKanji::kanjis[kindex]->element;
        if (kelem >= 0)
            ZKanji::elements()->elementParts(kelem, !Settings::kanji.listparts, true, l);
        partsmodel->setItems(l);
        l.clear();
        if (kelem >= 0)
            ZKanji::elements()->elementParents(kelem, !Settings::kanji.listparts, true, l);
        partofmodel->setItems(l);
        l.clear();
    }

    ui->similarScroller->update();
    ui->partsScroller->update();
    ui->partofScroller->update();

    QFont radf = Settings::radicalFont();
    radf.setPointSize(10);
    ui->radSymLabel->setFont(radf);

    QSize siz = ui->playButton->iconSize();
    
    ui->loopButton->setIcon(loadColorIcon(":/repeatplaybtn.svg", qApp->palette().color(QPalette::Text), siz));
    ui->playButton->setIcon(loadColorIcon(":/playbtn.svg", qApp->palette().color(QPalette::Text), siz));
    ui->pauseButton->setIcon(loadColorIcon(":/playpausebtn.svg", qApp->palette().color(QPalette::Text), siz));
    ui->stopButton->setIcon(loadColorIcon(":/playstopbtn.svg", qApp->palette().color(QPalette::Text), siz));
    ui->rewindButton->setIcon(loadColorIcon(":/playfrwbtn.svg", qApp->palette().color(QPalette::Text), siz));
    ui->backButton->setIcon(loadColorIcon(":/playsteprwbtn.svg", qApp->palette().color(QPalette::Text), siz));
    ui->foreButton->setIcon(loadColorIcon(":/playstepfwdbtn.svg", qApp->palette().color(QPalette::Text), siz));
    //ui->endButton->setIcon(loadColorIcon(":/playffwdbtn.svg", qApp->palette().color(QPalette::Text), siz));
}

void KanjiInfoForm::dictionaryReset()
{
    //Dictionary *d = dict;
    //int kindex = ui->kanjiView->kanjiIndex();
    setKanji(dict, ui->kanjiView->kanjiIndex());
}

void KanjiInfoForm::dictionaryToBeRemoved(int /*index*/, int /*orderindex*/, Dictionary *d)
{
    if (dict == d)
        close();
}

void KanjiInfoForm::dictionaryReplaced(Dictionary *olddict, Dictionary* /*newdict*/, int /*index*/)
{
    if (olddict == dict)
        setKanji(dict, ui->kanjiView->kanjiIndex());
}

void KanjiInfoForm::wordSelChanged()
{
    if (wordsmodel == nullptr || dict == nullptr)
        return;

    int siz = ui->dictWidget->view()->selectionSize();
    int sel = 0;
    for (int ix = 0; ix != siz; ++ix)
    {
        int row = ui->dictWidget->view()->selectedRow(ix);
        if (dict->isKanjiExample(ui->kanjiView->kanjiIndex(), ui->dictWidget->wordIndex(row)))
            ++sel;
    }

    exampleSelButton->setChecked(siz != 0 && siz == sel);
}

QIcon KanjiInfoForm::loadColorIcon(QString name, QColor col, QSize siz)
{
    return QIcon(QPixmap::fromImage(loadColorSVG(name, siz, qRgb(0, 0, 0), col)));
}

void KanjiInfoForm::showContextMenu(QPoint pos)
{
    QMenu menu;
    QAction *a;
    a = menu.addAction(tr("Copy kanji"));
    connect(a, &QAction::triggered, this, &KanjiInfoForm::copy);
    a->setEnabled(ui->kanjiView->kanjiIndex() >= 0);
    a = menu.addAction(tr("Append kanji"));
    connect(a, &QAction::triggered, this, &KanjiInfoForm::append);
    a->setEnabled(ui->kanjiView->kanjiIndex() >= 0);
    menu.addSeparator();
    a = menu.addAction(tr("Copy kanji data"));
    connect(a, &QAction::triggered, this, &KanjiInfoForm::copyData);
    a->setEnabled(ui->kanjiView->kanjiIndex() >= 0);
    menu.addSeparator();
    a = menu.addAction(tr("Add kanji to group..."));
    connect(a, &QAction::triggered, this, &KanjiInfoForm::addToGroup);
    a->setEnabled(ui->kanjiView->kanjiIndex() >= 0);
    menu.addSeparator();
    QMenu *sub = menu.addMenu(tr("Panels"));
    a = sub->addAction(tr("Words"));
    a->setCheckable(true);
    a->setChecked(ui->wordsButton->isChecked());
    connect(a, &QAction::triggered, this, [this]() {
        ui->wordsButton->toggle();
        on_wordsButton_clicked(ui->wordsButton->isChecked());
    });

    sub->addSeparator();
    a = sub->addAction(tr("Similar kanji"));
    a->setCheckable(true);
    a->setEnabled(!ui->wordsButton->isChecked());
    if (a->isEnabled())
    {
        a->setCheckable(true);
        a->setChecked(ui->simButton->isChecked());
        connect(a, &QAction::triggered, this, [this]() {
            ui->simButton->toggle();
            on_simButton_clicked(ui->simButton->isChecked());
        });
    }
    a = sub->addAction(tr("Kanji parts"));
    a->setCheckable(true);
    a->setEnabled(!ui->wordsButton->isChecked());
    if (a->isEnabled())
    {
        a->setCheckable(true);
        a->setChecked(ui->partsButton->isChecked());
        connect(a, &QAction::triggered, this, [this]() {
            ui->partsButton->toggle();
            on_partsButton_clicked(ui->partsButton->isChecked());
        });
    }
    a = sub->addAction(tr("Part of"));
    a->setCheckable(true);
    a->setEnabled(!ui->wordsButton->isChecked());
    if (a->isEnabled())
    {
        a->setCheckable(true);
        a->setChecked(ui->partofButton->isChecked());
        connect(a, &QAction::triggered, this, [this]() {
            ui->partofButton->toggle();
            on_partofButton_clicked(ui->partofButton->isChecked());
        });
    }
    menu.addSeparator();
    a = menu.addAction(tr("Background grid"));
    a->setCheckable(true);
    a->setChecked(ui->gridButton->isChecked());
    connect(a, &QAction::triggered, this, [this]() {
        ui->gridButton->toggle();
        on_gridButton_clicked(ui->gridButton->isChecked());
    });
    a = menu.addAction(tr("Stroke order diagram"));
    a->setCheckable(true);
    a->setChecked(ui->sodButton->isChecked());
    a->setEnabled(ui->sodButton->isEnabled());
    if (a->isEnabled())
    {
        connect(a, &QAction::triggered, this, [this]() {
            ui->sodButton->toggle();
            on_sodButton_clicked(ui->sodButton->isChecked());
        });
        if (ui->sodButton->isChecked())
        {
            menu.addSeparator();
            sub = menu.addMenu(tr("Diagram options"));
            a = sub->addAction(tr("Indicator shadows"));
            a->setCheckable(true);
            a->setChecked(ui->shadowButton->isChecked());
            connect(a, &QAction::triggered, this, [this]() {
                ui->shadowButton->toggle();
                on_shadowButton_clicked(ui->shadowButton->isChecked());
            });

            a = sub->addAction(tr("Order numbers"));
            a->setCheckable(true);
            a->setChecked(ui->numberButton->isChecked());
            connect(a, &QAction::triggered, this, [this]() {
                ui->numberButton->toggle();
                on_numberButton_clicked(ui->numberButton->isChecked());
            });

            a = sub->addAction(tr("Starting points"));
            a->setCheckable(true);
            a->setChecked(ui->dirButton->isChecked());
            connect(a, &QAction::triggered, this, [this]() {
                ui->dirButton->toggle();
                on_dirButton_clicked(ui->dirButton->isChecked());
            });

            sub->addSeparator();
            //sub = menu.addMenu(tr("Stroke drawing speed"));
            a = sub->addAction(tr("Slow"));
            a->setCheckable(true);
            a->setChecked(ui->speedSlider->value() == 1);
            connect(a, &QAction::triggered, this, [this]() {
                ui->speedSlider->setValue(1);
                on_speedSlider_valueChanged(1);
            });
            a = sub->addAction(tr("Normal"));
            a->setCheckable(true);
            a->setChecked(ui->speedSlider->value() == 2);
            connect(a, &QAction::triggered, this, [this]() {
                ui->speedSlider->setValue(2);
                on_speedSlider_valueChanged(2);
            });
            a = sub->addAction(tr("Fast"));
            a->setCheckable(true);
            a->setChecked(ui->speedSlider->value() == 3);
            connect(a, &QAction::triggered, this, [this]() {
                ui->speedSlider->setValue(3);
                on_speedSlider_valueChanged(3);
            });
            a = sub->addAction(tr("Fastest"));
            a->setCheckable(true);
            a->setChecked(ui->speedSlider->value() == 4);
            connect(a, &QAction::triggered, this, [this]() {
                ui->speedSlider->setValue(4);
                on_speedSlider_valueChanged(4);
            });
        }
    }

    menu.exec(pos);
}


//-------------------------------------------------------------
