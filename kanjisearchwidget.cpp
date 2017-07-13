#include <QApplication>
#include <QClipboard>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QStringBuilder>
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


//-------------------------------------------------------------


namespace FormStates
{
    KanjiFilterData kanjifilters;

    bool emptyState(const KanjiFilterData &data)
    {
        return data.filters == 0xff && data.ordertype == KanjiGridSortOrder::Jouyou && data.fromtype == KanjiFromT::All &&
            data.strokemin == 0 && data.strokemax == 0 && data.meaning.isEmpty() && data.reading.isEmpty() &&
            data.readingon && data.readingkun && data.readingoku && data.jlptmin == 0 && data.jlptmax == 0 &&
            data.skip1 == 0 && data.skip2 == -1 && data.skip3 == -1 && data.jouyou == 0 && data.indextype == KanjiIndexT::Unicode &&
            data.index.isEmpty() && data.rads == RadicalFilter() /*data.fromtype == KanjiFromT::All && *//*data.rads.mode == RadicalFilter::Parts && data.rads.grouped == false &&
            data.rads.groups.empty()*/;
    }

    void saveXMLSettings(const KanjiFilterData &data, QXmlStreamWriter &writer)
    {
        const QString True = QStringLiteral("1");
        const QString False = QStringLiteral("0");

        const QString ordertext[] = { "jouyou", "jlpt", "freq", "words", "wfreq", "strokes", "rad", "unicode", "jis208" };
        if (Settings::general.savewinstates)
            writer.writeAttribute("order", ordertext[(int)data.ordertype]);

        const QString fromtext[] = { "all", "clipbrd", "common", "jouyou", "jlpt",
            "oneil", "gakken", "halper", "heisig", "heisign", "heisigf", "henshall",
            "nelson", "nnelson", "snh", "knk", "busy", "crowley", "flashc", "kguide",
            "halpernn", "deroo", "sakade", "henshallg", "context", "halpernk", "halpernl",
            "tuttle" };

        if (Settings::general.savewinstates)
            writer.writeAttribute("from", fromtext[(int)data.fromtype]);

        if (((data.strokemin > 0 || data.strokemax > 0) && Settings::kanji.savefilters) || (!(data.filters & 0x01) && Settings::general.savewinstates))
        {
            writer.writeEmptyElement("Stroke");
            if ((data.strokemin > 0 || data.strokemax > 0) && Settings::kanji.savefilters)
                writer.writeAttribute("count", IntMinMaxToString(1, std::numeric_limits<int>::max() - 1, data.strokemin, data.strokemax));
            if (!(data.filters & 0x01) && Settings::general.savewinstates)
                writer.writeAttribute("hide", "1");
        }

        if (Settings::kanji.savefilters || (!(data.filters & 0x02) && Settings::general.savewinstates))
        {
            writer.writeEmptyElement("Meaning");
            if (Settings::kanji.savefilters)
                writer.writeAttribute("text", data.meaning);
            if (!(data.filters & 0x02) && Settings::general.savewinstates)
                writer.writeAttribute("hide", "1");
        }

        if (Settings::kanji.savefilters || (!(data.filters & 0x04) && Settings::general.savewinstates))
        {
            writer.writeEmptyElement("Reading");
            if (Settings::kanji.savefilters)
            {
                writer.writeAttribute("type", data.readingon && data.readingkun ? "on+kun" : data.readingon ? "on" : "kun");
                writer.writeAttribute("oku", data.readingoku ? True : False);
                writer.writeAttribute("text", data.reading);
            }

            if (!(data.filters & 0x04) && Settings::general.savewinstates)
                writer.writeAttribute("hide", "1");
        }

        if (((data.jlptmin > 0 || data.jlptmax > 0) && Settings::kanji.savefilters) || (!(data.filters & 0x08) && Settings::general.savewinstates))
        {
            writer.writeEmptyElement("JLPT");
            if ((data.jlptmin > 0 || data.jlptmax > 0) && Settings::kanji.savefilters)
                writer.writeAttribute("text", IntMinMaxToString(1, 5, data.jlptmin, data.jlptmax));
            if (!(data.filters & 0x08) && Settings::general.savewinstates)
                writer.writeAttribute("hide", "1");
        }

        if (Settings::kanji.savefilters || (!(data.filters & 0x10) && Settings::general.savewinstates))
        {
            const QString jouyoutext[] = { "all", "e1", "e2", "e3", "e4", "e5", "e6", "eall", "s", "n", "vn" };
            writer.writeEmptyElement("Jouyou");
            if (Settings::kanji.savefilters)
                writer.writeAttribute("grade", jouyoutext[data.jouyou]);
            if (!(data.filters & 0x10) && Settings::general.savewinstates)
                writer.writeAttribute("hide", "1");
        }

        if ((Settings::kanji.savefilters && (data.skip1 != 0 || data.skip2 != -1 || data.skip3 != -1)) || (!(data.filters & 0x80) && Settings::general.savewinstates))
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
            if (!(data.filters & 0x80) && Settings::general.savewinstates)
                writer.writeAttribute("hide", "1");
        }

        if (Settings::kanji.savefilters || (!(data.filters & 0x40) && Settings::general.savewinstates))
        {
            const QString indextext[] = { "unicode", "euc", "sjis", "jis208", "kuten",
                "oneil", "gakken", "halper", "heisig", "heisign", "heisigf", "henshall",
                "nelson", "nnelson", "snh", "knk", "busy", "crowley", "flashc", "kguide",
                "halpernn", "deroo", "sakade", "henshallg", "context", "halpernk", "halpernl",
                "tuttle" };

            writer.writeEmptyElement("Index");
            if (Settings::kanji.savefilters)
            {
                writer.writeAttribute("type", indextext[(int)data.indextype]);
                if (!data.index.isEmpty())
                    writer.writeAttribute("text", data.index);
            }
                if (!(data.filters & 0x40) && Settings::general.savewinstates)
                    writer.writeAttribute("hide", "1");
        }

        if ((Settings::kanji.savefilters && data.rads != RadicalFilter()) || (!(data.filters & 0x20) && Settings::general.savewinstates))
        {
            writer.writeStartElement("Radicals");
            if (data.rads != RadicalFilter())
            {
                writer.writeAttribute("mode", data.rads.mode == RadicalFilterModes::Parts ? "parts" : data.rads.mode == RadicalFilterModes::Radicals ? "radicals" : "named");
                writer.writeAttribute("grouped", data.rads.grouped ? True : False);
            }
            if (!(data.filters & 0x20) && Settings::general.savewinstates)
                writer.writeAttribute("hide", True);
            if (!data.rads.groups.empty())
            {
                for (int ix = 0, siz = data.rads.groups.size(); ix != siz; ++ix)
                {
                    if (data.rads.groups[ix].empty())
                        continue;

                    writer.writeEmptyElement("List");
                    QString val = QString::number(data.rads.groups[ix][0]);
                    for (int iy = 1, ysiz = data.rads.groups[ix].size(); iy != ysiz; ++iy)
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

        if (Settings::general.savewinstates && reader.attributes().hasAttribute("order"))
        {
            const int ordersize = 9;
            const QString ordertext[ordersize] = { "jouyou", "jlpt", "freq", "words", "wfreq", "strokes", "rad", "unicode", "jis208" };
            const QString *pos = std::find(ordertext, ordertext + ordersize, reader.attributes().value("order").toString());
            if (pos != ordertext + ordersize)
                data.ordertype = (KanjiGridSortOrder)(pos - ordertext);
        }

        if (Settings::general.savewinstates && reader.attributes().hasAttribute("from"))
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
                if (Settings::general.savewinstates && reader.attributes().value("hide") == True)
                    data.filters &= ~0x01;
                reader.skipCurrentElement();
            }

            if (reader.name() == "Meaning")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("text"))
                    data.meaning = reader.attributes().value("text").toString();
                if (Settings::general.savewinstates && reader.attributes().value("hide") == True)
                    data.filters &= ~0x02;
                reader.skipCurrentElement();
            }

            if (reader.name() == "Reading")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("type"))
                {
                    QStringRef str = reader.attributes().value("type");
                    data.readingon = str == QStringLiteral("on+kun") || str == QStringLiteral("on");
                    data.readingkun = str == QStringLiteral("on+kun") || str == QStringLiteral("kun");
                }
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("oku"))
                    data.readingoku = reader.attributes().value("oku")!= False;
                if (Settings::kanji.savefilters)
                    data.reading = reader.attributes().value("text").toString();

                if (Settings::general.savewinstates && reader.attributes().value("hide") == True)
                    data.filters &= ~0x04;
                reader.skipCurrentElement();
            }

            if (reader.name() == "JLPT")
            {
                if (Settings::kanji.savefilters && reader.attributes().hasAttribute("text"))
                    findStrIntMinMax(reader.attributes().value("text").toString(), 1, 5, data.jlptmin, data.jlptmax);
                if (Settings::general.savewinstates && reader.attributes().value("hide") == True)
                    data.filters &= ~0x08;
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
                if (Settings::general.savewinstates && reader.attributes().value("hide") == True)
                    data.filters &= ~0x10;

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
                if (Settings::general.savewinstates && reader.attributes().value("hide") == True)
                    data.filters &= ~0x80;
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

                if (Settings::general.savewinstates && reader.attributes().value("hide") == True)
                    data.filters &= ~0x40;

                reader.skipCurrentElement();
            }

            if (reader.name() == "Radicals")
            {
                if (Settings::general.savewinstates && reader.attributes().value("hide") == True)
                    data.filters &= ~0x20;

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


KanjiSearchWidget::KanjiSearchWidget(QWidget *parent) : base(parent), ui(new Ui::KanjiSearchWidget), ignorefilter(false), radform(nullptr), radindex(0)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    restrictWidgetSize(ui->strokeEdit, 8, AdjustedValue::MinMax);
    restrictWidgetSize(ui->jlptEdit, 6, AdjustedValue::MinMax);
    restrictWidgetSize(ui->skip1Edit, 4, AdjustedValue::MinMax);
    restrictWidgetSize(ui->skip2Edit, 4, AdjustedValue::MinMax);
    restrictWidgetSize(ui->radicalsCBox, 10, AdjustedValue::Min);

    ui->meaningEdit->setCharacterSize(8);
    ui->readingEdit->setCharacterSize(6);
    ui->indexEdit->setCharacterSize(8);

    ui->radicalsCBox->setCharacterSize(20);

    //ui->kanjiGrid->setCellSize(std::ceil(Settings::fonts.kanjifontsize / 0.7));

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
    optionsLayout->setHorizontalSpacing(10);
    ui->optionsWidget->setLayout(optionsLayout);

    ui->kanjiGrid->setModel(new KanjiGridSortModel(&mainKanjiListModel(), KanjiGridSortOrder::Jouyou, ui->kanjiGrid->dictionary(), ui->kanjiGrid));
    ui->readingEdit->setValidator(&kanaValidator());

    ui->strokeEdit->setValidator(new IntRangeValidator(1, std::numeric_limits<int>::max() - 1, true, this));
    ui->jlptEdit->setValidator(new IntRangeValidator(1, 5, true, this));

    fillFilters(filters);

    // Checkboxes updated when any of these changes
    //connect(ui->strokeEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->meaningEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->readingEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->readingCBox, SIGNAL(activated(int)), this, SLOT(updateCheckbox()));
    //connect(ui->okuriganaButton, &QToolButton::toggled, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->jlptEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->skip1Button, &QToolButton::toggled, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->skip2Button, &QToolButton::toggled, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->skip3Button, &QToolButton::toggled, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->skip4Button, &QToolButton::toggled, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->skip1Edit, &QLineEdit::textChanged, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->skip2Edit, &QLineEdit::textChanged, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->skipEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->jouyouCBox, SIGNAL(activated(int)), this, SLOT(updateCheckbox()));
    //connect(ui->indexEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::updateCheckbox);
    //connect(ui->indexCBox, SIGNAL(activated(int)), this, SLOT(updateCheckbox()));

    // Filtering activated on these
    connect(ui->strokeEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->meaningEdit, &QLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->readingEdit, &ZLineEdit::textChanged, this, &KanjiSearchWidget::filter);
    connect(ui->readingCBox, SIGNAL(activated(int)), this, SLOT(filter()));
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

    // Radical selection
    connect(ui->radicalsCBox, SIGNAL(activated(int)), this, SLOT(radicalsBoxChanged(int)));

    RadicalFiltersModel &m = radicalFiltersModel();
    connect(&m, &RadicalFiltersModel::filterAdded, this, &KanjiSearchWidget::radsAdded);
    connect(&m, &RadicalFiltersModel::filterRemoved, this, &KanjiSearchWidget::radsRemoved);
    connect(&m, &RadicalFiltersModel::filterMoved, this, &KanjiSearchWidget::radsMoved);
    connect(&m, &RadicalFiltersModel::cleared, this, &KanjiSearchWidget::radsCleared);
    while (ui->radicalsCBox->count() != m.size() + 2)
        ui->radicalsCBox->insertItem(ui->radicalsCBox->count() - 1, m.filterText(ui->radicalsCBox->count() - 2));

    connect(&popmap, SIGNAL(mapped(int)), this, SLOT(showHideAction(int)));

    // Page selection
    //connect(ui->meaningButton, &QToolButton::toggled, this, &KanjiSearchWidget::showPage);
    //connect(ui->readingButton, &QToolButton::toggled, this, &KanjiSearchWidget::showPage);
    //connect(ui->jlptButton, &QToolButton::toggled, this, &KanjiSearchWidget::showPage);
    //connect(ui->jouyouButton, &QToolButton::toggled, this, &KanjiSearchWidget::showPage);
    //connect(ui->partsButton, &QToolButton::toggled, this, &KanjiSearchWidget::showPage);
    //connect(ui->skipButton, &QToolButton::toggled, this, &KanjiSearchWidget::showPage);
    //connect(ui->indexButton, &QToolButton::toggled, this, &KanjiSearchWidget::showPage);
}

KanjiSearchWidget::~KanjiSearchWidget()
{
    delete ui;
}

void KanjiSearchWidget::saveXMLSettings(QXmlStreamWriter &writer) const
{
    KanjiFilterData data;
    saveState(data);

    FormStates::saveXMLSettings(data, writer);

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

void KanjiSearchWidget::saveState(KanjiFilterData &data) const
{
    data.fromtype = (KanjiFromT)ui->fromCBox->currentIndex();
    data.ordertype = (KanjiGridSortOrder)ui->sortCBox->currentIndex();

    bool success;
    int val;

    //if (ui->strokeWidget->isVisibleTo(ui->optionsWidget))
    findStrIntMinMax(ui->strokeEdit->text(), 1, std::numeric_limits<int>::max() - 1, data.strokemin, data.strokemax);
    //else
    //    data.strokemin = 0, data.strokemax = 0;

    data.filters = ((ui->strokeWidget->isVisibleTo(ui->optionsWidget) ?  0x01 : 0)) |
                   ((ui->meaningWidget->isVisibleTo(ui->optionsWidget) ? 0x02 : 0)) |
                   ((ui->readingWidget->isVisibleTo(ui->optionsWidget) ? 0x04 : 0)) |
                   ((ui->jlptWidget->isVisibleTo(ui->optionsWidget) ? 0x08 : 0)) |
                   ((ui->jouyouWidget->isVisibleTo(ui->optionsWidget) ? 0x10 : 0)) |
                   ((ui->radicalsWidget->isVisibleTo(ui->optionsWidget) ? 0x20 : 0)) |
                   ((ui->indexWidget->isVisibleTo(ui->optionsWidget) ? 0x40 : 0)) |
                   ((ui->skipWidget->isVisibleTo(ui->optionsWidget) ? 0x80 : 0));
    

    data.meaning = /*ui->meaningWidget->isVisibleTo(ui->optionsWidget) ?*/ ui->meaningEdit->text().trimmed() /*: QString()*/;
    data.reading = /*ui->readingWidget->isVisibleTo(ui->optionsWidget) ?*/ ui->readingEdit->text().trimmed() /*: QString()*/;
    ui->readingEdit->validator()->fixup(data.reading);
    data.readingon = ui->readingCBox->currentIndex() == 0 || ui->readingCBox->currentIndex() == 1;
    data.readingkun = ui->readingCBox->currentIndex() == 0 || ui->readingCBox->currentIndex() == 2;
    data.readingoku = ui->okuriganaButton->isChecked();

    //if (ui->jlptWidget->isVisibleTo(ui->optionsWidget))
    findStrIntMinMax(ui->jlptEdit->text(), 1, 5, data.jlptmin, data.jlptmax);
    //else
    //    data.jlptmin = 0, data.jlptmax = 0;

    //if (ui->skipWidget->isVisibleTo(ui->optionsWidget))
    //{
    data.skip1 = ui->skip1Button->isChecked() ? 1 : ui->skip2Button->isChecked() ? 2 : ui->skip3Button->isChecked() ? 3 : ui->skip4Button->isChecked() ? 4 : 0;
    val = ui->skip1Edit->text().trimmed().toInt(&success);
    if (success)
        data.skip2 = val;
    else
        data.skip2 = -1;
    val = ui->skip2Edit->text().trimmed().toInt(&success);
    if (success)
        data.skip3 = val;
    else
        data.skip3 = -1;
    //}
    //else
    //{
    //    data.skip1 = 0;
    //    data.skip2 = 0;
    //    data.skip3 = 0;
    //}

    data.jouyou = /*ui->jouyouWidget->isVisibleTo(ui->optionsWidget) ?*/ ui->jouyouCBox->currentIndex() /*: 0*/;

    data.indextype = (KanjiIndexT)ui->indexCBox->currentIndex();
    data.index = /*ui->indexWidget->isVisibleTo(ui->optionsWidget) ?*/ ui->indexEdit->text().trimmed() /*: QString()*/;

    //if (ui->radicalsWidget->isVisibleTo(ui->optionsWidget))
    //{
    //    if (radform != nullptr)
    //    {
    //        data.rads = radform->activeFilter();
    //        tmprads = radform->selectionToFilter();
    //    }
    //    else
    //   {

    if (ui->radicalsCBox->currentIndex() > 0 && ui->radicalsCBox->currentIndex() < ui->radicalsCBox->count() - 1)
        data.rads = radicalFiltersModel().filters(ui->radicalsCBox->currentIndex() - 1);
    else
        data.rads = RadicalFilter();
    //            f.data.rads = radicalFiltersModel().filters(ui->radicalsCBox->currentIndex() - 1);
    //    }
    //}
    //else
    //{
    //    f.data.rads.groups.clear();
    //    f.tmprads.clear();
    //}
}

void KanjiSearchWidget::restoreState(const KanjiFilterData &data)
{
    if (ignorefilter)
        return;

    ignorefilter = true;
    if (Settings::general.savewinstates)
    {
        ui->fromCBox->setCurrentIndex((int)data.fromtype);// reader.attributes().value("from").toString());
        ui->sortCBox->setCurrentIndex((int)data.ordertype);// reader.attributes().value("sort").toString());
    }

    if (Settings::general.savewinstates)
        ui->strokeWidget->setVisible((data.filters & 0x01) != 0);
    if (Settings::kanji.savefilters)
        ui->strokeEdit->setText(IntMinMaxToString(1, std::numeric_limits<int>::max() - 1, data.strokemin, data.strokemax));

    if (Settings::general.savewinstates)
        ui->meaningWidget->setVisible((data.filters & 0x02) != 0);
    if (Settings::kanji.savefilters)
        ui->meaningEdit->setText(data.meaning);

    if (Settings::general.savewinstates)
        ui->readingWidget->setVisible((data.filters & 0x04) != 0);
    if (Settings::kanji.savefilters)
    {
        ui->readingCBox->setCurrentIndex(data.readingon && data.readingkun ? 0 : data.readingon ? 1 : 2);
        ui->okuriganaButton->setChecked(data.readingoku);
        ui->readingEdit->setText(data.reading);
    }

    if (Settings::general.savewinstates)
        ui->jlptWidget->setVisible((data.filters & 0x08) != 0);
    if (Settings::kanji.savefilters)
        ui->jlptEdit->setText(IntMinMaxToString(1, 5, data.jlptmin, data.jlptmax));

    if (Settings::general.savewinstates)
        ui->jouyouWidget->setVisible((data.filters & 0x10) != 0);
    if (Settings::kanji.savefilters)
        ui->jouyouCBox->setCurrentIndex(data.jouyou);

    if (Settings::general.savewinstates)
        ui->skipWidget->setVisible((data.filters & 0x80) != 0);
    if (Settings::kanji.savefilters)
    {
        ui->skip1Button->setChecked(data.skip1 == 1);
        ui->skip2Button->setChecked(data.skip1 == 2);
        ui->skip3Button->setChecked(data.skip1 == 3);
        ui->skip4Button->setChecked(data.skip1 == 4);
        ui->skip1Edit->setText(data.skip2 <= 0 ? QString() : QString::number(data.skip2));
        ui->skip2Edit->setText(data.skip3 <= 0 ? QString() : QString::number(data.skip3));
    }

    if (Settings::general.savewinstates)
        ui->indexWidget->setVisible((data.filters & 0x40) != 0);
    if (Settings::kanji.savefilters)
    {
        ui->indexCBox->setCurrentIndex((int)data.indextype);
        ui->indexEdit->setText(data.index);
    }

    if (Settings::general.savewinstates)
        ui->radicalsWidget->setVisible((data.filters & 0x20) != 0);
    if (Settings::kanji.savefilters)
    {
        int index = 0;
        if (data.rads != RadicalFilter())
        {
            index = radicalFiltersModel().filterIndex(data.rads);
            if (index == -1)
                index = 0;
            else
                index += 1;
        }
        ui->radicalsCBox->setCurrentIndex(std::max(0, index));
    }

    ui->line->setVisible(ui->strokeWidget->isVisibleTo(ui->optionsWidget) || ui->meaningWidget->isVisibleTo(ui->optionsWidget) || ui->readingWidget->isVisibleTo(ui->optionsWidget) ||
        ui->jlptWidget->isVisibleTo(ui->optionsWidget) || ui->radicalsWidget->isVisibleTo(ui->optionsWidget) || ui->jouyouWidget->isVisibleTo(ui->optionsWidget) ||
        ui->skipWidget->isVisibleTo(ui->optionsWidget) || ui->indexWidget->isVisibleTo(ui->optionsWidget));

    ui->f1Button->setChecked(ui->strokeWidget->isVisibleTo(ui->optionsWidget));
    ui->f2Button->setChecked(ui->meaningWidget->isVisibleTo(ui->optionsWidget));
    ui->f3Button->setChecked(ui->readingWidget->isVisibleTo(ui->optionsWidget));
    ui->f4Button->setChecked(ui->jlptWidget->isVisibleTo(ui->optionsWidget));
    ui->f5Button->setChecked(ui->jouyouWidget->isVisibleTo(ui->optionsWidget));
    ui->f6Button->setChecked(ui->radicalsWidget->isVisibleTo(ui->optionsWidget));
    ui->f7Button->setChecked(ui->indexWidget->isVisibleTo(ui->optionsWidget));
    ui->f8Button->setChecked(ui->skipWidget->isVisibleTo(ui->optionsWidget));

    ui->optionsWidget->setVisible(ui->line->isVisibleTo(this));

    ignorefilter = false;
    filterKanji(true);
}

//void KanjiSearchWidget::makeModeSpace(const QSize &size)
//{
//    ((ZFlowLayout*)ui->optionsWidget->layout())->makeModeSpace(size);
//}

void KanjiSearchWidget::setDictionary(int index)
{
    Dictionary *d = ZKanji::dictionary(index);

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

    const QWidget *w = parentWidget();
    while (w != nullptr && dynamic_cast<const ZKanjiWidget*>(w) == nullptr)
        w = w->parentWidget();

    if (w == nullptr)
        return CommandCategories::NoCateg;

    ZKanjiWidget *zw = ((ZKanjiWidget*)w);
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
    case (int)Commands::MeaningFilter:
        ui->f2Button->toggle();
        ui->f2Button->click();
        break;
    case (int)Commands::ReadingFilter:
        ui->f3Button->toggle();
        ui->f3Button->click();
        break;
    case (int)Commands::JLPTFilter:
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
    case (int)Commands::MeaningFilter:
        checked = ui->f2Button->isChecked();
        break;
    case (int)Commands::ReadingFilter:
        checked = ui->f3Button->isChecked();
        break;
    case (int)Commands::JLPTFilter:
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

//void KanjiSearchWidget::useFilters(KanjiFilters f)
//{
//    KanjiFilters f;
//
//    bool success;
//    int val;
//
//    if (ui->strokeWidget->isVisibleTo(ui->optionsWidget))
//        findStrIntMinMax(ui->strokeEdit->text(), 1, std::numeric_limits<int>::max() - 1, f.strokemin, f.strokemax);
//    else
//        f.strokemin = 0, f.strokemax = 0;
//
//    f.meaning = ui->meaningWidget->isVisibleTo(ui->optionsWidget) ? ui->meaningEdit->text().trimmed() : QString();
//    f.reading = ui->readingWidget->isVisibleTo(ui->optionsWidget) ? ui->readingEdit->text().trimmed() : QString();
//    ui->readingEdit->validator()->fixup(f.reading);
//    f.readingon = ui->readingCBox->currentIndex() == 0 || ui->readingCBox->currentIndex() == 1;
//    f.readingkun = ui->readingCBox->currentIndex() == 0 || ui->readingCBox->currentIndex() == 2;
//    f.readingokurigana = ui->okuriganaButton->isChecked();
//
//    if (ui->jlptWidget->isVisibleTo(ui->optionsWidget))
//        findStrIntMinMax(ui->jlptEdit->text(), 1, 5, f.jlptmin, f.jlptmax);
//    else
//        f.jlptmin = 0, f.jlptmax = 0;
//
//    if (ui->skipWidget->isVisibleTo(ui->optionsWidget))
//    {
//        f.skip1 = ui->skip1Button->isChecked() ? 1 : ui->skip2Button->isChecked() ? 2 : ui->skip3Button->isChecked() ? 3 : ui->skip4Button->isChecked() ? 4 : 0;
//        val = ui->skip1Edit->text().trimmed().toInt(&success);
//        if (success)
//            f.skip2 = val;
//        else
//            f.skip2 = 0;
//        val = ui->skip2Edit->text().trimmed().toInt(&success);
//        if (success)
//            f.skip3 = val;
//        else
//            f.skip3 = 0;
//    }
//    else
//    {
//        f.skip1 = 0;
//        f.skip2 = 0;
//        f.skip3 = 0;
//    }
//
//    f.jouyou = ui->jouyouWidget->isVisibleTo(ui->optionsWidget) ? ui->jouyouCBox->currentIndex() : 0;
//
//    f.indextype = (KanjiIndexT)ui->indexCBox->currentIndex();
//    f.index = ui->indexWidget->isVisibleTo(ui->optionsWidget) ? ui->indexEdit->text().trimmed() : QString();
//
//    f.fromtype = (KanjiFromT)ui->fromCBox->currentIndex();
//
//    if (ui->radicalsWidget->isVisibleTo(ui->optionsWidget))
//    {
//        if (radform != nullptr)
//        {
//            f.rads = radform->activeFilter();
//            f.tmprads = radform->selectionToFilter();
//        }
//        else
//        {
//            f.rads.groups.clear();
//            f.tmprads.clear();
//            if (ui->radicalsCBox->currentIndex() != 0 && ui->radicalsCBox->currentIndex() < ui->radicalsCBox->count() - 1)
//                f.rads = radicalFiltersModel().filters(ui->radicalsCBox->currentIndex() - 1);
//        }
//    }
//    else
//    {
//        f.rads.groups.clear();
//        f.tmprads.clear();
//    }
//
//    return f;
//}

void KanjiSearchWidget::reset()
{
    ui->strokeEdit->setText(QString());
    ui->radicalsCBox->setCurrentIndex(0);
    ui->meaningEdit->setText(QString());
    ui->readingEdit->setText(QString());
    ui->readingCBox->setCurrentIndex(0);
    ui->okuriganaButton->setChecked(true);
    ui->jlptEdit->setText(QString());
    ui->jouyouCBox->setCurrentIndex(0);
    //ui->partsCBox->setCurrentIndex(0);
    ui->skip1Button->setChecked(false);
    ui->skip2Button->setChecked(false);
    ui->skip3Button->setChecked(false);
    ui->skip4Button->setChecked(false);
    ui->skip1Edit->setText(QString());
    ui->skip2Edit->setText(QString());
    //ui->skipEdit->setText(QString());
    ui->indexCBox->setCurrentIndex(0);
    ui->indexEdit->setText(QString());

    filterKanji();
}

void KanjiSearchWidget::showEvent(QShowEvent *e)
{
    base::showEvent(e);
    
    ZFlowLayout *flow = (ZFlowLayout*)ui->optionsWidget->layout();
    flow->restrictWidth(ui->meaningWidget, restrictedWidgetSize(ui->meaningEdit, 16) + ui->meaningWidget->layout()->spacing() + ui->meaningLabel->width());
    flow->restrictWidth(ui->readingWidget, restrictedWidgetSize(ui->readingEdit, 12) + ui->readingLabel->width() + ui->readingCBox->width() + ui->okuriganaButton->width() + ui->readingWidget->layout()->spacing() * 3);
    //flow->restrictWidth(ui->indexWidget, restrictedWidgetSize(ui->indexEdit, 12) + ui->indexLabel->width() + ui->indexCBox->width() + ui->indexWidget->layout()->spacing() * 2);
    flow->restrictWidth(ui->radicalsWidget, ui->radicalsLabel->width() + restrictedWidgetSize(ui->radicalsCBox, 20) + ui->radicalsWidget->layout()->spacing());

}

void KanjiSearchWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QAction *a;

    popup.clear();

    a = popup.addAction("Show all filters");
    a->setEnabled(!ui->strokeEdit->isVisible() || !ui->meaningEdit->isVisible() || !ui->readingEdit->isVisible() ||
        !ui->jlptEdit->isVisible() || !ui->radicalsCBox->isVisible() || !ui->jouyouCBox->isVisible() ||
        !ui->skip1Edit->isVisible() || !ui->indexEdit->isVisible());
    if (a->isEnabled())
    {
        connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
        popmap.setMapping(a, 0);
    }

    a = popup.addAction("Hide all filters");
    a->setEnabled(ui->strokeEdit->isVisible() || ui->meaningEdit->isVisible() || ui->readingEdit->isVisible() ||
        ui->jlptEdit->isVisible() || ui->radicalsCBox->isVisible() || ui->jouyouCBox->isVisible() ||
        ui->skip1Edit->isVisible() || ui->indexEdit->isVisible());
    if (a->isEnabled())
    {
        connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
        popmap.setMapping(a, 1);
    }

    popup.addSeparator();

    a = popup.addAction("Stroke count filter");
    a->setCheckable(true);
    a->setChecked(ui->strokeEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
    popmap.setMapping(a, 2);

    a = popup.addAction("Meaning filter");
    a->setCheckable(true);
    a->setChecked(ui->meaningEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
    popmap.setMapping(a, 3);

    a = popup.addAction("Reading filter");
    a->setCheckable(true);
    a->setChecked(ui->readingEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
    popmap.setMapping(a, 4);

    a = popup.addAction("JLPT level filter");
    a->setCheckable(true);
    a->setChecked(ui->jlptEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
    popmap.setMapping(a, 5);

    a = popup.addAction("Jouyou grade filter");
    a->setCheckable(true);
    a->setChecked(ui->jouyouCBox->isVisible());
    connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
    popmap.setMapping(a, 6);

    a = popup.addAction("Radicals filter");
    a->setCheckable(true);
    a->setChecked(ui->radicalsCBox->isVisible());
    connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
    popmap.setMapping(a, 7);

    a = popup.addAction("SKIP code filter");
    a->setCheckable(true);
    a->setChecked(ui->skip1Edit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
    popmap.setMapping(a, 8);

    a = popup.addAction("Index filter");
    a->setCheckable(true);
    a->setChecked(ui->indexEdit->isVisible());
    connect(a, SIGNAL(triggered(bool)), &popmap, SLOT(map()));
    popmap.setMapping(a, 9);

    ZKanjiWidget::getZParent(this)->addDockAction(&popup);

    popup.popup(e->globalPos());
    e->accept();
}

bool KanjiSearchWidget::filtersMatch(const RuntimeKanjiFilters &a, const RuntimeKanjiFilters &b)
{
    return
        (((!(a.data.filters & 0x01) || (a.data.strokemin == 0 && a.data.strokemax == 0)) && (!(b.data.filters & 0x01) || (b.data.strokemin == 0 && b.data.strokemax == 0))) ||
        (clamp(!(a.data.filters & 0x01) ? 0 : a.data.strokemin, 0, 30) == clamp(!(b.data.filters & 0x01) ? 0 : b.data.strokemin, 0, 30) && 
        clamp(!(a.data.filters & 0x01) ? 0 : a.data.strokemax, 0, 30) == clamp(!(b.data.filters & 0x01) ? 0 : b.data.strokemax, 0, 30))) &&

        ((!(a.data.filters & 0x02) ? QString() : a.data.meaning.toLower()) == (!(b.data.filters & 0x02) ? QString() : b.data.meaning.toLower())) &&

        (((!(a.data.filters & 0x04) || a.data.reading.isEmpty()) && (!(b.data.filters & 0x04) || b.data.reading.isEmpty())) ||
        ((!(a.data.filters & 0x04) ? QString() : hiraganize(a.data.reading)) == (!(b.data.filters & 0x04) ? QString() : hiraganize(b.data.reading)) &&
        ((a.data.filters & 0x04) != 0 && (b.data.filters & 0x04) != 0 && a.data.readingon == b.data.readingon && a.data.readingkun == b.data.readingkun && a.data.readingoku == b.data.readingoku))) &&

        (std::max(0, !(a.data.filters & 0x08) ? 0 : a.data.jlptmin) == std::max(0, !(b.data.filters & 0x08) ? 0 : b.data.jlptmin) &&
        std::min(5, !(a.data.filters & 0x08) ? 5 : a.data.jlptmax) == std::min(5, !(b.data.filters & 0x08) ? 5 : b.data.jlptmax)) &&

        ((!(a.data.filters & 0x10) ? 0 : a.data.jouyou) == (!(b.data.filters & 0x10) ? 0 : b.data.jouyou)) &&

        ((!(a.data.filters & 0x80) ? 0 : a.data.skip1) == (!(b.data.filters & 0x80) ? 0 : b.data.skip1) &&
        clamp(!(a.data.filters & 0x80) ? 0 : a.data.skip2, 0, 30) == clamp(!(b.data.filters & 0x80) ? 0 : b.data.skip2, 0, 30) &&
        clamp(!(a.data.filters & 0x80) ? 0 : a.data.skip3, 0, 30) == clamp(!(b.data.filters & 0x80) ? 0 : b.data.skip3, 0, 30)) &&

        (((!(a.data.filters & 0x40) || a.data.index.isEmpty()) && (!(b.data.filters & 0x40) || b.data.index.isEmpty())) ||
        ((!(a.data.filters & 0x40) ? QString() : a.data.index.toLower()) == (!(b.data.filters & 0x40) ? QString() : b.data.index.toLower()) &&
        a.data.indextype == b.data.indextype)) &&

        ((!(a.data.filters & 0x20) ? RadicalFilter() : a.data.rads) == (!(b.data.filters & 0x20) ? RadicalFilter() : b.data.rads)) &&
        ((!(a.data.filters & 0x20) ? std::vector<ushort>() : a.tmprads) == (!(b.data.filters & 0x20) ? std::vector<ushort>() : b.tmprads)) &&

        (a.data.fromtype == b.data.fromtype);
}

bool KanjiSearchWidget::filtersEmpty(const RuntimeKanjiFilters &f)
{
    return
        (!(f.data.filters & 0x01) || (f.data.strokemin == 0 && f.data.strokemax == 0) || (f.data.strokemax != 0 && f.data.strokemax < f.data.strokemin)) &&
        (!(f.data.filters & 0x02) || f.data.meaning.isEmpty()) &&
        (!(f.data.filters & 0x04) || f.data.reading.isEmpty()) &&
        (!(f.data.filters & 0x08) || (f.data.jlptmin == 0 && f.data.jlptmax == 0) || (f.data.jlptmax != 0 && f.data.jlptmax < f.data.jlptmin)) &&
        (!(f.data.filters & 0x10) || (f.data.jouyou == 0)) &&
        (!(f.data.filters & 0x80) || (f.data.skip1 == 0 && (f.data.skip2 <= 0 || f.data.skip2 > 30) && (f.data.skip3 <= 0 || f.data.skip3 > 30))) &&
        (!(f.data.filters & 0x40) || f.data.index.isEmpty()) &&
        (!(f.data.filters & 0x20) || (f.data.rads == RadicalFilter() && f.tmprads.empty())) &&
        f.data.fromtype == KanjiFromT::All;
}

void KanjiSearchWidget::listFilteredKanji(const RuntimeKanjiFilters &f, std::vector<ushort> &list)
{
    list.clear();

    RadicalFilter rads = f.data.rads;

    if ((f.data.filters & 0x20) == 0 || rads.groups.empty())
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
        for (int ix = 0; ix != rads.groups.size(); ++ix)
        {
            const std::vector<ushort> &g = rads.groups[ix];
            for (int iy = 0; iy != g.size(); ++iy)
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
        for (int ix = 0; ix != rads.groups.size(); ++ix)
        {
            const std::vector<ushort> &g = rads.groups[ix];
            for (int iy = 0; iy != g.size(); ++iy)
            {
                if (!rads.grouped)
                    group.insert(group.end(), ZKanji::radlist[g[iy]]->kanji.begin(), ZKanji::radlist[g[iy]]->kanji.end());
                else
                {
                    for (int j = g[iy], siz = ZKanji::radlist.size(); j != siz; ++j)
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

    for (int ix = list.size() - 1; ix != -1; --ix)
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
            }
            if (!match)
            {
                list.erase(list.begin() + ix);
                continue;
            }
        }

        if ((f.data.filters & 0x40) != 0 && !f.data.index.isEmpty())
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

        if ((f.data.filters & 0x01) != 0 && ((f.data.strokemin != 0 && k->strokes < f.data.strokemin) || (f.data.strokemax != 0 && k->strokes > f.data.strokemax) || (f.data.strokemax == 0 && f.data.strokemin != 0 && k->strokes != f.data.strokemin)))
        {
            list.erase(list.begin() + ix);
            continue;
        }

        if ((f.data.filters & 0x08) != 0 && ((f.data.jlptmin <= f.data.jlptmax || f.data.jlptmax == 0) && ((f.data.jlptmin != 0 && k->jlpt < f.data.jlptmin) || (f.data.jlptmax != 0 && k->jlpt > f.data.jlptmax))))
        {
            list.erase(list.begin() + ix);
            continue;
        }

        if ((f.data.filters & 0x80) != 0 && ((f.data.skip1 != 0 && f.data.skip1 != k->skips[0]) || (f.data.skip2 > 0 && f.data.skip2 != k->skips[1]) || (f.data.skip3 > 0 && f.data.skip3 != k->skips[2])))
        {
            list.erase(list.begin() + ix);
            continue;
        }

        if ((f.data.filters & 0x10) != 0 && (f.data.jouyou != 0 && (k->jouyou == 0 || (f.data.jouyou != 7 && f.data.jouyou != k->jouyou) || (f.data.jouyou == 7 && k->jouyou > 6))))
        {
            list.erase(list.begin() + ix);
            continue;
        }

        if ((f.data.filters & 0x04) != 0 && !f.data.reading.isEmpty())
        {
            match = false;
            QString str = hiraganize(f.data.reading);
            if (f.data.readingon)
            {
                for (int iy = 0; !match && iy != k->on.size(); ++iy)
                {
                    QString str1 = hiraganize(k->on[iy].toQString());
                    match = qcharncmp(str1.constData(), str.constData(), str.size()) == 0;
                }
            }
            if (!match && f.data.readingkun)
            {
                for (int iy = 0; !match && iy != k->kun.size(); ++iy)
                {
                    QString kunstr = QString::fromRawData(k->kun[iy].data(), qcharlen(k->kun[iy].data()));
                    int p = kunstr.indexOf('.');
                    if (p != -1)
                        kunstr = kunstr.left(p) + (!f.data.readingoku ? QString() : kunstr.right(kunstr.size() - p - 1));
                    kunstr = hiraganize(kunstr);

                    match = qcharncmp(kunstr.constData(), str.constData(), str.size()) == 0;
                }
            }

            if (!match)
            {
                list.erase(list.begin() + ix);
                continue;
            }
        }

        // Throw out anything not matching the meaning. There is no tree for kanji
        // meanings, so the only way is to go through each of them and check for
        // matches.
        if ((f.data.filters & 0x02) != 0 && !f.data.meaning.isEmpty())
        {
            const int mlen = f.data.meaning.size();
            const QString mstr = f.data.meaning.toLower();
            const QChar *mdat = mstr.constData();

            match = false;
            const QString datstr = dict->kanjiMeaning(list[ix]).toLower();
            const QChar *dat = datstr.constData();
            const int datlen = datstr.size();
            for (int pos = 0; !match && pos <= datlen - mlen; ++pos)
            {
                if (qcharisdelim(dat[pos]) == QCharKind::Delimiter)
                    continue;
                if (qcharncmp(dat + pos, mdat, mlen) == 0)
                    match = true;
                while (pos < datlen - mlen && qcharisdelim(dat[++pos]) != QCharKind::Delimiter)
                    ;
            }
            if (!match)
            {
                list.erase(list.begin() + ix);
                continue;
            }
        }
    }

    if ((f.data.filters & 0x20) != 0 && radform != nullptr)
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
            for (int ix = 0; ix != f.tmprads.size(); ++ix)
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

            for (int ix = 0; ix != f.tmprads.size(); ++ix)
            {
                if (!rads.grouped)
                    group.insert(group.end(), ZKanji::radlist[f.tmprads[ix]]->kanji.begin(), ZKanji::radlist[f.tmprads[ix]]->kanji.end());
                else
                {
                    for (int j = f.tmprads[ix], siz = ZKanji::radlist.size(); j != siz; ++j)
                    {
                        if (j != f.tmprads[ix] && ZKanji::radlist[j]->radical != ZKanji::radlist[j - 1]->radical)
                            break;
                        else
                            group.insert(group.end(), ZKanji::radlist[j]->kanji.begin(), ZKanji::radlist[j]->kanji.end());
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

//void KanjiSearchWidget::updateCheckbox()
//{
//    auto checkfunc = [](QLineEdit *edit, QCheckBox *box) {
//        box->setChecked(!edit->text().isEmpty());
//    };
//
//
//    if (sender() == ui->strokeEdit)
//        checkfunc(ui->strokeEdit, ui->strokeBox);
//    else if (sender() == ui->meaningEdit)
//        checkfunc(ui->meaningEdit, ui->meaningBox);
//    else if (sender() == ui->readingEdit)
//        checkfunc(ui->readingEdit, ui->readingBox);
//    else if (sender() == ui->readingCBox)
//        checkfunc(ui->readingEdit, ui->readingBox);
//    else if (sender() == ui->okuriganaButton)
//        checkfunc(ui->readingEdit, ui->readingBox);
//    else if (sender() == ui->jlptEdit)
//        checkfunc(ui->jlptEdit, ui->jlptBox);
//    else if (sender() == ui->jouyouCBox)
//        ui->jouyouBox->setChecked(true);
//    else if (sender() == ui->indexEdit)
//        checkfunc(ui->indexEdit, ui->indexBox);
//    else if (sender() == ui->indexCBox)
//        checkfunc(ui->indexEdit, ui->indexBox);
//    else if (sender() == ui->skip1Button || sender() == ui->skip2Button || sender() == ui->skip3Button || sender() == ui->skip4Button)
//        ui->skipBox->setChecked(true);
//    else if (sender() == ui->skip1Edit && !ui->skip1Edit->text().isEmpty())
//        ui->skipBox->setChecked(true);
//    else if (sender() == ui->skip2Edit && !ui->skip2Edit->text().isEmpty())
//        ui->skipBox->setChecked(true);
//    else if (sender() == ui->skipEdit && !ui->skipEdit->text().isEmpty())
//        ui->skipBox->setChecked(true);
//}

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


//void KanjiSearchWidget::on_clearButton_clicked()
//{
//    switch (ui->pagesStack->currentIndex())
//    {
//    case 0: // Meaning
//        ui->meaningEdit->setText(QString());
//        break;
//    case 1: // Reading
//        ui->readingEdit->setText(QString());
//        ui->readingCBox->setCurrentIndex(0);
//        ui->okuriganaButton->setChecked(true);
//        filterKanji();
//        break;
//    case 2: // JLPT
//        ui->jlptEdit->setText(QString());
//        break;
//    case 3: // Jouyou
//        ui->jouyouCBox->setCurrentIndex(0);
//        filterKanji();
//        break;
//    case 4: // Parts
//        ui->partsCBox->setCurrentIndex(0);
//        filterKanji();
//        break;
//    case 5: // SKIP
//        ui->skip1Button->setChecked(false);
//        ui->skip2Button->setChecked(false);
//        ui->skip3Button->setChecked(false);
//        ui->skip4Button->setChecked(false);
//        ui->skip1Edit->setText(QString());
//        ui->skip2Edit->setText(QString());
//        filterKanji();
//        break;
//    case 6: // Index
//        ui->indexCBox->setCurrentIndex(0);
//        ui->indexEdit->setText(QString());
//        filterKanji();
//        break;
//    }
//}

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
    case 3: // Meaning
        ui->meaningWidget->setVisible(!ui->meaningWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 4: // Reading
        ui->readingWidget->setVisible(!ui->readingWidget->isVisibleTo(ui->optionsWidget));
        break;
    case 5: // JLPT
        ui->jlptWidget->setVisible(!ui->jlptWidget->isVisibleTo(ui->optionsWidget));
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
    ui->f2Button->setChecked(ui->meaningWidget->isVisibleTo(ui->optionsWidget));
    ui->f3Button->setChecked(ui->readingWidget->isVisibleTo(ui->optionsWidget));
    ui->f4Button->setChecked(ui->jlptWidget->isVisibleTo(ui->optionsWidget));
    ui->f5Button->setChecked(ui->jouyouWidget->isVisibleTo(ui->optionsWidget));
    ui->f6Button->setChecked(ui->radicalsWidget->isVisibleTo(ui->optionsWidget));
    ui->f7Button->setChecked(ui->indexWidget->isVisibleTo(ui->optionsWidget));
    ui->f8Button->setChecked(ui->skipWidget->isVisibleTo(ui->optionsWidget));

    CommandCategories categ = activeCategory();
    if (categ != CommandCategories::NoCateg)
    {
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::StrokeFilter, categ), ui->f1Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::MeaningFilter, categ), ui->f2Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::ReadingFilter, categ), ui->f3Button->isChecked());
        ((ZKanjiForm*)window())->checkCommand(makeCommand(Commands::JLPTFilter, categ), ui->f4Button->isChecked());
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

void KanjiSearchWidget::radicalsBoxChanged(int ix)
{
    if (ix == ui->radicalsCBox->count() - 1)
    {
        ui->radicalsCBox->setEnabled(false);
        radform = new RadicalForm(this);
        connect(radform, &RadicalForm::selectionChanged, this, &KanjiSearchWidget::radicalsChanged);
        connect(radform, &RadicalForm::groupingChanged, this, &KanjiSearchWidget::radicalGroupingChanged);
        connect(radform, &RadicalForm::resultIsOk, this, &KanjiSearchWidget::radicalsOk);
        connect(radform, &RadicalForm::destroyed, this, &KanjiSearchWidget::radicalsClosed);
        radform->setFromModel(ui->kanjiGrid->model());
        if (radindex != 0)
            radform->setFilter(radindex - 1);
        radform->show();
        filterKanji();
    }
    else if (radindex != ix)
    {
        radindex = ix;
        filterKanji();
    }
}

void KanjiSearchWidget::radicalsChanged()
{
    filterKanji();
}

void KanjiSearchWidget::radicalGroupingChanged(bool grouping)
{
    filterKanji();
}

void KanjiSearchWidget::radicalsOk()
{
    RadicalFilter f;
    radform->activeFilter(f);
    radindex = radicalFiltersModel().add(f) + 1;
    radform = nullptr;
    ui->radicalsCBox->setCurrentIndex(radindex);
}

void KanjiSearchWidget::radicalsClosed()
{
    if (radform != nullptr)
    {
        radform = nullptr;
        ui->radicalsCBox->setCurrentIndex(radindex);
    }
    filterKanji();
    ui->radicalsCBox->setEnabled(true);
}

void KanjiSearchWidget::radsAdded()
{
    RadicalFiltersModel &m = radicalFiltersModel();
    ui->radicalsCBox->insertItem(ui->radicalsCBox->count() - 1, m.filterText(m.size() - 1));
}

void KanjiSearchWidget::radsRemoved(int index)
{
    ++index;
    if (radindex == index)
        radindex = 0;
    else if (radindex > index)
        --radindex;
    ui->radicalsCBox->removeItem(index);
}

void KanjiSearchWidget::radsMoved(int index, bool up)
{
    ++index;
    QString str = ui->radicalsCBox->itemText(index);
    ui->radicalsCBox->setUpdatesEnabled(false);
    ui->radicalsCBox->setItemText(index, ui->radicalsCBox->itemText(index + (up ? -1 : 1)));
    ui->radicalsCBox->setItemText(index + (up ? -1 : 1), str);
    ui->radicalsCBox->setUpdatesEnabled(true);
    if (radindex == index)
        radindex += (up ? -1 : 1);
}

void KanjiSearchWidget::radsCleared()
{
    ui->radicalsCBox->setUpdatesEnabled(false);
    while (ui->radicalsCBox->count() != 2)
        ui->radicalsCBox->removeItem(1);
    ui->radicalsCBox->setUpdatesEnabled(true);
    radindex = 0;
}

//void KanjiSearchWidget::showPage(bool checked)
//{
//    if (!checked)
//        return;
//
//    if (sender() == ui->meaningButton)
//        ui->pagesStack->setCurrentIndex(0);
//    else if (sender() == ui->readingButton)
//        ui->pagesStack->setCurrentIndex(1);
//    else if (sender() == ui->jlptButton)
//        ui->pagesStack->setCurrentIndex(2);
//    else if (sender() == ui->jouyouButton)
//        ui->pagesStack->setCurrentIndex(3);
//    else if (sender() == ui->partsButton)
//        ui->pagesStack->setCurrentIndex(4);
//    else if (sender() == ui->skipButton)
//        ui->pagesStack->setCurrentIndex(5);
//    else if (sender() == ui->indexButton)
//        ui->pagesStack->setCurrentIndex(6);
//
//    ui->meaningButton->setChecked(sender() == ui->meaningButton);
//    ui->readingButton->setChecked(sender() == ui->readingButton);
//    ui->jlptButton->setChecked(sender() == ui->jlptButton);
//    ui->jouyouButton->setChecked(sender() == ui->jouyouButton);
//    ui->partsButton->setChecked(sender() == ui->partsButton);
//    ui->skipButton->setChecked(sender() == ui->skipButton);
//    ui->indexButton->setChecked(sender() == ui->indexButton);
//
//}


//-------------------------------------------------------------
