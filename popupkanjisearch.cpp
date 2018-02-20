/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QDesktopWidget>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "popupkanjisearch.h"
#include "ui_popupkanjisearch.h"

#include "popupsettings.h"
#include "kanjisettings.h"
#include "zevents.h"
#include "globalui.h"
#include "zui.h"


PopupKanjiSearch *PopupKanjiSearch::instance = nullptr;

struct PopupKanjiData
{
    QRect floatrect;
    KanjiFilterData filters;
};

namespace FormStates
{
    PopupKanjiData popupkanji;

    bool emptyState(const KanjiFilterData &data);
    void saveXMLSettings(const KanjiFilterData &data, QXmlStreamWriter &writer);
    void loadXMLSettings(KanjiFilterData &data, QXmlStreamReader &reader);

    bool emptyState(const PopupKanjiData &data)
    {
        return !data.floatrect.isValid() && emptyState(data.filters);
    }

    void saveXMLSettings(const PopupKanjiData &data, QXmlStreamWriter &writer)
    {
        if (!data.floatrect.isEmpty())
        {
            writer.writeAttribute("floatleft", QString::number(data.floatrect.left()));
            writer.writeAttribute("floattop", QString::number(data.floatrect.top()));
            writer.writeAttribute("floatwidth", QString::number(data.floatrect.width()));
            writer.writeAttribute("floatheight", QString::number(data.floatrect.height()));
        }

        if (Settings::kanji.savefilters)
        {
            writer.writeStartElement("Filters");
            saveXMLSettings(data.filters, writer);
            writer.writeEndElement(); /* Dictionary */
        }
    }

    void loadXMLSettings(PopupKanjiData &data, QXmlStreamReader &reader)
    {
        bool ok = false;
        int val;

        data.floatrect = QRect(0, 0, 0, 0);
        if (reader.attributes().hasAttribute("floatleft"))
        {
            val = reader.attributes().value("floatleft").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.floatrect.setLeft(val);
        }
        if (ok && reader.attributes().hasAttribute("floattop"))
        {
            val = reader.attributes().value("floattop").toInt(&ok);
            if (val < -999999 || val > 999999)
                ok = false;
            if (ok)
                data.floatrect.setTop(val);
        }
        if (reader.attributes().hasAttribute("floatwidth"))
        {
            val = reader.attributes().value("floatwidth").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.floatrect.setWidth(val);
        }
        if (ok && reader.attributes().hasAttribute("floatheight"))
        {
            val = reader.attributes().value("floatheight").toInt(&ok);
            if (val < 0 || val > 999999)
                ok = false;
            if (ok)
                data.floatrect.setHeight(val);
        }
        if (!ok)
            data.floatrect = QRect();

        while (reader.readNextStartElement())
        {
            if (Settings::kanji.savefilters && reader.name() == "Filters")
                loadXMLSettings(data.filters, reader);
            else
                reader.skipCurrentElement();
        }
    }
}


//-------------------------------------------------------------


PopupKanjiSearch::PopupKanjiSearch(QWidget *parent) : base(parent), ui(new Ui::PopupKanjiSearch), ignoreresize(true)
{
    ui->setupUi(this);
    windowInit();

    gUI->scaleWidget(this);

    setAttribute(Qt::WA_QuitOnClose, false);
    //setAttribute(Qt::WA_ShowWithoutActivating, false);

    ui->pinButton->setCheckable(Settings::popup.kanjiautohide);
    //ui->captionWidget->installEventFilter(this);

    connect(qApp, &QApplication::applicationStateChanged, this, &PopupKanjiSearch::appStateChange);
    connect(gUI, &GlobalUI::settingsChanged, this, &PopupKanjiSearch::settingsChanged);

    instance = this;

    //if (FormStates::popupkanji.floatrect.isValid())
    //    setGeometry(FormStates::popupkanji.floatrect);
    //else
    //{
    //    QRect r = qApp->desktop()->availableGeometry(instance);
    //    move(r.left() + 32, r.top() + 128);
    //}

    ui->kanjiSearch->restoreState(FormStates::popupkanji.filters);
}

PopupKanjiSearch::~PopupKanjiSearch()
{
    delete ui;
}

void PopupKanjiSearch::popup(int screen)
{
    if (instance == nullptr)
    {
        new PopupKanjiSearch();
        instance->setWindowOpacity((10.0 - Settings::popup.transparency / 2.0) / 10.0);
    }

    instance->doPopup(screen);
}

void PopupKanjiSearch::hidePopup()
{
    if (instance != nullptr)
        instance->hide();
}

PopupKanjiSearch * const PopupKanjiSearch::getInstance()
{
    return instance;
}

void PopupKanjiSearch::doPopup(int screen)
{
    if (isVisible())
        return;

    qApp->postEvent(this, new EndEvent());

    if (Settings::kanji.resetpopupfilters)
        ui->kanjiSearch->reset();

    ui->pinButton->setIcon(QIcon(!Settings::popup.kanjiautohide ? ":/closex.svg" : ":/pin.svg"));

    QRect sg = screen != -1 ? qApp->desktop()->screenGeometry(screen) : qApp->desktop()->screenGeometry(instance);

    if (FormStates::popupkanji.floatrect.isValid())
    {
        QRect geom = FormStates::popupkanji.floatrect;
        if (geom.width() > sg.width() - 8)
            geom.setWidth(sg.width() - 8);
        if (geom.height() > sg.height() - 8)
            geom.setHeight(sg.height() - 8);
        geom.moveTo(sg.topLeft() + geom.topLeft());
        if (geom.left() < sg.left())
            geom.moveLeft(sg.left());
        else if (geom.left() + geom.width() > sg.left() + sg.width())
            geom.moveLeft(sg.left() + sg.width() - geom.width());
        if (geom.top() < sg.top())
            geom.moveTop(sg.top());
        else if (geom.top() + geom.height() > sg.top() + sg.height())
            geom.moveTop(sg.top() + sg.height() - geom.height());

        setGeometry(geom);
    }
    else
    {
        QRect ag = screen != -1 ? qApp->desktop()->availableGeometry(screen) : qApp->desktop()->availableGeometry(instance);
        move(ag.left() + 32, ag.top() + 128);
    }



    show();
    raise();
    activateWindow();
}

QWidget* PopupKanjiSearch::captionWidget() const
{
    return ui->captionWidget;
}

//QWidget* PopupKanjiSearch::centralWidget() const
//{
//    return ui->centralwidget;
//}

void PopupKanjiSearch::keyPressEvent(QKeyEvent *e)
{
    base::keyPressEvent(e);

    if (e->key() == Qt::Key_Escape)
        close();
}

void PopupKanjiSearch::resizeEvent(QResizeEvent *e)
{
    base::resizeEvent(e);

    if (!isVisible() || ignoreresize)
        return;

    QRect sg = qApp->desktop()->screenGeometry(instance);
    QRect r = geometry();
    FormStates::popupkanji.floatrect = QRect(r.topLeft() - sg.topLeft(), r.size());
}

void PopupKanjiSearch::moveEvent(QMoveEvent *e)
{
    base::moveEvent(e);

    if (!isVisible() || ignoreresize)
        return;

    QRect sg = qApp->desktop()->screenGeometry(instance);
    QRect r = geometry();
    FormStates::popupkanji.floatrect = QRect(r.topLeft() - sg.topLeft(), r.size());
}

void PopupKanjiSearch::closeEvent(QCloseEvent *e)
{
    emit beingClosed();

    // TODO: save state in destroy and make sure the data is only saved after that
    ui->kanjiSearch->saveState(FormStates::popupkanji.filters);

    base::closeEvent(e);
}

bool PopupKanjiSearch::event(QEvent *e)
{
    if (e->type() == (int)EndEvent::Type())
    {
        ignoreresize = false;
        return true;
    }

    return base::event(e);
}

void PopupKanjiSearch::appStateChange(Qt::ApplicationState state)
{
    if (!isVisible() || !Settings::popup.kanjiautohide || ui->pinButton->isChecked()/* || ignoreresize*/)
        return;

    if (state == Qt::ApplicationInactive)
        close();
}

void PopupKanjiSearch::on_pinButton_clicked(bool checked)
{
    if (!Settings::popup.kanjiautohide)
        close();
}

void PopupKanjiSearch::settingsChanged()
{
    ui->pinButton->setCheckable(Settings::popup.kanjiautohide);
    ui->pinButton->setIcon(QIcon(!Settings::popup.kanjiautohide ? ":/closex.svg" : ":/pin.svg"));

    setWindowOpacity((10.0 - Settings::popup.transparency / 2.0) / 10.0);
}


//-------------------------------------------------------------

