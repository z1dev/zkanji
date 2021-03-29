/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QStackedLayout>
#include <QPainter>
#include <QDesktopWidget>
#include <QSplitter>
#include <QStylePainter>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QPainterPath>

#include "zkanjiwidget.h"
#include "ui_zkanjiwidget.h"

#include "words.h"
#include "zui.h"
#include "zevents.h"
#include "globalui.h"
//#include "grouppickerform.h"
#include "groups.h"
#include "zdictionarymodel.h"
#include "dialogs.h"
#include "zkanjiform.h"
#include "zkanjigridview.h"
#include "zdictionarylistview.h"
#include "generalsettings.h"
#include "colorsettings.h"


//-------------------------------------------------------------


DropMenu::DropMenu(QWidget *parent) : base(parent), button(nullptr)
{

}

void DropMenu::setButton(QAbstractButton *btn)
{
    button = btn;
}

void DropMenu::showEvent(QShowEvent *event)
{
    if (button == nullptr)
        return;

    QRect desk = qApp->desktop()->availableGeometry(button);
    QSize s = sizeHint();

    QPoint pos = button->mapToGlobal(QPoint(0, button->height()));
    bool above = false;
    if (desk.bottom() <= pos.y() + s.height())
        above = true;

    bool toleft = false;
    if (desk.right() <= pos.x() + s.width())
        toleft = true;

    if (above)
        pos.setY(std::max(desk.top(), button->mapToGlobal(QPoint(0, 0)).y() - s.height()));
    if (toleft)
        pos.setX(std::max(desk.left(), button->mapToGlobal(QPoint(button->width(), button->height())).x() - s.width()));

    move(pos);

    base::showEvent(event);
}


//-------------------------------------------------------------


/* static */ ZKanjiWidget* ZKanjiWidget::getZParent(QWidget *widget)
{
    if (!widget)
        return nullptr;
    widget = widget->parentWidget();
    while (widget && dynamic_cast<ZKanjiWidget*>(widget) == nullptr)
        widget = widget->parentWidget();
    return (ZKanjiWidget*)widget;
}

ZKanjiWidget::ZKanjiWidget(QWidget *parent) : base(parent), ui(new Ui::ZKanjiWidget), dictindex(0), modemenu(this), dockmenu(this), paintactive(false), allowdocking(true), allowdockaction(nullptr)
{
    ui->setupUi(this);

    gUI->scaleWidget(this);

    modemenu.setButton(ui->modeButton);

    int _iconW = Settings::scaled(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize));
    int _iconH = _iconW;

    QPixmap pix = renderFromSvg(QStringLiteral(":/magnisearch.svg"), _iconW, _iconH, QRect(0, 0, _iconW, _iconH));
    dictimg = triangleImage(pix);
    connect(modemenu.addAction(QIcon(pix), QString()), &QAction::triggered, this, &ZKanjiWidget::setModeByAction);
    pix = renderFromSvg(QStringLiteral(":/wordgroups.svg"), _iconW, _iconH, QRect(0, 0, _iconW, _iconH));
    wgrpimg = triangleImage(pix);
    connect(modemenu.addAction(QIcon(pix), QString()), &QAction::triggered, this, &ZKanjiWidget::setModeByAction);
    pix = renderFromSvg(QStringLiteral(":/kanjigroups.svg"), _iconW, _iconH, QRect(0, 0, _iconW, _iconH));
    kgrpimg = triangleImage(pix);
    connect(modemenu.addAction(QIcon(pix), QString()), &QAction::triggered, this, &ZKanjiWidget::setModeByAction);
    pix = renderFromSvg(QStringLiteral(":/kanjisearch.svg"), _iconW, _iconH, QRect(0, 0, _iconW, _iconH));
    ksrcimg = triangleImage(pix);
    connect(modemenu.addAction(QIcon(pix), QString()), &QAction::triggered, this, &ZKanjiWidget::setModeByAction);

    setTranslationTexts();

    connect(gUI, &GlobalUI::dictionaryAdded, this, &ZKanjiWidget::dictionaryAdded);
    connect(gUI, &GlobalUI::dictionaryRemoved, this, &ZKanjiWidget::dictionaryRemoved);
    connect(gUI, &GlobalUI::dictionaryMoved, this, &ZKanjiWidget::dictionaryMoved);
    connect(gUI, &GlobalUI::dictionaryRenamed, this, &ZKanjiWidget::dictionaryRenamed);
    connect(gUI, &GlobalUI::dictionaryFlagChanged, this, &ZKanjiWidget::dictionaryFlagChanged);
    connect(gUI, &GlobalUI::dictionaryReplaced, this, &ZKanjiWidget::dictionaryReplaced);

    for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
        connect(dictmenu.addAction(ZKanji::dictionaryMenuFlag(ZKanji::dictionary(ZKanji::dictionaryPosition(ix))->name()), ZKanji::dictionary(ZKanji::dictionaryPosition(ix))->name()), &QAction::triggered, this, &ZKanjiWidget::setDictByAction);

    QSize s = QSize(_iconW, _iconH);
    QSize ts = triangleSize(QSize(_iconW, _iconH));
    ui->dictButton->setIconSize(ts);
    ui->dictButton->setPopupMode(QToolButton::InstantPopup);
    ui->dictButton->setMenu(&dictmenu);
    ui->dictButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");

    //s.setWidth(s.width() - _triS - _triPH);
    //s.setHeight(s.height() - _triPV);

    ui->dictButton->setIcon(QIcon(triangleImage(ZKanji::dictionaryFlag(s, ZKanji::dictionary(dictindex)->name(), Flags::Flag))));

    dictmenu.setButton(ui->dictButton);

    connect(dockmenu.addAction(tr("&Float")), &QAction::triggered, this, &ZKanjiWidget::floatToWindow);

    ui->modeButton->setIconSize(ts);
    ui->modeButton->setPopupMode(QToolButton::InstantPopup);
    ui->modeButton->setMenu(&modemenu);
    ui->modeButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");

    ui->pagesStack->setCurrentIndex(2);

    ui->dictionary->setWordDragEnabled(true);
    ui->dictionary->view()->setGroupDisplay(true);

    // Corner button and combo box for new windows and mode change. It must be resized to its
    // optimal size and make it fixed width and height, so when it's added to the layout it
    // won't be stretched out.
    //ui->modeWidget->adjustSize();

    //QSize modeSize = ui->modeWidget->size();
    //ui->modeWidget->setMinimumSize(modeSize);
    //ui->modeWidget->setMaximumSize(modeSize);

    //modeSize.setHeight(0);
    //ui->dictionary->makeModeSpace(modeSize);
    //ui->kanjiGroups->makeModeSpace(modeSize);
    //ui->wordGroups->makeModeSpace(modeSize);
    //ui->kanjiSearch->makeModeSpace(modeSize);

    //QStackedLayout *layout = new QStackedLayout(this);

    //layout->addWidget(ui->pagesStack);
    //layout->addWidget(ui->modeWidget);
    //layout->setStackingMode(QStackedLayout::StackAll);

    //// Set the modeWidget as active or the pageStack would hide it.
    //layout->setCurrentIndex(1);

    //setAttribute(Qt::WA_DeleteOnClose);


    // Fill in the available dictionaries combo box.
    //for (int ix = 0; ix != ZKanji::dictionaryCount(); ++ix)
    //    ui->dictBox->addItem(ZKanji::dictionary(ix)->name());
}

ZKanjiWidget::~ZKanjiWidget()
{
    delete ui;
}


void ZKanjiWidget::saveXMLSettings(QXmlStreamWriter &writer) const
{
    switch (ui->pagesStack->currentIndex())
    {
    case 0:
        writer.writeAttribute("mode", "dictionary");
        break;
    case 1:
        writer.writeAttribute("mode", "wordgroups");
        break;
    case 2:
        writer.writeAttribute("mode", "kanjigroups");
        break;
    case 3:
        writer.writeAttribute("mode", "kanjisearch");
        break;
    }
    writer.writeAttribute("language", ui->dictionary->dictionary()->name());
    writer.writeAttribute("docking", allowdocking ? "1" : "0");

    writer.writeStartElement("Dictionary");
    ui->dictionary->saveXMLSettings(writer);
    writer.writeEndElement(); /* Dictionary */

    writer.writeStartElement("WordGroups");
    ui->wordGroups->saveXMLSettings(writer);
    writer.writeEndElement(); /* WordGroups */

    writer.writeStartElement("KanjiGroups");
    ui->kanjiGroups->saveXMLSettings(writer);
    writer.writeEndElement(); /* KanjiGroups */

    writer.writeStartElement("KanjiSearch");
    ui->kanjiSearch->saveXMLSettings(writer);
    writer.writeEndElement(); /* KanjiSearch */
}

void ZKanjiWidget::loadXMLSettings(QXmlStreamReader &reader)
{
    QStringRef modestr = reader.attributes().value("mode");
    if (modestr == "wordgroups")
        setMode(ViewModes::WordGroup);
    else if (modestr == "kanjigroups")
        setMode(ViewModes::KanjiGroup);
    else if (modestr == "kanjisearch")
        setMode(ViewModes::KanjiSearch);
    QStringRef langstr = reader.attributes().value("language");
    for (int ix = 0, siz = ZKanji::dictionaryCount(); ix != siz; ++ix)
    {
        if (langstr == ZKanji::dictionary(ix)->name())
        {
            setDictionary(ix);
            break;
        }
    }

    allowdocking = reader.attributes().value("docking") != "0";

    while (reader.readNextStartElement())
    {
        if (reader.name() == "Dictionary")
            ui->dictionary->loadXMLSettings(reader);
        else if (reader.name() == "WordGroups")
            ui->wordGroups->loadXMLSettings(reader);
        else if (reader.name() == "KanjiGroups")
            ui->kanjiGroups->loadXMLSettings(reader);
        else if (reader.name() == "KanjiSearch")
            ui->kanjiSearch->loadXMLSettings(reader);
        else
            reader.skipCurrentElement();
    }
}

Dictionary* ZKanjiWidget::dictionary()
{
    return ZKanji::dictionary(dictindex);
}

int ZKanjiWidget::dictionaryIndex() const
{
    return dictindex;
}

void ZKanjiWidget::setDictionary(int index)
{
    if (dictindex == index)
        return;

    dictindex = index;

    Dictionary *d = ZKanji::dictionary(index);

    //ui->dictBox->setCurrentIndex(index);
    ui->dictionary->setDictionary(d);
    ui->wordGroups->setDictionary(d);
    ui->kanjiGroups->setDictionary(d);
    ui->kanjiSearch->setDictionary(d);

    QPixmap pix;

    int _iconW = Settings::scaled(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize));
    int _iconH = _iconW;
    QSize s = QSize(_iconW, _iconH);

    ui->dictButton->setIcon(QIcon(triangleImage(ZKanji::dictionaryFlag(s, d->name(), Flags::Flag))));

    if (dynamic_cast<ZKanjiForm*>(window()) == nullptr)
        return;

    ((ZKanjiForm*)window())->updateDictionaryMenu();
}

void ZKanjiWidget::executeCommand(int command)
{
    if (command >= (int)CommandLimits::SearchBegin && command < (int)CommandLimits::SearchEnd)
    {
        DictionaryWidget *w = ui->pagesStack->currentWidget()->findChild<DictionaryWidget*>();
        if (w != nullptr)
            w->executeCommand(command);
    }

    if (command >= (int)CommandLimits::KanjiBegin && command < (int)CommandLimits::KanjiEnd)
    {
        ZKanjiGridView *w = ui->pagesStack->currentWidget()->findChild<ZKanjiGridView*>();
        if (w != nullptr)
            w->executeCommand(command);
    }

    if (command >= (int)CommandLimits::WordsBegin && command < (int)CommandLimits::WordsEnd)
    {
        ZDictionaryListView *w = ui->pagesStack->currentWidget()->findChild<ZDictionaryListView*>();
        if (w != nullptr)
            w->executeCommand(command);
    }

    if (command >= (int)CommandLimits::KanjiSearchBegin && command < (int)CommandLimits::KanjiSearchEnd)
    {
        KanjiSearchWidget *w = ui->pagesStack->currentWidget()->findChild<KanjiSearchWidget*>();
        if (w != nullptr)
            w->executeCommand(command);
    }
}

void ZKanjiWidget::commandState(int command, bool &enabled, bool &checked, bool &visible) const
{
    if (command >= (int)CommandLimits::SearchBegin && command < (int)CommandLimits::SearchEnd)
    {
        DictionaryWidget *w = ui->pagesStack->currentWidget()->findChild<DictionaryWidget*>();
        if (w != nullptr)
            w->commandState(command, enabled, checked, visible);
        else
            enabled = false;
    }

    if (command >= (int)CommandLimits::KanjiBegin && command < (int)CommandLimits::KanjiEnd)
    {
        ZKanjiGridView *w = ui->pagesStack->currentWidget()->findChild<ZKanjiGridView*>();
        if (w != nullptr)
            w->commandState(command, enabled, checked, visible);
        else
            enabled = false;
    }

    if (command >= (int)CommandLimits::WordsBegin && command < (int)CommandLimits::WordsEnd)
    {
        ZDictionaryListView *w = ui->pagesStack->currentWidget()->findChild<ZDictionaryListView*>();
        if (w != nullptr)
            w->commandState(command, enabled, checked, visible);
        else
            enabled = false;
    }

    if (command >= (int)CommandLimits::KanjiSearchBegin && command < (int)CommandLimits::KanjiSearchEnd)
    {
        KanjiSearchWidget *w = ui->pagesStack->currentWidget()->findChild<KanjiSearchWidget*>();
        if (w != nullptr)
            w->commandState(command, enabled, checked, visible);
        else
            enabled = false;
    }
}

void ZKanjiWidget::setMode(ViewModes val)
{
    if ((int)val == ui->pagesStack->currentIndex())
        return;

    ui->pagesStack->setCurrentIndex((int)val);

    switch (val)
    {
    case ViewModes::WordSearch:
        ui->modeButton->setIcon(QIcon(dictimg));
        break;
    case ViewModes::WordGroup:
        ui->modeButton->setIcon(QIcon(wgrpimg));
        break;
    case ViewModes::KanjiGroup:
        ui->modeButton->setIcon(QIcon(kgrpimg));
        break;
    case ViewModes::KanjiSearch:
        ui->modeButton->setIcon(QIcon(ksrcimg));
        break;
    }


//#ifdef Q_OS_MAX
//    if (window()->isActiveWindow() && isActiveWidget())
//        gUI->activateMenu(this);
//#else
    //if (/*window()->isActiveWindow() && docked() &&*/ isActiveWidget())
    if (dynamic_cast<ZKanjiForm*>(window()) == nullptr)
        return;

    ((ZKanjiForm*)window())->updateMainMenu();
//#endif
}

ViewModes ZKanjiWidget::mode() const
{
    return (ViewModes)ui->pagesStack->currentIndex();
}

//void ZKanjiWidget::docking(bool dock)
//{
//}

//QColor activecolor; //= QColor(Qt::white);

void ZKanjiWidget::showActivated(bool active)
{
    if (paintactive == active)
        return;

    paintactive = active;
    //activecolor = qApp->palette().color(QPalette::Highlight); //mixColors(/*qApp->palette().color(QPalette::Highlight)*/QColor(64, 192, 128), qApp->palette().color(QPalette::Window), 0.4);
    update();
}

//void ZKanjiWidget::setDockAllowAction(QAction *a)
//{
//    if (allowdockaction != nullptr)
//    {
//        disconnect(allowdockaction, &QAction::triggered, this, &ZKanjiWidget::allowActionTriggered);
//        disconnect(allowdockaction, &QObject::destroyed, this, &ZKanjiWidget::allowActionDestroyed);
//    }
//    allowdockaction = a;
//    if (allowdockaction != nullptr)
//    {
//        allowdockaction->setChecked(allowdocking);
//        connect(allowdockaction, &QAction::triggered, this, &ZKanjiWidget::allowActionTriggered);
//        connect(allowdockaction, &QObject::destroyed, this, &ZKanjiWidget::allowActionDestroyed);
//    }
//}

void ZKanjiWidget::addDockAction(QMenu *dest)
{
    if (dynamic_cast<ZKanjiForm*>(window()) == nullptr)
        return;

    bool onmain = ((ZKanjiForm*)window())->mainForm();
    if (!onmain || !((ZKanjiForm*)window())->hasDockChild())
        return;

    //if (!dest->isEmpty())
    dest->addSeparator();
    //if (onmain)
    connect(dest->addAction(tr("&Float")), &QAction::triggered, this, &ZKanjiWidget::floatToWindow);
    dest->addSeparator();
    //else
    //{
    //    QAction *a = dest->addAction("Allow docking");
    //    a->setCheckable(true);
    //    a->setChecked(allowdocking);
    //    connect(a, &QAction::triggered, this, &ZKanjiWidget::allowActionTriggered);
    //}
}

//bool ZKanjiWidget::dockingAllowed() const
//{
//    return allowdocking;
//}

bool ZKanjiWidget::docked() const
{
    ZKanjiForm *f = dynamic_cast<ZKanjiForm*>(window());
    return f != nullptr && f->mainForm();
}

bool ZKanjiWidget::isActiveWidget() const
{
    ZKanjiForm *f = dynamic_cast<ZKanjiForm*>(window());
    return f != nullptr && (!f->mainForm() || paintactive);
}

void ZKanjiWidget::allowActionTriggered(bool checked)
{
    allowdocking = checked;
    if (allowdockaction != nullptr)
        allowdockaction->setChecked(checked);
}

bool ZKanjiWidget::event(QEvent *e)
{
    if (e->type() == SetDictEvent::Type())
    {
        // Changes active dictionary to the dictionary specified by the event.
        int index = ((SetDictEvent*)e)->index();

        setDictionary(index);
        return true;
    }
    else if (e->type() == GetDictEvent::Type())
    {
        GetDictEvent *de = (GetDictEvent*)e;
        de->setResult(dictionaryIndex());
        return true;
    }
    else if (e->type() == QEvent::LanguageChange)
        setTranslationTexts();

    return base::event(e);
}

void ZKanjiWidget::paintEvent(QPaintEvent * /*e*/)
{
    if (!paintactive)
        return;

    QPainter p(this);

    //QRect r = rect();
    //r.setBottom(r.top() + 6);

    //QLinearGradient grad(r.topLeft(), r.bottomLeft());
    //grad.setColorAt(0, activecolor);
    //grad.setColorAt(1, Qt::transparent);
    //p.fillRect(r, grad);

    //QPen pen = activecolor; //palette().color(QPalette::Highlight);
    //pen.setWidth(2);
    //p.setPen(pen);
    //p.drawRect(rect().adjusted(1, 1, -1, -1));


    //QRect r = QRect(0, rect().bottom() - ui->statusWidget->height() * 0.5, ui->modeWidget->width() * 0.5, ui->modeWidget->height() * 0.5);
    int siz = ui->modeButton->height();

    //QLinearGradient grad(r.bottomLeft(), r.topRight());
    //grad.setColorAt(1, Qt::transparent);
    //grad.setColorAt(0.5, Qt::transparent);
    //grad.setColorAt(0.4, activecolor);
    //grad.setColorAt(0, activecolor);
    //p.fillRect(r, grad);

    //QRect r = rect();

    QPainterPath path;
    //path.addPolygon(QPolygonF({ QPointF(0, r.bottom() - siz * 0.5), QPointF(siz * 0.5, r.bottom()), QPointF(0, r.bottom()) }));
    //path.closeSubpath();
    path.addPolygon(QPolygonF({ QPointF(0, siz * 0.5), QPointF(0, 0), QPointF(siz * 0.5, 0) }));
    path.closeSubpath();
    //path.addPolygon(QPolygonF({ QPointF(r.right() - siz * 0.5, 0), QPointF(r.right(), 0), QPointF(r.right(), siz * 0.5) }));
    //path.closeSubpath();
    //path.addPolygon(QPolygonF({ QPointF(r.right() - siz * 0.5, r.bottom()), QPointF(r.right(), r.bottom() - siz * 0.5), QPointF(r.right(), r.bottom()) }));
    //path.closeSubpath();
    p.fillPath(path, qApp->palette().color(QPalette::Highlight));


    //p->fillRect(e->rect(), activecolor);

    //p->drawLine(0, 0, 0, rect().height());
    //p->drawLine(0, 0, rect().width(), 0);
}

void ZKanjiWidget::contextMenuEvent(QContextMenuEvent *e)
{
    //if (((ZKanjiForm*)window())->mainForm() && dynamic_cast<QSplitter*>(parentWidget()) != nullptr)
    //{
    //    dockmenu.popup(e->globalPos());
    //    e->accept();
    //}
    dockmenu.clear();
    addDockAction(&dockmenu);
    dockmenu.popup(e->globalPos());
    e->accept();
}

void ZKanjiWidget::allowActionDestroyed()
{
    allowdockaction = nullptr;
}

//void ZKanjiWidget::keyPressEvent(QKeyEvent *e)
//{
//    if (e->key() >= Qt::Key_1 && e->key() <= Qt::Key_4 && e->modifiers().testFlag(Qt::ControlModifier))
//    {
//        setMode(e->key() - Qt::Key_1);
//        e->accept();
//    }
//    base::keyPressEvent(e);
//}

void ZKanjiWidget::on_dictionary_wordDoubleClicked(int windex, int /*dindex*/)
{
    gUI->wordToDestSelect(ZKanji::dictionary(dictindex), windex);
}

void ZKanjiWidget::on_pagesStack_currentChanged(int index)
{
    if (index == -1)
        return;

    QSizePolicy pol = sizePolicy();
    // When the displayed widget is for kanji search, it shouldn't stretch horizontally when
    // placed in a splitter.
    pol.setHorizontalStretch(index == 3 ? 0 : 1);
    setSizePolicy(pol);

    switch (index)
    {
    case 0: // Dictionary
        ui->dictionary->assignStatusBar(ui->status);
        break;
    case 1: // Word groups
        ui->wordGroups->assignStatusBar(ui->status);
        break;
    case 2: // Kanji groups
        ui->kanjiGroups->assignStatusBar(ui->status);
        break;
    case 3: // Kanji search
        ui->kanjiSearch->assignStatusBar(ui->status);
        break;
    default:
        break;
    }
}

void ZKanjiWidget::dictionaryAdded()
{
    while (dictmenu.actions().size() != ZKanji::dictionaryCount())
    {
        int ix = dictmenu.actions().size();
        Dictionary *d = ZKanji::dictionary(ZKanji::dictionaryPosition(ix));
        connect(dictmenu.addAction(ZKanji::dictionaryMenuFlag(d->name()), d->name()), &QAction::triggered, this, &ZKanjiWidget::setDictByAction);
    }
}

void ZKanjiWidget::dictionaryRemoved(int index, int order)
{
    dictmenu.removeAction(dictmenu.actions().at(order));
    if (dictindex == index)
    {
        dictindex = -1;
        setDictionary(0);
    }
    else if (dictindex > index)
        --dictindex;
}

void ZKanjiWidget::dictionaryMoved(int from, int to)
{
    if (to > from)
        --to;
    dictmenu.actions().at(from)->deleteLater();
    dictmenu.removeAction(dictmenu.actions().at(from));

    Dictionary *d = ZKanji::dictionary(ZKanji::dictionaryPosition(to));
    QAction *action = new QAction(ZKanji::dictionaryMenuFlag(d->name()), d->name(), &dictmenu);
    if (to == dictmenu.actions().size())
        dictmenu.addAction(action);
    else
        dictmenu.insertAction(dictmenu.actions().at(to), action);
    connect(action, &QAction::triggered, this, &ZKanjiWidget::setDictByAction);
}

void ZKanjiWidget::dictionaryRenamed(const QString &/*oldname*/, int index, int order)
{
    dictmenu.actions().at(order)->setText(ZKanji::dictionary(index/*ZKanji::dictionaryPosition(order)*/)->name());
}

void ZKanjiWidget::dictionaryFlagChanged(int index, int order)
{
    if (index == dictindex)
    {
        int _iconW = Settings::scaled(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize));
        int _iconH = _iconW;

        QSize s = QSize(_iconW, _iconH);
        //QSize ts = triangleSize(QSize(_iconW, _iconH));
        ui->dictButton->setIcon(QIcon(triangleImage(ZKanji::dictionaryFlag(s, ZKanji::dictionary(dictindex)->name(), Flags::Flag))));

        dictmenu.actions().at(order)->setIcon(ZKanji::dictionaryMenuFlag(ZKanji::dictionary(index)->name()));
        //        connect(dictmenu.addAction(ZKanji::dictionaryMenuFlag(ZKanji::dictionary(ZKanji::dictionaryPosition(ix))->name()), ZKanji::dictionary(ZKanji::dictionaryPosition(ix))->name()), &QAction::triggered, this, &ZKanjiWidget::setDictByAction);

    }
}

void ZKanjiWidget::dictionaryReplaced(Dictionary* /*olddict*/, Dictionary* /*newdict*/, int index)
{
    if (dictindex == index)
    {
        dictindex = -1;
        setDictionary(index);
    }
}

void ZKanjiWidget::setTranslationTexts()
{
    modemenu.actions().at(0)->setText(QString("%1\tCtrl+1").arg(tr("Dictionary search")));
    modemenu.actions().at(1)->setText(QString("%1\tCtrl+2").arg(tr("Word groups")));
    modemenu.actions().at(2)->setText(QString("%1\tCtrl+3").arg(tr("Kanji groups")));
    modemenu.actions().at(3)->setText(QString("%1\tCtrl+4").arg(tr("Kanji search")));
}

void ZKanjiWidget::setModeByAction()
{
    setMode((ViewModes)(modemenu.actions().indexOf((QAction*)sender())));
}

void ZKanjiWidget::setDictByAction()
{
    setDictionary(ZKanji::dictionaryPosition(dictmenu.actions().indexOf((QAction*)sender())));
}

void ZKanjiWidget::floatToWindow()
{
#ifdef _DEBUG
    if (dynamic_cast<ZKanjiForm*>(window()) == nullptr || !dynamic_cast<ZKanjiForm*>(window())->mainForm())
        throw "Can't float already floating widget.";
#endif
    ((ZKanjiForm*)window())->floatWidget(this);
}

//void ZKanjiWidget::renderButton(QPixmap &dest, const QPixmap &orig)
//{
//    //QImage img = QImage(_iconW, _iconH, QImage::Format_ARGB32_Premultiplied);
//
//    QPainter p;
//    //img.fill(qRgba(0, 0, 0, 0));
//    //p.begin(&orig);
//    //renderFromSvg(p, path, QRect(0, 0, _iconW, _iconH));
//    //p.end();
//
//    int _iconW = orig.width();
//    int _iconH = orig.height();
//
//    dest = QPixmap(_iconW + _triS + _triPH, _iconH + _triPV);
//    dest.fill(QColor(0, 0, 0, 0));
//    p.begin(&dest);
//    p.drawPixmap(0, 0, orig);
//    p.end();
//}


//-------------------------------------------------------------
