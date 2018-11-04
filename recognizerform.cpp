/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <qsizepolicy.h>
#include <QtEvents>
#include <QStylePainter>
#include <qfont.h>
#include <QDesktopWidget>
#include <QScreen>
#include <QWindow>

#include <cmath>

#include "recognizerform.h"
#include "ui_recognizer.h"
#include "zui.h"
#include "kanjistrokes.h"
#include "zevents.h"
#include "zkanalineedit.h"
#include "zcombobox.h"
#include "kanji.h"
#include "fontsettings.h"
#include "recognizersettings.h"
#include "colorsettings.h"
#include "globalui.h"
#include "generalsettings.h"
#include "formstates.h"


//-------------------------------------------------------------


class RecognizerPopupEvent : public EventTBase<RecognizerPopupEvent>
{
private:
    typedef EventTBase<RecognizerPopupEvent>    base;
public:
    RecognizerPopupEvent(QToolButton *btn, ZKanaLineEdit *edt, RecognizerPosition pos) : base(), btn(btn), edt(edt), pos(pos) {}

    QToolButton *button()
    {
        return btn;
    }

    ZKanaLineEdit *edit()
    {
        return edt;
    }

    RecognizerPosition position()
    {
        return pos;
    }
private:
    QToolButton *btn;
    ZKanaLineEdit *edt;
    RecognizerPosition pos;
};

ZEVENT(RecognizerHiddenEvent)


//-------------------------------------------------------------


RecognizerArea::RecognizerArea(QWidget *parent) : base(parent), grid(true), match(Kanji | Kana | Other), recalc(true), drawing(false), strokepos(0)
{
    //setBackgroundRole(QPalette::Base);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent);

    //connect(qApp, &QGuiApplication::focusWindowChanged, this, static_cast<void(QWidget::*)(void)>(&QWidget::update));
}

RecognizerArea::~RecognizerArea()
{

}

bool RecognizerArea::empty() const
{
    return strokepos == 0 || strokes.empty();
}

bool RecognizerArea::hasNext() const
{
    return strokepos == tosigned(strokes.size());
}

bool RecognizerArea::showGrid()
{
    return grid;
}

void RecognizerArea::setShowGrid(bool showgrid)
{
    if (grid == showgrid)
        return;
    grid = showgrid;
    update();
}

RecognizerArea::CharacterMatches RecognizerArea::characterMatch()
{
    return match;
}

void RecognizerArea::setCharacterMatch(CharacterMatches charmatch)
{
    if (charmatch == match)
        return;
    match = charmatch;
    updateCandidates();
}

void RecognizerArea::prev()
{
    if (strokepos == 0)
        return;

    --strokepos;
    updateCandidates();
}

void RecognizerArea::next()
{
    if (strokepos >= tosigned(strokes.size()))
        return;

    ++strokepos;
    updateCandidates();
}

void RecognizerArea::clear()
{
    strokes.clear();
    strokepos = 0;
    updateCandidates();
}

void RecognizerArea::resizeEvent(QResizeEvent * /*event*/)
{
    recalc = true;
    update();
}

void RecognizerArea::paintEvent(QPaintEvent *event)
{
    // Find a sensible point size for the text to draw.
    recalculate();

    QStylePainter p(this);

    p.fillRect(rect(), Settings::textColor(this, ColorSettings::Bg));
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(area.left() + ((area.width() % 2) ? 0. : 0.5), area.top() + ((area.width() % 2) ? 0. : 0.5));

    //QStyleOptionViewItem opts;
    //opts.initFrom(this);
    //opts.showDecorationSelected = true;
    //int gridHint = p.style()->styleHint(QStyle::SH_Table_GridLineColor, &opts, this);
    QColor gridcolor = Settings::uiColor(ColorSettings::Grid);
    QColor textcolor = mixColors(Settings::textColor(this, ColorSettings::Bg), Settings::textColor(this, ColorSettings::Text), 0.7);

    double hx = area.width() / 20.;

    p.setPen(gridcolor);

    if (grid)
    {
        // Grid borders.
        p.drawLine(QLineF(hx, hx * 2, area.width() - hx, hx * 2));
        p.drawLine(QLineF(hx, area.height() - hx * 2, area.width() - hx, area.height() - hx * 2));
        p.drawLine(QLineF(hx * 2, hx, hx * 2, area.height() - hx));
        p.drawLine(QLineF(area.width() - hx * 2, hx, area.width() - hx * 2, area.height() - hx));

        // Inner lines.
        p.drawLine(QLineF(hx, area.height() / 2., hx * 2, area.height() / 2.));
        p.drawLine(QLineF(area.width() - hx * 2, area.height() / 2., area.width() - hx, area.height() / 2.));

        p.drawLine(QLineF(area.width() / 2., hx, area.width() / 2., hx * 2.));
        p.drawLine(QLineF(area.width() / 2., area.height() - hx * 2., area.width() / 2., area.height() - hx));

        p.drawLine(QLineF(hx * 6, area.height() / 2., area.width() - hx * 6, area.height() / 2.));
        p.drawLine(QLineF(area.width() / 2., hx * 6, area.width() / 2., area.height() - hx * 6));
    }

    // Instruction text when the recognizer is empty.
    if (!drawing && strokes.empty())
    {
        p.setPen(textcolor);
        p.setFont(QFont(Settings::fonts.main, ptsize));
        p.drawText(QRectF(hx * 3, hx * 3, area.width() - hx * 6, area.height() - hx * 6), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, instructions());
    }

    p.setPen(Settings::textColor(this, ColorSettings::Text));
    // Paint the strokes already in drawn.
    for (int ix = 0, siz = std::min(tosigned(strokes.size()), strokepos); ix != siz; ++ix)
        paintStroke(ix, p, Settings::textColor(this, ColorSettings::Text), event->rect());

    // Paint the currently drawn stroke.
    if (newstroke.size() > 1)
        paintStroke(-1, p, textcolor, event->rect());
}

void RecognizerArea::mousePressEvent(QMouseEvent *event)
{
    //base::mousePressEvent(event);

    if (!drawing && event->button() == Qt::RightButton)
    {
        clear();
        return;
    }

    if (event->button() != Qt::LeftButton)
        return;

    draw(event->pos());
}

void RecognizerArea::mouseReleaseEvent(QMouseEvent *event)
{
    //base::mouseReleaseEvent(event);

    if (event->button() != Qt::LeftButton)
        return;

    draw(event->pos());
    stopDrawing();
}

void RecognizerArea::mouseMoveEvent(QMouseEvent *event)
{
    //base::mouseMoveEvent(event);

    if (!drawing)
        return;
    draw(event->pos());
}

void RecognizerArea::draw(QPointF pt)
{
    recalculate();

    if (!drawing)
    {
        drawing = true;
        update();
    }

    newstroke.add(areaToUnit(pt - area.topLeft()));

    if (newstroke.size() == 1)
        return;

    QRectF sr = segmentRect(-1, -1);
    int rleft = (int)std::floor(sr.left() * area.width()) + area.left();
    int rtop = (int)std::floor(sr.top() * area.height()) + area.top();
    int rright = (int)std::ceil(sr.right() * area.width()) + area.left();
    int rbottom = (int)std::ceil(sr.bottom() * area.height()) + area.top();

    QRect r(rleft, rtop, rright - rleft, rbottom - rtop);
    update(r);
}

void RecognizerArea::stopDrawing()
{
    if (!drawing)
        return;

    if (!newstroke.empty())
    {
        if (newstroke.size() == 1)
        {
            QPointF pt = newstroke[0];
            pt.setX(pt.x() + 0.0005);
            pt.setY(pt.y() + 0.0005);
            newstroke.add(pt);
        }

        strokes.resize(strokepos);
        strokes.add(std::move(newstroke), true);
        ++strokepos;
    }

    newstroke.clear();
    drawing = false;

    updateCandidates();
}

void RecognizerArea::updateCandidates()
{
    std::vector<int> l;
    if (strokepos != 0)
        ZKanji::elements()->findCandidates(strokes, l, strokepos, match.testFlag(Kanji), match.testFlag(Kana), match.testFlag(Other));

    emit changed(l);

    recalculate();
    update();
}

void RecognizerArea::paintStroke(int index, QStylePainter &p, QColor color, const QRect &clip)
{
    for (int ix = 0; ix != segmentCount(index); ++ix)
    {
        QRectF sr = segmentRect(index, ix);
        int rleft = (int)std::floor(sr.left() * area.width()) + area.left();
        int rtop = (int)std::floor(sr.top() * area.height()) + area.top();
        int rright = (int)std::ceil(sr.right() * area.width()) + area.left();
        int rbottom = (int)std::ceil(sr.bottom() * area.height()) + area.top();
        QRect dr(rleft, rtop, rright - rleft, rbottom - rtop);
        if (!clip.intersects(dr))
            continue;

        p.setPen(QPen(QBrush(color), segmentWidth(index, ix) * area.width(), Qt::SolidLine, Qt::RoundCap));
        QLineF line = segmentLine(index, ix);
        line.setPoints(line.p1() * area.width(), line.p2() * area.height());
        p.drawLine(line);
    }
}

int RecognizerArea::segmentCount(int index)
{
    if (index == -1)
        return std::max(0, tosigned(newstroke.size()) - 1);
    return std::max(0, tosigned(strokes[index].size()) - 1);
}

double RecognizerArea::segmentLength(int index, int seg)
{
    if (seg == -1)
        seg = segmentCount(index) - 1;

#ifdef _DEBUG
    if (segmentCount(index) <= seg)
        throw "At least 2 points required.";
#endif

    if (index == -1)
        return newstroke.segmentLength(seg); // RecMath::len(newstroke[seg], newstroke[seg + 1]);
    return strokes[index].segmentLength(seg); // RecMath::len(strokes[index][seg], strokes[index][seg + 1]);
}

QLineF RecognizerArea::segmentLine(int index, int seg)
{
    if (seg == -1)
        seg = segmentCount(index) - 1;

#ifdef _DEBUG
    if (segmentCount(index) <= seg)
        throw "At least 2 points required.";
#endif

    if (index == -1)
        return QLineF(newstroke[seg], newstroke[seg + 1]);
    return QLineF(strokes[index][seg], strokes[index][seg + 1]);
}

double RecognizerArea::segmentWidth(int index, int seg)
{
    if (index != -1)
        return 0.02;

    if (seg == -1)
        seg = tosigned(newstroke.size()) - 2;

#ifdef _DEBUG
    if (tosigned(newstroke.size()) < seg + 2)
        throw "At least 2 points required.";
#endif

    int cnt = std::min(seg + 1, 15);

    // Sum of the length of the last 15 segments. If less than 15 segments are
    // available, the first segment is used to fake the rest.
    double lensum = (15 - cnt) * segmentLength(-1, 0);

    for (int ix = 0; ix != cnt; ++ix)
        lensum += segmentLength(-1, seg - ix);

    // Average of the segment widths of the past 15 segments. If less than 15
    // segments are available, the first segment is used to fake the rest.
    double segw = ((15 - cnt) * 0.015) * (segmentLength(-1, 0) / lensum);

    for (int ix = cnt - 1; ix != -1; --ix)
    {
        double w = std::max(0.015, std::min(0.05, 0.00015 / segmentLength(-1, seg - ix)));
        segw += w * (segmentLength(-1, seg - ix) / lensum);
    }

    return segw;
}

QRectF RecognizerArea::segmentRect(int index, int seg)
{
    if (seg == -1)
        seg = segmentCount(index) - 1;

#ifdef _DEBUG
    if (segmentCount(index) <= seg)
        throw "At least 2 points required.";
#endif

    QLineF line = segmentLine(index, seg);
    double w = segmentWidth(index, seg) / 2.;


    return QRectF(std::min(line.p1().x(), line.p2().x()) - w, std::min(line.p1().y(), line.p2().y()) - w, w * 2 + std::fabs(line.p1().x() - line.p2().x()), w * 2 + std::fabs(line.p1().y() - line.p2().y()));
}

void RecognizerArea::recalculate()
{
    if (!recalc)
        return;

    // Compute the area.
    int siz = std::min(size().width(), size().height());

    if (area.width() != siz)
    {
        ptsize = 6;

        // Recalculate font size.
        int ptdif = 1;
        int w;
        do
        {
            QFont f(Settings::fonts.main, ptsize);
            QFontMetrics fm(f);

            w = fm.boundingRect(instructions()).width();
            ptsize += ptdif;
            ++ptdif;
        } while (w < siz * 2);
        ptsize -= ptdif - 1;
    }

    //bool w = size().width() >= size().height();
    area = QRect(QPoint((size().width() - siz) / 2, (size().height() - siz) / 2), QSize(siz, siz));

    recalc = false;
}

QString RecognizerArea::instructions()
{
    return tr("Draw a Japanese character here and select a candidate! (Right-click removes drawing)");
}

QPointF RecognizerArea::areaToUnit(QPointF pt)
{
    return QPointF(pt.x() / area.width(), pt.y() / area.height());
}

QPointF RecognizerArea::unitToArea(QPointF pt)
{
    return QPointF(pt.x() * area.width(), pt.y() * area.height());
}


//-------------------------------------------------------------



RecognizerForm *RecognizerForm::instance = nullptr;
std::map<QToolButton*, std::pair<ZKanaLineEdit*, RecognizerPosition>> RecognizerForm::controls;
std::map<ZKanaLineEdit*, QToolButton*> RecognizerForm::editforbuttons;
CandidateKanjiScrollerModel *RecognizerForm::candidates = nullptr;
QToolButton *RecognizerForm::button = nullptr;
ZKanaLineEdit *RecognizerForm::edit = nullptr;
ZKanaLineEdit *RecognizerForm::connected = nullptr;
RecognizerObject *RecognizerForm::p = nullptr;

RecognizerForm::RecognizerForm(QWidget *parent) : base(parent, Qt::WindowDoesNotAcceptFocus), ui(new Ui::RecognizerForm)
{
#ifdef _DEBUG
    if (instance != nullptr)
        throw "Cannot create two of this form.";
#endif
    instance = this;

    ui->setupUi(this);
    windowInit();

    setAttribute(Qt::WA_QuitOnClose, false);

    gUI->scaleWidget(this);

    connect(ui->closeButton, &QAbstractButton::clicked, this, &RecognizerForm::close);

    if (candidates == nullptr)
        candidates = new CandidateKanjiScrollerModel(qApp);
    ui->candidateScroller->setModel(candidates);
    ui->candidateScroller->setScrollerType(ZScrollArea::Buttons);

    connect(ui->drawArea, &RecognizerArea::changed, this, &RecognizerForm::candidatesChanged);
    connect(ui->candidateScroller, &ZItemScroller::itemClicked, this, &RecognizerForm::candidateClicked);

    connect(qApp, &QApplication::focusChanged, this, &RecognizerForm::appFocusChanged);
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &RecognizerForm::appStateChanged);

    // "Show" once so the window sizes can be retrieved later.
    setAttribute(Qt::WA_DontShowOnScreen);
    show();
    hide();
    setAttribute(Qt::WA_DontShowOnScreen, false);

    ui->prevButton->setEnabled(!ui->drawArea->empty());
    ui->nextButton->setEnabled(!ui->drawArea->hasNext());
    ui->clearButton->setEnabled(!ui->drawArea->empty());

#ifdef Q_OS_WIN
    // This doesn't work on Linux. Only use the Qt::WindowDoesNotAcceptFocus window flag.
    setAttribute(Qt::WA_ShowWithoutActivating);
#endif

    ui->gridButton->setChecked(FormStates::recognizer.showgrid);
    ui->generalButton->setChecked(FormStates::recognizer.allresults);
}

RecognizerForm::~RecognizerForm()
{
    delete ui;
    instance = nullptr;
    RecognizerForm::connected = nullptr;
}

void RecognizerForm::install(QToolButton *btn, ZKanaLineEdit *ledit, RecognizerPosition pos)
{
    if (controls.count(btn) != 0)
        return;

    if (p == nullptr)
        p = new RecognizerObject;

    controls[btn] = std::make_pair(ledit, pos);
    editforbuttons[ledit] = btn;

    btn->setCheckable(true);
    btn->setFocusPolicy(Qt::NoFocus);

    connect(btn, &QToolButton::clicked, p, &RecognizerObject::buttonClicked);
    connect(btn, &QToolButton::destroyed, p, &RecognizerObject::buttonDestroyed);

    ledit->installEventFilter(p);
    if (dynamic_cast<ZKanaComboBox*>(ledit->parent()) != nullptr)
        ledit->parent()->installEventFilter(p);


    if (ledit->isReadOnly() || !ledit->isEnabled())
        btn->setEnabled(false);
}

void RecognizerForm::uninstall(QToolButton *btn)
{
    douninstall(btn, false);
}

void RecognizerForm::clear()
{
    if (instance == nullptr)
        return;
    instance->on_clearButton_clicked();
}

void RecognizerForm::popup(QToolButton *btn)
{
    auto it = controls.find(dynamic_cast<QToolButton*>(btn));
    if (it == controls.end())
        return;

    ZKanaLineEdit *ed = it->second.first;

    if (instance != nullptr && instance->isVisible())
    {
        bool same = (button == btn);
        if (button != nullptr)
            button->setChecked(false);

        if (connected != nullptr && connected != ed)
        {
            disconnect(connected, &ZKanaLineEdit::dictionaryChanged, instance, &RecognizerForm::editorDictionaryChanged);
            connected = nullptr;
        }

        instance->hide();

        edit = nullptr;
        button = nullptr;

        if (same)
        {
            instance->setParent(qApp->desktop());
            return;
        }
    }

    if (button != btn && button != nullptr)
        button->setChecked(false);

    if (instance != nullptr)
        instance->setParent(qApp->desktop());

    button = nullptr;
    edit = nullptr;

    btn->setChecked(true);
    ed->setFocus();

    qApp->postEvent(p, new RecognizerPopupEvent(btn, ed, it->second.second));

    if (it->second.second == RecognizerPosition::StartBelow || it->second.second == RecognizerPosition::StartAbove)
        it->second.second = RecognizerPosition::SavedPos;
}

void RecognizerForm::on_gridButton_toggled(bool checked)
{
    FormStates::recognizer.showgrid = checked;
    ui->drawArea->setShowGrid(checked);
}

void RecognizerForm::on_generalButton_toggled(bool checked)
{
    FormStates::recognizer.allresults = checked;
    ui->drawArea->setCharacterMatch(!checked ? (RecognizerArea::Kanji) : (RecognizerArea::Kanji | RecognizerArea::Kana | RecognizerArea::Other));
}

void RecognizerForm::on_clearButton_clicked()
{
    ui->drawArea->clear();
}

void RecognizerForm::on_prevButton_clicked()
{
    ui->drawArea->prev();
}

void RecognizerForm::on_nextButton_clicked()
{
    ui->drawArea->next();
}

void RecognizerForm::candidatesChanged(const std::vector<int> &l)
{
    std::vector<int> copy;
    copy.reserve(l.size());

    // The model in the candidate scroller can show both elements and characters. Elements
    // must be passed as a negative number starting from -1, while the result received in l
    // holds the element indexes as a positive number, starting at 0. To make the scroller
    // display our characters from element indexes, l must be converted.
    for (int val : l)
        copy.push_back(-val - 1);

    candidates->setItems(copy);
    ui->prevButton->setEnabled(!ui->drawArea->empty());
    ui->nextButton->setEnabled(!ui->drawArea->hasNext());
    ui->clearButton->setEnabled(!ui->drawArea->empty());
}

void RecognizerForm::candidateClicked(int index)
{
    if (!edit->isEnabled())
        return;
    edit->insert(ZKanji::elements()->itemUnicode(-candidates->items(index) - 1));
}

void RecognizerForm::editorDictionaryChanged()
{
    ui->candidateScroller->setDictionary(edit->dictionary());
}

QRect RecognizerForm::resizing(int side, QRect r)
{
    QSize sh = minimumSizeHint();
    if (r.width() < sh.width())
        r.setWidth(sh.width());
    if (r.height() < sh.height())
        r.setHeight(sh.height());

    if (side == (int)GrabSide::None)
        return r;

    QSize sizdif = size() - ui->drawArea->size();
    QSize rectdif = r.size() - sizdif;
    int dif = rectdif.width() - rectdif.height();
    if (dif < 0)
    {
        if (side == (int)GrabSide::Left || side == (int)GrabSide::Right)
        {
            int ldif = 0;
            int rdif = 0;
            if (sh.height() > r.height() + dif)
            {
                if (side == (int)GrabSide::Left)
                    ldif = dif - (sh.height() - r.height());
                else
                    rdif = -dif - (r.height() - sh.height());
                dif = sh.height() - r.height();
            }
            r.adjust(ldif, -dif, rdif, 0);
        }
        else if (side == (int)GrabSide::Top || side == (int)GrabSide::Bottom)
            r.adjust(0, 0, -dif, 0);
        else if ((side & (int)GrabSide::Left) == (int)GrabSide::Left)
            r.adjust(dif, 0, 0, 0);
        else
            r.adjust(0, 0, -dif, 0);
    }
    else if (dif > 0)
    {
        if (side == (int)GrabSide::Top || side == (int)GrabSide::Bottom)
            r.adjust(0, 0, -dif, 0);
        else if (side == (int)GrabSide::Left || side == (int)GrabSide::Right)
            r.adjust(0, -dif, 0, 0);
        else if ((side & (int)GrabSide::Top) == (int)GrabSide::Top)
            r.adjust(0, -dif, 0, 0);
        else
            r.adjust(0, 0, 0, dif);
    }

    return r;
}

QWidget* RecognizerForm::captionWidget() const
{
    return ui->buttonWidget;
}

bool RecognizerForm::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    return base::event(e);
}

void RecognizerForm::hideEvent(QHideEvent *e)
{
    base::hideEvent(e);

    // The attribute is set when the form is first created, see constructor.
    if (!testAttribute(Qt::WA_DontShowOnScreen))
    {
        setParent(qApp->desktop());

        qApp->postEvent(p, new RecognizerHiddenEvent);

        if (connected != nullptr)
        {
            disconnect(connected, &ZKanaLineEdit::dictionaryChanged, RecognizerForm::instance, &RecognizerForm::editorDictionaryChanged);
            connected = nullptr;
        }
    }
}

void RecognizerForm::keyPressEvent(QKeyEvent *e)
{
    base::keyPressEvent(e);

    if (!isVisible())
        return;

    // Shouldn't happen, but in case it does, just hide the window.
    if (edit == nullptr)
    {
        hide();
        return;
    }

    edit->setFocus();
    edit->activateWindow();
    qApp->sendEvent(edit, e);
}

void RecognizerForm::keyReleaseEvent(QKeyEvent *e)
{
    base::keyReleaseEvent(e);

    if (!isVisible())
        return;

    // Shouldn't happen, but in case it does, just hide the window.
    if (edit == nullptr)
    {
        hide();
        return;
    }

    edit->setFocus();
    edit->activateWindow();
    qApp->sendEvent(edit, e);
}

void RecognizerForm::moveEvent(QMoveEvent *e)
{
    savePosition();
    base::moveEvent(e);
}

void RecognizerForm::resizeEvent(QResizeEvent *e)
{
    savePosition();
    base::resizeEvent(e);
}

void RecognizerForm::appFocusChanged(QWidget * /*lost*/, QWidget *received)
{
    if (!isVisible() || received == this || received == edit || received == nullptr || (dynamic_cast<ZKanaComboBox*>(edit->parent()) != nullptr && received == edit->parent()) || (received != nullptr && received->window() == this) || qApp->activeWindow() == this)
        return;
    hide();
}

void RecognizerForm::appStateChanged(Qt::ApplicationState state)
{
    if (isVisible() && state != Qt::ApplicationActive)
        hide();
}

void RecognizerForm::douninstall(QToolButton *btn, bool destroyed)
{
    auto it = controls.find(btn);
    if (it == controls.end())
        return;

    if (btn == button && instance != nullptr && instance->isVisible())
        instance->hide();

    if (btn == button)
    {
        if (instance != nullptr)
            instance->setParent(qApp->desktop());

        if (!destroyed && button != nullptr)
            button->setChecked(false);

        if (!destroyed && connected == it->second.first)
        {
            disconnect(connected, &ZKanaLineEdit::dictionaryChanged, instance, &RecognizerForm::editorDictionaryChanged);
            connected = nullptr;
        }

        button = nullptr;
        edit = nullptr;
    }

    ZKanaLineEdit *ed = it->second.first;
    controls.erase(it);
    editforbuttons.erase(editforbuttons.find(ed));


    if (!destroyed)
    {
        ed->removeEventFilter(p);
        if (dynamic_cast<ZKanaComboBox*>(ed->parent()) != nullptr)
            ed->parent()->removeEventFilter(p);

        btn->setCheckable(false);
        disconnect(btn, nullptr, p, nullptr);
    }
}

void RecognizerForm::savePosition()
{
    if (button == nullptr || !isVisible())
        return;

    //auto it = controls.find(dynamic_cast<QToolButton*>(button));
    //if (it == controls.end())
    //    return;

    FormStates::recognizer.rect = geometry();
    FormStates::recognizer.screen = qApp->desktop()->screenGeometry(this);
}


//-------------------------------------------------------------


bool RecognizerObject::event(QEvent *e)
{
    if (e->type() == RecognizerPopupEvent::Type())
    {
        QToolButton *btn = ((RecognizerPopupEvent*)e)->button();
        ZKanaLineEdit *ed = ((RecognizerPopupEvent*)e)->edit();
        RecognizerPosition pos = ((RecognizerPopupEvent*)e)->position();

        if (RecognizerForm::instance == nullptr)
            RecognizerForm::instance = new RecognizerForm();

        if (RecognizerForm::connected != nullptr)
        {
            // Qt terminates the program on exceptions in event handlers. If this line is hit
            // it is a bug, so termination is a good idea.
            throw "Double connection for editorDictionaryChanged";
        }

        connect(ed, &ZKanaLineEdit::dictionaryChanged, RecognizerForm::instance, &RecognizerForm::editorDictionaryChanged);
        RecognizerForm::connected = ed;

        RecognizerForm::instance->setParent(btn->window());
        RecognizerForm::instance->ui->candidateScroller->setDictionary(ed != nullptr ? ed->dictionary() : nullptr);
        QRect sg;

        if (!FormStates::recognizer.rect.isEmpty() && (Settings::recognizer.saveposition || pos == RecognizerPosition::SavedPos))
        {
            if (!Settings::recognizer.savesize)
                FormStates::recognizer.rect.setSize(QSize(Settings::scaled(300), Settings::scaled(300)));

            QRect geom = RecognizerForm::instance->resizing((int)GrabSide::Bottom, FormStates::recognizer.rect);
            int screennum = screenNumber(geom);
            if (screennum != -1)
            {
                sg = qApp->screens().at(screennum)->geometry();
                if (sg != FormStates::recognizer.screen)
                {
                    FormStates::recognizer.rect = QRect();
                    screennum = -1;
                }
            }
            //else
            //{
            //    sg = qApp->desktop()->screenGeometry(btn->window());
            //    geom.moveTo(geom.topLeft() - FormStates::recognizer.screenpos);
            //}

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

            geom = RecognizerForm::instance->resizing((int)GrabSide::Bottom, geom);

            if (screennum != -1)
                RecognizerForm::instance->setGeometry(geom);
        }

        if (FormStates::recognizer.rect.isEmpty())
            pos = RecognizerPosition::Above;

        if ((FormStates::recognizer.rect.isEmpty() || !Settings::recognizer.saveposition) && (pos == RecognizerPosition::Below || pos == RecognizerPosition::Above || pos == RecognizerPosition::StartBelow || pos == RecognizerPosition::StartAbove))
        {
            if (!Settings::recognizer.savesize || FormStates::recognizer.rect.isEmpty())
                FormStates::recognizer.rect.setSize(QSize(Settings::scaled(300), Settings::scaled(300)));

            // Move the window above or below the button.
            QSize siz = RecognizerForm::instance->resizing((int)GrabSide::Bottom, FormStates::recognizer.rect).size();

            QRect r = qApp->desktop()->availableGeometry(btn);
            QRect br = btn->geometry();
            QWidget *btnparent = (QWidget*)btn->parent();

            // Global button coordinates.
            br = QRect(btnparent->mapToGlobal(br.topLeft()), btnparent->mapToGlobal(br.bottomRight()));

            int newtop;
            int newleft;

            if (pos == RecognizerPosition::Above || pos == RecognizerPosition::StartAbove)
            {
                // Move us above the button if we fit. Below if not.
                if (br.top() - siz.height() - 8 /*- dif.top() - dif.bottom()*/ >= r.top())
                    newtop = br.top() - siz.height() - 8 /*- dif.bottom()*/;
                else
                    newtop = br.bottom() + 7 /*+ dif.top()*/;
            }
            else
            {
                // Move us below the button if we fit. Above if not.
                if (br.bottom() + 7 + siz.height() /*+ dif.top() + dif.bottom()*/ < r.bottom())
                    newtop = br.bottom() + 7 /*+ dif.top()*/;
                else
                    newtop = br.top() - siz.height() - 8 /*- dif.bottom()*/;
            }

            // Set the left coordinate. If we fit, place it right where the button is,
            // otherwise move only as much as needed.
            newleft = std::min(r.right() - siz.width() - 4 /*- dif.left() - dif.right()*/, br.left() /*+ dif.left()*/);

            //if (pos == RecognizerPosition::StartBelow || pos == RecognizerPosition::StartAbove)
            //    pos = RecognizerPosition::SavedPos;

            RecognizerForm::instance->setGeometry(QRect(newleft, newtop, siz.width(), siz.height()));
        }

        RecognizerForm::instance->button = btn;
        RecognizerForm::instance->edit = ed;

        RecognizerForm::instance->show();

        FormStates::recognizer.rect  = RecognizerForm::instance->geometry();
        if (sg.isEmpty())
            sg = qApp->desktop()->screenGeometry(RecognizerForm::instance);
        FormStates::recognizer.screen = sg;
        return true;
    }
    else if (e->type() == RecognizerHiddenEvent::Type() && RecognizerForm::instance != nullptr)
    {
        if (RecognizerForm::button != nullptr)
            RecognizerForm::button->setChecked(false);

        //    // Deletes instance.
        //    qApp->postEvent(this, new EndEvent);

        //    return true;
        //}
        //else if (e->type() == EndEvent::Type())
        //{

        //delete RecognizerForm::instance;
        //RecognizerForm::instance = nullptr;

        if (RecognizerForm::instance != nullptr)
        {
            RecognizerForm::instance->setParent(qApp->desktop());
            RecognizerForm::instance->windowHandle()->destroy();
        }

        return true;
    }

    return base::event(e);
}

bool RecognizerObject::eventFilter(QObject *obj, QEvent *e)
{
    ZKanaComboBox *box = dynamic_cast<ZKanaComboBox*>(obj);
    ZKanaLineEdit *edit = box == nullptr ? dynamic_cast<ZKanaLineEdit*>(obj) : obj->findChild<ZKanaLineEdit*>();

    if (e->type() == QEvent::EnabledChange || e->type() == QEvent::ReadOnlyChange)
    {
        if (edit != nullptr)
        {
            auto it = RecognizerForm::editforbuttons.find(edit);
            if (it != RecognizerForm::editforbuttons.end())
                it->second->setEnabled(box != nullptr ? (box->isEnabled() && !edit->isReadOnly()) : (edit->isEnabled() && !edit->isReadOnly()));
        }
    }

    /*
    if (RecognizerForm::instance == nullptr || !RecognizerForm::instance->isVisible())
    {
    // Pop up if space is pressed, ignore everything else.
    if (e->type() == QEvent::KeyPress)
    {
    QKeyEvent *ke = (QKeyEvent*)e;
    if (ke->key() == Qt::Key_Space)
    {
    if (edit != nullptr)
    RecognizerForm::popup(RecognizerForm::editforbuttons[edit]);
    }
    }

    return base::eventFilter(obj, e);
    }
    */

    // Hide or show for another edit box if space is pressed. Hide when esc is
    // pressed.
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = (QKeyEvent*)e;
        if (ke->key() == Qt::Key_Space && edit->isVisible())
        {
            if (edit != nullptr)
                RecognizerForm::popup(RecognizerForm::editforbuttons[edit]);
        }
        else if (RecognizerForm::instance != nullptr && RecognizerForm::instance->isVisible() && ke->key() == Qt::Key_Escape)
            RecognizerForm::instance->hide();
    }
    else if (e->type() == QEvent::Hide && RecognizerForm::instance != nullptr && RecognizerForm::instance->isVisible())
        RecognizerForm::instance->hide();

    return base::eventFilter(obj, e);
}

void RecognizerObject::buttonClicked()
{
    RecognizerForm::popup(dynamic_cast<QToolButton*>(sender()));
}

void RecognizerObject::buttonDestroyed()
{
    QToolButton* btn = (QToolButton*)sender();

    RecognizerForm::douninstall(btn, true);
}


//-------------------------------------------------------------
