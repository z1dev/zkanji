/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QPainter>
#include <QDesktopWidget>
#include <QApplication>

#include "zexamplestrip.h"
#include "words.h"
#include "sentences.h"
#include "zui.h"
#include "zevents.h"
#include "fontsettings.h"
#include "globalui.h"


//-------------------------------------------------------------


class ZExPopupDestroyedEvent : public EventTBase<ZExPopupDestroyedEvent>
{
private:
    typedef EventTBase<ZExPopupDestroyedEvent>  base;
public:
    ZExPopupDestroyedEvent(ZExamplePopup *popup) : base(), p(popup) { ; }
    ZExamplePopup* what()
    {
        return p;
    }
private:
    ZExamplePopup *p;
};


//-------------------------------------------------------------


static const int popupMargin = 3;

ZExamplePopup::ZExamplePopup(ZExampleStrip *owner) :
        base(owner->window(), Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus
            // Under Linux (tested on KDE) Qt::Tool must be indicated or the window won't stay on top of its parent.
            // TODO: (later) On Mac the Qt::Tool might keep a window on top of everything. This is not intended. 
            | Qt::X11BypassWindowManagerHint | Qt::Tool),
        owner(owner), hovered(-1)
{
    setAutoFillBackground(false);

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_CustomWhatsThis);
    setAttribute(Qt::WA_MouseTracking);
    setAttribute(Qt::WA_NoMousePropagation);
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // Test if not working right on linux
    //setAttribute(Qt::WA_X11NetWmWindowTypePopupMenu);
    //setAttribute(Qt::WA_X11DoNotAcceptFocus);
}

ZExamplePopup::~ZExamplePopup()
{
    ZExPopupDestroyedEvent e(this);
    qApp->sendEvent(owner, &e);
}

void ZExamplePopup::popup(Dictionary *d, const fastarray<ExampleWordsData::Form, quint16> &wordforms, const QRect &wordrect)
{
    QFont f = Settings::kanaFont();
    QFontMetrics fm(f);

    forms = &wordforms;

    lineheight = fm.height() + 2;

    formsavailable = 0;

    // Width of the longest word line.
    int w = wordrect.width() - popupMargin * 2 - 1;
    QString str;
    for (int ix = 0; ix != wordforms.size(); ++ix)
    {
        const auto &dat = wordforms[ix];
        if (d->findKanjiKanaWord(dat.kanji, dat.kana) == -1)
        {
            available.push_back(0);
            continue;
        }

        ++formsavailable;
        available.push_back(1);
        if (dat.kanji == dat.kana)
            str = dat.kanji.toQStringRaw();
        else
            str = QStringLiteral("%1[%2]").arg(dat.kanji.toQStringRaw()).arg(dat.kana.toQStringRaw());
        w = std::max(w, fm.boundingRect(str).width() + 2);
    }

    QRect r(0, 0, w + popupMargin * 2, formsavailable * lineheight + popupMargin * 2);
    setMinimumSize(r.size());
    setMaximumSize(r.size());

    QRect dr = qApp->desktop()->availableGeometry(owner);

    side = Bottom | Left;
    int left = wordrect.left();
    int top = wordrect.top() - r.height();
    if (top < dr.top())
    {
        top = wordrect.bottom();
        side &= ~Bottom;
    }

    if (left + r.width() >= dr.right())
    {
        left = wordrect.right() - r.width();
        side |= Right;
    }

    rectwidth = wordrect.width();

    move(left, top);

    show();
    //raise();
}

bool ZExamplePopup::event(QEvent *e)
{
    if (e->type() == EndEvent::Type())
    {
        if (!underMouse() && !owner->underMouse())
            deleteLater();
        return true;
    }

    return base::event(e);
}

void ZExamplePopup::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);

    QRectF r = rect();
    r.adjust(0.5, 0.5, -0.5, -0.5);

    QRect rr = QRect(owner->mapToGlobal(QPoint(0, 0)), owner->size());
    rr = QRect(mapFromGlobal(rr.topLeft()), mapFromGlobal(rr.bottomRight()));

    painter.setPen(Qt::black);
    if (side == (Bottom | Left))
    {
        QPointF pt[] = { QPointF(std::max<double>(r.left(), rr.left() + 0.5), r.bottom()), r.bottomLeft(), r.topLeft(), r.topRight(), r.bottomRight(), QPointF(std::min<double>(r.left() + rectwidth - 2, rr.right() + 0.5), r.bottom()) };
        painter.drawPolyline(pt, 6);
        painter.setPen(Qt::white);
        --pt[5].rx();
        ++pt[0].rx();
        painter.drawLine(pt[5], pt[0]);
    }
    else if (side == (Bottom | Right))
    {
        QPointF pt[] = { QPointF(std::min<double>(r.right(), rr.right() + 0.5), r.bottom()), r.bottomRight(), r.topRight(), r.topLeft(), r.bottomLeft(), QPointF(std::max<double>(r.right() - rectwidth + 2, rr.left() + 0.5), r.bottom()) };
        painter.drawPolyline(pt, 6);
        painter.setPen(Qt::white);
        ++pt[5].rx();
        --pt[0].rx();
        painter.drawLine(pt[5], pt[0]);
    }
    else if (side == (Top | Left))
    {
        QPointF pt[] = { QPointF(std::max<double>(r.left(), rr.left() + 0.5), r.top()), r.topLeft(), r.bottomLeft(), r.bottomRight(), r.topRight(), QPointF(std::min<double>(r.left() + rectwidth - 2, rr .right() + 0.5), r.top()) };
        painter.drawPolyline(pt, 6);
        painter.setPen(Qt::white);
        --pt[5].rx();
        ++pt[0].rx();
        painter.drawLine(pt[5], pt[0]);
    }
    else // Top | Right
    {
        QPointF pt[] = { QPointF(std::min<double>(r.right(), rr.right() + 0.5), r.top()), r.topRight(), r.bottomRight(), r.topLeft(), r.topLeft(), QPointF(std::max<double>(r.right() - rectwidth + 2, rr .left() + 0.5), r.top()) };
        painter.drawPolyline(pt, 6);
        painter.setPen(Qt::white);
        ++pt[5].rx();
        --pt[0].rx();
        painter.drawLine(pt[5], pt[0]);
    }

    r.adjust(0, 0, -1, -1);

    painter.fillRect(r, Qt::white);

    painter.setPen(Qt::black);

    QFont f = Settings::kanaFont();
    painter.setFont(f);

    int top = popupMargin + 1;
    int flags = Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip | Qt::TextSingleLine;
    QString str;
    for (int ix = 0; ix != forms->size(); ++ix)
    {
        if (available[ix] == 0)
            continue;

        const auto &dat = forms->operator[](ix);
        if (dat.kanji == dat.kana)
            str = dat.kanji.toQStringRaw();
        else
            str = QStringLiteral("%1[%2]").arg(dat.kanji.toQStringRaw()).arg(dat.kana.toQStringRaw());

        if (hovered == ix)
            painter.fillRect(popupMargin, top - 1, rect().width() - popupMargin * 2, lineheight, Qt::lightGray);
        painter.drawText(popupMargin + 1, top, 1, 1, flags, str);

        top += lineheight;
    }
}

void ZExamplePopup::leaveEvent(QEvent *e)
{
    base::leaveEvent(e);

    if (hovered != -1)
    {
        update(QRect(popupMargin, popupMargin + lineheight * hovered, rect().width() - popupMargin * 2, lineheight));
        hovered = -1;
    }

    if (owner->underMouse())
        return;

    qApp->postEvent(this, new EndEvent);
}

void ZExamplePopup::mousePressEvent(QMouseEvent *e)
{
    base::mousePressEvent(e);

    if (hovered == -1)
    {
        e->accept();
        return;
    }

    // Not all word forms are shown. Notify the parent about the real word
    // form position.
    int pos = (e->pos().y() - popupMargin) / lineheight + 1;
    if (pos <= 0 || pos > formsavailable)
        return;

    for (int ix = 0; ix != forms->size(); ++ix)
    {
        if (available[ix] != 0)
            --pos;
        if (pos == 0)
        {
            owner->selectForm(ix);
            break;
        }
    }

}

void ZExamplePopup::mouseMoveEvent(QMouseEvent *e)
{
    base::mouseMoveEvent(e);

    int hpos = (e->pos().y() - popupMargin) / lineheight;
    if (hpos >= formsavailable || e->pos().y() < popupMargin || e->pos().x() < popupMargin || e->pos().x() >= rect().width() - popupMargin)
        hpos = -1;

    if (hpos == hovered)
        return;

    if (hovered != -1)
        update(QRect(popupMargin, popupMargin + lineheight * hovered, rect().width() - popupMargin * 2, lineheight));
    hovered = hpos;
    if (hovered != -1)
        update(QRect(popupMargin, popupMargin + lineheight * hovered, rect().width() - popupMargin * 2, lineheight));
}

void ZExamplePopup::mouseReleaseEvent(QMouseEvent *e)
{
    base::mouseReleaseEvent(e);
    e->accept();
}

void ZExamplePopup::mouseDoubleClickEvent(QMouseEvent *e)
{
    base::mouseDoubleClickEvent(e);
    e->accept();
}


//-------------------------------------------------------------


ZExampleStrip::ZExampleStrip(QWidget *parent) : base(parent), dict(nullptr), display(ExampleDisplay::Both), index(-1), dirty(false), block(0), line(0), wordpos(-1), common(nullptr), current(-1), hovered(-1), jpwidth(-1), trwidth(-1)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    QFont jf = Settings::kanaFont();
    QFont tf = Settings::defFont();
    jf.setPointSize(11);
    tf.setPointSize(8);
    QFontMetrics jfm(jf);
    QFontMetrics tfm(tf);

    int jh = jfm.height();
    int th = tfm.height();

    setMinimumHeight(jh + th + 6 + scrollerSize());

    connect(gUI, &GlobalUI::settingsChanged, this, &ZExampleStrip::reset);
    connect(gUI, &GlobalUI::dictionaryToBeRemoved, this, &ZExampleStrip::dictionaryRemoved);
}

ZExampleStrip::~ZExampleStrip()
{
}

void ZExampleStrip::reset()
{
    dirty = true;
    updateSentence();
}

void ZExampleStrip::setItem(Dictionary *d, int windex, int wpos, int wordform)
{
    if (windex == -1 && index == -1)
    {
        if (dict != nullptr)
            disconnect(dict, 0, this, 0);
        dict = nullptr;
        return;
    }

    bool keepsentence = false;
    WordEntry *w = d != nullptr && windex != -1 ? d->wordEntry(windex) : nullptr;

    if (d == nullptr)
        windex = -1;

    if (d != nullptr && windex != -1 && wpos != -1 && wordform != -1 && index != -1 && sentence.words.size() > wpos && sentence.words[wpos].forms.size() > wordform)
    {
        const ExampleWordsData::Form &form = sentence.words[wpos].forms[wordform];
        if (w->kanji == form.kanji && w->kana == form.kana)
            keepsentence = true;
    }

    if (!keepsentence && index != -1 && windex != -1)
    {
        WordEntry *eold = dict->wordEntry(index);
        if (eold->kanji != w->kanji || eold->kana != w->kana)
            dirty = true;
    }
    else if (!keepsentence)
        dirty = true;

    if (d != dict)
    {
        wordrect.clear();
        if (!dirty && isVisible())
            update();

        if (dict != nullptr)
            disconnect(dict, 0, this, 0);
    }

    dict = d;
    index = windex;

    if (dict != nullptr)
    {
        connect(dict, &Dictionary::entryAdded, this, &ZExampleStrip::dictionaryChanged);
        connect(dict, &Dictionary::entryRemoved, this, &ZExampleStrip::dictionaryChanged);
        connect(dict, &Dictionary::dictionaryReset, this, &ZExampleStrip::dictionaryChanged);
    }

    if (dirty)
        current = 0;

    if (keepsentence)
    {
        // When the same sentence can be kept that means the word's commons data is available.
        // No need to check anything in here.

        wordpos = wpos;
        common = ZKanji::commons.findWord(w->kanji.data(), w->kana.data(), w->romaji.data());

        for (int ix = 0; ix != common->examples.size(); ++ix)
        {
            if (common->examples[ix].block == block && common->examples[ix].line == line && common->examples[ix].wordindex == wpos)
            {
                current = ix;
                break;
            }
#ifdef _DEBUG
            if (ix == common->examples.size())
                throw "Couldn't find word sentence.";
#endif
        }

        emit sentenceChanged();
    }

    if (!isVisible())
        return;

    updateSentence();
}

ExampleDisplay ZExampleStrip::displayed() const
{
    return display;
}

void ZExampleStrip::setDisplayed(ExampleDisplay newdisp)
{
    if (display == newdisp)
        return;
    display = newdisp;

    contentsReset();

    if (!dirty && isVisible() && index != -1)
    {
        wordrect.clear();
        popup.reset();
        hovered = -1;
        jpwidth = -1;
        trwidth = -1;
    }

    update();
}

int ZExampleStrip::sentenceCount() const
{
    if (index == -1)
        return 0;
    return common->examples.size();
}

int ZExampleStrip::currentSentence() const
{
    if (index == -1)
        return 0;
    return current;
}

void ZExampleStrip::setCurrentSentence(int which)
{
    which = std::max(0, std::min(which, sentenceCount() - 1));

    if (which == current)
        return;

    dirty = true;
    current = which;

    if (!isVisible())
        return;

    updateSentence();
}

bool ZExampleStrip::event(QEvent *e)
{
    if (e->type() == ZExPopupDestroyedEvent::Type())
    {
        // Event is received when a popup window previously owned by this strip is deleted.
        ZExPopupDestroyedEvent *pe = (ZExPopupDestroyedEvent*)e;

        if (popup.get() == pe->what())
        {
            popup.release();
            if (hovered != -1 && hovered < wordrect.size())
                updateWordRect(hovered);
            hovered = -1;
            updateDots();
        }
        return true;
    }

    return base::event(e);
}

void ZExampleStrip::showEvent(QShowEvent *e)
{
    base::showEvent(e);

    updateSentence();
}

void ZExampleStrip::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    if (index == -1)
        return;

    QPainter painter(this);

    int x = -scrollPos() + 4;
    QRect r = drawArea();
    painter.setClipRect(r);

    QFont jf = Settings::kanaFont();
    QFont tf = Settings::defFont();
    if (display == ExampleDisplay::Both)
    {
        adjustFontSize(jf, (r.height() - 4) * 0.5);
        adjustFontSize(tf, (r.height() - 4) * 0.38);
    }
    else
    {
        adjustFontSize(jf, r.height() * 0.54);
        adjustFontSize(tf, r.height() * 0.4);
    }

    QFontMetrics jfm(jf);
    QFontMetrics tfm(tf);

    int jh = jfm.height() - jfm.descent() + 3;
    int th = tfm.height();

    int flags = Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip | Qt::TextSingleLine;

    QPalette pal;

    if (display == ExampleDisplay::Both)
    {
        int jtop = std::max(r.top() + 2, r.top() + (r.height() - jh - th - 2) / 2);
        int ttop = jtop + jh + 2;

        painter.setFont(jf);
        //painter.drawText(QRect(x, jtop, 1, 1), flags, sentence.japanese.toQStringRaw());
        paintJapanese(&painter, jfm, jtop);

        painter.setPen(pal.color(QPalette::Text));
        painter.setFont(tf);
        painter.drawText(QRect(x, ttop, 1, 1), flags, sentence.translated.toQStringRaw());
    }
    else if (display == ExampleDisplay::Japanese)
    {
        int jtop = r.top() + (r.height() - jh) / 2;
        painter.setFont(jf);
        //painter.drawText(QRect(x, jtop, 1, 1), flags, sentence.japanese.toQStringRaw());
        paintJapanese(&painter, jfm, jtop);
    }
    else
    {
        int ttop = r.top() + (r.height() - th) / 2;
        painter.setPen(pal.color(QPalette::Text));
        painter.setFont(tf);
        painter.drawText(QRect(x, ttop, 1, 1), flags, sentence.translated.toQStringRaw());
    }
}

void ZExampleStrip::mousePressEvent(QMouseEvent *e)
{
    base::mousePressEvent(e);

    if (!e->isAccepted())
    {
        if (hovered != -1 && hovered != wordrect.size())
            selectForm(0, hovered);
        e->accept();
    }
}

void ZExampleStrip::mouseDoubleClickEvent(QMouseEvent *e)
{
    base::mouseDoubleClickEvent(e);
    e->accept();
}

void ZExampleStrip::mouseReleaseEvent(QMouseEvent *e)
{
    base::mouseReleaseEvent(e);
    e->accept();
}

void ZExampleStrip::mouseMoveEvent(QMouseEvent *e)
{
    base::mouseMoveEvent(e);

    // The scroller handled the event.
    if (e->isAccepted())
    {
        const QRect r = contentsRect();
        int hpos = r.contains(e->pos()) ? wordrect.size() : -1;

        // Mouse cursor was in a word rectangle, but it's now over the scroll bar. Update the
        // hovered rectangle and the dotted strip below the words.
        if (index != -1 && hovered != hpos)
        {
            if (hovered != -1 && hovered != wordrect.size())
                updateWordRect(hovered);
            hovered = hpos;
            popup.reset();
            updateDots();

            update();
        }
        return;
    }
    e->accept();

    if (index == -1)
        return;

    int hpos = -1;

    if (hovered != -1 && hovered != wordrect.size() && wordrect[hovered].adjusted(-2, -2, 2, 2).contains(e->pos()))
        hpos = hovered;
    for (int ix = 0; ix != wordrect.size() && hpos == -1; ++ix)
    {
        if (!wordrect[ix].isEmpty() && wordrect[ix].adjusted(-2, -2, 2, 2).contains(e->pos()))
            hpos = ix;
    }

    if (hpos == -1)
        hpos = wordrect.size();

    if (hpos != hovered)
    {
        if (hovered == -1 || hpos == -1)
            updateDots();

        if (hovered != -1 && hovered != wordrect.size())
            updateWordRect(hovered);
        if (hpos != -1 && hpos != wordrect.size())
        {
            hovered = hpos;
            updateWordRect(hovered);

            const ExampleWordsData &worddata = sentence.words[hovered];
            if (worddata.forms.size() == 1)
            {
                const ExampleWordsData::Form &wordform = worddata.forms[0];
                if (wordform.kanji == wordform.kana && wordform.kanji.size() == worddata.len && qcharncmp(sentence.japanese.data() + worddata.pos, wordform.kana.data(), worddata.len) == 0)
                {
                    popup.reset();
                    return;
                }
            }
            popup.reset(new ZExamplePopup(this));
            QRect wr = QRect(mapToGlobal(wordrect[hovered].topLeft()), mapToGlobal(wordrect[hovered].bottomRight())).adjusted(-2, -1, 3, 2);
            popup->popup(dict, worddata.forms, wr);
        }
        else
        {
            hovered = hpos;
            popup.reset();
        }
    }

}

void ZExampleStrip::leaveEvent(QEvent *e)
{
    base::leaveEvent(e);
    e->accept();

    if (hovered != -1)
    {
        updateDots();
        if (hovered < wordrect.size())
            updateWordRect(hovered);
        if (popup)
        {
            if (popup->rect().contains(popup->mapFromGlobal(QCursor::pos())))
                qApp->postEvent(popup.get(), new EndEvent);
            else
            {
                hovered = -1;
                popup.reset();
            }
        }
        else
            hovered = -1;
    }
}

int ZExampleStrip::scrollMin() const
{
    return 0;
}

int ZExampleStrip::scrollMax() const
{
    if (index == -1)
        return 0;

    if ((display == ExampleDisplay::Japanese && jpwidth == -1) || (display == ExampleDisplay::Translated && trwidth == -1) || (display == ExampleDisplay::Both && (trwidth == -1 || jpwidth == -1)))
    {
        QFont jf = Settings::kanaFont();
        QFont tf = Settings::defFont();

        QRect r = drawArea();

        if (display == ExampleDisplay::Both)
        {
            adjustFontSize(jf, (r.height() - 4) * 0.5);
            adjustFontSize(tf, (r.height() - 4) * 0.38);
        }
        else
        {
            adjustFontSize(jf, r.height() * 0.54);
            adjustFontSize(tf, r.height() * 0.4);
        }

        QFontMetrics jfm(jf);
        QFontMetrics tfm(tf);

        int jh = jfm.height();
        int th = tfm.height();
        if (display == ExampleDisplay::Both || display == ExampleDisplay::Japanese)
            jpwidth = jfm.boundingRect(sentence.japanese.toQStringRaw()).width();
        if (display == ExampleDisplay::Both || display == ExampleDisplay::Translated)
            trwidth = tfm.boundingRect(sentence.translated.toQStringRaw()).width();
    }

    if (display == ExampleDisplay::Both)
        return std::max(jpwidth, trwidth) + 8;
    else if (display == ExampleDisplay::Japanese)
        return jpwidth + 8;
    else
        return trwidth + 8;
}

int ZExampleStrip::scrollPage() const
{
    return drawArea().width();
}

int ZExampleStrip::scrollStep() const
{
    return 16;
}

void ZExampleStrip::scrolled(int oldpos, int &newpos)
{
    for (int ix = 0; ix != wordrect.size(); ++ix)
        wordrect[ix].translate(oldpos - newpos, 0);
    if (popup != nullptr && popup->isVisible())
    {
        QPoint p = popup->pos();
        p.rx() += oldpos - newpos;
        popup->move(p);
        popup->update();
    }
    update();
}

void ZExampleStrip::dictionaryRemoved(int index, int orderindex, Dictionary *d)
{
    if (dict != d)
        return;
    setItem(nullptr, -1);
}

void ZExampleStrip::dictionaryChanged()
{
    if (dict == nullptr || index == -1 || !isVisible())
        return;

    QMouseEvent e = QMouseEvent(QEvent::MouseMove, mapFromGlobal(QCursor::pos()), Qt::NoButton, Qt::NoButton, Qt::KeyboardModifiers());
    wordrect.clear();
    if (rect().contains(e.pos()))
    {
        fillWordRects();
        mouseMoveEvent(&e);
    }

    update();
}

void ZExampleStrip::updateSentence()
{
    if (!dirty)
        return;

    popup.reset();
    wordrect.clear();

    hovered = -1;
    jpwidth = -1;
    trwidth = -1;

    sentence.japanese.clear();
    sentence.translated.clear();
    sentence.words.clear();

    if (index != -1)
    {
        WordEntry *e = dict->wordEntry(index);
        common = ZKanji::commons.findWord(e->kanji.data(), e->kana.data(), e->romaji.data());
        if (common == nullptr || common->examples.empty())
            index = -1;
        else
        {
            block = common->examples[current].block;
            line = common->examples[current].line;
            wordpos = common->examples[current].wordindex;
            sentence = ZKanji::sentences.getSentence(block, line);
        }
    }

    dirty = false;
    emit sentenceChanged();

    contentsReset();

    if (index != -1)
    {
        QMouseEvent e = QMouseEvent(QEvent::MouseMove, mapFromGlobal(QCursor::pos()), Qt::NoButton, Qt::NoButton, Qt::KeyboardModifiers());
        if (rect().contains(e.pos()))
        {
            fillWordRects();
            mouseMoveEvent(&e);
        }
    }

    update();
}

void ZExampleStrip::fillWordRects()
{
    if (!wordrect.empty() || index == -1 || (display != ExampleDisplay::Japanese && display != ExampleDisplay::Both))
        return;
    hovered = -1;

    QRect r = drawArea();

    QFont jf = Settings::kanaFont();
    QFont tf = Settings::defFont();
    if (display == ExampleDisplay::Both)
    {
        adjustFontSize(jf, (r.height() - 4) * 0.5);
        adjustFontSize(tf, (r.height() - 4) * 0.38);
    }
    else
    {
        adjustFontSize(jf, r.height() * 0.54);
        adjustFontSize(tf, r.height() * 0.4);
    }

    QFontMetrics fm(jf);
    QFontMetrics tfm(tf);

    int jh = fm.height() - fm.descent() + 3;
    int th = tfm.height();

    int y;
    if (display == ExampleDisplay::Both)
        y = std::max(r.top() + 2, r.top() + (r.height() - jh - th - 2) / 2);
    else if (display == ExampleDisplay::Japanese)
        y = r.top() + (r.height() - jh) / 2;


    int x = -scrollPos() + 4;

    // Currently word.
    int pos = 0;

    while (pos != sentence.words.size())
    {
        int gappos = 0;
        int gaplen = sentence.words[pos].pos;

        if (pos != 0)
        {
            gappos = sentence.words[pos - 1].pos + sentence.words[pos - 1].len;
            gaplen = sentence.words[pos].pos - gappos;
        }

        // Non-word part of sentence between two words.
        if (gaplen != 0)
        {
            QString str = sentence.japanese.toQString(gappos, gaplen);
            x += fm.width(str);
        }

        QString str = sentence.japanese.toQString(sentence.words[pos].pos, sentence.words[pos].len);

        int w = fm.width(str);

        bool found = false;
        for (int ix = 0; ix != sentence.words[pos].forms.size() && !found; ++ix)
        {
            const ExampleWordsData::Form &f = sentence.words[pos].forms[ix];
            found = dict->findKanjiKanaWord(f.kanji, f.kana) != -1;
        }

        if (found)
            wordrect.push_back(QRect(x, y, w, fm.height() - fm.descent() + 3));
        else
            wordrect.push_back(QRect(-1, -1, 0, 0));

        x += w;

        ++pos;
    }
}

void ZExampleStrip::paintJapanese(QPainter *p, QFontMetrics &fm, int y)
{
    if (index == -1)
        return;

    bool fillrects = wordrect.empty();

    // Sometimes the rectangles are not yet set but hovered is set to wordrect.size() because
    // mouse move event can be called before painting. Reset hovered from 0, so nothing is
    // hovered initially by mistake.
    if (fillrects)
        hovered = -1;

    int x = -scrollPos() + 4;

    int flags = Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip | Qt::TextSingleLine;

    // Currently drawn word.
    int pos = 0;

    QPalette pal;

    while (pos != sentence.words.size())
    {
        // Drawing two sentence parts. One before the current word but after the previous, and
        // the word at position.

        int gappos = 0;
        int gaplen = sentence.words[pos].pos;

        if (pos != 0)
        {
            gappos = sentence.words[pos - 1].pos + sentence.words[pos - 1].len;
            gaplen = sentence.words[pos].pos - gappos;
        }

        // Non-word part of sentence between two words.
        if (gaplen != 0)
        {
            QString str = sentence.japanese.toQString(gappos, gaplen);
            p->setPen(Qt::black);
            p->drawText(x, y, 1, 1, flags, str);
            x += fm.width(str);
        }

        QString str = sentence.japanese.toQString(sentence.words[pos].pos, sentence.words[pos].len);
        // Skip the hovered word because it will be drawn separately below, so the drawn
        // bounding rectangle can cover neighbouring words.
        if (hovered != pos)
        {
            p->setPen(wordpos == pos ? Qt::red : pal.color(QPalette::Text));
            p->drawText(x, y, 1, 1, flags, str);
        }
        int w = fm.width(str);

        if (fillrects)
        {
            // Only add rectangle to words and word forms present in the current dictionary.

            bool found = false;
            for (int ix = 0; ix != sentence.words[pos].forms.size() && !found; ++ix)
            {
                const ExampleWordsData::Form &f = sentence.words[pos].forms[ix];
                found = dict->findKanjiKanaWord(f.kanji, f.kana) != -1;
            }

            if (found)
                wordrect.push_back(QRect(x, y, w, fm.height() - fm.descent() + 3));
            else
                wordrect.push_back(QRect(-1, -1, 0, 0));
        }
        x += w;

        ++pos;
    }

    p->setPen(pal.color(QPalette::Text));

    // Last part of the sentence without a word rectangle.
    int gappos = sentence.words[pos - 1].pos + sentence.words[pos - 1].len;
    if (gappos < sentence.japanese.size())
    {
        int gaplen = sentence.japanese.size() - gappos;
        QString str = sentence.japanese.toQString(gappos, gaplen);
        p->drawText(x, y, 1, 1, flags, str);
    }

    // Hovered word is painted last with its selection rectangle.
    if (hovered != -1 && hovered != wordrect.size())
    {
        QRectF r = wordrect[hovered];
        r.adjust(-1.5, -1.5, 1.5, 1.5);
        p->setPen(Qt::black);
        p->drawRect(r);
        r.adjust(0, 0, -1, -1);
        p->fillRect(r, Qt::white);

        QString str = sentence.japanese.toQString(sentence.words[hovered].pos, sentence.words[hovered].len);

        p->setPen(wordpos == hovered ? Qt::red : pal.color(QPalette::Text));
        p->drawText(wordrect[hovered].left(), y, 1, 1, flags, str);
    }

    // Paint dotted line below the words.
    if (hovered != -1)
    {
        QPen pen(Qt::black);
        pen.setDashPattern({ 1, 1 });
        p->setPen(pen);

        for (int ix = 0; ix != wordrect.size(); ++ix)
        {
            if (ix == hovered || wordrect[ix].isEmpty())
                continue;
            QRect &r = wordrect[ix];
            p->drawLine(QPointF(r.left() + 0.5, r.bottom() + 1.5), QPointF(r.right() - 0.5, r.bottom() + 1.5));
        }

    }

    p->setPen(pal.color(QPalette::Text));
}

void ZExampleStrip::updateWordRect(int index)
{
    update(wordrect[index].adjusted(-3, -3, 3, 3));
}

void ZExampleStrip::updateDots()
{
    int left = 0;
    int right = wordrect.size() - 1;
    while (left < wordrect.size() && wordrect[left].isEmpty())
        ++left;
    while (right > left && wordrect[right].isEmpty())
        --right;
    if (left == wordrect.size())
        return;

    update(wordrect[left].left() - 1, wordrect[left].bottom() + 1, wordrect[right].right() - wordrect[left].left() + 2, 2);
}

void ZExampleStrip::selectForm(int form, int wpos)
{
    if (wordpos == hovered && wpos == -1)
    {
        ExampleWordsData::Form &dat = sentence.words[wordpos].forms[form];
        int windex = dict->findKanjiKanaWord(dat.kanji, dat.kana);
        if (windex == index)
            return;
    }


    if (popup && popup->underMouse())
        popup->deleteLater();

    if (form < 0 || form >= sentence.words.size())
        form = 0;

    if (wpos == -1)
    {
        if (hovered == -1 || hovered == wordrect.size())
            return;
        wpos = hovered;
    }

    if (wpos != wordpos)
    {
        if (wordpos != -1)
            updateWordRect(wordpos);
        wordpos = wpos;
        if (wordpos != -1)
            updateWordRect(wordpos);
    }

    emit wordSelected(common->examples[current].block, common->examples[current].line, wordpos, form);
}


//-------------------------------------------------------------



