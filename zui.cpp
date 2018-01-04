/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QAction>
#include <QSignalMapper>
#include <QMessageBox>
#include <QToolButton>
#include <QLineEdit>
#include <QWindow>
#include <QtSvg>
#include <QStringBuilder>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QPainter>
#include <QPainterPath>

#include <cmath>
#include <memory>
#include <stack>
#include <map>

#include "zui.h"
#include "zkanjimain.h"
#include "words.h"
#include "zdictionarymodel.h"
#include "zkanjigridmodel.h"
#include "zgrouptreemodel.h"
#include "groups.h"
#include "romajizer.h"
#include "zradicalgrid.h"
#include "recognizerform.h"
#include "groupexportform.h"
#include "zkanjiform.h"
#include "kanji.h"
#include "generalsettings.h"
#include "zcombobox.h"
#include "zkanalineedit.h"
#include "studysettings.h"

//-------------------------------------------------------------

class ZKanaLineEdit;


// Model for the tree views showing the word groups.
//static std::map<Dictionary*, GroupTreeModel*> wordgroupmodels;
// Model for the tree views showing the kanji groups.
//static std::map<Dictionary*, GroupTreeModel*> kanjigroupmodels;

static MainKanjiListModel mainkanjimodel;
static RadicalFiltersModel radicalfiltersmodel;

static JapaneseValidator japanesevalidator(true, true);
static JapaneseValidator kanavalidator(false, false);

//void generateStore(Dictionary *d)
//{
//    GroupTreeModel *wordgroupmodel = new GroupTreeModel();
//    wordgroupmodel->setGroups(&d->wordGroups());
//    kanjigroupmodels[d] = wordgroupmodel;
//    GroupTreeModel *kanjigroupmodel = new GroupTreeModel();
//    kanjigroupmodel->setGroups(&d->kanjiGroups());
//    wordgroupmodels[d] = kanjigroupmodel;

//    std::stack<GroupCategoryBase*> stack;
//    stack.push(&d->wordGroups());

//    while (!stack.empty())
//    {
//        WordGroupCategory *cat = (WordGroupCategory*)stack.top();
//        stack.pop();
//        for (int ix = 0; ix != cat->categoryCount(); ++ix)
//            stack.push(cat->categories(ix));

//        for (int ix = 0; ix != cat->size(); ++ix)
//        {
//            DictionaryItemModel *model = new DictionaryItemModel();
//            model->setGroup(cat->items(ix));
//            dictmodels[cat->items(ix)] = model;
//        }
//    }

//    stack.push(&d->kanjiGroups());

//    while (!stack.empty())
//    {
//        KanjiGroupCategory *cat = (KanjiGroupCategory*)stack.top();
//        stack.pop();
//        for (int ix = 0; ix != cat->categoryCount(); ++ix)
//            stack.push(cat->categories(ix));

//        for (int ix = 0; ix != cat->size(); ++ix)
//        {
//            KanjiListModel *model = new KanjiListModel();
//            model->setGroup(cat->items(ix));
//            kanjimodels[cat->items(ix)] = model;
//        }
//    }
//}

//GroupTreeModel* wordGroupTreeModel(Dictionary *d)
//{
//    auto it = wordgroupmodels.find(d);
//    if (it == wordgroupmodels.end())
//    {
//        GroupTreeModel *wordgroupmodel = new GroupTreeModel();
//        wordgroupmodel->setGroups(&d->wordGroups());
//        wordgroupmodels[d] = wordgroupmodel;
//        return wordgroupmodel;
//    }
//    return it->second;
//}
//
//GroupTreeModel* kanjiGroupTreeModel(Dictionary *d)
//{
//    auto it = kanjigroupmodels.find(d);
//    if (it == kanjigroupmodels.end())
//    {
//        GroupTreeModel *kanjigroupmodel = new GroupTreeModel();
//        kanjigroupmodel->setGroups(&d->kanjiGroups());
//        kanjigroupmodels[d] = kanjigroupmodel;
//        return kanjigroupmodel;
//    }
//    return it->second;
//}

MainKanjiListModel& mainKanjiListModel()
{
    //if (mainkanjimodel.empty())
    //{
    //    std::vector<ushort> mainlist;
    //    mainlist.reserve(ZKanji::kanjicount);
    //    for (int ix = 0; ix != ZKanji::kanjicount; ++ix)
    //        mainlist.push_back(ix);
    //    mainkanjimodel.setList(std::move(mainlist));
    //}
    return mainkanjimodel;
}

RadicalFiltersModel& radicalFiltersModel()
{
    return radicalfiltersmodel;
}

JapaneseValidator& japaneseValidator()
{
    return japanesevalidator;
}

JapaneseValidator& kanaValidator()
{
    return kanavalidator;
}

void saveXMLRadicalFilters(QXmlStreamWriter &writer)
{
    for (int ix = 0, siz = radicalfiltersmodel.size(); ix != siz; ++ix)
    {
        const RadicalFilter &f = radicalfiltersmodel.filters(ix);
        writer.writeStartElement("Filter");
        writer.writeAttribute("mode", f.mode == RadicalFilterModes::Parts ? "parts" : f.mode == RadicalFilterModes::Radicals ? "rads" : "named");
        writer.writeAttribute("grouped", f.grouped ? "1" : "0");

        // Making sure there's an end element.
        if (f.groups.empty())
            writer.writeCharacters(QString());

        for (int iy = 0, siz2 = f.groups.size(); iy != siz2; ++iy)
        {
            writer.writeEmptyElement("Group");

            const std::vector<ushort> &vec = f.groups[iy];
            QString rads;
            if (!vec.empty())
            {
                rads = QString::number(vec[0]);
                for (int l = 1, siz = vec.size(); l != siz; ++l)
                    rads = rads % "," % QString::number(vec[l]);
            }
            writer.writeAttribute("rads", rads);

        }

        writer.writeEndElement(); /* Filter */
    }
}

void loadXMLRadicalFilters(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement())
    {
        if (reader.name() == "Filter")
        {
            QStringRef m = reader.attributes().value("mode");
            RadicalFilter filter;
            if (m == "parts")
                filter.mode = RadicalFilterModes::Parts;
            else if (m == "rads")
                filter.mode = RadicalFilterModes::Radicals;
            else if (m == "named")
                filter.mode = RadicalFilterModes::NamedRadicals;
            else
            {
                // The filter must have a valid mode.
                reader.skipCurrentElement();
                continue;
            }

            filter.grouped = reader.attributes().value("grouped") == "1";

            while (reader.readNextStartElement())
            {
                if (reader.name() != "Group" || !reader.attributes().hasAttribute("rads"))
                {
                    reader.skipCurrentElement();
                    continue;
                }
                auto numbers = reader.attributes().value("rads").split(",");
                bool ok = true;

                std::vector<ushort> grp;
                for (const QStringRef &num : numbers)
                {
                    int val = num.toInt(&ok);
                    if (!ok)
                        break;

                    if ((!grp.empty() && grp.back() >= val) ||
                        (filter.mode == RadicalFilterModes::Parts && (val < 0 || val > 252)) ||
                        (filter.mode == RadicalFilterModes::Radicals && (val < 1 || val > 214)) ||
                        (filter.mode == RadicalFilterModes::NamedRadicals && val >= ZKanji::radlist.size()))
                        ok = false;
                    else
                        grp.push_back(val);
                }

                if (ok)
                    filter.groups.push_back(std::move(grp));

                // Leaving "Group".
                reader.skipCurrentElement();
            }

            if (!filter.groups.empty())
                radicalfiltersmodel.add(filter);
        }
    }

}

void saveXMLWordFilters(QXmlStreamWriter &writer)
{
    ZKanji::wordfilters().saveXMLSettings(writer);
}

void loadXMLWordFilters(QXmlStreamReader &reader)
{
    ZKanji::wordfilters().loadXMLSettings(reader);
}

bool wordFiltersEmpty()
{
    return ZKanji::wordfilters().empty();
}

//QString kanjiFontName()
//{
//    return Settings::fonts.kanji;
//    //return QStringLiteral("EPSONThickTextbook");
//}
//
//QString kanaFontName()
//{
//    return Settings::fonts.kana;
//    //return QStringLiteral("MS Gothic");
//}
//
//QString meaningFontName()
//{
//    return Settings::fonts.definition;
//    //return QStringLiteral("Arial");
//}
//
//FontSettings::FontStyle meaningFontStyle()
//{
//    return Settings::fonts.defstyle;
//}
//
//QString smallFontName()
//{
//    return Settings::fonts.info;
//    //return QStringLiteral("Franklin Gothic Medium");
//}
//
//FontSettings::FontStyle smallFontStyle()
//{
//    return Settings::fonts.infostyle;
//}
//
//QString extrasFontName()
//{
//    return Settings::fonts.extra;
//    //return QStringLiteral("Franklin Gothic Medium");
//}
//
//FontSettings::FontStyle extrasFontStyle()
//{
//    return Settings::fonts.extrastyle;
//}

//QString printKanaFontName()
//{
//    return Settings::fonts.kana;
//    //return QStringLiteral("MS Gothic");
//}
//
//QString printMeaningFontName()
//{
//    return Settings::fonts.definition;
//    //return QStringLiteral("Arial");
//}
//
//QString printSmallFontName()
//{
//    return Settings::fonts.info;
//    //return QStringLiteral("Franklin Gothic Medium");
//}

void drawTextBaseline(QPainter *p, int x, int basey, bool hcenter, QRect clip, QString str)
{
    QFontMetrics fm(p->font());

    QPainterPath pp;
    if (p->hasClipping())
        pp = p->clipPath();

    if (!clip.isEmpty())
    {
        p->setClipRect(clip);
        if (hcenter)
            x = clip.left() + (clip.width() - fm.width(str)/*.width()*/) / 2;
    }
    p->drawText(x, basey, str);

    if (pp.isEmpty())
        p->setClipping(false);
    else
        p->setClipPath(pp);
}

void drawTextBaseline(QPainter *p, QPointF pos, bool hcenter, QRectF clip, QString str)
{
    QFontMetricsF fm(p->font());

    QPainterPath pp;
    if (p->hasClipping())
        pp = p->clipPath();

    if (!clip.isEmpty())
    {
        p->setClipRect(clip);
        if (hcenter)
            pos.setX(clip.left() + (clip.width() - fm.width(str)/*.width()*/) / 2);
    }
    p->drawText(pos, str);

    if (pp.isEmpty())
        p->setClipping(false);
    else
        p->setClipPath(pp);
}

void adjustFontSize(QFont &f, int height/*, QString str*/)
{
    f.setPixelSize(Settings::scaled(100));
    QFontMetrics fm(f);
    //QRect br = fm.boundingRect(str);
    double h = fm.overlinePos() + fm.underlinePos() /*br.height()*/;
    double dif = double(h) / height;
    f.setPixelSize(f.pixelSize() / dif);

    // Do a second measurement for more precision.
    fm = QFontMetrics(f);
    //br = fm.boundingRect(str);
    h = fm.overlinePos() + fm.underlinePos() /*br.height()*/;
    dif = double(h) / height;
    f.setPixelSize(f.pixelSize() / dif);
}

static int _sizeHelper(double charnum, int charw)
{
    return std::ceil(charw * charnum);
}

void restrictWidgetSize(QWidget *widget, double charnum, AdjustedValue val)
{
    int w = _sizeHelper(charnum, widget->fontMetrics().averageCharWidth());
    if (val == AdjustedValue::Min || val == AdjustedValue::MinMax)
        widget->setMinimumWidth(w);
    if (val == AdjustedValue::Max || val == AdjustedValue::MinMax)
        widget->setMaximumWidth(w);
}

int restrictedWidgetSize(QWidget *widget, double charnum)
{
    return _sizeHelper(charnum, widget->fontMetrics().averageCharWidth());
}

void restrictWidgetWiderSize(QWidget *widget, double charnum, AdjustedValue val)
{
    QFontMetrics fm = widget->fontMetrics();
    int w = _sizeHelper(charnum, mmax(fm.averageCharWidth(), fm.width('W'), fm.width('M'), fm.width('X')));
    if (val == AdjustedValue::Min || val == AdjustedValue::MinMax)
        widget->setMinimumWidth(w);
    if (val == AdjustedValue::Max || val == AdjustedValue::MinMax)
        widget->setMaximumWidth(w);
}

int restrictedWidgetWiderSize(QWidget *widget, double charnum)
{
    QFontMetrics fm = widget->fontMetrics();
    return _sizeHelper(charnum, mmax(fm.averageCharWidth(), fm.width('W'), fm.width('M'), fm.width('X')));
}

int fixedLabelWidth(QLabel *label)
{
    return label->fontMetrics().width(label->text());
}

void fixWrapLabelsHeight(QWidget *form, int labelwidth)
{
    QList<QWidget*> widgs = form->findChildren<QWidget*>();
    for (QWidget *w : widgs)
    {
        if (w->layout() != nullptr)
            //if (dynamic_cast<QFormLayout*>(w->layout()) != nullptr || dynamic_cast<QVBoxLayout*>(w->layout()) != nullptr)
            w->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    }

    form->adjustSize();

    QList<QLabel*> children = form->findChildren<QLabel*>();
    for (QLabel *ch : children)
    {
        if (!ch->wordWrap())
            continue;
        ch->setMinimumWidth(labelwidth <= 0 ? ch->width() : labelwidth);
        ch->setMaximumWidth(labelwidth <= 0 ? ch->width() : labelwidth);
        ch->setMinimumHeight(ch->height());
        //ch->setMaximumHeight(ch->height());
        QSizePolicy sp = ch->sizePolicy();
        sp.setHorizontalPolicy(QSizePolicy::Fixed);
        sp.setVerticalPolicy(QSizePolicy::Fixed);
        ch->setSizePolicy(sp);
    }
}

namespace {
    // Helpers for scaleWidget

    void _scaleLayout(QLayout *l);

    void _scaleWidget(QWidget *w)
    {
        int minw = w->minimumWidth();
        int maxw = w->maximumWidth();
        if (minw != 0)
            w->setMinimumWidth(Settings::scaled(minw));
        if (maxw != QWIDGETSIZE_MAX && maxw < Settings::scaled(maxw))
            w->setMaximumWidth(Settings::scaled(maxw));
        int minh = w->minimumHeight();
        int maxh = w->maximumHeight();
        if (minh != 0)
            w->setMinimumHeight(Settings::scaled(minh));
        if (maxh != QWIDGETSIZE_MAX && maxh < Settings::scaled(maxh))
            w->setMaximumHeight(Settings::scaled(maxh));

        if (w->font() != qApp->font())
        {
            QFont f = w->font();
            f.setPointSizeF(Settings::scaled(f.pointSizeF()));
            w->setFont(f);
        }

        //int l, t, r, b;
        //w->getContentsMargins(&l, &t, &r, &b);
        //w->setContentsMargins(Settings::scaled(l), Settings::scaled(t), Settings::scaled(r), Settings::scaled(b));

        if (dynamic_cast<QSplitter*>(w) != nullptr)
        {
            QSplitter *spl = (QSplitter*)w;
            spl->setHandleWidth(Settings::scaled(spl->handleWidth()));
        }

        QAbstractButton *btn = dynamic_cast<QAbstractButton*>(w);
        if (btn != nullptr)
        {
            QSize siz = btn->iconSize();
            btn->setIconSize(QSize(Settings::scaled(siz.width()), Settings::scaled(siz.height())));

        }

        // Some attributes can only be set recursively because they use what was inherited by
        // the parent, and would increase spacing/padding etc. too much in nested widgets. Use
        // direct-children-only to avoid this.

        QList<QWidget*> wlist = w->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (int ix = 0, siz = wlist.size(); ix != siz; ++ix)
            _scaleWidget(wlist.at(ix));

        _scaleLayout(w->layout());
    }

    void _scaleSpacerItem(QSpacerItem *s)
    {
        QSizePolicy sp = s->sizePolicy();
        QSize sh = s->sizeHint();
        s->changeSize(Settings::scaled(sh.width()), Settings::scaled(sh.height()), sp.horizontalPolicy(), sp.verticalPolicy());
    }

    void _scaleLayout(QLayout *l)
    {
        if (l == nullptr)
            return;

        int ll, t, r, b;
        l->getContentsMargins(&ll, &t, &r, &b);
        l->setContentsMargins(Settings::scaled(ll), Settings::scaled(t), Settings::scaled(r), Settings::scaled(b));

        for (int ix = 0, siz = l->count(); ix != siz; ++ix)
        {
            QLayoutItem *la = l->itemAt(ix);
            if (la->isEmpty())
                continue;
            QSpacerItem *si = la->spacerItem();
            if (si != nullptr)
                _scaleSpacerItem(si);
            QLayout *ll = la->layout();
            if (ll != nullptr)
                _scaleLayout(ll);
        }

        // Spacing can be inherited so we set it only after the rest is done.

        if (dynamic_cast<QBoxLayout*>(l) != nullptr)
            l->setSpacing(Settings::scaled(l->spacing()));
        else if (dynamic_cast<QFormLayout*>(l) != nullptr)
        {
            ((QFormLayout*)l)->setHorizontalSpacing(Settings::scaled(((QFormLayout*)l)->horizontalSpacing()));
            ((QFormLayout*)l)->setVerticalSpacing(Settings::scaled(((QFormLayout*)l)->verticalSpacing()));
        }
        else if (dynamic_cast<QGridLayout*>(l) != nullptr)
        {
            ((QGridLayout*)l)->setHorizontalSpacing(Settings::scaled(((QGridLayout*)l)->horizontalSpacing()));
            ((QGridLayout*)l)->setVerticalSpacing(Settings::scaled(((QGridLayout*)l)->verticalSpacing()));
        }
    }
}

void scaleWidget(QWidget *w)
{
    if (w == nullptr)
        return;

    _scaleWidget(w);

    // Recursion of child widgets is done in _scaleWidget()
}

void helper_createStatusWidget(QWidget *w, QLabel *lb1, QString lbstr1, double sizing1)
{
    if (lb1 == nullptr)
        lb1 = new QLabel(w);
    lb1->setText(lbstr1);
    QSizePolicy pol = lb1->sizePolicy();
    pol.setVerticalPolicy(QSizePolicy::Fixed);
    if (sizing1 > 0)
        restrictWidgetSize(lb1, sizing1);
    else
    {
        if (sizing1 == 0)
            pol.setHorizontalPolicy(QSizePolicy::Fixed);
        else
            pol.setHorizontalPolicy(QSizePolicy::Expanding);
        lb1->setSizePolicy(pol);
    }

    w->layout()->addWidget(lb1);
}

//QWidget* createStatusWidget(QWidget *parent, QLabel *lb1, QString lbstr1, int sizing1, QLabel *lb2, QString lbstr2, int sizing2, int spacing)
//{
//    QWidget *w = new QWidget(parent);
//    if (lb1 == nullptr)
//        lb1 = new QLabel(w);
//    if (lb2 == nullptr)
//        lb2 = new QLabel(w);
//    lb1->setText(lbstr1);
//    QSizePolicy pol = lb1->sizePolicy();
//    pol.setVerticalPolicy(QSizePolicy::Fixed);
//    if (sizing1 > 0)
//        restrictWidgetSize(lb1, sizing1);
//    else
//    {
//        if (sizing1 == 0)
//            pol.setHorizontalPolicy(QSizePolicy::Fixed);
//        else
//            pol.setHorizontalPolicy(QSizePolicy::Expanding);
//        lb1->setSizePolicy(pol);
//    }
//
//    lb2->setText(lbstr2);
//    lb2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//    pol = lb2->sizePolicy();
//    pol.setVerticalPolicy(QSizePolicy::Fixed);
//    if (sizing2 > 0)
//        restrictWidgetSize(lb2, sizing2);
//    else
//    {
//        if (sizing2 == 0)
//            pol.setHorizontalPolicy(QSizePolicy::Fixed);
//        else
//            pol.setHorizontalPolicy(QSizePolicy::Expanding);
//        lb2->setSizePolicy(pol);
//    }
//
//    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, w);
//    layout->addWidget(lb1);
//    layout->addWidget(lb2);
//    layout->setMargin(0);
//    if (spacing != -1)
//        layout->setSpacing(spacing);
//
//    return w;
//}


void installRecognizer(QToolButton *btn, ZKanaLineEdit *edit, RecognizerPosition pos)
{
    RecognizerForm::install(btn, edit, pos);
}

void uninstallRecognizer(QToolButton *btn, ZKanaLineEdit *edit)
{
    RecognizerForm::uninstall(btn);
}

void installRecognizer(QToolButton *btn, ZKanaComboBox *edit, RecognizerPosition pos)
{
    if (dynamic_cast<ZKanaLineEdit*>(edit->lineEdit()) == nullptr)
        throw "Invalid combo box. It must have a ZKanaLineEdit set as editor.";
    RecognizerForm::install(btn, (ZKanaLineEdit*)edit->lineEdit(), pos);
}

void uninstallRecognizer(QToolButton *btn, ZKanaComboBox *edit)
{
    RecognizerForm::uninstall(btn);
}

int showAndReturn(QString title, QString text, QString details, const std::vector<SARButton> &buttons, int defaultbtn)
{
    return showAndReturn(nullptr, title, text, details, buttons, defaultbtn);
}

int showAndReturn(QWidget *parent, QString title, QString text, QString details, const std::vector<SARButton> &buttons, int defaultbtn)
{
    // TODO: check whether this works in other OSs too. If not, clone to another version
    // which works without a qApp, because this function is used both when it exists and
    // when it doesn't.

    //QTimer timer;
    //timer.setSingleShot(true);
    //timer.connect(&timer, &QTimer::timeout, [&]() {
    QMessageBox msg(parent);
    msg.setWindowTitle(title);
    msg.setText(text);
    msg.setInformativeText(details);

    QPushButton *def = nullptr;
    for (const SARButton &btn : buttons)
    {
        if (defaultbtn == 0)
            def = msg.addButton(btn.text, btn.role);
        else
            msg.addButton(btn.text, btn.role);
        --defaultbtn;
    }
    if (def != nullptr)
        msg.setDefaultButton(def);
    msg.exec();

    int ix = -1;
    for (QAbstractButton *btn : msg.buttons())
    {
        if (btn == msg.clickedButton())
        {
            // Qt randomly shuffles the buttons depending on the OS, so we have to match the
            // exact button clicked with the buttons passed to this function.
            for (int iy = 0; iy != buttons.size() && ix == -1; ++iy)
                if (btn->text() == buttons[iy].text && msg.buttonRole(btn) == buttons[iy].role)
                    ix = iy;
            break;
        }
    }

    //    qApp->exit(ix);
    //});
    //timer.start(1000);

    //return qApp->exec();

    return ix;
}



QString formatDate(const QDateTime &d, QString unset)
{
    if (!d.isValid())
        return unset;
    return DateTimeFunctions::format(d, DateTimeFunctions::NormalDate);
}

QString formatDate(const QDate &d, QString unset)
{
    if (!d.isValid())
        return unset;
    return DateTimeFunctions::format(QDateTime(d), DateTimeFunctions::NormalDate);
}

QString formatSpacing(quint32 sec)
{
    return DateTimeFunctions::formatSpacing(sec);
}

QString fixFormatDate(const QDateTime &d, QString unset)
{
    if (!d.isValid())
        return unset;
    return DateTimeFunctions::format(d, DateTimeFunctions::DayFixedDate);
}

QString fixFormatPastDate(const QDateTime &d)
{
    return DateTimeFunctions::formatPast(d);
}

QString fixFormatNextDate(const QDateTime &d)
{
    return DateTimeFunctions::formatNext(d);
}

QDate ltDay(const QDateTime &d)
{
    return DateTimeFunctions::getLTDay(d);
}


//QImage* makeImageFromSvg(QImage* &img, QString svgpath, int width, int height)
//{
//    if (img != nullptr)
//        return img;
//
//    QSvgRenderer r;
//    r.load(svgpath);
//
//    img = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
//    img->fill(qRgba(0, 0, 0, 0));
//
//    QPainter p(img);
//    r.render(&p, QRect(0, 0, width, height));
//
//    return img;
//}

namespace {
    struct _SvgImgT
    {
        int w;
        int h;
        QImage *img;
    };

    std::map<QString, std::map<int, _SvgImgT>> _svgimagemap;
}


QImage imageFromSvg(QString svgpath, int width, int height, int version)
{
    if (version < 0)
        return QImage();

    // Finding the cached image in case it's stored.
    auto it = _svgimagemap.find(svgpath);
    std::map<int, _SvgImgT>::iterator mit;
    if (it != _svgimagemap.end())
    {
        mit = it->second.find(version);
        if (mit != it->second.end())
        {
            if (mit->second.w == width && mit->second.h == height)
                return *mit->second.img;
            delete mit->second.img;
            //it->second.erase(mit);
            //mit = it->second.end();
        }
    }

    QSvgRenderer r;
    r.load(svgpath);

    QImage *img = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    img->fill(qRgba(0, 0, 0, 0));

    QPainter p(img);
    r.render(&p, QRect(0, 0, width, height));

    if (it == _svgimagemap.end())
    {
        std::map<int, _SvgImgT> versionmap;
        _SvgImgT imgt;
        imgt.w = width;
        imgt.h = height;
        imgt.img = img;
        versionmap[version] = imgt;

        _svgimagemap.insert(std::make_pair(svgpath, std::move(versionmap)));
    }
    else
    {
        if (mit == it->second.end())
        {
            _SvgImgT imgt;
            imgt.w = width;
            imgt.h = height;
            imgt.img = img;
            it->second[version] = imgt;
        }
        else
        {
            mit->second.w = width;
            mit->second.h = height;
            mit->second.img = img;
        }
    }

    return *img;
}

//QImage renderFromSvg(QImage &dest, QString svgpath, QRect r)
//{
//    dest.fill(QColor(0, 0, 0, 0));
//
//    QSvgRenderer sr;
//    sr.load(svgpath);
//
//    QPainter p(&dest);
//    sr.render(&p, r);
//
//    p.end();
//    return dest;
//}

QPixmap renderFromSvg(QString svgpath, int w, int h, QRect r)
{
    QSvgRenderer sr;
    sr.load(svgpath);

    QPixmap result(w, h);
    result.fill(QColor(0, 0, 0, 0));

    QPainter p(&result);
    sr.render(&p, r);

    p.end();
    return result;
}

//void renderFromSvg(QPainter &dest, QString svgpath, QRect r)
//{
//    QSvgRenderer sr;
//    sr.load(svgpath);
//
//    sr.render(&dest, r);
//}

bool operator<(const std::pair<QSize, Flags> &a, const std::pair<QSize, Flags> &b)
{
    int aw = a.first.width();
    int bw = b.first.width();
    int ah = a.first.height();
    int bh = b.first.height();
    return  aw < bw || (aw == bw && (ah < bh || (ah == bh && (int)a.second < (int)b.second)));
}

namespace ZKanji
{
    static std::map<QString, std::map<std::pair<QSize, Flags>, QPixmap>> flagcache;
    QPixmap dictionaryFlag(QSize siz, QString dictname, Flags flag)
    {
        if (flag == Flags::Browse)
            dictname = QString();

        auto it = flagcache.find(dictname);
        std::map<std::pair<QSize, Flags>, QPixmap> *flagmap;
        if (it != flagcache.end())
        {
            flagmap = &it->second;
            auto it2 = flagmap->find({ siz, flag });
            if (it2 != flagmap->end())
                return it2->second;
        }
        else
            flagmap = &(flagcache[dictname] = std::map<std::pair<QSize, Flags>, QPixmap>());

        QSvgRenderer sr;

        QPixmap result(siz);
        result.fill(QColor(0, 0, 0, 0));
        QPainter p(&result);

        int s = std::min(siz.width(), siz.height());

        if (flag == Flags::Browse)
        {
            sr.load(QStringLiteral(":/flagjp.svg"));
            sr.render(&p, QRectF(0, s * 0.0625, s, s));
            p.end();

            flagmap->insert({{ siz, flag }, result });
            return result;
        }

        QString name = dictname.toLower();
        QString dictflag = name == QStringLiteral("english") ? QStringLiteral(":/flagen.svg") : QStringLiteral(":/flaggen.svg");

        if (flag == Flags::FromJapanese)
            sr.load(QStringLiteral(":/flagjp.svg"));
        else
            sr.load(dictflag);

        if (flag == Flags::Flag)
        {
            sr.render(&p, QRectF(0, s * 0.0625, s, s));
            p.end();

            flagmap->insert({ { siz, flag }, result });
            return result;
        }

        // First paint the top flag.
        sr.render(&p, QRectF(0, 0, s * 0.73, s * 0.73));

        if (flag == Flags::FromJapanese)
            sr.load(dictflag);
        else
            sr.load(QStringLiteral(":/flagjp.svg"));

        // Paint the bottom flag.
        sr.render(&p, QRectF(s * 0.28, s * 0.5, s * 0.73, s * 0.73));

        p.end();

        flagmap->insert({ { siz, flag }, result });
        return result;
    }

    QPixmap dictionaryMenuFlag(const QString &dictname)
    {
        int siz = qApp->style()->pixelMetric(QStyle::PM_SmallIconSize);
        return dictionaryFlag(QSize(siz, siz), dictname, Flags::Flag);
    }

    void eraseDictionaryFlag(const QString &dictname)
    {
        auto it = flagcache.find(dictname);
        if (it != flagcache.end())
            flagcache.erase(it);
    }

}

QColor mixColors(const QColor &a, const QColor &b, double a_part)
{
    if (a_part < 0)
        a_part = 0;
    else if (a_part > 1)
        a_part = 1;
    return QColor(std::min(255, int(a.red() * a_part + b.red() * (1 - a_part))), std::min(255, int(a.green() * a_part + b.green() * (1 - a_part))), std::min(255, int(a.blue() * a_part + b.blue() * (1 - a_part))));
}

int colorComponentFromBase(int base, int curr, int col)
{
    if (curr == base)
        return col;

    return std::min<int>(255, std::max<int>(0, col + col * (double(curr - base) / 255)));

    /*
    int lighter = 255 - base;
    int darker = 255 - lighter;

    int currpart;
    if (curr > base)
        currpart = (curr - base) * 100 / lighter;
    else
        currpart = darker == 0 ? 0 : (base - curr) * 100 / darker;

    int colpart;
    if (col > base)
        colpart = (col - base) * 100 / lighter;
    else
        colpart = darker == 0 ? 0 : (base - col) * 100 / darker;

    int csum = currpart + colpart;

    return csum == 0 ? base : std::min<int>(255, std::max<int>(0, (curr * (currpart * 100 / csum) + col * (colpart * 100 / csum))) / 100);
    */
}

QColor colorFromBase(const QColor &base, const QColor &curr, const QColor &col)
{
    return qRgb(colorComponentFromBase(base.red(), curr.red(), col.red()),
        colorComponentFromBase(base.green(), curr.green(), col.green()),
        colorComponentFromBase(base.blue(), curr.blue(), col.blue()));
}


void deleteSvgImageCache(QString svgpath, int version)
{
    auto it = _svgimagemap.find(svgpath);
    if (it == _svgimagemap.end())
        return;

    if (version != -1)
    {
        auto mit = it->second.find(version);
        if (mit == it->second.end())
            return;

        delete mit->second.img;

        it->second.erase(mit);
        return;
    }

    for (auto &m : it->second)
        delete m.second.img;

    _svgimagemap.erase(it);

}


//-------------------------------------------------------------


void initCheckBoxOption(const QWidget *widget, CheckBoxMouseState mstate, bool enabled, Qt::CheckState chkstate, QStyleOptionButton *opt)
{
    opt->state = 0;

    // We assume the parent is a ZListView.
    opt->initFrom(widget);

    opt->state &= ~(QStyle::State_MouseOver | QStyle::State_Off | QStyle::State_On | QStyle::State_NoChange | QStyle::State_Sunken);

    opt->rect = QRect();

    if (!enabled)
        opt->state &= ~QStyle::State_Enabled;

    if (/*mstate == ZListView::Down ||*/ mstate == CheckBoxMouseState::DownHover)
        opt->state |= QStyle::State_Sunken;

    if (chkstate == Qt::PartiallyChecked)
        opt->state |= QStyle::State_NoChange;
    else if (chkstate == Qt::Checked)
        opt->state |= QStyle::State_On;
    else
        opt->state |= QStyle::State_Off;

    if (mstate == CheckBoxMouseState::Hover || mstate == CheckBoxMouseState::DownHover)
        opt->state |= QStyle::State_MouseOver;
    //else
    //    opt->state &= ~QStyle::State_MouseOver;

    opt->text = QString();
    opt->icon = QIcon();
    opt->iconSize = QSize();

    opt->styleObject = nullptr;
}

QSize checkBoxSize()
{
    QStyleOption option;
    return qApp->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, nullptr).size();
}

QRect checkBoxRect(const QWidget *widget, int leftpadding, QRect rect, QStyleOptionButton *opt)
{
    // TODO: On windows the subElementRect for a checkbox is on the left side and centered vertically. We measure it
    // here for an empty rect, so the result will be vertically centered on (0, 0). Test on linux.
    QRect r = widget->style()->subElementRect(QStyle::SE_CheckBoxIndicator, opt, /*owner()*/ nullptr);

    // Originally used:
    // owner()->style()->subElementRect(QStyle::SE_CheckBoxClickRect, &opb, /*owner()*/ nullptr)
    // This gives back a smaller size than what's really drawn.

    QRect r2 = rect;

    return QRect(r2.left() + leftpadding + r.left(), r2.top() + r2.height() / 2 + r.top() - 1, r.width(), r.height());

    //QStyleOptionButton opb;
    //initCheckBoxOption(index, &opb);
    //QSize siz = checkBoxRectHelper(&opb);
    //QRect r = owner()->visualRect(index);

    //return QRect(r.left() + 4, r.top() + (r.height() - siz.height()) / 2, siz.width(), siz.height());
}

QRect checkBoxHitRect(const QWidget *widget, int leftpadding, QRect rect, QStyleOptionButton *opt)
{
    // TODO: On windows the subElementRect for a checkbox is on the left side and centered vertically. We measure it
    // here for an empty rect, so the result will be vertically centered on (0, 0). Test on linux.
    QRect r = widget->style()->subElementRect(QStyle::SE_CheckBoxClickRect, opt, /*owner()*/ nullptr);

    //QSize siz = checkBoxRectHelper(&opb);
    QRect r2 = rect;

    return QRect(r2.left() + leftpadding + r.left(), r2.top() + r2.height() / 2 + r.top() - 1, r.width(), r.height());
}

void drawCheckBox(QPainter *painter, const QWidget *widget, int leftpadding, QRect rect, QStyleOptionButton *opt)
{
    //QSize siz = owner()->style()->sizeFromContents(QStyle::CT_CheckBox, &opbtn, QSize(), owner()); //checkBoxRectHelper(&opbtn);
    //QRect r = owner()->visualRect(index);
    //r = opbtn.rect = QRect(r.left() + 4, r.top() + (r.height() - siz.height()) / 2, siz.width(), siz.height());


    QRect r = widget->style()->subElementRect(QStyle::SE_CheckBoxIndicator, opt, /*owner()*/ nullptr);

    // Originally used:
    // owner()->style()->subElementRect(QStyle::SE_CheckBoxClickRect, &opb, /*owner()*/ nullptr)
    // This gives back a smaller size than what's really drawn.

    QRect r2 = rect;

    opt->rect = QRect(r2.left() + 4 + r.left(), r2.top() + r2.height() / 2 + r.top() - 1, r.width(), r.height());

    //QRect r = opbtn.rect = checkBoxRect(index);

    widget->style()->drawControl(QStyle::CE_CheckBox, opt, painter);
}


//-------------------------------------------------------------


bool operator==(QKeyEvent *e, const QKeySequence &seq)
{
    if (seq.count() != 1)
        return false;

    int sh = seq[0];

    int k = e->key();
    int m = ((int)e->modifiers() & (int)Qt::KeyboardModifierMask);
    if (k == Qt::Key_Control || k == Qt::Key_Shift || k == Qt::Key_Alt || k == Qt::Key_Meta)
    {
        switch (k)
        {
        case Qt::Key_Control:
            m |= Qt::ControlModifier;
            break;
        case Qt::Key_Alt:
            m |= Qt::AltModifier;
            break;
        case Qt::Key_Shift:
            m |= Qt::ShiftModifier;
            break;
        case Qt::Key_Meta:
            m |= Qt::MetaModifier;
            break;
        }
        return sh == m;
    }

    return sh == (k | m);
}

bool operator==(const QKeySequence &seq, QKeyEvent *e)
{
    return e == seq;
}


//-------------------------------------------------------------


JapaneseValidator::JapaneseValidator(bool acceptkanji, bool acceptsymbols, QObject *parent) : base(parent), acceptkanji(acceptkanji), acceptsymbols(acceptsymbols)
{

}

void JapaneseValidator::fixup(QString &input) const
{
    for (int ix = input.size() - 1; ix != -1; --ix)
    {
        if (!validChar(input.at(ix).unicode()))
        {
            if (input.at(ix) == QChar('n'))
                input[ix] = QChar(0x3093); /* hiragana n*/
            else if (input.at(ix) == QChar('N'))
                input[ix] = QChar(0x30F3); /* katakana n*/
            else
                input.remove(ix, 1);
        }
    }
}

JapaneseValidator::State JapaneseValidator::validate(QString &input, int &pos) const
{
    pos = std::min(pos, input.size());

    // Remove invalid characters from the input.
    for (int ix = input.size() - 1; ix != -1; --ix)
    {
        ushort ch = input.at(ix).unicode();
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '-' || ch == '/' || (ch == '\'' && ix != 0 && input.at(ix - 1).toLower().unicode() == 'n')) && !validChar(input.at(ix).unicode()))
        {
            input.remove(ix, 1);
            if (pos > ix)
                --pos;
        }
    }

    // Replace any non-Japanese string found in input, that matches one item
    // of kanainput, with the same indexed item from kanaoutput, up to the
    // position of pos.

    // Starting position in kanainput/kanaoutput for items that can be
    // converted to the syllable at the current input position.
    int kanapos = 0;

    // Number of items matching the latin character at the current position
    // in kanainput/kanaoutput.
    int kanacnt = 0;

    // Katakana input, when any upper case character is found in the romaji
    // string.
    bool kata = false;

    // Index of the current candidate in kanainput.
    int cindex = 0;

    // Character position in the current candidate in kanainput (indexed by
    // cindex).
    int cpos = 0;

    // Length of the current candidate in kanainput. Only set when the
    // candidate's characters up to cpos are valid.
    int clen = -1;

    // Current position in input.
    int inputpos = 0;

    while (inputpos != pos)
    {
        // Skips Japanese input.
        while (inputpos != pos && validChar(input.at(inputpos).unicode()))
            ++inputpos;

        if (inputpos == pos)
            break;

        // The length and position of the candidate can only be the same if a
        // match was found.
        if (cpos == clen)
        {
            // Add a small tsu character if the first consonant is doubled.
            if (inputpos != 0 && input.at(inputpos - 1).toLower().toLatin1() == kanainput[cindex][0] &&
                kanainput[cindex][0] != 'a' && kanainput[cindex][0] != 'i' && kanainput[cindex][0] != 'u' &&
                kanainput[cindex][0] != 'e' && kanainput[cindex][0] != 'o' && kanainput[cindex][0] != 'x' && kanainput[cindex][0] != 'l')
            {
                input.insert(inputpos, !kata ? QChar(MINITSU) : QChar(MINITSUKATA));
                ++inputpos;
                ++pos;
            }


            QString kana = QString::fromUtf16(kanaoutput[cindex]);
            if (kata)
                kana = toKatakana(kana);

            input.replace(inputpos, clen, kana);
            pos -= clen;
            pos += kana.size();
            inputpos += kana.size();

            kata = false;
            cindex = 0;
            cpos = 0;
            clen = -1;
            continue;
        }

        QChar c;

        if (input.at(inputpos + cpos).isUpper())
            kata = true;
        c = input.at(inputpos + cpos).toLower();

        // Looking for a new candidate that starts with the character c.
        if (cpos == 0)
        {
            findKanaArrays(c, kanapos, kanacnt);
            cindex = kanapos;
            kanacnt += kanapos;
            if (kanacnt != 0)
            {
                cpos = 1;
                clen = strlen(kanainput[cindex]);

                if (clen == cpos)
                    continue;

                if (inputpos + cpos >= pos)
                    c = 0;
                else
                    c = input.at(inputpos + cpos).toLower();
            }
            else
                clen = -1;
        }

        if (clen != -1 && kanainput[cindex][cpos] == c.unicode())
        {
            ++cpos;
            continue;
        }
        else if (cpos != 0)
        {
            ++cindex;
            clen = -1;
        }

        for (; cindex != kanacnt; ++cindex)
        {
            clen = strlen(kanainput[cindex]);
            if (clen > pos - inputpos)
                continue;

            int j = 1;
            for (; j != cpos + 1; ++j)
            {
                if (kanainput[cindex][j] != input.at(inputpos + j).toLower().toLatin1())
                    break;
            }
            if (j == cpos + 1)
            {
                ++cpos;
                break;
            }
        }

        if (cindex == kanacnt)
        {
            //kata = false;
            ++inputpos;
            cindex = 0;
            cpos = 0;
            clen = -1;

            kanapos = 0;
            kanacnt = 0;
        }
    }

    // Whether the returned string is acceptable or intermediate.
    bool good = true;

    // Remove any leftover consecutive romaji which doesn't have pos right
    // after its end index. Handle the n character as a special case, and
    // replace it with the correct kana.

    // Fix the string after pos.
    for (int ix = pos; ix != input.size(); ++ix)
    {
        if (!validChar(input.at(ix).unicode()))
        {
            if (input.at(ix) == QChar('n'))
                input[ix] = QChar(0x3093); /* hiragana n*/
            else if (input.at(ix) == QChar('N'))
                input[ix] = QChar(0x30F3); /* katakana n*/
            else
                input.remove(ix--, 1);
        }
    }

    // Nothing is removed right before the cursor position. This is only set
    // when the first valid character was found and anything else can now be
    // removed.
    bool remove = false;

    // Fix the string before pos.
    for (int ix = pos - 1; ix != -1; --ix)
    {
        if (validChar(input.at(ix).unicode()))
            remove = true;
        else if (ix != pos - 1 && input.at(ix) == QChar('n') && input.at(ix + 1).toLower() != QChar('y'))
        {
            input[ix] = QChar(0x3093); /* hiragana n*/
            remove = true;
        }
        else if (ix != pos - 1 && input.at(ix) == QChar('N') && input.at(ix + 1).toLower() != QChar('y'))
        {
            input[ix] = QChar(0x30F3); /* katakana n*/
            remove = true;
        }
        else if (remove)
        {
            --pos;
            input.remove(ix, 1);
        }
        else // Still in the romaji string right before pos. Don't remove but mark input as intermediate.
            good = false;
    }

    return good ? Acceptable : Intermediate;
}

bool JapaneseValidator::validChar(ushort ch) const
{
    return (acceptkanji && acceptsymbols && JAPAN(ch)) ||
        (!acceptkanji && acceptsymbols && UNICODE_J(ch)) ||
        (!acceptkanji && !acceptsymbols && (VALIDKANA(ch) || MIDDOT(ch))) ||
        (acceptkanji && !acceptsymbols && KANJI(ch));

}


//-------------------------------------------------------------


IntRangeValidator::IntRangeValidator(QObject *parent) : base(parent)
{

}

IntRangeValidator::IntRangeValidator(quint32 minimum, quint32 maximum, bool allowempty, QObject *parent) : base(parent), lo(minimum), hi(maximum), allowempty(allowempty)
{

}

void IntRangeValidator::fixup(QString &input) const
{
    int tmp = -1;
    if (validate(input, tmp) != Acceptable)
        input = lastgood;
}

IntRangeValidator::State IntRangeValidator::validate(QString &input, int &pos) const
{
    if (input.isEmpty())
    {
        if (allowempty)
            lastgood = QString();
        return allowempty ? Acceptable : Intermediate;
    }

    int p = input.indexOf('-');
    if (p != -1 && input.lastIndexOf('-') != p)
        return Invalid;

    bool ok;
    int val;

    if (p == -1)
    {
        val = locale().toInt(input, &ok);
        if (!ok || val > hi || val < 0)
            return Invalid;

        if (val >= lo)
            lastgood = input;
        return (val < lo) ? Intermediate : Acceptable;
    }

    int val2;

    if (p > 0)
    {
        val = locale().toInt(input.left(p), &ok);
        if (!ok || val > hi || val < 0)
            return Invalid;
    }
    else if (input.size() == 1)
        return Intermediate;

    if (p < input.size() - 1)
    {
        val2 = locale().toInt(input.mid(p + 1), &ok);
        if (p == 0)
            val = val2;

        if (!ok || val2 > hi || val2 < 0)
            return Invalid;
    }
    else
        val2 = val;

    if (val2 < val || val2 < lo)
        return Intermediate;

    lastgood = input;
    return Acceptable;
}


//-------------------------------------------------------------


DateTimeFunctions::DateTimeFunctions(QObject *parent) : base(parent)
{

}

DateTimeFunctions::~DateTimeFunctions()
{

}

QString DateTimeFunctions::formatString()
{
    switch (Settings::general.dateformat)
    {
    case GeneralSettings::DayMonthYear:
        return tr("dd.MM.yyyy");
    case GeneralSettings::MonthDayYear:
        return tr("MM.dd.yyyy");
    case GeneralSettings::YearMonthDay:
    default:
        return tr("yyyy.MM.dd");
    }
}

QString DateTimeFunctions::format(QDateTime dt, FormatTypes type, bool utc)
{
    if (utc)
        dt = dt.toUTC();
    else
        dt = dt.toLocalTime();

    switch (type)
    {
    case DayFixedDate:
        dt = QDateTime(getLTDay(dt));
    case NormalDate:
        dt.toString(formatString());
    }

    return QString();
}

QString DateTimeFunctions::formatDay(QDate dt)
{
    return dt.toString(formatString());
}

QString DateTimeFunctions::formatPast(QDateTime dt)
{
    if (!dt.isValid())
        return tr("Never");

    if (getLTDay(QDateTime::currentDateTimeUtc()) == getLTDay(dt))
        return tr("Today");
    return format(dt, DayFixedDate);
}

QString DateTimeFunctions::formatNext(QDateTime dt)
{
    if (!dt.isValid())
        return tr("Unknown");

    if (getLTDay(QDateTime::currentDateTimeUtc()) >= getLTDay(dt))
        return tr("Due");
    return format(dt, DayFixedDate);
}

QString DateTimeFunctions::formatPastDay(QDate dt)
{
    if (!dt.isValid())
        return tr("Never");

    int d = dt.daysTo(QDateTime::currentDateTime().date());
    if (d == 0)
        return tr("Today");
    if (d == 1)
        return tr("Yesterday");
    return formatDay(dt);
}

QString DateTimeFunctions::formatLength(int sec)
{
    int sc = sec % 60;
    sec = (sec - sc) / 60;
    int min = sec % 60;
    sec = (sec - min) / 60;
    int hour = sec % 24;
    sec = (sec - hour) / 24;
    int month = (int)(sec / 30.417);
    int day = (int)std::fmod((sec - (int)(month * 30.417)), 30.417);
    month %= 12;
    sec = (int)std::round((sec - day) / 30.417);
    sec = (sec - month) / 12;
    int year = sec % 10;
    sec = (sec - year) / 10;
    int decade = sec;

    QString r = (decade == 0 ? QString() : QString(tr("%1d", "decade") % " ").arg(decade)) % (year == 0 ? QString() : QString(tr("%1y", "year") % " ").arg(year))
        % (month == 0 ? QString() : QString(tr("%1m", "month") % " ").arg(month)) % (day == 0 ? QString() : QString(tr("%1d", "day") % " ").arg(day))
        % (hour == 0 ? QString() : QString(tr("%1h", "hour") % " ").arg(hour)) % (min == 0 ? QString() : QString(tr("%1m", "minute") % " ").arg(min))
        % (sc == 0 ? QString() : QString(tr("%1s", "second") % " ").arg(sc));

    r = r.trimmed();
    if (r.isEmpty())
        return "-";
    return r;
}

QString DateTimeFunctions::formatSpacing(quint32 sec) 
{
    if (sec < 60)
        return tr("%1 seconds").arg(double(sec), 0, 'f', 1);
    if (sec < 60 * 60)
        return tr("%1 minutes").arg(double(sec) / (60.), 0, 'f', 1);
    if (sec < 60 * 60 * 24)
        return tr("%1 hours").arg(double(sec) / (60. * 60), 0, 'f', 1);
    if (sec < 60 * 60 * 24 * 30.5)
        return tr("%1 days").arg(double(sec) / (60. * 60 * 24), 0, 'f', 1);
    if (sec < 60 * 60 * 24 * 365)
        return tr("%1 months").arg(double(sec) / (60. * 60 * 24 * 30.5), 0, 'f', 1);
    if (sec < 60 * 60 * 24 * 365 * 10)
        return tr("%1 years").arg(double(sec) / (60. * 60 * 24 * 365), 0, 'f', 1);
    return tr("%1 decades").arg(double(sec) / (60. * 60 * 24 * 365 * 10), 0, 'f', 1);
}

QDate DateTimeFunctions::getLTDay(const QDateTime &dt)
{
    // Currently: subtracts X hours from the date time. Its QDate part will represent a test
    // day starting at Settings::study.starthour.
    return dt.addMSecs(-1000 * 60 * 60 * Settings::study.starthour).toLocalTime().date();
}

QString DateTimeFunctions::formatPassedTime(int seconds, bool showhours)
{
    int h = (seconds / 60 / 60);
    int m = (seconds / 60) % 60;
    int s = seconds % 60;

    return showhours ? tr("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')) : tr("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}


//-------------------------------------------------------------


