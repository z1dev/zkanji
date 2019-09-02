/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QApplication>
#include <QClipboard>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QStringBuilder>
#include <QStylePainter>
#include <QMenu>

#include "kanjisearchwidget.h"
#include "ui_kanjisearchwidget.h"
#include "zkanjimain.h"
#include "words.h"
#include "kanji.h"
#include "zkanjigridmodel.h"
#include "zui.h"
#include "zevents.h"
#include "romajizer.h"
#include "radform.h"
#include "zradicalgrid.h"
#include "zflowlayout.h"
#include "globalui.h"
#include "zkanjiwidget.h"
#include "kanjisettings.h"
#include "generalsettings.h"
#include "zkanjiform.h"
#include "zkanjiwidget.h"
#include "fontsettings.h"
#include "popupkanjisearch.h"

#include "checked_cast.h"


//-------------------------------------------------------------


ZComboButton::ZComboButton(QWidget *parent) : base(parent), mouseover(false)
{
    setContextMenuPolicy(Qt::PreventContextMenu);
    setAttribute(Qt::WA_MouseTracking);
}

ZComboButton::~ZComboButton()
{
    ;
}

void ZComboButton::paintEvent(QPaintEvent * /*e*/)
{
    int radindex = currentIndex();
    QString str = radindex == 0 || mouseover ? tr("Click to select...") : radicalFiltersModel().filterText(radindex - 1);
    QStylePainter p(this);

    QStyleOptionComboBox boxopt;
    initStyleOption(&boxopt);
    boxopt.editable = true;
    p.setPen(palette().color(QPalette::Text));

    p.drawComplexControl(QStyle::CC_ComboBox, boxopt);

    QRect r = rect();
    r.setRight(style()->subControlRect(QStyle::CC_ComboBox, &boxopt, QStyle::SC_ComboBoxArrow, this).left() - 1);

#ifdef Q_OS_WIN
    if (hasFocus())
    {
        QStyleOptionFocusRect focus;
        focus.QStyleOption::operator=(boxopt);
        focus.rect = style()->subElementRect(QStyle::SE_ComboBoxFocusRect, &boxopt, this);
        focus.state |= QStyle::State_FocusAtBorder;
        focus.backgroundColor = boxopt.palette.base().color();
        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focus, &p, this);

        //QStyleOptionFocusRect option;
        //option.initFrom(this);
        //option.backgroundColor = palette().color(QPalette::Background);
        //option.rect = r.adjusted(Settings::scaled(1), Settings::scaled(1), Settings::scaled(0), Settings::scaled(-1));

        //style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &p, this);
    }
#endif

    r.adjust(Settings::scaled(4), Settings::scaled(3), -Settings::scaled(2), -Settings::scaled(3));
    p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, str);
}

void ZComboButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;

    QStyleOptionComboBox boxopt;
    initStyleOption(&boxopt);
    boxopt.editable = true;
    QRect r = style()->subControlRect(QStyle::CC_ComboBox, &boxopt, QStyle::SC_ComboBoxArrow, this);

    if (!r.contains(e->pos()))
        emit clicked();
    else
        base::mousePressEvent(e);
}

void ZComboButton::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() == 0)
    {
        QStyleOptionComboBox boxopt;
        initStyleOption(&boxopt);
        boxopt.editable = true;
        QRect r = style()->subControlRect(QStyle::CC_ComboBox, &boxopt, QStyle::SC_ComboBoxArrow, this);

        if (!r.contains(e->pos()))
        {
            if (!mouseover)
            {
                mouseover = true;
                update();
            }
        }
        else
        {
            if (mouseover)
            {
                mouseover = false;
                update();
            }
        }
    }

    base::mouseMoveEvent(e);
}

void ZComboButton::leaveEvent(QEvent *e)
{
    if (mouseover)
    {
        mouseover = false;
        update();
    }

    base::leaveEvent(e);
}

void ZComboButton::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
    {
        emit clicked();
        return;
    }
    base::keyPressEvent(e);
}


//-------------------------------------------------------------


bool filterActive(KanjiFilterData dat, KanjiFilters f)
{
    return (dat.filters & (1 << (int)f)) != 0;
}

bool filterActive(uchar dat, KanjiFilters f)
{
    return (dat & (1 << (int)f)) != 0;
}

void setFilter(KanjiFilterData &dat, KanjiFilters f, bool set)
{
    if (set)
        dat.filters |= (1 << (int)f);
    else
        dat.filters &= ~(1 << (int)f);
}

void setFilter(uchar &dat, KanjiFilters f, bool set)
{
    if (set)
        dat |= (1 << (int)f);
    else
        dat &= ~(1 << (int)f);
}


//-------------------------------------------------------------


namespace FormStates
{
    KanjiFilterData kanjifilters;

    bool emptyState(const KanjiFilterData &data)
    {
        return data.filters == 0xff && data.ordertype == KanjiGridSortOrder::Jouyou && data.fromtype == KanjiFromT::All &&
            data.strokemin == 0 && data.strokemax == 0 && data.meaning.isEmpty() && data.meaningafter && data.reading.isEmpty() &&
            data.readingafter && !data.readingstrict && data.readingoku && data.jlptmin == -1 && data.jlptmax == -1 &&
            data.skip1 == 0 && data.skip2 == -1 && data.skip3 == -1 && data.jouyou == 0 && data.indextype == KanjiIndexT::Unicode &&
            data.index.isEmpty() && data.rads == RadicalFilter() /*data.fromtype == KanjiFromT::All && *//*data.rads.mode == RadicalFilter::Parts && data.rads.grouped == false &&
            data.rads.groups.empty()*/;
    }

    void saveXMLSettings(const KanjiFilterData &data, QXmlStreamWriter &writer)
    {
        const QString True = QStringLiteral("1");
        const QString False = QStringLiteral("0");

        const QString ordertext[] = { "jouyou", "jlpt", "freq", "words", "wfreq", "strokes", "rad", "unicode", "jis208" };
        //if (Settings::general.savewinstates)
        writer.writeAttribute("order", ordertext[(int)data.ordertype]);

        const QString fromtext[] = { "all", "clipbrd", "common", "jouyou", "jlpt",
            "oneil", "gakken", "halper", "heisig", "heisign", "heisigf", "henshall",
            "nelson", "nnelson", "snh", "knk", "busy", "crowley", "flashc", "kguide",
            "halpernn", "deroo", "sakade", "henshallg", "context", "halpernk", "halpernl",
            "tuttle" };

        //if (Settings::general.savewinstates)
        writer.writeAttribute("from", fromtext[(int)data.fromtype]);

        if (((data.strokemin > 0 || data.strokemax > 0)/* && Settings::kanji.savefilters*/) || (!filterActive(data, KanjiFilters::Strokes)/* && Settings::general.savewinstates*/))
        {
            writer.writeEmptyElement("Stroke");
            if ((data.strokemin > 0 || data.strokemax > 0)/* && Settings::kanji.savefilters*/)
                writer.writeAttribute("count", IntMinMaxToString(1, std::numeric_limits<int>::max() - 1, data.strokemin, data.strokemax));
            if (!filterActive(data, KanjiFilters::Strokes)/* && Settings::general.savewinstates*/)
                writer.writeAttribute("hide", "1");
        }

        //if (Settings::kanji.savefilters || (!filterActive(data, KanjiFilters::Meaning)/* && Settings::general.savewinstates*/))
        //{
        writer.writeEmptyElement("Meaning");
        //if (Settings::kanji.savefilters)
        //{
        writer.writeAttribute("anyafter", data.meaningafter ? True : False);
        writer.writeAttribute("text", data.meaning);
        //}
        if (!filterActive(data, KanjiFilters::Meaning))
            writer.writeAttribute("hide", "1");
        //}

        //if (Settings::kanji.savefilters || (!filterActive(data, KanjiFilters::Reading)/* && Settings::general.savewinstates*/))
        //{
        writer.writeEmptyElement("Reading");
        //if (Settings::kanji.savefilters)
        //{
        writer.writeAttribute("anyafter", data.readingafter ? True : False);
        writer.writeAttribute("strict", data.readingstrict ? True : False);
        writer.writeAttribute("oku", data.readingoku ? True : False);
        writer.writeAttribute("text", data.reading);
        //}

        if (!filterActive(data, KanjiFilters::Reading))
            writer.writeAttribute("hide", "1");
        //}

        if (((data.jlptmin >= 0 || data.jlptmax >= 0)/* && Settings::kanji.savefilters*/) || (!filterActive(data, KanjiFilters::JLPT)/* && Settings::general.savewinstates*/))
        {
            writer.writeEmptyElement("JLPT");
            if ((data.jlptmin >= 0 || data.jlptmax >= 0)/* && Settings::kanji.savefilters*/)
                writer.writeAttribute("text", IntMinMaxToString(0, 5, data.jlptmin, data.jlptmax));
            if (!filterActive(data, KanjiFilters::JLPT))
                writer.writeAttribute("hide", "1");
        }

        //if (Settings::kanji.savefilters || (!filterActive(data, KanjiFilters::Jouyou)/* && Settings::general.savewinstates*/))
        //{
        const QString jouyoutext[] = { "all", "e1", "e2", "e3", "e4", "e5", "e6", "eall", "s", "n", "vn" };
        writer.writeEmptyElement("Jouyou");
        //if (Settings::kanji.savefilters)
        writer.writeAttribute("grade", jouyoutext[data.jouyou]);
        if (!filterActive(data, KanjiFilters::Jouyou))
            writer.writeAttribute("hide", "1");
        //}

        if ((/*Settings::kanji.savefilters && */(data.skip1 != 0 || data.skip2 != -1 || data.skip3 != -1)) || (!filterActive(data, KanjiFilters::SKIP)/* && Settings::general.savewinstates*/))
        {
            writer.writeEmptyElement("SKIP");
            if (Settings::kanji.savefilters && (data.skip1 != 0 || data.skip2 != -1 || data.skip3 != -1))
            {
                if (data.skip1 != 0)
                    writer.writeAttribute("type", QString::number(data.skip1));
                if (data.skip2 != -1)
                    writer.writeAttribute("field1", QString::number(data.skip2));
                if (data.skip3 != -1)
                    writer.writeAttribute("field2", QString::number(data.skip3));
            }
            if (!filterActive(data, KanjiFilters::SKIP))
                writer.writeAttribute("hide", "1");
        }

        //if (Settings::kanji.savefilters || (!filterActive(data, KanjiFilters::Index) && Settings::general.savewinstates))
        //{
        const QString indextext[] = { "unicode", "euc", "sjis", "jis208", "kuten",
            "oneil", "gakken", "halper", "heisig", "heisign", "heisigf", "henshall",
            "nelson", "nnelson", "snh", "knk", "busy", "crowley", "flashc", "kguide",
            "halpernn", "deroo", "sakade", "henshallg", "context", "halpernk", "halpernl",
            "tuttle" };

        writer.writeEmptyElement("Index");
        //if (Settings::kanji.savefilters)
        //{
        writer.writeAttribute("type", indextext[(int)data.indextype]);
        if (!data.index.isEmpty())
            writer.writeAttribute("text", data.index);
        //}
        if (!filterActive(data, KanjiFilters::Index))
            writer.writeAttribute("hide", "1");
        //}

        if ((/*Settings::kanji.savefilters && */data.rads != RadicalFilter()) || (!filterActive(data, KanjiFilters::Radicals)/* && Settings::general.savewinstates*/))
        {
            writer.writeStartElement("Radicals");
            if (data.rads != RadicalFilter())
            {
                writer.writeAttribute("mode", data.rads.mode == RadicalFilterModes::Parts ? "parts" : data.rads.mode == RadicalFilterModes::Radicals ? "radicals" : "named");
                writer.writeAttribute("grouped", data.rads.grouped ? True : False);
            }
            if (!filterActive(data, KanjiFilters::Radicals)/* && Settings::general.savewinstates*/)
                writer.writeAttribute("hide", True);
            if (!data.rads.groups.empty())
            {
                for (int ix = 0, siz = tosigned(data.rads.groups.size()); ix != siz; ++ix)
                {
                    if (data.rads.groups[ix].empty())
                        continue;

                    writer.writeEmptyElement("List");
                    QString val = QString::number(data.rads.groups[ix][0]);
                    for (int iy = 1, ysiz = tosigned(data.rads.groups[ix].size()); iy != ysiz; ++iy)
                        val += "," + QString::number(data.rads.groups[ix][iy]);
                    writer.writeAttribute("values", val);
                }
            }
            writer.writeEndElement();
        }
    }

    void loadXMLSettings(KanjiFilterData &data, QXmlStreamReader &reader)
    {
        const QString True = QStringLiteral("1");
        const QString False = QStringLiteral("0");

        data = KanjiFilterData();

        if (/*Settings::general.savewinstates && */reader.attributes().hasAttribute("order"))
        {
            const int ordersize = 9;
            const QString ordertext[ordersize] = { "jouyou", "jlpt", "freq", "words", "wfreq", "strokes", "rad", "unicode", "jis208" };
            const QString *pos = std::find(ordertext, ordertext + ordersize, reader.attributes().value("order").toString());
            if (pos != ordertext + ordersize)
                data.ordertype = (KanjiGridSortOrder)(pos - ordertext);
        }

        if (/*Settings::general.savewinstates && */reader.attributes().hasAttribute("from"))
        {
            const int fromsize = 28;
            const QString fromtext[fromsize] = { "all", "clipbrd", "common", "jouyou", "jlpt",
                "oneil", "gakken", "halper", "heisig", "heisign", "heisigf", "henshall",
                "nelson", "nnelson", "snh", "knk", "busy", "crowley", "flashc", "kguide",
                "halpernn", "deroo", "sakade", "henshallg", "context", "halpernk", "halpernl",
                "tuttle" };
            const QString *pos = std::find(fromtext, fromtext + fromsize, reader.attributes().value("from").toString());
            if (pos != fromtext + fromsize)
                data.fromtype = (KanjiFromT)(pos - fromtext);
        }

        while (reader.readNextStartElement())
        {
            if (reader.name() == "Stroke")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("count"))
                    findStrIntMinMax(reader.attributes().value("count").toString(), 1, std::numeric_limits<int>::max() - 1, data.strokemin, data.strokemax);
                if (/*Settings::general.savewinstates && */reader.attributes().value("hide") == True)
                    setFilter(data, KanjiFilters::Strokes, false);
                reader.skipCurrentElement();
            }

            if (reader.name() == "Meaning")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("anyafter"))
                    data.meaningafter = reader.attributes().value("anyafter") != False;
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("text"))
                    data.meaning = reader.attributes().value("text").toString();
                if (/*Settings::general.savewinstates && */reader.attributes().value("hide") == True)
                    setFilter(data, KanjiFilters::Meaning, false);
                reader.skipCurrentElement();
            }

            if (reader.name() == "Reading")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("anyafter"))
                    data.readingafter = reader.attributes().value("anyafter") != False;
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("strict"))
                    data.readingstrict = reader.attributes().value("strict") != False;
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("oku"))
                    data.readingoku = reader.attributes().value("oku") != False;
                if (Settings::kanji.savefilters)
                    data.reading = reader.attributes().value("text").toString();

                if (/*Settings::general.savewinstates && */reader.attributes().value("hide") == True)
                    setFilter(data, KanjiFilters::Reading, false);
                reader.skipCurrentElement();
            }

            if (reader.name() == "JLPT")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("text"))
                    findStrIntMinMax(reader.attributes().value("text").toString(), 0, 5, data.jlptmin, data.jlptmax);
                if (/*Settings::general.savewinstates && */reader.attributes().value("hide") == True)
                    setFilter(data, KanjiFilters::JLPT, false);
                reader.skipCurrentElement();
            }

            if (reader.name() == "Jouyou")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("grade"))
                {
                    const int jouyousize = 11;
                    const QString jouyoutext[jouyousize] = { "all", "e1", "e2", "e3", "e4", "e5", "e6", "eall", "s", "n", "vn" };
                    const QString *pos = std::find(jouyoutext, jouyoutext + jouyousize, reader.attributes().value("grade").toString());
                    if (pos != jouyoutext + jouyousize)
                        data.jouyou = (pos - jouyoutext);
                }
                if (/*Settings::general.savewinstates && */reader.attributes().value("hide") == True)
                    setFilter(data, KanjiFilters::Jouyou, false);

                reader.skipCurrentElement();
            }

            if (reader.name() == "SKIP")
            {
                bool ok = false;
                int val;
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("type"))
                {
                    val = reader.attributes().value("type").toInt(&ok);
                    if (ok && val >= 1 && val <= 4)
                        data.skip1 = val;
                }
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("field1"))
                {
                    val = reader.attributes().value("field1").toInt(&ok);
                    if (ok && val >= 0)
                        data.skip2 = val;
                }
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("field2"))
                {
                    val = reader.attributes().value("field2").toInt(&ok);
                    if (ok && val >= 0)
                        data.skip3 = val;
                }
                if (/*Settings::general.savewinstates && */reader.attributes().value("hide") == True)
                    setFilter(data, KanjiFilters::SKIP, false);
                reader.skipCurrentElement();
            }


            if (reader.name() == "Index")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("type"))
                {
                    const int indexsize = 28;
                    const QString indextext[indexsize] = { "unicode", "euc", "sjis", "jis208", "kuten",
                        "oneil", "gakken", "halper", "heisig", "heisign", "heisigf", "henshall",
                        "nelson", "nnelson", "snh", "knk", "busy", "crowley", "flashc", "kguide",
                        "halpernn", "deroo", "sakade", "henshallg", "context", "halpernk", "halpernl",
                        "tuttle" };
                    const QString *pos = std::find(indextext, indextext + indexsize, reader.attributes().value("type").toString());
                    if (pos != indextext + indexsize)
                    {
                        data.indextype = (KanjiIndexT)(pos - indextext);
                        data.index = reader.attributes().value("text").toString();
                    }
                }

                if (/*Settings::general.savewinstates && */reader.attributes().value("hide") == True)
                    setFilter(data, KanjiFilters::Index, false);

                reader.skipCurrentElement();
            }

            if (reader.name() == "Radicals")
            {
                if (/*Settings::general.savewinstates && */reader.attributes().value("hide") == True)
                    setFilter(data, KanjiFilters::Radicals, false);

                bool finished = false;
                if (Settings::kanji.savefilters)
                {
                    QStringRef val = reader.attributes().value("mode");
                    if (val == "parts")
                        data.rads.mode = RadicalFilterModes::Parts;
                    else if (val == "radicals")
                        data.rads.mode = RadicalFilterModes::Radicals;
                    else if (val == "named")
                        data.rads.mode = RadicalFilterModes::NamedRadicals;
                    data.rads.grouped = reader.attributes().value("grouped") == True;

                    data.rads.groups.clear();

                    finished = true;
                    while (reader.readNextStartElement())
                    {
                        if (reader.name() == "List")
                        {
                            data.rads.groups.push_back(std::vector<ushort>());
                            std::vector<ushort> &list = data.rads.groups.back();

                            QVector<QStringRef> vals = reader.attributes().value("values").split(",");
                            list.reserve(vals.size());

                            bool ok = false;
                            int num;
                            for (const QStringRef &ref : vals)
                            {
                                num = ref.toInt(&ok);
                                if (ok)
                                    list.push_back(num);
                            }

                            reader.skipCurrentElement();
                        }
                    }

                    if (radicalFiltersModel().filterIndex(data.rads) == -1)
                    {
                        data.rads = RadicalFilter();
                    }
                }
                if (!finished)
                    reader.skipCurrentElement();
            }
        }
    }
}

//-------------------------------------------------------------


KanjiSearchWidget::KanjiSearchWidget(QWidget *parent) : base(parent), ui(new Ui::KanjiSearchWidget), ignorefilter(false), radform(nullptr)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    restrictWidgetSize(ui->strokeEdit, 6, AdjustedValue::MinMax);
    restrictWidgetSize(ui->jlptEdit, 6, AdjustedValue::MinMax);
    restrictWidgetSize(ui->skip1Edit, 4, AdjustedValue::MinMax);
    restrictWidgetSize(ui->skip2Edit, 4, AdjustedValue::MinMax);
    restrictWidgetSize(ui->radicalsCButton, ui->radicalsCButton->minimumContentsLength(), AdjustedValue::Min);

    ui->meaningEdit->setCharacterSize(12);
    ui->readingEdit->setCharacterSize(3);
    ui->indexEdit->setCharacterSize(6);

    QLayout *tmplayout = ui->optionsWidget->layout();
    std::vector<QLayoutItem*> tmpitems;
    while (tmplayout->count())
        tmpitems.push_back(ui->optionsWidget->layout()->takeAt(0));
    delete tmplayout;

    ZFlowLayout *optionsLayout = new ZFlowLayout(ui->optionsWidget);

    for (QLayoutItem *item : tmpitems)
        optionsLayout->addItem(item);

    optionsLayout->setMargin(0);
    optionsLayout->setVerticalSpacing(3);
    optionsLayout->setHorizontalSpacing(8);
    ui->optionsWidget->setLayout(optionsLayout);

    tmplayout = ui->buttonsWidget->layout();
    tmpitems.clear();
    while (tmplayout->count())
        tmpitems.push_back(ui->buttonsWidget->layout()->takeAt(0));
    delete tmplayout;

    ZFlowLayout *buttonsLayout = new ZFlowLayout(ui->buttonsWidget);

    for (QLayoutItem *item : tmpitems)
        buttonsLayout->addItem(item);

    buttonsLayout->setMargin(0);
    buttonsLayout->setVerticalSpacing(0);
    buttonsLayout->setHorizontalSpacing(2);
    ui->buttonsWidget->setLayout(buttonsLayout);

    ui->kanjiGrid->setModel(new KanjiGridSortModel(&mainKanjiListModel(), KanjiGridSortOrder::Jouyou, ui->kanjiGrid->dictionary(), ui->kanjiGrid));
    ui->readingEdit->setValidator(&kanaValidator());

    ui->strokeEdit->setValidator(new IntRangeValidator(1, std::numeric_limits<int>::max() - 1, true, this));
    ui->jlptEdit->setValidator(new IntRangeValidator(0, 5, true, this));

    fillFilters(filters);

    // Filtering activated on these
    connect(ui->strokeEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->meaningEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->mAfterButton, &QToolButton::clicked, this, &KanjiSearchWidget::filter);
    connect(ui->readingEdit, &ZLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->rAfterButton, &QToolButton::clicked, this, &KanjiSearchWidget::filter);
    connect(ui->rStrictButton, &QToolButton::clicked, this, &KanjiSearchWidget::filter);
    connect(ui->okuriganaButton, &QToolButton::toggled, this, &KanjiSearchWidget::filter);
    connect(ui->jlptEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->skip1Button, &QToolButton::toggled, this, &KanjiSearchWidget::skipButtonsToggled);
    connect(ui->skip2Button, &QToolButton::toggled, this, &KanjiSearchWidget::skipButtonsToggled);
    connect(ui->skip3Button, &QToolButton::toggled, this, &KanjiSearchWidget::skipButtonsToggled);
    connect(ui->skip4Button, &QToolButton::toggled, this, &KanjiSearchWidget::skipButtonsToggled);
    connect(ui->skip1Edit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->skip2Edit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    //connect(ui->skipEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->jouyouCBox, SIGNAL(activated(int)), this, SLOT(filter()));
    connect(ui->indexEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->indexCBox, SIGNAL(activated(int)), this, SLOT(filter()));
    connect(ui->fromCBox, SIGNAL(activated(int)), this, SLOT(filter()));

    connect(ui->sortCBox, SIGNAL(activated(int)), this, SLOT(sortKanji()));

    RadicalFiltersModel &m = radicalFiltersModel();
    connect(&m, &RadicalFiltersModel::filterAdded, this, &KanjiSearchWidget::radsAdded);
    connect(&m, &RadicalFiltersModel::filterRemoved, this, &KanjiSearchWidget::radsRemoved);
    connect(&m, &RadicalFiltersModel::filterMoved, this, &KanjiSearchWidget::radsMoved);
    connect(&m, &RadicalFiltersModel::cleared, this, &KanjiSearchWidget::radsCleared);

    connect(ui->radicalsCButton, &ZComboButton::clicked, this, &KanjiSearchWidget::radicalsCButtonClicked);
    connect(ui->radicalsCButton, static_cast<void (ZComboButton::*)(int)>(&ZComboButton::currentIndexChanged), this, &KanjiSearchWidget::radicalSelected);

    //: No radicals filter selected
    ui->radicalsCButton->addItem(tr("None"));

    for (int ix = 0, siz = tosigned(m.size()); ix != siz; ++ix)
        ui->radicalsCButton->addItem(m.filterText(ix));

    connect(&filterMap, SIGNAL(mapped(int)), this, SLOT(showHideAction(int)));

    if (dynamic_cast<ZKanjiForm*>(window()) != nullptr || dynamic_cast<PopupKanjiSearch*>(window()) != nullptr)
        ui->kanjiGrid->assignStatusBar(ui->kanjiStatus);
    else
        ui->kanjiStatus->hide();
}

KanjiSearchWidget::~KanjiSearchWidget()
{
    delete ui;
}

void KanjiSearchWidget::saveXMLSettings(QXmlStreamWriter &writer) const
{
    KanjiFilterData dat;
    saveState(dat);

    FormStates::saveXMLSettings(dat, writer);

    //writer.writeAttribute("from", ui->fromCBox->currentText());
    //writer.writeAttribute("sort", ui->sortCBox->currentText());

    //if ((Settings::kanji.savefilters &&!ui->strokeEdit->text().isEmpty()) || !ui->strokeWidget->isVisibleTo(ui->optionsWidget))
    //{
    //    writer.writeEmptyElement("Stroke");
    //    if (Settings::kanji.savefilters && !ui->strokeEdit->text().isEmpty())
    //        writer.writeAttribute("count", ui->strokeEdit->text());
    //    if (!ui->strokeWidget->isVisibleTo(ui->optionsWidget))
    //        writer.writeAttribute("hide", "1");
    //}
    //if ((Settings::kanji.savefilters &&!ui->meaningEdit->text().isEmpty()) || !ui->meaningWidget->isVisibleTo(ui->optionsWidget))
    //{
    //    writer.writeEmptyElement("Meaning");
    //    if (Settings::kanji.savefilters && !ui->meaningEdit->text().isEmpty())
    //        writer.writeAttribute("text", ui->meaningEdit->text());
    //    if (!ui->meaningWidget->isVisibleTo(ui->optionsWidget))
    //        writer.writeAttribute("hide", "1");
    //}

    //if (Settings::kanji.savefilters)
    //{
    //    writer.writeEmptyElement("Reading");
    //    if (ui->readingCBox->currentIndex() != 0)
    //        writer.writeAttribute("type", ui->readingCBox->currentText());
    //    writer.writeAttribute("oku", ui->okuriganaButton->isChecked() ? "1" : "0");
    //    if (!ui->readingEdit->text().isEmpty())
    //        writer.writeAttribute("text", ui->readingEdit->text());
    //}
    //if (!ui->readingWidget->isVisibleTo(ui->optionsWidget))
    //    writer.writeAttribute("hide", "1");

    //if ((Settings::kanji.savefilters && !ui->jlptEdit->text().isEmpty()) || !ui->jlptWidget->isVisibleTo(ui->optionsWidget))
    //{
    //    writer.writeEmptyElement("JLPT");
    //    if (Settings::kanji.savefilters && !ui->jlptEdit->text().isEmpty())
    //        writer.writeAttribute("text", ui->jlptEdit->text());
    //    if (!ui->jlptWidget->isVisibleTo(ui->optionsWidget))
    //        writer.writeAttribute("hide", "1");
    //}

    //writer.writeEmptyElement("Jouyou");
    //if (Settings::kanji.savefilters)
    //    writer.writeAttribute("grade", ui->jouyouCBox->currentText());
    //if (!ui->jouyouWidget->isVisibleTo(ui->optionsWidget))
    //    writer.writeAttribute("hide", "1");

    //if ((Settings::kanji.savefilters && (ui->skip1Button->isChecked() || ui->skip2Button->isChecked() || ui->skip3Button->isChecked() || ui->skip4Button->isChecked() || !ui->skip1Edit->text().isEmpty() || !ui->skip2Edit->text().isEmpty())) || !ui->skipWidget->isVisibleTo(ui->optionsWidget))
    //{
    //    writer.writeEmptyElement("Skip");
    //    if (Settings::kanji.savefilters && (ui->skip1Button->isChecked() || ui->skip2Button->isChecked() || ui->skip3Button->isChecked() || ui->skip4Button->isChecked()))
    //        writer.writeAttribute("type", (ui->skip1Button->isChecked() ? "1" : ui->skip2Button->isChecked() ? "2" : ui->skip3Button->isChecked() ? "3" : ui->skip4Button->isChecked() ? "4" : ""));
    //    if (Settings::kanji.savefilters && !ui->skip1Edit->text().isEmpty())
    //        writer.writeAttribute("field1", ui->skip1Edit->text());
    //    if (Settings::kanji.savefilters && !ui->skip2Edit->text().isEmpty())
    //        writer.writeAttribute("field2", ui->skip2Edit->text());
    //    if (!ui->skipWidget->isVisibleTo(ui->optionsWidget))
    //        writer.writeAttribute("hide", "1");
    //}

    //if (Settings::kanji.savefilters || !ui->indexWidget->isVisibleTo(ui->optionsWidget))
    //{
    //    writer.writeEmptyElement("Index");
    //    if (Settings::kanji.savefilters)
    //        writer.writeAttribute("from", ui->indexCBox->currentText());
    //    if (Settings::kanji.savefilters && !ui->indexEdit->text().isEmpty())
    //        writer.writeAttribute("text", ui->indexEdit->text());
    //    if (!ui->indexWidget->isVisibleTo(ui->optionsWidget))
    //        writer.writeAttribute("hide", "1");
    //}

    //if ((Settings::kanji.savefilters && ui->radicalsCBox->currentIndex() != 0) || !ui->radicalsWidget->isVisibleTo(ui->optionsWidget))
    //{
    //    writer.writeEmptyElement("Radical");
    //    if (ui->radicalsCBox->currentIndex() != 0)
    //        writer.writeAttribute("value", ui->radicalsCBox->currentText());
    //    if (!ui->radicalsWidget->isVisibleTo(ui->optionsWidget))
    //        writer.writeAttribute("hide", "1");
    //}
}

void KanjiSearchWidget::loadXMLSettings(QXmlStreamReader &reader)
{
    KanjiFilterData dat;
    FormStates::loadXMLSettings(dat, reader);

    restoreState(dat);
}

void KanjiSearchWidget::saveState(KanjiFilterData &dat) const
{
    dat.fromtype = (KanjiFromT)ui->fromCBox->currentIndex();
    dat.ordertype = (KanjiGridSortOrder)ui->sortCBox->currentIndex();

    bool success;
    int val;

    //if (ui->strokeWidget->isVisibleTo(ui->optionsWidget))
    findStrIntMinMax(ui->strokeEdit->text(), 1, std::numeric_limits<int>::max() - 1, dat.strokemin, dat.strokemax);
    //else
    //    data.strokemin = 0, data.strokemax = 0;

    //data.filters = ((ui->strokeWidget->isVisibleTo(ui->optionsWidget) ?  0x01 : 0)) |
    //               ((ui->jlptWidget->isVisibleTo(ui->optionsWidget) ? 0x02 : 0)) |
    //               ((ui->meaningWidget->isVisibleTo(ui->optionsWidget) ? 0x04 : 0)) |
    //               ((ui->readingWidget->isVisibleTo(ui->optionsWidget) ? 0x08 : 0)) |
    //               ((ui->jouyouWidget->isVisibleTo(ui->optionsWidget) ? 0x10 : 0)) |
    //               ((ui->radicalsWidget->isVisibleTo(ui->optionsWidget) ? 0x20 : 0)) |
    //               ((ui->indexWidget->isVisibleTo(ui->optionsWidget) ? 0x40 : 0)) |
    //               ((ui->skipWidget->isVisibleTo(ui->optionsWidget) ? 0x80 : 0));
    setFilter(dat, KanjiFilters::Strokes, ui->strokeWidget->isVisibleTo(ui->optionsWidget));
    setFilter(dat, KanjiFilters::JLPT, ui->jlptWidget->isVisibleTo(ui->optionsWidget));
    setFilter(dat, KanjiFilters::Meaning, ui->meaningWidget->isVisibleTo(ui->optionsWidget));
    setFilter(dat, KanjiFilters::Reading, ui->readingWidget->isVisibleTo(ui->optionsWidget));
    setFilter(dat, KanjiFilters::Jouyou, ui->jouyouWidget->isVisibleTo(ui->optionsWidget));
    setFilter(dat, KanjiFilters::Radicals, ui->radicalsWidget->isVisibleTo(ui->optionsWidget));
    setFilter(dat, KanjiFilters::Index, ui->indexWidget->isVisibleTo(ui->optionsWidget));
    setFilter(dat, KanjiFilters::SKIP, ui->skipWidget->isVisibleTo(ui->optionsWidget));


    dat.meaning = /*ui->meaningWidget->isVisibleTo(ui->optionsWidget) ?*/ ui->meaningEdit->text().trimmed() /*: QString()*/;
    dat.meaningafter = ui->mAfterButton->isChecked();
    dat.reading = /*ui->readingWidget->isVisibleTo(ui->optionsWidget) ?*/ ui->readingEdit->text().trimmed() /*: QString()*/;
    ui->readingEdit->validator()->fixup(dat.reading);
    dat.readingafter = ui->rAfterButton->isChecked();
    dat.readingstrict = ui->rStrictButton->isChecked();
    dat.readingoku = ui->okuriganaButton->isChecked();

    //if (ui->jlptWidget->isVisibleTo(ui->optionsWidget))
    findStrIntMinMax(ui->jlptEdit->text(), 0, 5, dat.jlptmin, dat.jlptmax);
    //else
    //    dat.jlptmin = 0, dat.jlptmax = 0;

    //if (ui->skipWidget->isVisibleTo(ui->optionsWidget))
    //{
    dat.skip1 = ui->skip1Button->isChecked() ? 1 : ui->skip2Button->isChecked() ? 2 : ui->skip3Button->isChecked() ? 3 : ui->skip4Button->isChecked() ? 4 : 0;
    val = ui->skip1Edit->text().trimmed().toInt(&success);
    if (success)
        dat.skip2 = val;
    else
        dat.skip2 = -1;
    val = ui->skip2Edit->text().trimmed().toInt(&success);
    if (success)
        dat.skip3 = val;
    else
        dat.skip3 = -1;

    dat.jouyou = ui->jouyouCBox->currentIndex();

    dat.indextype = (KanjiIndexT)ui->indexCBox->currentIndex();
    dat.index = ui->indexEdit->text().trimmed();

    //if (ui->radicalsCBox->currentIndex() > 0)
    //    dat.rads = radicalFiltersModel().filters(ui->radicalsCBox->currentIndex() - 1);
    //else
    //    dat.rads = RadicalFilter();

    if (ui->radicalsCButton->currentIndex() > 0)
        dat.rads = radicalFiltersModel().filters(ui->radicalsCButton->currentIndex() - 1);
    else
        dat.rads = RadicalFilter();
}

void KanjiSearchWidget::restoreState(const KanjiFilterData &dat)
{
    if (ignorefilter)
        return;

    ignorefilter = true;
    //if (Settings::general.savewinstates)
    //{
    ui->fromCBox->setCurrentIndex((int)dat.fromtype);// reader.attributes().value("from").toString());
    ui->sortCBox->setCurrentIndex((int)dat.ordertype);// reader.attributes().value("sort").toString());
                                                       //}

                                                       //if (Settings::general.savewinstates)
    ui->strokeWidget->setVisible(filterActive(dat, KanjiFilters::Strokes));
    if (Settings::kanji.savefilters)
        ui->strokeEdit->setText(IntMinMaxToString(1, std::numeric_limits<int>::max() - 1, dat.strokemin, dat.strokemax));

    //if (Settings::general.savewinstates)
    ui->jlptWidget->setVisible(filterActive(dat, KanjiFilters::JLPT));
    if (Settings::kanji.savefilters)
        ui->jlptEdit->setText(IntMinMaxToString(0, 5, dat.jlptmin, dat.jlptmax));

    //if (Settings::general.savewinstates)
    ui->meaningWidget->setVisible(filterActive(dat, KanjiFilters::Meaning));
    if (Settings::kanji.savefilters)
    {
        ui->mAfterButton->setChecked(dat.meaningafter);
        ui->meaningEdit->setText(dat.meaning);
    }

    //if (Settings::general.savewinstates)
    ui->readingWidget->setVisible(filterActive(dat, KanjiFilters::Reading));
    if (Settings::kanji.savefilters)
    {
        ui->rAfterButton->setChecked(dat.readingafter);
        ui->rStrictButton->setChecked(dat.readingstrict);
        ui->okuriganaButton->setChecked(dat.readingoku);
        ui->readingEdit->setText(dat.reading);
    }

    //if (Settings::general.savewinstates)
    ui->jouyouWidget->setVisible(filterActive(dat, KanjiFilters::Jouyou));
    if (Settings::kanji.savefilters)
        ui->jouyouCBox->setCurrentIndex(dat.jouyou);

    //if (Settings::general.savewinstates)
    ui->skipWidget->setVisible(filterActive(dat, KanjiFilters::SKIP));
    if (Settings::kanji.savefilters)
    {
        ui->skip1Button->setChecked(dat.skip1 == 1);
        ui->skip2Button->setChecked(dat.skip1 == 2);
        ui->skip3Button->setChecked(dat.skip1 == 3);
        ui->skip4Button->setChecked(dat.skip1 == 4);
        ui->skip1Edit->setText(dat.skip2 <= 0 ? QString() : QString::number(dat.skip2));
        ui->skip2Edit->setText(dat.skip3 <= 0 ? QString() : QString::number(dat.skip3));
    }

    //if (Settings::general.savewinstates)
    ui->indexWidget->setVisible(filterActive(dat, KanjiFilters::Index));
    if (Settings::kanji.savefilters)
    {
        ui->indexCBox->setCurrentIndex((int)dat.indextype);
        ui->indexEdit->setText(dat.index);
    }

    //if (Settings::general.savewinstates)
    ui->radicalsWidget->setVisible(filterActive(dat, KanjiFilters::Radicals));
    if (Settings::kanji.savefilters)
    {
        int index = 0;
        if (dat.rads != RadicalFilter())
        {
            index = radicalFiltersModel().filterIndex(dat.rads);
            if (index == -1)
                index = 0;
            else
                index += 1;
        }

        ui->radicalsCButton->setCurrentIndex(std::max(0, index));
    }

    ui->line->setVisible(ui->strokeWidget->isVisibleTo(ui->optionsWidget) || ui->meaningWidget->isVisibleTo(ui->optionsWidget) || ui->readingWidget->isVisibleTo(ui->optionsWidget) ||
        ui->jlptWidget->isVisibleTo(ui->optionsWidget) || ui->radicalsWidget->isVisibleTo(ui->optionsWidget) || ui->jouyouWidget->isVisibleTo(ui->optionsWidget) ||
        ui->skipWidget->isVisibleTo(ui->optionsWidget) || ui->indexWidget->isVisibleTo(ui->optionsWidget));

    ui->f1Button->setChecked(ui->strokeWidget->isVisibleTo(ui->optionsWidget));
    ui->f2Button->setChecked(ui->jlptWidget->isVisibleTo(ui->optionsWidget));
    ui->f3Button->setChecked(ui->meaningWidget->isVisibleTo(ui->optionsWidget));
    ui->f4Button->setChecked(ui->readingWidget->isVisibleTo(ui->optionsWidget));
    ui->f5Button->setChecked(ui->jouyouWidget->isVisibleTo(ui->optionsWidget));
    ui->f6Button->setChecked(ui->radicalsWidget->isVisibleTo(ui->optionsWidget));
    ui->f7Button->setChecked(ui->indexWidget->isVisibleTo(ui->optionsWidget));
    ui->f8Button->setChecked(ui->skipWidget->isVisibleTo(ui->optionsWidget));
    ui->allButton->setChecked(ui->f1Button->isChecked() && ui->f2Button->isChecked() && ui->f3Button->isChecked() && ui->f4Button->isChecked() && ui->f5Button->isChecked() && ui->f6Button->isChecked() && ui->f7Button->isChecked() && ui->f8Button->isChecked());

    ui->optionsWidget->setVisible(ui->line->isVisibleTo(this));

    ignorefilter = false;
    filterKanji(true);
}

void KanjiSearchWidget::assignStatusBar(ZStatusBar *bar)
{
    ui->kanjiGrid->assignStatusBar(bar);
}

//void KanjiSearchWidget::makeModeSpace(const QSize &size)
//{
//    ((ZFlowLayout*)ui->optionsWidget->layout())->makeModeSpace(size);
//}

void KanjiSearchWidget::setDictionary(Dictionary *d)
{
    if (ui->kanjiGrid->dictionary() == d)
        return;
    ui->kanjiGrid->setDictionary(d);
    //if (!filters.data.meaning.isEmpty())
    //{
    filters.data.meaning.clear();
    filterKanji(true);
    //}
    //else
    //    sortKanji();
}

CommandCategories KanjiSearchWidget::activeCategory() const
{
    if (!isVisibleTo(window()) || dynamic_cast<ZKanjiForm*>(window()) == nullptr)
        return CommandCategories::NoCateg;

    const QWidget *parentw = parentWidget();
    while (parentw != nullptr && dynamic_cast<const ZKanjiWidget*>(parentw) == nullptr)
        parentw = parentw->parentWidget();

    if (parentw == nullptr)
        return CommandCategories::NoCateg;

    ZKanjiWidget *zw = ((ZKanjiWidget*)parentw);
    if (zw->isActiveWidget())
        return CommandCategories::SearchCateg;

    QList<ZKanjiWidget*> wlist = ((ZKanjiForm*)window())->findChildren<ZKanjiWidget*>();
    for (ZKanjiWidget *w : wlist)
    {
        if (w == zw || w->window() != window())
            continue;
        else if (w->mode() == zw->mode())
            return CommandCategories::NoCateg;
    }

    return CommandCategories::SearchCateg;
}

void KanjiSearchWidget::executeCommand(int command)
{
    if (command < (int)CommandLimits::KanjiSearchBegin || command >= (int)CommandLimits::KanjiSearchEnd)
        return;

    switch (command)
    {
    case (int)Commands::ResetKanjiSearch:
        ui->resetButton->click();
        break;
    case (int)Commands::StrokeFilter:
        ui->f1Button->toggle();
        ui->f1Button->click();
        break;
    case (int)Commands::JLPTFilter:
        ui->f2Button->toggle();
        ui->f2Button->click();
        break;
    case (int)Commands::MeaningFilter:
        ui->f3Button->toggle();
        ui->f3Button->click();
        break;
    case (int)Commands::ReadingFilter:
        ui->f4Button->toggle();
        ui->f4Button->click();
        break;
    case (int)Commands::JouyouFilter:
        ui->f5Button->toggle();
        ui->f5Button->click();
        break;
    case (int)Commands::RadicalsFilter:
        ui->f6Button->toggle();
        ui->f6Button->click();
        break;
    case (int)Commands::IndexFilter:
        ui->f7Button->toggle();
        ui->f7Button->click();
        break;
    case (int)Commands::SKIPFilter:
        ui->f8Button->toggle();
        ui->f8Button->click();
        break;
    }
}

void KanjiSearchWidget::commandState(int command, bool &enabled, bool &checked, bool &visible) const
{
    if (command < (int)CommandLimits::KanjiSearchBegin || command >= (int)CommandLimits::KanjiSearchEnd)
        return;

    enabled = true;
    checked = false;
    visible = true;

    switch (command)
    {
    case (int)Commands::ResetKanjiSearch:
        enabled = ui->resetButton->isEnabled();
        break;
    case (int)Commands::StrokeFilter:
        checked = ui->f1Button->isChecked();
        break;
    case (int)Commands::JLPTFilter:
        checked = ui->f2Button->isChecked();
        break;
    case (int)Commands::MeaningFilter:
        checked = ui->f3Button->isChecked();
        break;
    case (int)Commands::ReadingFilter:
        checked = ui->f4Button->isChecked();
        break;
    case (int)Commands::JouyouFilter:
        checked = ui->f5Button->isChecked();
        break;
    case (int)Commands::RadicalsFilter:
        checked = ui->f6Button->isChecked();
        break;
    case (int)Commands::IndexFilter:
        checked = ui->f7Button->isChecked();
        break;
    case (int)Commands::SKIPFilter:
        checked = ui->f8Button->isChecked();
        break;
    }
}

void KanjiSearchWidget::fillFilters(RuntimeKanjiFilters &f) const
{
    saveState(f.data);

    if (radform != nullptr)
    {
        radform->activeFilter(f.data.rads);
        f.tmprads = radform->selectionToFilter();
    }
}

void KanjiSearchWidget::reset()
{
    ui->strokeEdit->setText(QString());
    ui->radicalsCButton->setCurrentIndex(0);
    ui->meaningEdit->setText(QString());
    ui->mAfterButton->setChecked(true);
    ui->readingEdit->setText(QString());
    ui->rAfterButton->setChecked(true);
    ui->rStrictButton->setChecked(false);
    ui->okuriganaButton->setChecked(true);
    ui->jlptEdit->setText(QString());
    ui->jouyouCBox->setCurrentIndex(0);
    ui->skip1Button->setChecked(false);
    ui->skip2Button->setChecked(false);
    ui->skip3Button->setChecked(false);
    ui->skip4Button->setChecked(false);
    ui->skip1Edit->setText(QString());
    ui->skip2Edit->setText(QString());
    ui->indexCBox->setCurrentIndex(0);
    ui->indexEdit->setText(QString());

    filterKanji();
}

bool KanjiSearchWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        ui->radicalsCButton->setItemText(0, tr("None"));
    }

    return base::event(e);
}

void KanjiSearchWidget::showEvent(QShowEvent *e)
{
    base::showEvent(e);

    ZFlowLayout *flow = (ZFlowLayout*)ui->optionsWidget->layout();
    flow->setAlignment(Qt::AlignVCenter);
    flow->restrictWidth(ui->meaningWidget, ui->meaningEdit->sizeHint().width() /*restrictedWidgetSize(ui->meaningEdit, 8)*/ + ui->meaningWidget->layout()->spacing() * 2 + ui->meaningLabel->width() + ui->mAfterButton->width());
    flow->restrictWidth(ui->readingWidget, ui->readingEdit->sizeHint().width() /*restrictedWidgetSize(ui->readingEdit, 6)*/ + ui->readingLabel->width() + ui->rAfterButton->width() + ui->rStrictButton->width() + ui->okuriganaButton->width() + ui->readingWidget->layout()->spacing() * 2);
    flow->restrictWidth(ui->radicalsWidget, ui->radicalsLabel->width() + restrictedWidgetSize(ui->radicalsCButton, ui->radicalsCButton->minimumContentsLength()) + ui->radicalsWidget->layout()->spacing());
    flow->setSpacingAfter(ui->resetWidget, 0);

    if (ui->okuriganaButton->height() != ui->rAfterButton->height())
    {
        ui->okuriganaButton->setMinimumHeight(ui->rAfterButton->height());
        ui->rAfterButton->setMinimumHeight(ui->okuriganaButton->height());
    }

}

void KanjiSearchWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QAction *a;

    filterMenu.clear();

    a = filterMenu.addAction(tr("Sho&w all filters"));
    a->setEnabled(!ui->strokeEdit->isVisible() || !ui->meaningEdit->isVisible() || !ui->readingEdit->isVisible() ||
        !ui->jlptEdit->isVisible() || !ui->radicalsCButton->isVisible() || !ui->jouyouCBox->isVisible() ||
        !ui->skip1Edit->isVisible() || !ui->indexEdit->isVisible());
    if (a->isEnabled())
    {
        connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
        filterMap.setMapping(a, 0);
    }

    a = filterMenu.addAction(tr("&Hide all filters"));
    a->setEnabled(ui->strokeEdit->isVisible() || ui->meaningEdit->isVisible() || ui->readingEdit->isVisible() ||
        ui->jlptEdit->isVisible() || ui->radicalsCButton->isVisible() || ui->jouyouCBox->isVisible() ||
        ui->skip1Edit->isVisible() || ui->indexEdit->isVisible());
    if (a->isEnabled())
    {
        connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
        filterMap.setMapping(a, 1);
    }

    filterMenu.addSeparator();

    a = filterMenu.addAction(tr("Stroke &count filter"));
    a->setCheckable(true);
    a->setChecked(ui->strokeEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
    filterMap.setMapping(a, 2);

    a = filterMenu.addAction(tr("&JLPT level filter"));
    a->setCheckable(true);
    a->setChecked(ui->jlptEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
    filterMap.setMapping(a, 3);

    a = filterMenu.addAction(tr("&Meaning filter"));
    a->setCheckable(true);
    a->setChecked(ui->meaningEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
    filterMap.setMapping(a, 4);

    a = filterMenu.addAction(tr("Rea&ding filter"));
    a->setCheckable(true);
    a->setChecked(ui->readingEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
    filterMap.setMapping(a, 5);

    a = filterMenu.addAction(tr("Jouyou &grade filter"));
    a->setCheckable(true);
    a->setChecked(ui->jouyouCBox->isVisible());
    connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
    filterMap.setMapping(a, 6);

    a = filterMenu.addAction(tr("R&adicals filter"));
    a->setCheckable(true);
    a->setChecked(ui->radicalsCButton->isVisible());
    connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
    filterMap.setMapping(a, 7);

    a = filterMenu.addAction(tr("&Index filter"));
    a->setCheckable(true);
    a->setChecked(ui->indexEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
    filterMap.setMapping(a, 8);

    a = filterMenu.addAction(tr("&SKIP code filter"));
    a->setCheckable(true);
    a->setChecked(ui->skip1Edit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &filterMap, SLOT(map()));
    filterMap.setMapping(a, 9);

    ZKanjiWidget *w = ZKanjiWidget::getZParent(this);
    if (w != nullptr)
        w->addDockAction(&filterMenu);


    filterMenu.popup(e->globalPos());
    e->accept();
}

void KanjiSearchWidget::resizeEvent(QResizeEvent *e)
{
    base::resizeEvent(e);

    if (ui->sortWidget->layout()->count() == 3 && width() - ui->sortLabel->width() - ui->sortCBox->minimumSizeHint().width() - ui->sortWidget->layout()->spacing() * 2 >= ui->fromCBox->minimumSizeHint().width())
    {
        if (isVisibleTo(window()) && window()->isVisible())
        {
            ui->sortCBox->setMinimumWidth(ui->sortCBox->minimumSizeHint().width());
            ui->fromCBox->setMinimumWidth(ui->fromCBox->minimumSizeHint().width());
        }
        QBoxLayout *l = static_cast<QBoxLayout*>(ui->sortWidget->layout());
        l->insertWidget(2, ui->fromCBox);
        ui->fromWidget->hide();
        QSizePolicy pol = ui->sortWidget->sizePolicy();
        pol.setHorizontalPolicy(QSizePolicy::Ignored);
        ui->sortWidget->setSizePolicy(pol);
    }
    else if (ui->sortWidget->layout()->count() == 4 && width() - ui->sortLabel->width() - ui->sortCBox->minimumSizeHint().width() - ui->sortWidget->layout()->spacing() * 2 < ui->fromCBox->minimumSizeHint().width())
    {
        static_cast<QBoxLayout*>(ui->fromWidget->layout())->insertWidget(0, ui->fromCBox);
        ui->fromWidget->show();
        QSizePolicy pol = ui->sortWidget->sizePolicy();
        pol.setHorizontalPolicy(QSizePolicy::Preferred);
        ui->sortWidget->setSizePolicy(pol);
    }
}

bool KanjiSearchWidget::filtersMatch(const RuntimeKanjiFilters &a, const RuntimeKanjiFilters &b)
{
    return
        (((!filterActive(a.data, KanjiFilters::Strokes) || (a.data.strokemin == 0 && a.data.strokemax == 0)) && (!filterActive(b.data, KanjiFilters::Strokes) || (b.data.strokemin == 0 && b.data.strokemax == 0))) ||
        (clamp(!filterActive(a.data, KanjiFilters::Strokes) ? 0 : a.data.strokemin, 0, 30) == clamp(!filterActive(b.data, KanjiFilters::Strokes) ? 0 : b.data.strokemin, 0, 30) &&
        clamp(!filterActive(a.data, KanjiFilters::Strokes) ? 0 : a.data.strokemax, 0, 30) == clamp(!filterActive(b.data, KanjiFilters::Strokes) ? 0 : b.data.strokemax, 0, 30))) &&

        (((!filterActive(a.data, KanjiFilters::Meaning) || a.data.meaning.isEmpty()) && (!filterActive(b.data, KanjiFilters::Meaning) || b.data.meaning.isEmpty())) ||
        ((!filterActive(a.data, KanjiFilters::Meaning) ? QString() : a.data.meaning.toLower()) == (!filterActive(b.data, KanjiFilters::Meaning) ? QString() : b.data.meaning.toLower()) &&
        (filterActive(a.data, KanjiFilters::Meaning) && filterActive(b.data, KanjiFilters::Meaning) && a.data.meaningafter == b.data.meaningafter))) &&

        (((!filterActive(a.data, KanjiFilters::Reading) || a.data.reading.isEmpty()) && (!filterActive(b.data, KanjiFilters::Reading) || b.data.reading.isEmpty())) ||
        ((!filterActive(a.data, KanjiFilters::Reading) ? QString() : hiraganize(a.data.reading)) == (!filterActive(b.data, KanjiFilters::Reading) ? QString() : hiraganize(b.data.reading)) &&
        (filterActive(a.data, KanjiFilters::Reading) && filterActive(b.data, KanjiFilters::Reading) && a.data.readingstrict == b.data.readingstrict && a.data.readingafter == b.data.readingafter && a.data.readingoku == b.data.readingoku))) &&

        (clamp(!filterActive(a.data, KanjiFilters::JLPT) ? -1 : a.data.jlptmin, -1, 5) == clamp(!filterActive(b.data, KanjiFilters::JLPT) ? -1 : b.data.jlptmin, -1, 5) &&
        clamp(!filterActive(a.data, KanjiFilters::JLPT) ? -1 : a.data.jlptmax, -1, 5) == clamp(!filterActive(b.data, KanjiFilters::JLPT) ? -1 : b.data.jlptmax, -1, 5)) &&

        ((!filterActive(a.data, KanjiFilters::Jouyou) ? 0 : a.data.jouyou) == (!filterActive(b.data, KanjiFilters::Jouyou) ? 0 : b.data.jouyou)) &&

        ((!filterActive(a.data, KanjiFilters::SKIP) ? 0 : a.data.skip1) == (!filterActive(b.data, KanjiFilters::SKIP) ? 0 : b.data.skip1) &&
        clamp(!filterActive(a.data, KanjiFilters::SKIP) ? 0 : a.data.skip2, 0, 30) == clamp(!filterActive(b.data, KanjiFilters::SKIP) ? 0 : b.data.skip2, 0, 30) &&
        clamp(!filterActive(a.data, KanjiFilters::SKIP) ? 0 : a.data.skip3, 0, 30) == clamp(!filterActive(b.data, KanjiFilters::SKIP) ? 0 : b.data.skip3, 0, 30)) &&

        (((!filterActive(a.data, KanjiFilters::Index) || a.data.index.isEmpty()) && (!filterActive(b.data, KanjiFilters::Index) || b.data.index.isEmpty())) ||
        ((!filterActive(a.data, KanjiFilters::Index) ? QString() : a.data.index.toLower()) == (!filterActive(b.data, KanjiFilters::Index) ? QString() : b.data.index.toLower()) &&
        a.data.indextype == b.data.indextype)) &&

        ((!filterActive(a.data, KanjiFilters::Radicals) ? RadicalFilter() : a.data.rads) == (!filterActive(b.data, KanjiFilters::Radicals) ? RadicalFilter() : b.data.rads)) &&
        ((!filterActive(a.data, KanjiFilters::Radicals) ? std::vector<ushort>() : a.tmprads) == (!filterActive(b.data, KanjiFilters::Radicals) ? std::vector<ushort>() : b.tmprads)) &&

        (a.data.fromtype == b.data.fromtype);
}

bool KanjiSearchWidget::filtersEmpty(const RuntimeKanjiFilters &f)
{
    return
        (!filterActive(f.data.filters, KanjiFilters::Strokes) || (f.data.strokemin == 0 && f.data.strokemax == 0) || (f.data.strokemax != 0 && f.data.strokemax < f.data.strokemin)) &&
        (!filterActive(f.data.filters, KanjiFilters::Meaning) || f.data.meaning.isEmpty()) &&
        (!filterActive(f.data.filters, KanjiFilters::Reading) || f.data.reading.isEmpty()) &&
        (!filterActive(f.data.filters, KanjiFilters::JLPT) || (f.data.jlptmin == -1 && f.data.jlptmax == -1) || (f.data.jlptmax != -1 && f.data.jlptmax < f.data.jlptmin)) &&
        (!filterActive(f.data.filters, KanjiFilters::Jouyou) || (f.data.jouyou == 0)) &&
        (!filterActive(f.data.filters, KanjiFilters::SKIP) || (f.data.skip1 == 0 && (f.data.skip2 <= 0 || f.data.skip2 > 30) && (f.data.skip3 <= 0 || f.data.skip3 > 30))) &&
        (!filterActive(f.data.filters, KanjiFilters::Index) || f.data.index.isEmpty()) &&
        (!filterActive(f.data.filters, KanjiFilters::Radicals) || (f.data.rads == RadicalFilter() && f.tmprads.empty())) &&
        f.data.fromtype == KanjiFromT::All;
}

void KanjiSearchWidget::listFilteredKanji(const RuntimeKanjiFilters &f, std::vector<ushort> &list)
{
    list.clear();

    RadicalFilter rads = f.data.rads;

    if (!filterActive(f.data.filters, KanjiFilters::Radicals) || rads.groups.empty())
    {
        for (int ix = 0; ix != ZKanji::kanjicount; ++ix)
            list.push_back(ix);
    }
    else if (rads.mode == RadicalFilterModes::Radicals)
    {
        std::vector<ushort> group;
        for (int ix = 0; ix != ZKanji::kanjicount; ++ix)
        {
            auto it = std::find(rads.groups[0].begin(), rads.groups[0].end(), ZKanji::kanjis[ix]->rad);
            if (it != rads.groups[0].end())
                group.push_back(ix);
        }
        if (list.empty())
            list = group;
        else
        {
            std::vector<ushort> temp;
            temp.swap(list);
            list.resize(group.size());
            list.resize(std::set_intersection(group.begin(), group.end(), temp.begin(), temp.end(), list.begin()) - list.begin());
        }
        group.clear();
    }
    else if (rads.mode == RadicalFilterModes::Parts)
    {
        std::vector<ushort> group;
        for (int ix = 0, siz = tosigned(rads.groups.size()); ix != siz; ++ix)
        {
            const std::vector<ushort> &g = rads.groups[ix];
            for (int iy = 0, sizy = tosigned(g.size()); iy != sizy; ++iy)
                group.insert(group.end(), ZKanji::radklist[g[iy]].second.begin(), ZKanji::radklist[g[iy]].second.end());
            std::sort(group.begin(), group.end());
            group.resize(std::unique(group.begin(), group.end()) - group.begin());

            if (list.empty())
                list = group;
            else
            {
                std::vector<ushort> temp;
                temp.swap(list);
                list.resize(group.size());
                list.resize(std::set_intersection(group.begin(), group.end(), temp.begin(), temp.end(), list.begin()) - list.begin());
            }
            group.clear();
        }
    }
    else // NamedRadicals
    {
        std::vector<ushort> group;
        for (int ix = 0, siz = tosigned(rads.groups.size()); ix != siz; ++ix)
        {
            const std::vector<ushort> &g = rads.groups[ix];
            for (int iy = 0, sizy = tosigned(g.size()); iy != sizy; ++iy)
            {
                if (!rads.grouped)
                    group.insert(group.end(), ZKanji::radlist[g[iy]]->kanji.begin(), ZKanji::radlist[g[iy]]->kanji.end());
                else
                {
                    for (int j = g[iy], sizj = tosigned(ZKanji::radlist.size()); j != sizj; ++j)
                        if (j != g[iy] && ZKanji::radlist[j]->radical != ZKanji::radlist[j - 1]->radical)
                            break;
                        else
                            group.insert(group.end(), ZKanji::radlist[j]->kanji.begin(), ZKanji::radlist[j]->kanji.end());
                }
            }
            std::sort(group.begin(), group.end());
            group.resize(std::unique(group.begin(), group.end()) - group.begin());

            if (list.empty())
                list = group;
            else
            {
                std::vector<ushort> temp;
                temp.swap(list);
                list.resize(group.size());
                list.resize(std::set_intersection(group.begin(), group.end(), temp.begin(), temp.end(), list.begin()) - list.begin());
            }
            group.clear();
        }
    }

    if (list.empty())
        return;

    Dictionary *dict = ui->kanjiGrid->dictionary();

    QSet<QChar> clpbrd;
    if (f.data.fromtype == KanjiFromT::Clipbrd)
    {
        QString tmp = qApp->clipboard()->text();
        for (int ix = 0; ix != tmp.size(); ++ix)
        {
            if (KANJI(tmp.at(ix).unicode()))
                clpbrd.insert(tmp.at(ix));
        }
    }

    for (int ix = tosigned(list.size()) - 1; ix != -1; --ix)
    {
        KanjiEntry *k = ZKanji::kanjis[list[ix]];

        bool match;

        if (f.data.fromtype != KanjiFromT::All)
        {
            match = false;
            switch (f.data.fromtype)
            {
            case KanjiFromT::Clipbrd:
                match = clpbrd.contains(k->ch);
                break;
            case KanjiFromT::Common:
                match = k->frequency != 0;
                break;
            case KanjiFromT::Jouyou:
                match = k->jouyou != 0;
                break;
            case KanjiFromT::JLPT:
                match = k->jlpt != 0;
                break;
            case KanjiFromT::Oneil:
                match = k->oneil != 0;
                break;
            case KanjiFromT::Gakken:
                match = k->gakken != 0;
                break;
            case KanjiFromT::Halpern:
                match = k->halpern != 0;
                break;
            case KanjiFromT::Heisig:
                match = k->heisig != 0;
                break;
            case KanjiFromT::HeisigN:
                match = k->heisign != 0;
                break;
            case KanjiFromT::HeisigF:
                match = k->heisigf != 0;
                break;
            case KanjiFromT::Henshall:
                match = k->henshall != 0;
                break;
            case KanjiFromT::Nelson:
                match = k->nelson != 0;
                break;
            case KanjiFromT::NewNelson:
                match = k->newnelson != 0;
                break;
            case KanjiFromT::SnH:
                match = k->snh[0] != 0;
                break;
            case KanjiFromT::KnK:
                match = k->knk != 0;
                break;
            case KanjiFromT::KnKOld:
                match = k->knk != 0 && k->knk <= 1945;
                break;
            case KanjiFromT::Busy:
                match = k->busy[0] != 0;
                break;
            case KanjiFromT::Crowley:
                match = k->crowley != 0;
                break;
            case KanjiFromT::FlashC:
                match = k->flashc != 0;
                break;
            case KanjiFromT::KGuide:
                match = k->kguide != 0;
                break;
            case KanjiFromT::HalpernN:
                match = k->halpernn != 0;
                break;
            case KanjiFromT::Deroo:
                match = k->deroo != 0;
                break;
            case KanjiFromT::Sakade:
                match = k->sakade != 0;
                break;
            case KanjiFromT::HenshallG:
                match = k->henshallg != 0;
                break;
            case KanjiFromT::Context:
                match = k->context != 0;
                break;
            case KanjiFromT::HalpernK:
                match = k->halpernk != 0;
                break;
            case KanjiFromT::HalpernL:
                match = k->halpernl != 0;
                break;
            case KanjiFromT::Tuttle:
                match = k->tuttle != 0;
                break;
            default:
                break;
            }
            if (!match)
            {
                list.erase(list.begin() + ix);
                continue;
            }
        }

        if (filterActive(f.data, KanjiFilters::Index) && !f.data.index.isEmpty())
        {
            QString str;
            match = false;
            switch (f.data.indextype)
            {
            case KanjiIndexT::Unicode:
                if ((f.data.index.size() == 1 && f.data.index == "0") || (f.data.index.size() == 2 && f.data.index == "0x"))
                    match = true;
                else
                {
                    str = QString::number(k->ch.unicode(), 16);
                    if (f.data.index.left(2) == "0x")
                        str = "0x" + str;
                    match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                }
                break;
            case KanjiIndexT::EUCJP:
                if ((f.data.index.size() == 1 && f.data.index == "0") || (f.data.index.size() == 2 && f.data.index == "0x"))
                    match = true;
                else
                {
                    str = QString::number(JIStoEUC(k->jis), 16);
                    if (f.data.index.left(2) == "0x")
                        str = "0x" + str;
                    match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                }
                break;
            case KanjiIndexT::ShiftJIS:
                if ((f.data.index.size() == 1 && f.data.index == "0") || (f.data.index.size() == 2 && f.data.index == "0x"))
                    match = true;
                else
                {
                    str = QString::number(JIStoShiftJIS(k->jis), 16);
                    if (f.data.index.left(2) == "0x")
                        str = "0x" + str;
                    match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                }
                break;
            case KanjiIndexT::JISX0208:
                if ((f.data.index.size() == 1 && f.data.index == "0") || (f.data.index.size() == 2 && f.data.index == "0x"))
                    match = true;
                else
                {
                    str = QString::number(k->jis, 16);
                    if (f.data.index.left(2) == "0x")
                        str = "0x" + str;
                    match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                }
                break;
            case KanjiIndexT::Kuten:
                str = JIStoKuten(k->jis);
                match = f.data.index == str;
                break;
            case KanjiIndexT::Oneil:
                str = QString::number(k->oneil);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Gakken:
                str = QString::number(k->gakken);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Halpern:
                str = QString::number(k->halpern);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Heisig:
                str = QString::number(k->heisig);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::HeisigN:
                str = QString::number(k->heisign);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::HeisigF:
                str = QString::number(k->heisigf);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Henshall:
                str = QString::number(k->henshall);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Nelson:
                str = QString::number(k->nelson);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::NewNelson:
                str = QString::number(k->newnelson);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::SnH:
                // Check whether the snh code is null terminated or not,
                // because it might take up the size of the whole buffer.
                if (memchr(k->snh, 0, 8) != nullptr)
                    str = QString::fromLatin1(k->snh);
                else
                    str = QString::fromLatin1(k->snh, 8);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::KnK:
                str = QString::number(k->knk);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Busy:
                // Might not be null terminated, check for that.
                if (memchr(k->busy, 0, 4) != nullptr)
                    str = QString::fromLatin1(k->busy);
                else
                    str = QString::fromLatin1(k->busy, 4);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Crowley:
                str = QString::number(k->crowley);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::FlashC:
                str = QString::number(k->flashc);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::KGuide:
                str = QString::number(k->kguide);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::HalpernN:
                str = QString::number(k->halpernn);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Deroo:
                str = QString::number(k->deroo);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Sakade:
                str = QString::number(k->sakade);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::HenshallG:
                str = QString::number(k->henshallg);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Context:
                str = QString::number(k->context);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::HalpernK:
                str = QString::number(k->halpernk);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::HalpernL:
                str = QString::number(k->halpernl);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            case KanjiIndexT::Tuttle:
                str = QString::number(k->tuttle);
                match = qcharncmp(f.data.index.constData(), str.constData(), std::min(f.data.index.size(), str.size())) == 0;
                break;
            }

            if (!match)
            {
                list.erase(list.begin() + ix);
                continue;
            }
        }

        if (filterActive(f.data, KanjiFilters::Strokes) && ((f.data.strokemin != 0 && k->strokes < f.data.strokemin) || (f.data.strokemax != 0 && k->strokes > f.data.strokemax) || (f.data.strokemax == 0 && f.data.strokemin != 0 && k->strokes != f.data.strokemin)))
        {
            list.erase(list.begin() + ix);
            continue;
        }

        if (filterActive(f.data, KanjiFilters::JLPT) && ((f.data.jlptmin != -1 && k->jlpt < f.data.jlptmin) || (f.data.jlptmax != -1 && k->jlpt > f.data.jlptmax) || (f.data.jlptmax == -1 && f.data.jlptmin != -1 && k->jlpt != f.data.jlptmin)))
        {
            list.erase(list.begin() + ix);
            continue;
        }

        if (filterActive(f.data, KanjiFilters::SKIP) && ((f.data.skip1 != 0 && f.data.skip1 != k->skips[0]) || (f.data.skip2 > 0 && f.data.skip2 != k->skips[1]) || (f.data.skip3 > 0 && f.data.skip3 != k->skips[2])))
        {
            list.erase(list.begin() + ix);
            continue;
        }

        if (filterActive(f.data, KanjiFilters::Jouyou) && (f.data.jouyou != 0 && (k->jouyou == 0 || (f.data.jouyou != 7 && f.data.jouyou != k->jouyou) || (f.data.jouyou == 7 && k->jouyou > 6))))
        {
            list.erase(list.begin() + ix);
            continue;
        }

        if (filterActive(f.data, KanjiFilters::Reading) && !f.data.reading.isEmpty())
        {
            match = false;
            bool ron = true;
            bool rkun = true;
            if (f.data.readingstrict)
            {
                for (int iy = 0, sizy = f.data.reading.size(); (ron || rkun) && iy != sizy; ++iy)
                {
                    if (KATAKANA(f.data.reading.at(iy).unicode()))
                        rkun = false;
                    if (HIRAGANA(f.data.reading.at(iy).unicode()))
                        ron = false;
                }
            }

            QString str = hiraganize(f.data.reading);
            if (ron)
            {
                for (int iy = 0, sizy = k->on.size(); !match && iy != sizy; ++iy)
                {
                    QString str1 = hiraganize(k->on[iy].toQString());
                    if (!f.data.readingafter && str1.size() != str.size())
                        continue;
                    match = qcharncmp(str1.constData(), str.constData(), str.size()) == 0;
                }
            }
            if (!match && rkun)
            {
                for (int iy = 0, sizy = k->kun.size(); !match && iy != sizy; ++iy)
                {
                    QString kunstr = QString::fromRawData(k->kun[iy].data(), tosigned(qcharlen(k->kun[iy].data())));
                    int p = kunstr.indexOf('.');
                    if (p != -1)
                        kunstr = kunstr.left(p) + (!f.data.readingoku ? QString() : kunstr.right(kunstr.size() - p - 1));
                    kunstr = hiraganize(kunstr);

                    if (!f.data.readingafter && kunstr.size() != str.size())
                        continue;

                    match = qcharncmp(kunstr.constData(), str.constData(), str.size()) == 0;
                }
            }

            if (!match)
            {
                list.erase(list.begin() + ix);
                continue;
            }
        }

        // Throw out anything not matching the meaning. There is no tree for kanji meanings,
        // so the only way is to go through each of them and check for matches.
        if (filterActive(f.data, KanjiFilters::Meaning) && !f.data.meaning.isEmpty())
        {
            const int mlen = f.data.meaning.size();
            const QString fmeaning = f.data.meaning.toLower();
            const QChar *fmeaningdat = fmeaning.constData();

            match = false;
            const QString kmeaning = dict->kanjiMeaning(list[ix]).toLower();
            const QChar *kmeaningdat = kmeaning.constData();
            const int datlen = kmeaning.size();

            // Go through each character one by one. The match is only valid if it's at the
            // front of the meaning, or it comes after a delimiter. When meaningafter is
            // false, the match must be directly followed by a delimiter or must tail the
            // meaning.
            for (int pos = 0; !match && pos <= datlen - mlen; ++pos)
            {
                if (qcharisdelim(kmeaningdat[pos]) == QCharKind::Delimiter)
                    continue;
                if (qcharncmp(kmeaningdat + pos, fmeaningdat, mlen) == 0)
                {
                    if (!f.data.meaningafter)
                        match = pos + mlen == datlen || qcharisdelim(kmeaningdat[pos + mlen]) == QCharKind::Delimiter;
                    else
                        match = true;
                }

                while (pos < datlen - mlen && qcharisdelim(kmeaningdat[++pos]) != QCharKind::Delimiter)
                    ;
            }
            if (!match)
            {
                list.erase(list.begin() + ix);
                continue;
            }
        }
    }

    if (filterActive(f.data, KanjiFilters::Radicals) && radform != nullptr)
    {
        // The radical selecting window needs the list before filtering by the temporary
        // radical selection, so passing a list of kanji after the function returns is too
        // late.
        radform->setFromList(list);

        if (f.tmprads.empty() || list.empty())
            return;

        if (rads.mode == RadicalFilterModes::Radicals)
        {
            std::vector<ushort> group;
            for (int ix = 0; ix != ZKanji::kanjicount; ++ix)
            {
                auto it = std::find(f.tmprads.begin(), f.tmprads.end(), ZKanji::kanjis[ix]->rad);
                if (it != f.tmprads.end())
                    group.push_back(ix);
            }
            std::vector<ushort> temp;
            temp.swap(list);
            list.resize(group.size());
            list.resize(std::set_intersection(group.begin(), group.end(), temp.begin(), temp.end(), list.begin()) - list.begin());
        }
        else if (rads.mode == RadicalFilterModes::Parts)
        {
            std::vector<ushort> group;
            for (int ix = 0, siz = tosigned(f.tmprads.size()); ix != siz; ++ix)
                group.insert(group.end(), ZKanji::radklist[f.tmprads[ix]].second.begin(), ZKanji::radklist[f.tmprads[ix]].second.end());
            std::sort(group.begin(), group.end());
            group.resize(std::unique(group.begin(), group.end()) - group.begin());

            std::vector<ushort> temp;
            temp.swap(list);
            list.resize(group.size());
            list.resize(std::set_intersection(group.begin(), group.end(), temp.begin(), temp.end(), list.begin()) - list.begin());
        }
        else // NamedRadicals
        {
            std::vector<ushort> group;

            for (int ix = 0, siz = tosigned(f.tmprads.size()); ix != siz; ++ix)
            {
                if (!rads.grouped)
                    group.insert(group.end(), ZKanji::radlist[f.tmprads[ix]]->kanji.begin(), ZKanji::radlist[f.tmprads[ix]]->kanji.end());
                else
                {
                    for (int iy = f.tmprads[ix], sizy = tosigned(ZKanji::radlist.size()); iy != sizy; ++iy)
                    {
                        if (iy != f.tmprads[ix] && ZKanji::radlist[iy]->radical != ZKanji::radlist[iy - 1]->radical)
                            break;
                        else
                            group.insert(group.end(), ZKanji::radlist[iy]->kanji.begin(), ZKanji::radlist[iy]->kanji.end());
                    }
                }
            }
            std::sort(group.begin(), group.end());
            group.resize(std::unique(group.begin(), group.end()) - group.begin());

            std::vector<ushort> temp;
            temp.swap(list);
            list.resize(group.size());
            list.resize(std::set_intersection(group.begin(), group.end(), temp.begin(), temp.end(), list.begin()) - list.begin());
        }
    }
}

void KanjiSearchWidget::filterKanji(bool forced)
{
    if (ignorefilter)
        return;

    RuntimeKanjiFilters f;
    fillFilters(f);
    if (!filtersMatch(f, filters) || forced)
    {
        KanjiGridModel *tmp = ui->kanjiGrid->model();

        if (filtersEmpty(f))
        {
            ui->kanjiGrid->setModel(new KanjiGridSortModel(&mainKanjiListModel(), (KanjiGridSortOrder)ui->sortCBox->currentIndex(), ui->kanjiGrid->dictionary(), ui->kanjiGrid));
            if (radform != nullptr)
                radform->setFromModel(ui->kanjiGrid->model());
        }
        else
        {
            //KanjiListModel *model = new KanjiListModel(this);
            std::vector<ushort> list;
            listFilteredKanji(f, list);
            //model->setList(list);
            ui->kanjiGrid->setModel(new KanjiGridSortModel(&mainKanjiListModel(), std::move(list), (KanjiGridSortOrder)ui->sortCBox->currentIndex(), ui->kanjiGrid->dictionary(), ui->kanjiGrid));
        }

        delete tmp;
    }
    else if (f.data.ordertype != filters.data.ordertype)
        sortKanji();
    filters = f;
}

void KanjiSearchWidget::filter()
{
    filterKanji(false);
}

void KanjiSearchWidget::on_resetButton_clicked()
{
    reset();
}

void KanjiSearchWidget::on_f1Button_clicked()
{
    showHideAction(2);
}

void KanjiSearchWidget::on_f2Button_clicked()
{
    showHideAction(3);
}

void KanjiSearchWidget::on_f3Button_clicked()
{
    showHideAction(4);
}

void KanjiSearchWidget::on_f4Button_clicked()
{
    showHideAction(5);
}

void KanjiSearchWidget::on_f5Button_clicked()
{
    showHideAction(6);
}

void KanjiSearchWidget::on_f6Button_clicked()
{
    showHideAction(7);
}

void KanjiSearchWidget::on_f7Button_clicked()
{
    showHideAction(8);
}

void KanjiSearchWidget::on_f8Button_clicked()
{
    showHideAction(9);
}

void KanjiSearchWidget::on_allButton_clicked(bool checked)
{
    showHideAction(checked ? 0 : 1);
}

void KanjiSearchWidget::radicalsCButtonClicked()
{
    ui->radicalsCButton->setEnabled(false);
    radform = new RadicalForm(window());
    connect(radform, &RadicalForm::selectionChanged, this, &KanjiSearchWidget::radicalsChanged);
    connect(radform, &RadicalForm::groupingChanged, this, &KanjiSearchWidget::radicalGroupingChanged);
    connect(radform, &RadicalForm::resultIsOk, this, &KanjiSearchWidget::radicalsOk);
    connect(radform, &RadicalForm::destroyed, this, &KanjiSearchWidget::radicalsClosed);
    radform->setFromModel(ui->kanjiGrid->model());
    if (ui->radicalsCButton->currentIndex() != 0)
        radform->setFilter(ui->radicalsCButton->currentIndex() - 1);
    radform->show();
    filterKanji();
}

void KanjiSearchWidget::showHideAction(int index)
{
    switch (index)
    {
    case 0: // Show all
        ui->strokeWidget->show();
        ui->meaningWidget->show();
        ui->readingWidget->show();
        ui->jlptWidget->show();
        ui->radicalsWidget->show();
        ui->jouyouWidget->show();
        ui->skipWidget->show();
        ui->indexWidget->show();
        break;
    case 1: // Hide all
        ui->strokeWidget->hide();
        ui->meaningWidget->hide();
        ui->readingWidget->hide();
        ui->jlptWidget->hide();
        ui->radicalsWidget->hide();
        ui->jouyouWidget->hide();
        ui->skipWidget->hide();
        ui->indexWidget->hide();
        break;
    case 2: // Stroke count
        ui->strokeWidget->setVisible(!ui->strokeWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 3: // JLPT
        ui->jlptWidget->setVisible(!ui->jlptWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 4: // Meaning
        ui->meaningWidget->setVisible(!ui->meaningWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 5: // Reading
        ui->readingWidget->setVisible(!ui->readingWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 6: // Jouyou
        ui->jouyouWidget->setVisible(!ui->jouyouWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 7: // Radicals
        ui->radicalsWidget->setVisible(!ui->radicalsWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 8: // Index
        ui->indexWidget->setVisible(!ui->indexWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 9: // SKIP
        ui->skipWidget->setVisible(!ui->skipWidget->isVisibleTo(ui->optionsWidget));
        break;

    }

    ui->line->setVisible(ui->strokeWidget->isVisibleTo(ui->optionsWidget) || ui->meaningWidget->isVisibleTo(ui->optionsWidget) || ui->readingWidget->isVisibleTo(ui->optionsWidget) ||
        ui->jlptWidget->isVisibleTo(ui->optionsWidget) || ui->radicalsWidget->isVisibleTo(ui->optionsWidget) || ui->jouyouWidget->isVisibleTo(ui->optionsWidget) ||
        ui->skipWidget->isVisibleTo(ui->optionsWidget) || ui->indexWidget->isVisibleTo(ui->optionsWidget));

    ui->f1Button->setChecked(ui->strokeWidget->isVisibleTo(ui->optionsWidget));
    ui->f2Button->setChecked(ui->jlptWidget->isVisibleTo(ui->optionsWidget));
    ui->f3Button->setChecked(ui->meaningWidget->isVisibleTo(ui->optionsWidget));
    ui->f4Button->setChecked(ui->readingWidget->isVisibleTo(ui->optionsWidget));
    ui->f5Button->setChecked(ui->jouyouWidget->isVisibleTo(ui->optionsWidget));
    ui->f6Button->setChecked(ui->radicalsWidget->isVisibleTo(ui->optionsWidget));
    ui->f7Button->setChecked(ui->indexWidget->isVisibleTo(ui->optionsWidget));
    ui->f8Button->setChecked(ui->skipWidget->isVisibleTo(ui->optionsWidget));

    ui->allButton->setChecked(ui->f1Button->isChecked() && ui->f2Button->isChecked() && ui->f3Button->isChecked() && ui->f4Button->isChecked() && ui->f5Button->isChecked() && ui->f6Button->isChecked() && ui->f7Button->isChecked() && ui->f8Button->isChecked());

    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
    {
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::StrokeFilter, categ), ui->f1Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::JLPTFilter, categ), ui->f2Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::MeaningFilter, categ), ui->f3Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ReadingFilter, categ), ui->f4Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::JouyouFilter, categ), ui->f5Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::RadicalsFilter, categ), ui->f6Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::IndexFilter, categ), ui->f7Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::SKIPFilter, categ), ui->f8Button->isChecked());
    }

    ui->optionsWidget->setVisible(ui->line->isVisibleTo(this));

    filterKanji();
}

void KanjiSearchWidget::skipButtonsToggled(bool checked)
{
    if (checked)
    {
        if (sender() != ui->skip1Button && ui->skip1Button->isChecked())
            ui->skip1Button->setChecked(false);
        if (sender() != ui->skip2Button && ui->skip2Button->isChecked())
            ui->skip2Button->setChecked(false);
        if (sender() != ui->skip3Button && ui->skip3Button->isChecked())
            ui->skip3Button->setChecked(false);
        if (sender() != ui->skip4Button && ui->skip4Button->isChecked())
            ui->skip4Button->setChecked(false);
    }

    filterKanji();
}

void KanjiSearchWidget::sortKanji()
{
    KanjiGridSortModel *model = (KanjiGridSortModel*)ui->kanjiGrid->model();
    model->sort((KanjiGridSortOrder)ui->sortCBox->currentIndex(), ui->kanjiGrid->dictionary());
    //ui->kanjiGrid->setModel(new KanjiGridSortModel(model->baseModel(), (KanjiGridSortModel::SortOrder)ui->sortCBox->currentIndex(), ui->kanjiGrid));
    //delete model;
}

void KanjiSearchWidget::radicalSelected(int /*ix*/)
{
    //if (ui->radicalsCButton->currentIndex() == ix)
    //    return;

    //ui->radicalsCButton->setCurrentIndex(ix);
    filterKanji();
}

void KanjiSearchWidget::radicalsChanged()
{
    filterKanji();
}

void KanjiSearchWidget::radicalGroupingChanged(bool /*grouping*/)
{
    filterKanji();
}

void KanjiSearchWidget::radicalsOk()
{
    RadicalFilter f;
    radform->activeFilter(f);
    radform = nullptr;
    ui->radicalsCButton->setCurrentIndex(radicalFiltersModel().add(f) + 1);
}

void KanjiSearchWidget::radicalsClosed()
{
    if (radform != nullptr)
        radform = nullptr;
    filterKanji();
    ui->radicalsCButton->setEnabled(true);
}

void KanjiSearchWidget::radsAdded()
{
    RadicalFiltersModel &m = radicalFiltersModel();
    ui->radicalsCButton->addItem(m.filterText(tosigned(m.size()) - 1));
}

void KanjiSearchWidget::radsRemoved(int index)
{
    ++index;

    int cindex = ui->radicalsCButton->currentIndex();
    if (cindex == index)
        cindex = 0;
    else if (cindex > index)
        --cindex;

    ui->radicalsCButton->removeItem(index);
    ui->radicalsCButton->setCurrentIndex(cindex);
}

void KanjiSearchWidget::radsMoved(int index, bool up)
{
    ++index;

    QString str = ui->radicalsCButton->itemText(index);
    ui->radicalsCButton->setItemText(index, ui->radicalsCButton->itemText(index + (up ? -1 : 1)));
    ui->radicalsCButton->setItemText(index + (up ? -1 : 1), str);

    if (ui->radicalsCButton->currentIndex() == index)
        ui->radicalsCButton->setCurrentIndex(ui->radicalsCButton->currentIndex() + (up ? -1 : 1));
}

void KanjiSearchWidget::radsCleared()
{
    while (ui->radicalsCButton->count() != 1)
        ui->radicalsCButton->removeItem(ui->radicalsCButton->count() - 1);

    ui->radicalsCButton->setCurrentIndex(0);
}


//-------------------------------------------------------------
