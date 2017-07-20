/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QScrollBar>
#include <QPainter>
#include <QStylePainter>
#include <QtEvents>
#include <QAbstractItemView>
#include "zkanjimain.h"
#include "zradicalgrid.h"
#include "kanji.h"
#include "zui.h"
#include "zkanjigridmodel.h"
#include "colorsettings.h"
#include "fontsettings.h"
#include "kanjisettings.h"
#include "globalui.h"

//-------------------------------------------------------------


// Same as the radsymbols in namespace ZKanji, with the added stroke count.
QChar ZRadicalGrid::radsymbols[231] = {
    1,
    0x4e00, 0x4e28, 0x4e36, 0x4e3f, 0x4e59, 0x4e85,
    2,
    0x4e8c, 0x4ea0, 0x4eba, 0x513f, 0x5165, 0x516b, 0x5182, 0x5196,
    0x51ab, 0x51e0, 0x51f5, 0x5200, 0x529B, 0x52F9, 0x5315, 0x531A,
    0x5338, 0x5341, 0x535C, 0x5369, 0x5382, 0x53B6, 0x53C8,
    3,
    0x53E3, 0x56D7, 0x571F, 0x58EB, 0x5902, 0x590A, 0x5915, 0x5927,
    0x5973, 0x5B50, 0x5B80, 0x5BF8, 0x5C0F, 0x5C22, 0x5C38, 0x5C6E,
    0x5C71, 0x5DDB, 0x5DE5, 0x5DF1, 0x5DFE, 0x5E72, 0x5E7A, 0x5E7F,
    0x5EF4, 0x5EFE, 0x5F0B, 0x5F13, 0x5F50, 0x5F61, 0x5F73,
    4,
    0x5FC3, 0x6208, 0x6236, 0x624B, 0x652F, 0x6534, 0x6587, 0x6597,
    0x65A4, 0x65B9, 0x65E0, 0x65E5, 0x66F0, 0x6708, 0x6728, 0x6B20,
    0x6B62, 0x6B79, 0x6BB3, 0x6BCB, 0x6BD4, 0x6BDB, 0x6C0F, 0x6C14,
    0x6C34, 0x706B, 0x722A, 0x7236, 0x723B, 0x723F, 0x7247, 0x7259,
    0x725B, 0x72AC,
    5,
    0x7384, 0x7389, 0x74DC, 0x74E6, 0x7518, 0x751F, 0x7528, 0x7530,
    0x758B, 0x7592, 0x7676, 0x767D, 0x76AE, 0x76BF, 0x76EE, 0x77DB,
    0x77E2, 0x77F3, 0x793A, 0x79B8, 0x79BE, 0x7A74, 0x7ACB,
    6,
    0x7AF9, 0x7C73, 0x7CF8, 0x7F36, 0x7F51, 0x7F8A, 0x7FBD, 0x8001,
    0x800C, 0x8012, 0x8033, 0x807F, 0x8089, 0x81E3, 0x81EA, 0x81F3,
    0x81FC, 0x820C, 0x821B, 0x821F, 0x826E, 0x8272, 0x8278, 0x864D,
    0x866B, 0x8840, 0x884C, 0x8863, 0x897F,
    7,
    0x898B, 0x89D2, 0x8A00, 0x8C37, 0x8C46, 0x8C55, 0x8C78, 0x8C9D,
    0x8D64, 0x8D70, 0x8DB3, 0x8EAB, 0x8ECA, 0x8F9B, 0x8FB0, 0x8FB5,
    0x9091, 0x9149, 0x91C6, 0x91CC,
    8,
    0x91D1, 0x9577, 0x9580, 0x961C, 0x96B6, 0x96B9, 0x96E8, 0x9751,
    0x975E,
    9,
    0x9762, 0x9769, 0x97CB, 0x97ED, 0x97F3, 0x9801, 0x98A8, 0x98DB,
    0x98DF, 0x9996, 0x9999,
    10,
    0x99AC, 0x9AA8, 0x9AD8, 0x9ADF, 0x9B25, 0x9B2F, 0x9B32, 0x9B3C,
    11,
    0x9B5A, 0x9CE5, 0x9E75, 0x9E7F, 0x9EA5, 0x9EBB,
    12,
    0x9EC4, 0x9ECD, 0x9ED1, 0x9EF9,
    13,
    0x9EFD, 0x9F0E, 0x9F13, 0x9F20,
    14,
    0x9F3B, 0x9F4A,
    15,
    0x9F52,
    16,
    0x9F8D,
    17,
    0x9F9C, 0x9FA0
};
ushort ZRadicalGrid::radindexes[231];

//QChar ZRadicalGrid::partsymbols[267] = {
//    1,
//    0x4e00, 0xff5c, 0x4e36, 0x30ce, 0x4e59, 0x4e85,
//    2,
//    0x4e8c, 0x4ea0, 0x4eba, 0x4ebb, 0x4e2a, 0x513f, 0x5165, 0x30cf,
//    0x4e37, 0x5182, 0x5196, 0x51ab, 0x51e0, 0x51f5, 0x5200, 0x5202,
//    0x529b, 0x52f9, 0x5315, 0x531a, 0x5341, 0x535c, 0x5369, 0x5382,
//    0x53b6, 0x53c8, 0x30de, 0x4e5d, 0x30e6, 0x4e43,
//    3,
//    0x8fb6, 0x53e3, 0x56d7, 0x571f, 0x58eb, 0x5902, 0x5915, 0x5927,
//    0x5973, 0x5b50, 0x5b80, 0x5bf8, 0x5c0f, 0x5c1a, 0x5c22, 0x5c38,
//    0x5c6e, 0x5c71, 0x5ddd, 0x5ddb, 0x5de5, 0x5df2, 0x5dfe, 0x5e72,
//    0x5e7a, 0x5e7f, 0x5ef4, 0x5efe, 0x5f0b, 0x5f13, 0x30e8, 0x5f51,
//    0x5f61, 0x5f73, 0x5fc4, 0x624c, 0x6c35, 0x72ad, 0x8279, 0x9092,
//    0x961d, 0x4e5f, 0x4ea1, 0x53ca, 0x4e45,
//    4,
//    0x8002, 0x5fc3, 0x6208, 0x6238, 0x624b, 0x652f, 0x6535, 0x6587,
//    0x6597, 0x65a4, 0x65b9, 0x65e0, 0x65e5, 0x66f0, 0x6708, 0x6728,
//    0x6b20, 0x6b62, 0x6b79, 0x6bb3, 0x6bd4, 0x6bdb, 0x6c0f, 0x6c14,
//    0x6c34, 0x706b, 0x706c, 0x722a, 0x7236, 0x723b, 0x723f, 0x7247,
//    0x725b, 0x72ac, 0x793b, 0x738b, 0x5143, 0x4e95, 0x52ff, 0x5c24,
//    0x4e94, 0x5c6f, 0x5df4, 0x6bcb,
//    5,
//    0x7384, 0x74e6, 0x7518, 0x751f, 0x7528, 0x7530, 0x758b, 0x7592,
//    0x7676, 0x767d, 0x76ae, 0x76bf, 0x76ee, 0x77db, 0x77e2, 0x77f3,
//    0x793a, 0x79b9, 0x79be, 0x7a74, 0x7acb, 0x8864, 0x4e16, 0x5de8,
//    0x518a, 0x6bcd, 0x7f52, 0x7259,
//    6,
//    0x74dc, 0x7af9, 0x7c73, 0x7cf8, 0x7f36, 0x7f8a, 0x7fbd, 0x800c,
//    0x8012, 0x8033, 0x807f, 0x8089, 0x81ea, 0x81f3, 0x81fc, 0x820c,
//    0x821f, 0x826e, 0x8272, 0x864d, 0x866b, 0x8840, 0x884c, 0x8863,
//    0x897f,
//    7,
//    0x81e3, 0x898b, 0x89d2, 0x8a00, 0x8c37, 0x8c46, 0x8c55, 0x8c78,
//    0x8c9d, 0x8d64, 0x8d70, 0x8db3, 0x8eab, 0x8eca, 0x8f9b, 0x8fb0,
//    0x9149, 0x91c6, 0x91cc, 0x821b, 0x9ea6,
//    8,
//    0x91d1, 0x9577, 0x9580, 0x96b6, 0x96b9, 0x96e8, 0x9752, 0x975e,
//    0x5944, 0x5ca1, 0x514d, 0x6589,
//    9,
//    0x9762, 0x9769, 0x97ed, 0x97f3, 0x9801, 0x98a8, 0x98db, 0x98df,
//    0x9996, 0x9999, 0x54c1,
//    10,
//    0x99ac, 0x9aa8, 0x9ad8, 0x9adf, 0x9b25, 0x9b2f, 0x9b32, 0x9b3c,
//    0x7adc, 0x97cb,
//    11,
//    0x9b5a, 0x9ce5, 0x9e75, 0x9e7f, 0x9ebb, 0x4e80, 0x6ef4, 0x9ec4,
//    0x9ed2,
//    12,
//    0x9ecd, 0x9ef9, 0x7121, 0x6b6f,
//    13,
//    0x9efd, 0x9f0e, 0x9f13, 0x9f20,
//    14,
//    0x9f3b, 0x9f4a,
//    17,
//    0x9fa0
//};
//ushort ZRadicalGrid::partindexes[267];


//-------------------------------------------------------------


bool operator!=(const RadicalFilter &a, const RadicalFilter &b)
{
    return a.mode != b.mode || a.grouped != b.grouped || a.groups != b.groups;
}

bool operator==(const RadicalFilter &a, const RadicalFilter &b)
{
    return a.mode == b.mode && a.grouped == b.grouped && a.groups == b.groups;
}


//-------------------------------------------------------------

ZRadicalGrid::ZRadicalGrid(QWidget *parent) : base(parent), heights(34), mode(RadicalFilterModes::Parts),
        smin(0), smax(0), group(false), names(false), exact(false)
{
    init();

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    viewport()->setAttribute(Qt::WA_NoSystemBackground, true);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);

    filterIncluded();
    filter();
}

ZRadicalGrid::~ZRadicalGrid()
{

}

void ZRadicalGrid::setActiveFilter(const RadicalFilter &rad)
{
    selection.clear();
    filters.clear();

    if (rad.mode != RadicalFilterModes::Radicals && rad.groups.size() > 1)
        filters.insert(filters.end(), rad.groups.begin(), rad.groups.end() - 1);

    if (!rad.groups.empty())
    {
        if (rad.mode == RadicalFilterModes::NamedRadicals)
            selection.insert(rad.groups.back().begin(), rad.groups.back().end());
        else if (rad.mode == RadicalFilterModes::Radicals)
        {
            for (auto ix : rad.groups.back())
            {
                int p = ix;
                while (radindexes[p] != ix)
                    ++p;
                selection.insert(p);
            }
        }
        else // Parts
            selection.insert(rad.groups.back().begin(), rad.groups.back().end());
    }

    mode = rad.mode;
    group = rad.grouped;
    if (isVisible())
        viewport()->update();

    emit selectionChanged();
    emit selectionTextChanged(generateFiltersText());
}

RadicalFilterModes ZRadicalGrid::displayMode() const
{
    return mode;
}

void ZRadicalGrid::setDisplayMode(RadicalFilterModes newmode)
{
    if (mode == newmode)
        return;

    clearSelection();
    mode = newmode;
    filterIncluded();
    filter();

}

int ZRadicalGrid::strokeLimitMin()  const
{
    return smin;
}

int ZRadicalGrid::strokeLimitMax() const
{
    return smax;
}

void ZRadicalGrid::setStrokeLimit(int limit)
{
    limit = clamp(limit, 0, std::numeric_limits<int>::max());
    if (smin == limit && smax == limit)
        return;

    smin = smax = limit;
    filter();
}

void ZRadicalGrid::setStrokeLimits(int minlimit, int maxlimit)
{
    minlimit = std::max(minlimit, 0);
    maxlimit = std::max(minlimit, maxlimit);

    if (smin == minlimit && smax == maxlimit)
        return;

    smin = minlimit;
    smax = maxlimit;
    filter();
}

bool ZRadicalGrid::groupDisplay() const
{
    return group;
}

void ZRadicalGrid::setGroupDisplay(bool newgroup)
{
    if (group == newgroup)
        return;
    group = newgroup;
    clearSelection();
    filterIncluded();
    filter();

    emit groupingChanged(group);
}

bool ZRadicalGrid::namesDisplay() const
{
    return names;
}

void ZRadicalGrid::setNamesDisplay(bool newnames)
{
    if (names == newnames)
        return;
    names = newnames;

    recomputeItems();

    const QSize &size = viewport()->size();
    recompute(size);
    recomputeScrollbar(size);
    viewport()->update();
}

QString ZRadicalGrid::searchName() const
{
    return search;
}

void ZRadicalGrid::setSearchName(const QString &newname)
{
    if (search == newname)
        return;
    search = newname;
    filter();
}

bool ZRadicalGrid::exactMatch() const
{
    return exact;
}

void ZRadicalGrid::setExactMatch(bool newexact)
{
    if (exact == newexact)
        return;
    exact = newexact;
    if (search.isEmpty())
        return;
    filter();
}

void ZRadicalGrid::finalizeSelection()
{
    if (selection.empty() || (mode == RadicalFilterModes::Radicals && !filters.empty()))
        return;

    filters.push_back(selectionToFilter());
    selection.clear();

    emit selectionChanged();
    emit selectionTextChanged(generateFiltersText());
}

void ZRadicalGrid::clearSelection()
{
    if (selection.empty() && filters.empty())
        return;

    for (auto it = selection.begin(); it != selection.end(); ++it)
    {
        int ix = indexOf(*it);
        if (ix != -1)
            viewport()->update(itemRect(ix));
    }
    selection.clear();
    filters.clear();

    emit selectionChanged();
    emit selectionTextChanged(QString());
}

void ZRadicalGrid::activeFilter(RadicalFilter &f)
{
    f.mode = mode;
    f.groups = filters;
    f.grouped = group;
}

std::vector<ushort> ZRadicalGrid::selectionToFilter()
{
    if (selection.empty())
        return std::vector<ushort>();

    std::vector<ushort> sels;
    for (ushort num : selection)
    {
        int ix = indexOf(num);
        if (ix != -1)
            viewport()->update(itemRect(ix));
        if (mode == RadicalFilterModes::Radicals)
            sels.push_back(radindexes[num]);
        else
        {
            sels.push_back(num);
            //if (group && mode == RadicalFilterModes::NamedRadicals)
            //{

            //    int siz = ZKanji::radlist.size();
            //    while (num + 1 != siz && ZKanji::radlist[num + 1]->radical == ZKanji::radlist[num]->radical)
            //        sels.push_back(++num);
            //}
        }
    }

    std::sort(sels.begin(), sels.end());
    return sels;
}

void ZRadicalGrid::paintEvent(QPaintEvent *event)
{
    QStylePainter p(viewport());
    const QSize &size = viewport()->size();
    QStyleOptionViewItem opts;
    opts.initFrom(this);
    opts.showDecorationSelected = true;
    QBrush oldbrush = p.brush();

    if (list.empty())
    {
        p.setBrush(opts.palette.color(QPalette::Normal, QPalette::Base));
        p.fillRect(event->rect() /*QRect(0, 0, size.width(), size.height())*/, p.brush());
        p.setBrush(oldbrush);
        return;
    }

    QPen oldpen = p.pen();
    QFont oldfont = p.font();

    int vpos = verticalScrollBar()->value();

    // First row partially or fully visible at the top at the current scroll position.
    int top = vpos / heights;
    // Upper Y coordinate of visible top cells to be drawn. y <= 0.
    int y = top * heights - vpos;

    // Current item to be drawn.
    int pos = top == 0 ? 0 : rows[top - 1];

    //int gridHint = p.style()->styleHint(QStyle::SH_Table_GridLineColor, &opts, this);
    //QColor gridColor = static_cast<QRgb>(gridHint);

    QColor gridColor;
    if (Settings::colors.grid.isValid())
        gridColor = Settings::colors.grid;
    else
    {
        int gridHint = qApp->style()->styleHint(QStyle::SH_Table_GridLineColor, &opts, this);
        gridColor = static_cast<QRgb>(gridHint);
    }

    ZRect r(0, y, -1, y + heights - 1);
    int lastwidth = std::numeric_limits<int>::max();
    while (top != rows.size() && r.top() <= size.height())
    {
        while (pos != rows[top])
        {
            r.setLeft(r.right() + 1);
            r.setWidth(itemWidth(pos) - 1);

            if (!selected(pos))
            {
                p.setPen(opts.palette.color(QPalette::Normal, QPalette::Text));
                p.setBrush(opts.palette.color(QPalette::Normal, QPalette::Base));
            }
            else
            {
                p.setPen(opts.palette.color(QPalette::Normal, QPalette::HighlightedText));
                p.setBrush(opts.palette.color(QPalette::Normal, QPalette::Highlight));
            }

            paintItem(pos, p, r);
            p.setPen(gridColor);
            p.drawLine(r.right(), y, r.right(), y + heights - 1);
            ++pos;
        }
        ++top;

        if (lastwidth < r.right() + 1)
            p.drawLine(lastwidth, y - 1, r.right() + 1, y - 1);
        p.drawLine(0, y + heights - 1, r.right() + 1, y + heights - 1);
        lastwidth = r.right() + 1;
        y += heights;

        p.setBrush(opts.palette.color(QPalette::Normal, QPalette::Base));
        p.fillRect(ZRect(QPoint(lastwidth, r.top()), QPoint(size.width(), r.bottom() + 1)), p.brush());

        r = ZRect(0, r.bottom() + 1, -1, r.bottom() + heights);
    }
    if (top == rows.size())
    {
        p.setBrush(opts.palette.color(QPalette::Normal, QPalette::Base));
        p.fillRect(QRect(QPoint(0, r.top()), QPoint(size.width(), size.height())), p.brush());
        p.setBrush(oldbrush);
    }

    p.setFont(oldfont);
    p.setPen(oldpen);
}

void ZRadicalGrid::resizeEvent(QResizeEvent *event)
{
    const QSize &size = viewport()->size();
    recompute(size);
    recomputeScrollbar(size);
}

void ZRadicalGrid::mousePressEvent(QMouseEvent *event)
{
    base::mousePressEvent(event);

    if (event->button() != Qt::LeftButton)
        return;

    int index = indexAt(event->pos());
    if (index == -1)
        return;
    auto it = std::find(selection.begin(), selection.end(), list[index]);
    if (it != selection.end())
        selection.erase(it);
    else
        selection.insert(list[index]);
    viewport()->update(itemRect(indexOf(list[index])));

    emit selectionChanged();
    emit selectionTextChanged(generateFiltersText());
}

void ZRadicalGrid::mouseReleaseEvent(QMouseEvent *e)
{
    base::mouseReleaseEvent(e);

    // Needed empty handler to avoid event propagation.
}

void ZRadicalGrid::mouseDoubleClickEvent(QMouseEvent *e)
{
    base::mouseDoubleClickEvent(e);

    // Needed empty handler to avoid event propagation.
}

void ZRadicalGrid::keyPressEvent(QKeyEvent *event)
{
    base::keyPressEvent(event);
}

void ZRadicalGrid::init()
{
    //QFont::StyleStrategy ss;

    float radh = 0.72;
    radfontsize = int(heights * radh);
    QFont radfont = Settings::radicalFont();
    //ss = QFont::StyleStrategy(radfont.styleStrategy() | QFont::NoSubpixelAntialias);
    //radfont.setStyleStrategy(ss);
    radfont.setPointSize(radfontsize);

    QFontMetrics radmet(radfont);
    while (radmet.boundingRect(QString(QChar(0x9F3B))).height() > int(heights * 0.72))
    {
        do {
            radh -= 0.03;
        } while (radfontsize == int(heights * radh));
        radfontsize = int(heights * radh);

        radfont.setPointSize(radfontsize);
        radmet = QFontMetrics(radfont);
    }

    float nameh = 0.4;
    namefontsize = int(heights * nameh);
    QFont namefont;

    //ss = QFont::StyleStrategy(namefont.styleStrategy() | QFont::NoSubpixelAntialias);
    //namefont.setStyleStrategy(ss);

    namefont.setFamily(Settings::fonts.kana);
    namefont.setPointSize(namefontsize);

    QFontMetrics namemet(namefont);
    while (namemet.boundingRect(QString(QChar(0x9F3B))).height() > int(heights * 0.4))
    {
        do {
            nameh -= 0.03;
        } while (namefontsize == int(heights * nameh));
        namefontsize = int(heights * nameh);

        namefont.setPointSize(namefontsize);
        namemet = QFontMetrics(namefont);
    }

    float infoh = 0.24;
    notesfontsize = int(heights * infoh);
    QFont notesfont { Settings::fonts.info, notesfontsize };
    QFontMetrics nfmet(notesfont);
    while (nfmet.boundingRect(QStringLiteral("123456789")).height() > int(heights * 0.24))
    {
        do {
            infoh -= 0.03;
        } while (notesfontsize == int(heights * infoh));
        notesfontsize = int(heights * infoh);

        notesfont.setPointSize(notesfontsize);
        nfmet = QFontMetrics(notesfont);
    }

    int p = 0;
    for (int ix = 0; ix != 231; ++ix)
    {
        if (radsymbols[ix] < 0x20)
            radindexes[ix] = 0;
        else
            radindexes[ix] = ++p;
    }

    //p = 0;
    //for (int ix = 0; ix != ***267; ++ix)
    //{
    //    if (partsymbols[ix] < 0x20)
    //        partindexes[ix] = 0;
    //    else
    //        partindexes[ix] = ++p;
    //}
}

void ZRadicalGrid::setFromModel(KanjiGridModel *model)
{
    std::vector<ushort> dest;
    dest.reserve(model->size());
    for (int ix = 0; ix != model->size(); ++ix)
        dest.push_back(model->kanjiAt(ix));

    std::sort(dest.begin(), dest.end());
    if (kanjilist != dest)
    {
        kanjilist = dest;

        filterIncluded();
        filter();
    }
}

void ZRadicalGrid::setFromList(const std::vector<ushort> &src)
{
    std::vector<ushort> dest(src);

    std::sort(dest.begin(), dest.end());
    if (kanjilist != dest)
    {
        kanjilist = dest;

        filterIncluded();
        filter();
    }
}

void ZRadicalGrid::filterIncluded()
{
    included.clear();
    if (mode == RadicalFilterModes::Radicals)
    {
        for (int ix = 0, siz = kanjilist.size(); ix != siz; ++ix)
            included.insert(ZKanji::kanjis[kanjilist[ix]]->rad);
    }
    else if (mode == RadicalFilterModes::Parts)
    {
        //for (int iy = 0; iy != 252; ++iy)
        //{
        //    for (int ix = 0; ix != kanjilist.size(); ++ix)
        //    {
        //        if (std::binary_search(ZKanji::partlist[iy].begin(), ZKanji::partlist[iy].end(), kanjilist[ix]))
        //        {
        //            included.insert(iy + 1);
        //            break;
        //        }
        //    }
        //}

        for (int ix = 0, siz = kanjilist.size(); ix != siz; ++ix)
        {
            auto &rads = ZKanji::kanjis[kanjilist[ix]]->radks;
            //included.insert(rads.begin(), rads.end());
            for (ushort r : rads)
                included.insert(r);
        }
    }
    else
    {
        //int radpos = 0;
        //for (int iy = 0; iy != ZKanji::radlist.size(); ++iy)
        //{
        //    if (!group || (group && iy != 0 && ZKanji::radlist[iy]->radical != ZKanji::radlist[iy - 1]->radical))
        //        radpos = iy;
        //    if (included.count(radpos))
        //        continue;

        //    for (int ix = 0; ix != kanjilist.size(); ++ix)
        //    {
        //        if (std::binary_search(ZKanji::radlist[iy]->kanji.begin(), ZKanji::radlist[iy]->kanji.end(), kanjilist[ix]))
        //        {
        //            included.insert(radpos);
        //            break;
        //        }
        //    }
        //}

        std::vector<ushort> v;

        if (kanjilist.size() != ZKanji::kanjicount)
        {
            for (int ix = 0, siz = kanjilist.size(); ix != siz; ++ix)
            {
                auto &rads = ZKanji::kanjis[kanjilist[ix]]->rads;
                if (group)
                    v.insert(v.end(), rads.begin(), rads.end());
                else
                    for (ushort r : rads)
                        included.insert(r);

                //included.insert(rads.begin(), rads.end());
            }
        }
        else
        {
            v.reserve(ZKanji::radlist.size());
            for (int ix = 0, siz = ZKanji::radlist.size(); ix != siz; ++ix)
                if (group)
                    v.push_back(ix);
                else
                    included.insert(ix);
        }

        if (group)
        {
            std::sort(v.begin(), v.end());
            v.resize(std::unique(v.begin(), v.end()) - v.begin());

            int vpos = 0;
            bool inserted = false;
            for (int ix = 0, siz = ZKanji::radlist.size(), vsiz = v.size(); ix != siz && vpos != vsiz; ++ix)
            {
                if (ix == 0 || ZKanji::radlist[ix]->radical != ZKanji::radlist[ix - 1]->radical)
                    inserted = false;

                if (inserted || ZKanji::radlist[v[vpos]]->radical > ZKanji::radlist[ix]->radical)
                    continue;

                included.insert(ix);
                inserted = true;

                ++vpos;
            }
        }
    }
}

void ZRadicalGrid::filter()
{
    list.clear();

    // Radicals that were already added to a previous selection shouldn't be shown again.
    std::set<ushort> excluded;
    for (int ix = 0; ix != filters.size(); ++ix)
    {
        std::vector<ushort> &v = filters[ix];
        for (int iy = 0; iy != v.size(); ++iy)
            excluded.insert(v[iy]);
    }

    if (mode == RadicalFilterModes::Radicals)
    {
        int cnt = 231;
        QChar *arr = radsymbols;
        int strokeindex;
        if (smin == 0 || smin == 1 && smax >= 17)
        {
            for (int ix = 0; ix != cnt; ++ix)
            {
                if (arr[ix].unicode() < 0x20)
                {
                    strokeindex = ix;
                    continue;
                }
                if (excluded.count(radindexes[ix]) || !included.contains(radindexes[ix]))
                    continue;
                if (strokeindex != -1)
                {
                    list.push_back((ushort)-arr[strokeindex].unicode());
                    strokeindex = -1;
                }
                list.push_back(ix);
            }
        }
        else
        {
            int spos = 0;
            int ix = 0;
            for (; spos != smin && ix != cnt; ++ix)
            {
                if (arr[ix].unicode() < 0x20)
                    spos = arr[ix].unicode();
            }
            strokeindex = ix - 1;

            for (; ix != cnt; ++ix)
            {
                if (arr[ix].unicode() < 0x20)
                {
                    strokeindex = ix;
                    spos = arr[ix].unicode();
                    continue;
                }
                if (spos > smax)
                    break;
                if (!excluded.count(radindexes[ix]) && included.contains(radindexes[ix]))
                {
                    if (strokeindex != -1)
                    {
                        list.push_back((ushort)-arr[strokeindex].unicode());
                        strokeindex = -1;
                    }
                    list.push_back(ix);
                }
            }

        }
    }
    else if (mode == RadicalFilterModes::Parts)
    {
        int spos = std::max(0, smin - 1);
        bool newspos = true;
        for (int ix = ZKanji::radkcnt[spos]; ix != ZKanji::radklist.size(); ++ix)
        {
            while (spos < ZKanji::radkcnt.size() - 1 && ix == ZKanji::radkcnt[spos + 1])
            {
                ++spos;
                newspos = true;
            }
            if (smin != 0 && spos >= smax)
                break;

            if (!excluded.count(ix /*ZKanji::radklist[ix].first*/) && included.contains(ix /*ZKanji::radklist[ix].first*/))
            {
                if (newspos)
                {
                    list.push_back((ushort)-(spos + 1));
                    newspos = false;
                }
                list.push_back(ix);
            }
        }
    }
    else
    {
        int ix = 0;
        int radcnt = ZKanji::radlist.size();
        int radpos = 0; // Holds the index of the first previous radical with the same radical number.
        for (; smin != 0 && ix != radcnt && ZKanji::radlist[ix]->strokes != smin; ++ix)
            ;   

        for (; ix != radcnt; ++ix)
        {
            if (!group || radpos == 0 || (ix != 0 && ZKanji::radlist[ix]->radical != ZKanji::radlist[ix - 1]->radical))
                radpos = ix;
            if (group && !list.empty() && list.back() == radpos)
                continue;
            if ((search.isEmpty() || (!search.isEmpty() && ZKanji::radlist[ix]->names.contains(search.constData(), exact))) &&
                (smin == 0 || (ZKanji::radlist[ix]->strokes >= smin && ZKanji::radlist[ix]->strokes <= smax)))
            {
                if (!excluded.count(radpos) && included.contains(radpos))
                    list.push_back(radpos);
            }
        }
    }
    recomputeItems();

    const QSize &size = viewport()->size();
    recompute(size);
    recomputeScrollbar(size);
    viewport()->update();
}

void ZRadicalGrid::recomputeItems()
{
    if (mode == RadicalFilterModes::Parts || mode == RadicalFilterModes::Radicals)
        return;

    items.clear();
    //QFont::StyleStrategy ss;
    QFont radfont = Settings::radicalFont();
    //ss = QFont::StyleStrategy(radfont.styleStrategy() | QFont::NoSubpixelAntialias);
    //radfont.setStyleStrategy(ss);
    //radfont.setFamily(radicalsFontName());
    radfont.setPointSize(radfontsize);
    QFontMetrics radmet(radfont);
    QFont namefont;
    //ss = QFont::StyleStrategy(namefont.styleStrategy() | QFont::NoSubpixelAntialias);
    //namefont.setStyleStrategy(ss);
    namefont.setFamily(Settings::fonts.kana);
    namefont.setPointSize(namefontsize);
    QFontMetrics namemet(namefont);
    QFont infofont(Settings::fonts.info, notesfontsize);
    QFontMetrics infomet(infofont);

    QChar comma(0x3001);

    ItemData *dat;

    int radcommaw = radmet.width(comma);
    int namecommaw = namemet.width(comma);
    int radcnt = ZKanji::radlist.size();

    int namesiz;
    int radpos;

    int spacingw = std::max(1., heights  * 0.1);
    for (int ix = 0; ix != list.size(); ++ix)
    {
        dat = new ItemData;
        dat->size = 1;
        dat->width = 0;
        radpos = list[ix];
        if (!group)
        {
            namesiz = ZKanji::radlist[radpos]->names.size();
            dat->width = heights - 1;
        }
        else
        {
            namesiz = ZKanji::radlist[radpos]->names.size();
            for (; dat->size + radpos != radcnt && ZKanji::radlist[radpos + dat->size]->radical == ZKanji::radlist[radpos + dat->size - 1]->radical; ++dat->size, namesiz += ZKanji::radlist[radpos + dat->size]->names.size())
                ;
            dat->width = (heights - 1) * dat->size + radcommaw * (dat->size - 1);

        }
        if (names)
        {

            dat->width += spacingw * 2;

            // Two rows of kanji names are displayed. The list is broken,
            // where the two rows make up the least width.

            // Collect all the names to one list, to make it easier to 
            dat->names.reserve(namesiz);

            for (int iy = 0; iy != dat->size; ++iy)
                dat->names.add(ZKanji::radlist[radpos + iy]->names, false);

            if (dat->names.size() == 1)
            {
                dat->namebreak = 1;
                dat->width += namemet.boundingRect(dat->names[0].toQString()).width() + namecommaw / 2;
                items.push_back(dat);
                continue;
            }

            // Compute the width of the two rows. The smaller one will be the result.
            int upperpos = 0;
            int lowerpos = dat->names.size() - 1;

            int upperwidth = namemet.width(dat->names[upperpos].toQString());
            int lowerwidth = namemet.width(dat->names[lowerpos].toQString());
            while (upperpos + 1 < lowerpos)
            {
                if (upperwidth <= lowerwidth)
                {
                    upperwidth += namecommaw;
                    ++upperpos;
                    upperwidth += namemet.width(dat->names[upperpos].toQString());
                }
                else
                {
                    lowerwidth += namecommaw;
                    --lowerpos;
                    lowerwidth += namemet.width(dat->names[lowerpos].toQString());
                }
            }
            upperwidth += namecommaw;
            lowerwidth += namecommaw / 2;
            dat->namebreak = upperpos + 1;
            dat->width += std::max(upperwidth, lowerwidth);
        }

        // Leave space for the grid line to the right.
        ++dat->width;

        items.push_back(dat);
    }
}

void ZRadicalGrid::recompute(const QSize &size)
{
    rows.clear();
    int nextw = 0;

    int x = 0;
    int cnt = list.size();
    for (int ix = 0; ix != cnt; ++ix)
    {
        int w = nextw == 0 ? itemWidth(ix) : nextw;
        nextw = 0;

        if (x != 0 && ix != cnt - 1 && blockStarter(ix) && !blockStarter(ix + 1))
            nextw = itemWidth(ix + 1);

        if (x != 0 && x + w + nextw > size.width())
        {
            rows.push_back(ix);
            x = w;
        }
        else
            x += w;
    }
    rows.push_back(cnt);
}

void ZRadicalGrid::recomputeScrollbar(const QSize &size)
{
    int fullheight = heights * rows.size();
    verticalScrollBar()->setMaximum(std::max(0, fullheight - size.height()));
    verticalScrollBar()->setSingleStep(heights * 0.7);
    verticalScrollBar()->setPageStep(std::max(size.height() - heights * 1.2, heights * 0.9));
}

bool ZRadicalGrid::blockStarter(int index)
{
    if (mode == RadicalFilterModes::NamedRadicals)
        return false;
    //if (mode == RadicalFilterModes::Parts)
    //    return partsymbols[list[index]].unicode() < 0x20;
    //return radsymbols[list[index]].unicode() < 0x20;

    return ((short)list[index]) < 0;
}

int ZRadicalGrid::itemWidth(int index)
{
    if (mode == RadicalFilterModes::Parts || mode == RadicalFilterModes::Radicals)
        return heights;

    return items[index]->width;
}

void ZRadicalGrid::paintItem(int index, QStylePainter &p, const ZRect &r)
{
    //QFont::StyleStrategy ss;

    QFont radfont = Settings::radicalFont();
    //ss = QFont::StyleStrategy(QFont::PreferDevice | QFont::NoSubpixelAntialias | QFont::PreferAntialias);
    //radfont.setStyleStrategy(ss);
    //radfont.setFamily(radicalsFontName());
    radfont.setPointSize(radfontsize);
    QFontMetrics radmet(radfont);

    QFont namefont;
    //ss = QFont::StyleStrategy(namefont.styleStrategy() | QFont::NoSubpixelAntialias | QFont::PreferAntialias);
    //namefont.setStyleStrategy(ss);
    namefont.setFamily(Settings::fonts.kana);
    namefont.setPointSize(namefontsize);
    QFontMetrics namemet(namefont);

    QFont infofont{ Settings::fonts.info, notesfontsize };
    QFontMetrics infomet(infofont);

    QChar comma(0x3001);

    int radcommaw = radmet.width(comma);
    int namecommaw = namemet.width(comma);
    int spacingw = std::max(1., heights  * 0.1);

    int radwidth = 0;

    QFont oldfont = p.font();
    p.fillRect(r, p.brush());

    QString str;
    if (mode == RadicalFilterModes::Parts || mode == RadicalFilterModes::Radicals || (!group && !names))
    {
        p.setFont(radfont);
        if ((mode == RadicalFilterModes::Parts || mode == RadicalFilterModes::Radicals) && ((short)list[index]) < 0)
        {
            str = QString::number(-(short)list[index]);
            radwidth = 0;
        }
        else
        {
            str = mode == RadicalFilterModes::Parts ? QChar(ZKanji::radklist[list[index]].first) : mode == RadicalFilterModes::Radicals ? radsymbols[list[index]] : ZKanji::radlist[list[index]]->ch;
            radwidth = heights - 1;
        }
        p.drawText(r.left(), r.top(), heights - 1, heights - 1 - heights * 0.25, Qt::AlignHCenter | Qt::AlignVCenter, str);
    }
    else
    {

        ItemData *data = items[index];

        int x = r.left();

        p.setFont(radfont);
        // Draw the radicals.
        for (int ix = 0; ix != data->size; ++ix)
        {
            p.drawText(x, r.top(), heights - 1, heights - 1 - heights * 0.25, Qt::AlignHCenter | Qt::AlignVCenter, ZKanji::radlist[list[index] + ix]->ch);
            x += heights - 1;
            if (ix != data->size - 1)
            {
                p.drawText(x, r.top(), radcommaw, heights - 1 - heights * 0.25, Qt::AlignHCenter | Qt::AlignVCenter, comma);
                x += radcommaw;
            }
        }
        radwidth = (heights - 1) * (data->size * 2 - 1);

        if (names)
        {
            p.setFont(namefont);
            x += spacingw;
            for (int ix = 0; ix != data->namebreak; ++ix)
            {
                str += data->names[ix].toQString();
                if (data->names.size() != 1)
                    str += comma;
            }
            p.drawText(x, r.top(), r.width() - (x - r.left()), data->names.size() == 1 ? heights - 1 : ((heights - 1) / 2), Qt::AlignVCenter | Qt::AlignLeft , str);
            str.clear();
            for (int ix = data->namebreak; ix != data->names.size(); ++ix)
            {
                str += data->names[ix].toQString();
                if (ix != data->names.size() - 1)
                    str += comma;
            }
            if (!str.isEmpty())
                p.drawText(x, r.top() + (heights - 1) / 2, r.width() - (x - r.left()), (heights - 1) / 2, Qt::AlignVCenter | Qt::AlignLeft, str);
        }
    }

    // Radical number.
    if (radwidth)
    {
        if (mode == RadicalFilterModes::Parts)
            str = QString::number(list[index] + 1);
        else if (mode == RadicalFilterModes::Radicals)
            str = QString::number(radindexes[list[index]]);
        else if (mode == RadicalFilterModes::NamedRadicals)
            str = QString::number(int(ZKanji::radlist[list[index]]->radical));

        p.setFont(infofont);
        p.drawText(r.left(), r.bottom() - heights * 0.24, radwidth, heights * 0.24, Qt::AlignHCenter | Qt::AlignVCenter, str);
    }

    p.setFont(oldfont);
}

int ZRadicalGrid::indexAt(int x, int y)
{
    int vpos = verticalScrollBar()->value();

    // First row partially or fully visible at the top at the current scroll position.
    int top = vpos / heights;
    // Upper Y coordinate of visible top cells. dy <= 0.
    int dy = top * heights - vpos;
    y -= dy;

    // Row at the y coordinate.
    int row = top + y / heights;
    if (row >= rows.size())
        return -1;

    // First item in the row at y.
    int pos = row == 0 ? 0 : rows[row - 1];

    while (pos != std::min<int>(list.size(), rows[row]) && x >= itemWidth(pos))
        x -= itemWidth(pos++);

    if (pos == std::min<int>(list.size(), rows[row]) || itemChar(pos).unicode() < 0x20)
        return -1;

    return pos;
}

int ZRadicalGrid::indexAt(const QPoint &pt)
{
    return indexAt(pt.x(), pt.y());
}

int ZRadicalGrid::indexOf(ushort val)
{
    auto it = std::find(list.begin(), list.end(), val);
    if (it == list.end())
        return -1;
    return it - list.begin();
}

ZRect ZRadicalGrid::itemRect(int index)
{
    if (index < 0 || index >= list.size())
        return ZRect();

    int vpos = verticalScrollBar()->value();

    // First row partially or fully visible at the top at the current scroll position.
    int top = vpos / heights;
    // Upper Y coordinate of visible top cells. y <= 0.
    int y = top * heights - vpos;

    // First item in the row at y.
    int pos = top == 0 ? 0 : rows[top - 1];
    if (index < pos)
        return ZRect();

    while (top != rows.size() && rows[top] <= index)
    {
        ++top;
        y += heights;
    }

    if (top != 0)
        pos = rows[top - 1];

    QSize size = viewport()->size();
    if (y >= size.height())
        return ZRect();

    int x = 0;
    while (pos != index)
        x += itemWidth(pos++);

    return ZRectS(x, y, itemWidth(index), heights);
}

bool ZRadicalGrid::selected(int index)
{
    return std::find(selection.begin(), selection.end(), list[index]) != selection.end();
}

QString ZRadicalGrid::generateFiltersText()
{
    QString seltext;

    if (!selection.empty())
    {
        seltext = "[";
        for (ushort ix : selection)
        {
            if (mode == RadicalFilterModes::Parts)
                seltext += QChar(ZKanji::radklist[ix].first);
            else if (mode == RadicalFilterModes::Radicals)
                seltext += radsymbols[ix];
            else
            {
                if (group)
                {
                    for (int iy = ix, siz = ZKanji::radlist.size(); iy != siz; ++iy)
                        if (iy != ix && iy != 0 && ZKanji::radlist[iy]->radical != ZKanji::radlist[iy - 1]->radical)
                            break;
                        else if (iy == ix || ZKanji::radlist[iy]->ch != ZKanji::radlist[iy - 1]->ch)
                            seltext += ZKanji::radlist[iy]->ch;
                }
                else
                    seltext += ZKanji::radlist[ix]->ch;
            }
        }
        if (!filters.empty())
            seltext += "] + ";
        else
            seltext += "]";
    }

    for (int ix = filters.size() - 1; ix != -1; --ix)
    {
        int partpos = 0;
        const std::vector<ushort> &sel = filters[ix];
        for (int iy = 0; iy != sel.size(); ++iy)
        {
            if (mode == RadicalFilterModes::Radicals)
            {
                while (ZKanji::radlist[partpos]->radical != sel[iy])
                    ++partpos;
                seltext += ZKanji::radlist[partpos]->ch;
            }
            else if (mode == RadicalFilterModes::Parts)
                seltext += QChar(ZKanji::radklist[sel[iy]].first);
            else
            {
                if (group)
                {
                    for (int j = sel[iy], siz = ZKanji::radlist.size(); j != siz; ++j)
                        if (j != sel[iy] && j != 0 && ZKanji::radlist[j]->radical != ZKanji::radlist[j - 1]->radical)
                            break;
                        else if (j == sel[iy] || ZKanji::radlist[j]->ch != ZKanji::radlist[j - 1]->ch)
                            seltext += ZKanji::radlist[j]->ch;
                }
                else
                    seltext += ZKanji::radlist[sel[iy]]->ch;
            }
        }
        if (ix != 0)
            seltext += QStringLiteral(" + ");
    }

    return seltext;
}

QChar ZRadicalGrid::itemChar(int index)
{
    if (mode == RadicalFilterModes::Parts)
    {
        if (list[index] >= ZKanji::radklist.size())
            return 0;
        return QChar(ZKanji::radklist[list[index]].first);
    }
    if (mode == RadicalFilterModes::Radicals)
    {
        if (list[index] >= 321)
            return 0;
        return radsymbols[list[index]];
    }
    if (list[index] >= ZKanji::radlist.size())
        return 0;
    return ZKanji::radlist[list[index]]->ch;
}


//-------------------------------------------------------------


RadicalFiltersModel::RadicalFiltersModel(QObject *parent) : base(parent)
{
}

RadicalFiltersModel::~RadicalFiltersModel()
{
    ;
}

bool RadicalFiltersModel::empty() const
{
    return list.empty();
}

int RadicalFiltersModel::size() const
{
    return list.size();
}

int RadicalFiltersModel::add(const RadicalFilter &src)
{
    // Look for a filter which exactly matches src and if found,
    // return its index.
    auto it = std::find_if(list.begin(), list.end(), [&src](const RadicalFilter *f) { return *f == src;  });
    if (it != list.end())
        return it - list.begin();

    list.push_back(src);
    emit filterAdded();

    return list.size() - 1;
}

void RadicalFiltersModel::clear()
{
    list.clear();
    emit cleared();
}

void RadicalFiltersModel::remove(int index)
{
    list.erase(list.begin() + index);
    emit filterRemoved(index);
}

void RadicalFiltersModel::move(int index, bool up)
{
    if ((index == 0 && up) || (index == list.size() - 1 && !up))
        return;

    RadicalFilter *f = list[index];
    list[index] = list[index + (up ? -1 : 1)];
    list[index + (up ? -1 : 1)] = f;

    emit filterMoved(index, up);
}

const RadicalFilter& RadicalFiltersModel::filters(int index) const
{
    return *list[index];
}

QString RadicalFiltersModel::filterText(const RadicalFilter &filter) const
{
    QString str;
    switch (filter.mode)
    {
    case RadicalFilterModes::Radicals:
        str = "1:";
        break;
    case RadicalFilterModes::Parts:
        str = "2:";
        break;
    case RadicalFilterModes::NamedRadicals:
        str = "3:";
        if (filter.grouped)
            str += "(";
        break;
    }
    for (int ix = 0, siz = filter.groups.size(); ix != siz; ++ix)
    {
        int partpos = 0;
        const std::vector<ushort> &sel = filter.groups[ix];
        for (int iy = 0; iy != sel.size(); ++iy)
        {
            if (filter.mode == RadicalFilterModes::Radicals)
            {
                while (ZKanji::radlist[partpos]->radical != sel[iy])
                    ++partpos;
                str += ZKanji::radlist[partpos]->ch;
            }
            else if (filter.mode == RadicalFilterModes::Parts)
                str += QChar(ZKanji::radklist[sel[iy]].first);
            else
            {
                if (filter.grouped)
                {
                    for (int j = sel[iy], siz = ZKanji::radlist.size(); j != siz; ++j)
                        if (j != sel[iy] && ZKanji::radlist[j]->radical != ZKanji::radlist[j - 1]->radical)
                            break;
                        else if (j == sel[iy] || ZKanji::radlist[j]->ch != ZKanji::radlist[j - 1]->ch)
                            str += ZKanji::radlist[j]->ch;
                }
                else
                    str += ZKanji::radlist[sel[iy]]->ch;
            }
        }
        if (ix != filter.groups.size() - 1)
            str += QStringLiteral(" + ");
    }

    if (filter.mode == RadicalFilterModes::NamedRadicals && filter.grouped)
        str += ")";
    return str;
}

QString RadicalFiltersModel::filterText(int ix) const
{
    return filterText(*list[ix]);
}

int RadicalFiltersModel::filterIndex(const RadicalFilter &filter)
{
    auto it = std::find_if(list.begin(), list.end(), [&filter](const RadicalFilter *a) { return *a == filter; });
    if (it == list.end())
        return -1;

    return it - list.begin();
}

//RadicalFilter RadicalFiltersModel::filterFromText(QString str)
//{
//    RadicalFilter f;
//
//    if (str.isEmpty() || str.size() == 1 || str.at(1) != ':')
//        return f;
//
//    QVector<QStringRef> grps;
//    switch (str.at(0).unicode())
//    {
//    case '1':
//        f.mode = RadicalFilterModes::Radicals;
//        if (str.size() == 2)
//            return f;
//
//        grps = str.midRef(2).split(" + ");
//        break;
//    case '2':
//        f.mode = RadicalFilterModes::Parts;
//        if (str.size() == 2)
//            return f;
//
//        grps = str.midRef(2).split(" + ");
//        break;
//    case '3':
//        f.mode = RadicalFilterModes::NamedRadicals;
//        if (str.size() == 2)
//            return f;
//        if (str.at(2) == '(')
//        {
//            f.grouped = true;
//            if (str.at(str.size() - 1) != ')')
//                return f;
//        }
//
//        grps = str.midRef(2 + (f.grouped ? 1 : 0), str.size() - 2 - (f.grouped ? 2 : 0)).split(" + ");
//        break;
//    default:
//        return f;
//    }
//
//    f.groups.resize(grps.size());
//    for (int ix = 0, siz = grps.size(); ix != siz; ++ix)
//    {
//        int partpos = 0;
//        const QStringRef &ref = grps.at(ix);
//        std::vector<ushort> &sel = f.groups[ix];
//        sel.reserve(ref.size());
//        for (int iy = 0, ysiz = ref.size(); iy != ysiz; ++iy)
//        {
//            QChar ch = ref.at(iy);
//            if (f.mode == RadicalFilterModes::Radicals)
//            {
//                while (partpos != ZKanji::radlist.size() && ZKanji::radlist[partpos]->ch != ch)
//                    ++partpos;
//                if (partpos == ZKanji::radlist.size())
//                    break;
//                sel.push_back(ZKanji::radlist[partpos]->radical);
//            }
//            else if (f.mode == RadicalFilterModes::Parts)
//            {
//                auto it = std::find_if(ZKanji::radklist.begin(), ZKanji::radklist.end(), [ch](const std::pair<ushort, fastarray<ushort>> &a) {
//                    return a.first == ch;
//                });
//                if (it != ZKanji::radklist.end())
//                    sel.push_back(it - ZKanji::radklist.begin());
//            }
//            else // NamedRadicals
//            {
//                partpos = ZKanji::radlist.find(ch);
//                if (partpos == -1)
//                    break;
//                sel.push_back(partpos);
//                if (f.grouped)
//                {
//                    /*
//                    for (int j = sel[iy], siz = ZKanji::radlist.size(); j != siz; ++j)
//                        if (j != sel[iy] && ZKanji::radlist[j]->radical != ZKanji::radlist[j - 1]->radical)
//                            break;
//                        else if (j == sel[iy] || ZKanji::radlist[j]->ch != ZKanji::radlist[j - 1]->ch)
//                            str += ZKanji::radlist[j]->ch;
//                    */
//                    ++iy;
//                    for (int j = partpos + 1, siz = ZKanji::radlist.size(); iy != ysiz && j != siz; ++j)
//                    {
//                        if (ZKanji::radlist[j]->radical != ZKanji::radlist[j - 1]->radical)
//                            break;
//                        if (ZKanji::radlist[j]->ch != ZKanji::radlist[j - 1]->ch && ref.at(iy) == ZKanji::radlist[j]->ch)
//                            ++iy;
//                    }
//                    --iy;
//
//                }
//            }
//        }
//    }
//
//    return f;
//}


//-------------------------------------------------------------
