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
#include "formstate.h"
#include "generalsettings.h"

// Event posted when deferring the resize of controls in the info form.
ZEVENT(InfoResizeEvent);


//-------------------------------------------------------------


KanjiInfoForm::KanjiInfoForm(QWidget *parent) : base(parent), ui(new Ui::KanjiInfoForm), dict(nullptr), ignoreresize(false), dicth(-1),
        simmodel(new SimilarKanjiScrollerModel), partsmodel(new KanjiScrollerModel), partofmodel(new KanjiScrollerModel)
{
    ui->setupUi(this);
    windowInit();

    gUI->preventWidgetScale(ui->countLabel);

    if (parent != nullptr && !parent->windowFlags().testFlag(Qt::WindowStaysOnTopHint))
        setStayOnTop(false);

    gUI->scaleWidget(this);

    setAttribute(Qt::WA_DeleteOnClose);

    connect(ui->kanjiView, &ZKanjiDiagram::strokeChanged, this, &KanjiInfoForm::strokeChanged);

    readingFilterButton = new QToolButton(this);
    readingFilterButton->setText("ReF");
    readingFilterButton->setCheckable(true);
    ui->dictWidget->addBackWidget(readingFilterButton);

    connect(readingFilterButton, &QToolButton::clicked, this, &KanjiInfoForm::readingFilterClicked);

    exampleSelButton = new QToolButton(this);
    exampleSelButton->setText("Sel");
    ui->dictWidget->addBackWidget(exampleSelButton);

    connect(exampleSelButton, &QToolButton::clicked, this, &KanjiInfoForm::exampleSelButtonClicked);

    ui->dictWidget->setSelectionType(ListSelectionType::Extended);
    ui->dictWidget->view()->setGroupDisplay(true);

    readingCBox = new QComboBox(this);
    readingCBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    readingCBox->setMinimumContentsLength(6);
    ui->dictWidget->addBackWidget(readingCBox);
    
    connect(readingCBox, SIGNAL(currentIndexChanged(int)), this, SLOT(readingBoxChanged(int)));

    updateGeometry();
    setAttribute(Qt::WA_DontShowOnScreen, true);
    show();
    ignoreresize = true;
    hide();
    ignoreresize = false;
    setAttribute(Qt::WA_DontShowOnScreen, false);

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

    //ZFlowLayout *flow = new ZFlowLayout(ui->refWidget);
    //flow->setContentsMargins(0, 0, 0, 0);
    //flow->setVerticalSpacing(4);
    //flow->setHorizontalSpacing(8);
    //ui->refWidget->setLayout(flow);
    //ui->refWidget->hide();

    ui->similarWidget->hide();
    ui->partsWidget->hide();
    ui->partofWidget->hide();
    ui->dictWidget->hide();

    ui->dictWidget->setExamplesVisible(false);
    ui->dictWidget->setInflButtonVisible(false);
    ui->dictWidget->setListMode(DictionaryWidget::Filtered);

    //setBackgroundRole(QPalette::Base);
    //ui->centralwidget->setAutoFillBackground(true);
    //ui->sodWidget->setBackgroundRole(QPalette::Base);
    //ui->sodWidget->setAutoFillBackground(true);

    //ui->similarWidget->setBackgroundRole(QPalette::Base);
    //ui->partsWidget->setBackgroundRole(QPalette::Base);
    //ui->partofWidget->setBackgroundRole(QPalette::Base);
    //ui->similarWidget->setAutoFillBackground(true);
    //ui->partsWidget->setAutoFillBackground(true);
    //ui->partofWidget->setAutoFillBackground(true);

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

    //ui->similarScroller->model()->setBgColor(Settings::uiColor(ColorSettings::SimilarBg));
    //ui->partsScroller->model()->setBgColor(Settings::uiColor(ColorSettings::PartsBg));
    //ui->partofScroller->model()->setBgColor(Settings::uiColor(ColorSettings::PartOfBg));

    //ui->similarWidget->setStyleSheet(QString("background-color: %1").arg((Settings::uiColor(ColorSettings::SimilarBg)).name()));
    //ui->partsWidget->setStyleSheet(QString("background-color: %1").arg((Settings::uiColor(ColorSettings::PartsBg)).name()));
    //ui->partofWidget->setStyleSheet(QString("background-color: %1").arg((Settings::uiColor(ColorSettings::PartOfBg)).name()));

    ui->infoText->setCursorWidth(0);

    installGrabWidget(ui->kanjiView);
    installGrabWidget(ui->toolbarWidget);

    addCloseButton(ui->captionLayout);

    connect(gUI, &GlobalUI::settingsChanged, this, &KanjiInfoForm::settingsChanged);
    connect(ui->similarScroller, &ZItemScroller::itemClicked, this, &KanjiInfoForm::scrollerClicked);
    connect(ui->partsScroller, &ZItemScroller::itemClicked, this, &KanjiInfoForm::scrollerClicked);
    connect(ui->partofScroller, &ZItemScroller::itemClicked, this, &KanjiInfoForm::scrollerClicked);

    settingsChanged();
}

KanjiInfoForm::~KanjiInfoForm()
{
    delete ui;
}

void KanjiInfoForm::saveXMLSettings(QXmlStreamWriter &writer)
{
    KanjiInfoFormData data;
    saveState(data);
    FormStates::saveXMLSettings(data, writer);
}

bool KanjiInfoForm::loadXMLSettings(QXmlStreamReader &reader)
{
    KanjiInfoFormData data;
    FormStates::loadXMLSettings(data, reader);
    return restoreState(data);
}

void KanjiInfoForm::saveState(KanjiInfoData &data) const
{
    QRect g = geometry();
    data.siz = isMaximized() ? normalGeometry().size() : g.size();
    data.pos = g.topLeft();
    data.screenpos = qApp->desktop()->screenGeometry(this).topLeft();

    int h = data.siz.height();
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
    data.siz.setHeight(h);

    data.sod = ui->sodButton->isChecked();
    data.grid = ui->gridButton->isChecked();
    data.shadow = ui->shadowButton->isChecked();
    data.numbers = ui->numberButton->isChecked();
    data.dir = ui->dirButton->isChecked();
    data.speed = ui->speedSlider->value();
    data.similar = ui->simButton->isChecked();
    data.parts = ui->partsButton->isChecked();
    data.partof = ui->partofButton->isChecked();
    data.words = ui->wordsButton->isChecked();
    ui->dictWidget->saveState(data.dict);
    data.refdict = readingFilterButton->isChecked();

    data.toph = ui->dictWidget->isVisibleTo(ui->dictWidget->parentWidget()) ? ui->splitter->sizes().at(0) : ui->dataWidget->height();
    data.dicth = dh;
}

void KanjiInfoForm::restoreState(const KanjiInfoData &data)
{
    ignoreresize = true;

    int kindex = ui->kanjiView->kanjiIndex();

    ui->sodButton->setChecked(ZKanji::elements()->size() != 0 && data.sod);
    ui->sodButton->setEnabled(ZKanji::elements()->size() != 0 && kindex >= 0);
    ui->gridButton->setChecked(data.grid);
    ui->shadowButton->setChecked(data.shadow);
    ui->numberButton->setChecked(data.numbers);
    ui->dirButton->setChecked(data.dir);
    ui->speedSlider->setValue(data.speed);
    ui->simButton->setChecked(data.similar);
    ui->partsButton->setChecked(ZKanji::elements()->size() != 0 && data.parts);
    ui->partofButton->setChecked(ZKanji::elements()->size() != 0 && data.partof);
    ui->simButton->setEnabled(kindex < 0 || !data.words);
    ui->partsButton->setEnabled(ZKanji::elements()->size() != 0 && (kindex < 0 || !data.words));
    ui->partofButton->setEnabled(ZKanji::elements()->size() != 0 && (kindex < 0 || !data.words));
    ui->wordsButton->setChecked(data.words);
    ui->wordsButton->setEnabled(kindex >= 0);
    readingFilterButton->setChecked(data.refdict);
    ui->dictWidget->restoreState(data.dict);

    ui->kanjiView->setDiagram(kindex < 0 || data.sod);
    ui->kanjiView->setGrid(data.grid);
    ui->kanjiView->setShadow(data.shadow);
    ui->kanjiView->setNumbers(data.numbers);
    ui->kanjiView->setDirection(data.dir);
    ui->sodWidget->setVisible(kindex < 0 || data.sod);
    ui->similarWidget->setVisible((kindex < 0 || !data.words) && data.similar);
    ui->partsWidget->setVisible(ZKanji::elements()->size() != 0 && (kindex < 0 || !data.words) && data.parts);
    ui->partofWidget->setVisible(ZKanji::elements()->size() != 0 && (kindex < 0 || !data.words) && data.partof);
    ui->dictWidget->setVisible(kindex >= 0 && data.words);

    ui->kanjiView->setSpeed(data.speed);

    if (kindex >= 0 && data.words)
    {
        disconnect(ui->splitter, &QSplitter::splitterMoved, this, &KanjiInfoForm::restrictKanjiViewSize);
        if (!wordsmodel)
            wordsmodel.reset(new KanjiWordsItemModel(this));
        wordsmodel->setKanji(dict, kindex, readingCBox->currentIndex() - 1);
        ui->dictWidget->setModel(wordsmodel.get());
        connect(ui->splitter, &QSplitter::splitterMoved, this, &KanjiInfoForm::restrictKanjiViewSize);
    }

    if (wordsmodel != nullptr)
        wordsmodel->setShowOnlyExamples(readingFilterButton->isChecked());

    ui->centralwidget->layout()->activate();
    layout()->activate();

    ui->centralwidget->layout()->invalidate();
    layout()->invalidate();
    ui->centralwidget->layout()->update();
    layout()->update();


    if (data.dicth == -1)
        dicth = ui->dataWidget->minimumSizeHint().height();
    else
        dicth = data.dicth;

    ui->splitter->setSizes({ data.toph, dicth });

    QRect geom = QRect(data.pos, data.siz);
    if (geom.size().isValid())
    {
        int h = geom.height();
        if (ui->sodWidget->isVisibleTo(this))
        {
            ui->sodWidget->adjustSize();
            h += ui->sodWidget->height() + ui->sodWidget->parentWidget()->layout()->spacing();
        }
        if (ui->similarWidget->isVisibleTo(this))
        {
            ui->similarWidget->adjustSize();
            h += ui->similarWidget->height() + ui->similarWidget->parentWidget()->layout()->spacing();
        }
        if (ui->partsWidget->isVisibleTo(this))
        {
            ui->partsWidget->adjustSize();
            h += ui->partsWidget->height() + ui->partsWidget->parentWidget()->layout()->spacing();
        }
        if (ui->partofWidget->isVisibleTo(this))
        {
            ui->partofWidget->adjustSize();
            h += ui->partofWidget->height() + ui->partofWidget->parentWidget()->layout()->spacing();
        }
        if (ui->dictWidget->isVisibleTo(this))
        {
            ui->dictWidget->adjustSize();
            h += dicth + ui->splitter->handleWidth();
        }
        geom.setHeight(h);
        //resize(siz);
    }
    else
        geom.setSize(geometry().size());

    int screennum = data.pos.isNull() ? -1 : screenNumber(QRect(data.pos, geom.size()));
    QRect sg;
    if (screennum == -1)
    {
        screennum = qApp->desktop()->screenNumber((QWidget*)gUI->mainForm());
        sg = qApp->desktop()->screenGeometry(screennum);
        if (!data.screenpos.isNull() && !data.pos.isNull())
            geom.moveTo(sg.topLeft() + (data.pos - data.screenpos));
        else
            geom.moveTo(sg.center() - QPoint(geom.width() / 2, geom.height() / 2));
    }
    else
        sg = qApp->desktop()->screenGeometry(screennum);

    // Geometry might be out of bounds. Move window within bounds.

    if (geom.left() < sg.left())
        geom.moveLeft(sg.left());
    else if (geom.left() + geom.width() > sg.left() + sg.width())
        geom.moveLeft(sg.left() + sg.width() - geom.width());
    if (geom.top() < sg.top())
        geom.moveTop(sg.top());
    else if (geom.top() + geom.height() > sg.top() + sg.height())
        geom.moveTop(sg.top() + sg.height() - geom.height());
    if (geom.width() > std::min(sg.width(), sg.height()))
        geom.setWidth(std::min(sg.width(), sg.height()));
    if (geom.height() > std::min(sg.height(), sg.height()))
        geom.setHeight(std::min(sg.height(), sg.height()));

    move(geom.topLeft());
    resize(geom.size());

    ignoreresize = false;
    _resized(true);
}

void KanjiInfoForm::saveState(KanjiInfoFormData &data) const
{
    data.kindex = ui->kanjiView->kanjiIndex();
    data.dictname = dict == nullptr ? QString() : dict->name();
    data.locked = ui->lockButton->isChecked();
    saveState(data.data);
}

bool KanjiInfoForm::restoreState(const KanjiInfoFormData &data)
{
    ignoreresize = true;
    int dix = ZKanji::dictionaryIndex(data.dictname);
    if (dix != -1)
        setKanji(ZKanji::dictionary(dix), data.kindex);
    ui->lockButton->setChecked(data.locked);
    ui->lockButton->setEnabled(!ui->lockButton->isChecked());
    ignoreresize = false;

    restoreState(data.data);

    return dix != -1;
}

namespace ZKanji
{
    // Same as the radsymbols in ZRadicalGrid, without the added stroke count.
    extern QChar radsymbols[231];
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
            for (int ix = 0; ix != arr.size(); ++ix)
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
        rl.push_back(tr("-"));
        rl.push_back(tr("Irreg."));
        for (int ix = 0; ix != k->on.size(); ++ix)
            rl.push_back(k->on[ix].toQString());

        if (!k->kun.empty())
        {
            const QChar *w = k->kun[0].data();
            const QChar *p = qcharchr(w, '.');

            if (!p)
                p = qcharchr(w, 0);

            rl.push_back(k->kun[0].toQString(0, p - w));

            for (int ix = 1; ix != k->kun.size(); ++ix)
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

        setAttribute(Qt::WA_DontShowOnScreen);
        show();
        hide();
        setAttribute(Qt::WA_DontShowOnScreen, false);
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
    if (e->type() == QEvent::WindowActivate || e->type() == QEvent::WindowDeactivate)
    {
        int oldpos = ui->infoText->verticalScrollBar()->value();
        ui->infoText->setPlainText(QString());
        ui->infoText->document()->setDefaultStyleSheet(QString());
        Settings::updatePalette(ui->infoText);
        ui->infoText->document()->setDefaultStyleSheet(QString("body { font-size: %1pt; color: %2; }").arg(Settings::scaled(10)).arg(Settings::textColor(this, ColorSettings::Text).name()));
        ui->infoText->document()->setHtml(ZKanji::kanjiInfoText(dict, ui->kanjiView->kanjiIndex()));
        ui->infoText->verticalScrollBar()->setValue(oldpos);
    }

    return base::event(e);
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

void KanjiInfoForm::on_endButton_clicked()
{
    ui->kanjiView->stop();
}

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
    int h = ui->similarWidget->height() + ui->similarWidget->parentWidget()->layout()->spacing();

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
    int h = ui->partsWidget->height() + ui->partsWidget->parentWidget()->layout()->spacing();

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
    int h = ui->partofWidget->height() + ui->partofWidget->parentWidget()->layout()->spacing();

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

    int h = (!checked && ui->simButton->isChecked()) || ui->similarWidget->isVisible() ? (ui->similarWidget->height() + ui->similarWidget->parentWidget()->layout()->spacing()) : 0;
    h += (!checked && ui->partsButton->isChecked()) || ui->partsWidget->isVisible() ? (ui->partsWidget->height() + ui->partsWidget->parentWidget()->layout()->spacing()) : 0;
    h += (!checked && ui->partofButton->isChecked()) || ui->partofWidget->isVisible() ? (ui->partofWidget->height() + ui->partofWidget->parentWidget()->layout()->spacing()) : 0;

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
        ui->stopButton->setChecked(true);
    else if (!ui->kanjiView->playing())
        ui->pauseButton->setChecked(true);
}

//void testFuriganaReadingTest();
void KanjiInfoForm::readingFilterClicked(bool checked)
{
    //testFuriganaReadingTest();

    if (wordsmodel != nullptr)
        wordsmodel->setShowOnlyExamples(checked);
}

void KanjiInfoForm::readingBoxChanged(int index)
{
    if (wordsmodel != nullptr && ui->kanjiView->kanjiIndex() >= 0)
        wordsmodel->setKanji(dict, ui->kanjiView->kanjiIndex(), readingCBox->currentIndex() - 1);
}

void KanjiInfoForm::exampleSelButtonClicked(bool checked)
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

    loadColorIcon(ui->playButton, ":/playbtn.svg", qApp->palette().color(QPalette::Text));
    loadColorIcon(ui->pauseButton, ":/playpausebtn.svg", qApp->palette().color(QPalette::Text));
    loadColorIcon(ui->stopButton, ":/playstopbtn.svg", qApp->palette().color(QPalette::Text));
    loadColorIcon(ui->rewindButton, ":/playfrwbtn.svg", qApp->palette().color(QPalette::Text));
    loadColorIcon(ui->backButton, ":/playsteprwbtn.svg", qApp->palette().color(QPalette::Text));
    loadColorIcon(ui->foreButton, ":/playstepfwdbtn.svg", qApp->palette().color(QPalette::Text));
    loadColorIcon(ui->endButton, ":/playffwdbtn.svg", qApp->palette().color(QPalette::Text));
}

void KanjiInfoForm::dictionaryReset()
{
    Dictionary *d = dict;
    int kindex = ui->kanjiView->kanjiIndex();
    setKanji(d, kindex);
}

void KanjiInfoForm::dictionaryToBeRemoved(int index, int orderindex, Dictionary *d)
{
    if (d == dict)
        close();
}

void KanjiInfoForm::loadColorIcon(QAbstractButton *w, QString name, QColor col)
{
    QFile file(name);
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    QString str = stream.readAll();
    file.close();
    str.replace(QLatin1String("#000000"), QString("%1").arg(col.name()));

    QSvgRenderer r;
    r.load(str.toUtf8());

    QImage img = QImage(w->iconSize().width(), w->iconSize().height(), QImage::Format_ARGB32_Premultiplied);
    img.fill(qRgba(0, 0, 0, 0));

    QPainter p(&img);
    r.render(&p, QRect(0, 0, w->iconSize().width(), w->iconSize().height()));
    p.end();

    
    QIcon ico(QPixmap::fromImage(img));
    w->setIcon(ico);
}


//-------------------------------------------------------------
