/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
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
#include "formstates.h"
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

    //floatWindow(!floating);
}

PopupDictionary::~PopupDictionary()
{
    delete ui;
}

void PopupDictionary::popup(int screen, bool fromjapanese)
{
    if (instance == nullptr)
    {
        new PopupDictionary();
        instance->setWindowOpacity((10.0 - Settings::popup.transparency / 2.0) / 10.0);
    }

    qApp->postEvent(instance, new ShowPopupEvent(screen, fromjapanese));
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

void PopupDictionary::doPopup(int screen, bool fromjapanese)
{
    setDictionary(dictindex);

    floatWindow(FormStates::popupdict.floating, screen);
    QString str;

    ui->dictionary->setSearchMode(fromjapanese ? SearchMode::Japanese : SearchMode::Definition);

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
    {
        FormStates::popupdict.floatrect = geometry();
        QRect sg = qApp->desktop()->screenGeometry(this);
        FormStates::popupdict.floatrect.moveTo(FormStates::popupdict.floatrect.topLeft() - sg.topLeft());
    }
    else
        FormStates::popupdict.normalsize = geometry().size();
}

void PopupDictionary::moveEvent(QMoveEvent *e)
{
    base::moveEvent(e);

    if (!isVisible() || ignoreresize || !FormStates::popupdict.floating)
        return;

    FormStates::popupdict.floatrect = geometry();
    QRect sg = qApp->desktop()->screenGeometry(this);
    FormStates::popupdict.floatrect.moveTo(FormStates::popupdict.floatrect.topLeft() - sg.topLeft());
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
        doPopup(pe->screen(), pe->from());
        return true;
    }

    return base::event(e);
}

void PopupDictionary::closeEvent(QCloseEvent *e)
{
    emit beingClosed();

    base::closeEvent(e);
}

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

void PopupDictionary::floatWindow(bool dofloat, int screen)
{
    qApp->postEvent(this, new EndEvent);
    ignoreresize = true;

    if (floating == dofloat && (isVisible() || screen == -1))
        return;

    bool wasvisible = isVisible();
    if (wasvisible)
        hide();

    floating = dofloat;

    QRect ag = (!wasvisible && screen != -1) ? qApp->desktop()->availableGeometry(screen) : qApp->desktop()->availableGeometry(instance);

    if (!dofloat)
    {
        QWindow *natwin = windowHandle();
        if (natwin)
            natwin->destroy();

        ui->dictionary->addBackWidget(ui->controlWidget);

        setBorderStyle(BorderStyle::Docked);

        QSize &s = FormStates::popupdict.normalsize;
        if (s.isNull() || !s.isValid())
            s = QSize(ag.width() - 8, 224);

        if (s.width() > ag.width() - 4)
            s.setWidth(ag.width() - 8);
        if (s.height() > ag.height() - 4)
            s.setHeight(ag.height() - 8);

        setGeometry(QRect(ag.right() - s.width() - 4, ag.bottom() - s.height() - 4, s.width(), s.height()));
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
        QWindow *natwin = windowHandle();
        if (natwin)
            natwin->destroy();

        ui->captionLayout->addWidget(ui->controlWidget);

        setBorderStyle(BorderStyle::Resizable);

        QRect &fr = FormStates::popupdict.floatrect;

        if (fr.isEmpty())
            fr = QRect(ag.left() + ag.width() / 2 - ag.width() / 8, ag.top() + ag.height() / 2 - ag.height() / 16, ag.width() / 4, ag.height() / 8);
        else
        {
            QRect sg = (!wasvisible && screen != -1) ? qApp->desktop()->screenGeometry(screen) : qApp->desktop()->screenGeometry(instance);
            fr.moveTo(sg.topLeft() + fr.topLeft());
        }

        if (fr.width() > ag.width())
            fr.setWidth(ag.width() - 8);
        if (fr.height() > ag.height())
            fr.setHeight(ag.height() - 8);
        if (fr.left() < ag.left())
            fr.moveLeft(ag.left());
        else if (fr.left() + fr.width() > ag.left() + ag.width())
            fr.moveLeft(ag.left() + ag.width() - fr.width());
        if (fr.top() < ag.top())
            fr.moveTop(ag.top());
        else if (fr.top() + fr.height() > ag.top() + ag.height())
            fr.moveTop(ag.top() + ag.height() - fr.height());

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

