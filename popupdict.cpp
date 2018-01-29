/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QStylePainter>
#include <QClipboard>
#include <QWindow>
#include <QScreen>

#include "popupdict.h"
#include "ui_popupdict.h"

#include "popupsettings.h"
#include "formstate.h"
#include "globalui.h"
#include "zevents.h"
#include "zdictionarylistview.h"
#include "words.h"
#include "dialogs.h"
#include "zui.h"


PopupDictionary *PopupDictionary::instance = nullptr;

//-------------------------------------------------------------

namespace FormStates
{
    PopupDictData popupdict;
}


PopupDictionary::PopupDictionary(QWidget *parent) : base(parent), ui(new Ui::PopupDictionary), floating(true), grabbing(false), ignoreresize(true), dictindex(-1)
{
    ui->setupUi(this);
    windowInit();

    gUI->scaleWidget(this);

    setAttribute(Qt::WA_QuitOnClose, false);
    //setAttribute(Qt::WA_ShowWithoutActivating, false);

    ui->dictionary->addBackWidget(ui->controlWidget);
    ui->pinButton->setCheckable(Settings::popup.autohide);

    ui->dictionary->view()->setSizeBase(ListSizeBase::Popup);
    ui->dictionary->view()->setGroupDisplay(true);

    connect(qApp, &QApplication::applicationStateChanged, this, &PopupDictionary::appStateChange);
    connect(gUI, &GlobalUI::settingsChanged, this, &PopupDictionary::settingsChanged);

    connect(gUI, &GlobalUI::dictionaryRemoved, this, &PopupDictionary::dictionaryRemoved);

    //ui->captionWidget->setVisible(false);
    //ui->captionWidget->installEventFilter(this);

    instance = this;

    floatWindow(!floating);
}

PopupDictionary::~PopupDictionary()
{
    delete ui;
}

void PopupDictionary::popup(bool translatefrom)
{
    if (instance == nullptr)
    {
        new PopupDictionary();
        instance->setWindowOpacity((10.0 - Settings::popup.transparency / 2.0) / 10.0);
    }

    qApp->postEvent(instance, new ShowPopupEvent(translatefrom));

    //instance->doPopup(translatefrom);
}

void PopupDictionary::hidePopup()
{
    if (instance != nullptr)
        instance->hide();
}

PopupDictionary* const PopupDictionary::getInstance()
{
    return instance;
}

int PopupDictionary::dictionaryIndex()
{
    if (instance == nullptr || instance->dictindex == -1)
        return std::max(0, ZKanji::dictionaryIndex(Settings::popup.dict));

    return instance->dictindex;
}

void PopupDictionary::doPopup(bool translatefrom)
{
    setDictionary(dictindex);

    floatWindow(FormStates::popupdict.floating);
    QString str;

    ui->dictionary->setSearchMode(translatefrom ? SearchMode::Japanese : SearchMode::Definition);

    switch (Settings::popup.activation)
    {
    case PopupSettings::Clear:
        ui->dictionary->setSearchText(QString());
        break;
    case PopupSettings::Clipboard:
        str = qApp->clipboard()->text();
        if (!str.isEmpty())
            ui->dictionary->setSearchText(str);
        break;
    case PopupSettings::Unchanged:
    default:
        break;
    }

    ui->pinButton->setIcon(QIcon(!Settings::popup.autohide ? ":/closex.svg" : ":/pin.svg"));

    show();
    if (Settings::popup.widescreen)
        resizeToFullWidth();
    raise();
    activateWindow();
}

QWidget* PopupDictionary::captionWidget() const
{
    return ui->captionWidget;
}

//QWidget* PopupDictionary::centralWidget() const
//{
//    return ui->centralwidget;
//}

void PopupDictionary::keyPressEvent(QKeyEvent *e)
{
    base::keyPressEvent(e);

    if (e->key() == Qt::Key_Escape)
        close();
}

void PopupDictionary::resizeEvent(QResizeEvent *e)
{
    base::resizeEvent(e);

    if (!isVisible() || ignoreresize)
        return;

    if (FormStates::popupdict.floating)
        FormStates::popupdict.floatrect = geometry();
    else
        FormStates::popupdict.normalsize = geometry().size();
}

void PopupDictionary::moveEvent(QMoveEvent *e)
{
    base::moveEvent(e);

    if (!isVisible() || ignoreresize || !FormStates::popupdict.floating)
        return;

    FormStates::popupdict.floatrect = geometry();
}

bool PopupDictionary::event(QEvent *e)
{
    if (e->type() == (int)EndEvent::Type())
    {
        ignoreresize = false;
        return true;
    }
    else if (e->type() == (int)SetDictEvent::Type())
    {
        SetDictEvent *se = (SetDictEvent*)e;
        setDictionary(se->index());
        return true;
    }
    else if (e->type() == (int)GetDictEvent::Type())
    {
        GetDictEvent *ge = (GetDictEvent*)e;
        ge->setResult(ZKanji::dictionaryIndex(ui->dictionary->dictionary()));
        return true;
    }
    else if (e->type() == (int)ShowPopupEvent::Type())
    {
        ShowPopupEvent *pe = (ShowPopupEvent*)e;
        doPopup(pe->from());
        return true;
    }

    return base::event(e);
}

void PopupDictionary::closeEvent(QCloseEvent *e)
{
    emit beingClosed();

    base::closeEvent(e);
}

//bool PopupDictionary::eventFilter(QObject *obj, QEvent *e)
//{
//    if (obj == ui->captionWidget && !beingResized() && (e->type() == QEvent::MouseMove || e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease))
//    {
//        QMouseEvent *me = (QMouseEvent*)e;
//        switch (me->type())
//        {
//        case QEvent::MouseMove:
//            if (grabbing)
//            {
//                QRect r = geometry();
//                move(r.left() + me->pos().x() - grabpos.x(), r.top() + me->pos().y() - grabpos.y());
//            }
//            break;
//        case QEvent::MouseButtonPress:
//            if (grabbing || me->button() != Qt::LeftButton)
//                break;
//            grabbing = true;
//            grabpos = me->pos();
//            break;
//        case QEvent::MouseButtonRelease:
//            if (!grabbing || me->button() != Qt::LeftButton)
//                break;
//            grabbing = false;
//            break;
//        }
//    }
//
//    return base::eventFilter(obj, e);
//}

void PopupDictionary::on_pinButton_clicked(bool checked)
{
    if (!Settings::popup.autohide)
        close();
}

void PopupDictionary::on_floatButton_clicked(bool checked)
{
    FormStates::popupdict.floating = !FormStates::popupdict.floating;
    floatWindow(FormStates::popupdict.floating);
}

void PopupDictionary::on_dictionary_wordDoubleClicked(int windex, int dindex)
{
    wordToGroupSelect(ZKanji::dictionary(dictindex), windex);
}

void PopupDictionary::appStateChange(Qt::ApplicationState state)
{
    if (!isVisible() || !Settings::popup.autohide || ui->pinButton->isChecked() || ignoreresize)
        return;

    if (state == Qt::ApplicationInactive)
        close();
}

void PopupDictionary::settingsChanged()
{
    ui->pinButton->setCheckable(Settings::popup.autohide);
    ui->pinButton->setIcon(QIcon(!Settings::popup.autohide ? ":/closex.svg" : ":/pin.svg"));
    setWindowOpacity((10.0 - Settings::popup.transparency / 2.0) / 10.0);

    if (FormStates::popupdict.floating || !Settings::popup.widescreen || !isVisible())
        return;

    resizeToFullWidth();
}

void PopupDictionary::dictionaryRemoved(int index)
{
    if (dictindex == index)
    {
        dictindex = -1;
        setDictionary(0);
    }
    else if (dictindex > index)
        --dictindex;
}

void PopupDictionary::floatWindow(bool dofloat)
{
    qApp->postEvent(this, new EndEvent);

    if (floating == dofloat)
        return;

    bool wasvisible = isVisible();
    if (wasvisible)
        hide();

    floating = dofloat;

    QRect r = qApp->desktop()->availableGeometry(instance);

    ignoreresize = true;
    if (!dofloat)
    {
        //// The native window must be destroyed because Qt incorrectly updates the geometry
        //// after removing/adding a frame with setWindowFlags().
        //QWindow *natwin = windowHandle();
        //if (natwin)
        //    natwin->destroy();

        //setWindowFlags(Qt::Window |
        //    // TODO: test on linux to see if this flag is needed, and if so whether it breaks current functionality.
        //    //Qt::X11BypassWindowManagerHint |
        //    Qt::FramelessWindowHint | /*Qt::CustomizeWindowHint |*/ Qt::WindowStaysOnTopHint);

        ui->dictionary->addBackWidget(ui->controlWidget);
        //ui->captionWidget->setVisible(false);

        setBorderStyle(BorderStyle::Docked);

        QSize &s = FormStates::popupdict.normalsize;
        if (s.isNull() || !s.isValid())
            s = QSize(r.width() - 8, 224);

        if (s.width() > r.width() - 4)
            s.setWidth(r.width() - 8);
        if (s.height() > r.height() - 4)
            s.setHeight(r.height() - 8);

        setGeometry(QRect(r.right() - s.width() - 4, r.bottom() - s.height() - 4, s.width(), s.height()));
        resizeToFullWidth();

        ui->floatButton->setIcon(QIcon(":/floatwnd.svg"));

        if (wasvisible)
        {
            show();
            raise();
            activateWindow();
        }
    }
    else
    {
        //// The native window must be destroyed because Qt incorrectly updates the geometry
        //// after removing/adding a frame with setWindowFlags().
        //QWindow *natwin = windowHandle();
        //if (natwin)
        //    natwin->destroy();

        //setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);

        //ui->captionWidget->setVisible(true);
        ui->captionLayout->addWidget(ui->controlWidget);

        setBorderStyle(BorderStyle::Resizable);

        QRect &fr = FormStates::popupdict.floatrect;

        if (fr.isNull() || !fr.isValid())
            fr = QRect(r.left() + r.width() / 2 - r.width() / 8, r.top() + r.height() / 2 - r.height() / 16, r.width() / 4, r.height() / 8);

        if (fr.width() > r.width())
            fr.setWidth(r.width() - 8);
        if (fr.height() > r.height())
            fr.setHeight(r.height() - 8);
        if (fr.left() < r.left())
            fr.moveLeft(r.left());
        if (fr.top() < r.top())
            fr.moveTop(r.top());

        //QRect fg = frameGeometry();
        //QRect g = geometry();

        //fr.adjust(g.left() - fg.left(), g.top() - fg.top(), g.right() - fg.right(), g.bottom() - fg.bottom());

        setGeometry(fr);

        ui->floatButton->setIcon(QIcon(":/dockwnd.svg"));

        if (wasvisible)
        {
            show();
            raise();
            activateWindow();
        }

        layout()->invalidate();
        layout()->activate();
        layout()->update();
        ui->layout->invalidate();
        ui->layout->activate();
        ui->layout->update();

        //ui->centralwidget->setGeometry(geometry());
    }
}

void PopupDictionary::resizeToFullWidth()
{
    if (floating || !Settings::popup.widescreen)
        return;

    winId();
    if (windowHandle()->screen() == nullptr)
        return;

    QRect sr = qApp->desktop()->availableGeometry(instance);
    QRect r = geometry();
    r.setLeft(sr.left() + 4);
    r.setWidth(sr.width() - 8);
    setGeometry(r);
}

void PopupDictionary::setDictionary(int index)
{
    if (index == -1)
        index = std::max(0, ZKanji::dictionaryIndex(Settings::popup.dict));

    Settings::popup.dict = ZKanji::dictionary(index)->name();

    if (dictindex == index)
        return;

    dictindex = index;
    ui->dictionary->setDictionary(dictindex);
}


//-------------------------------------------------------------

