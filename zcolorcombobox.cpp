/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QtEvents>
#include <QStylePainter>
#include <QApplication>
#include <QAbstractItemView>
#include <QColorDialog>

#include <cmath>

#include "zcolorcombobox.h"


//-------------------------------------------------------------


static std::vector<std::pair<QString, QColor>> basecolors =
{ { ZColorListModel::tr("aliceblue"), QColor(240, 248, 255) },
{ ZColorListModel::tr("antiquewhite"), QColor(250, 235, 215) },
{ ZColorListModel::tr("aqua"), QColor( 0, 255, 255) },
{ ZColorListModel::tr("aquamarine"), QColor(127, 255, 212) },
{ ZColorListModel::tr("azure"), QColor(240, 255, 255) },
{ ZColorListModel::tr("beige"), QColor(245, 245, 220) },
{ ZColorListModel::tr("bisque"), QColor(255, 228, 196) },
{ ZColorListModel::tr("black"), QColor( 0, 0, 0) },
{ ZColorListModel::tr("blanchedalmond"), QColor(255, 235, 205) },
{ ZColorListModel::tr("blue"), QColor( 0, 0, 255) },
{ ZColorListModel::tr("blueviolet"), QColor(138, 43, 226) },
{ ZColorListModel::tr("brown"), QColor(165, 42, 42) },
{ ZColorListModel::tr("burlywood"), QColor(222, 184, 135) },
{ ZColorListModel::tr("cadetblue"), QColor( 95, 158, 160) },
{ ZColorListModel::tr("chartreuse"), QColor(127, 255, 0) },
{ ZColorListModel::tr("chocolate"), QColor(210, 105, 30) },
{ ZColorListModel::tr("coral"), QColor(255, 127, 80) },
{ ZColorListModel::tr("cornflowerblue"), QColor(100, 149, 237) },
{ ZColorListModel::tr("cornsilk"), QColor(255, 248, 220) },
{ ZColorListModel::tr("crimson"), QColor(220, 20, 60) },
{ ZColorListModel::tr("cyan"), QColor( 0, 255, 255) },
{ ZColorListModel::tr("darkblue"), QColor( 0, 0, 139) },
{ ZColorListModel::tr("darkcyan"), QColor( 0, 139, 139) },
{ ZColorListModel::tr("darkgoldenrod"), QColor(184, 134, 11) },
{ ZColorListModel::tr("darkgray"), QColor(169, 169, 169) },
{ ZColorListModel::tr("darkgreen"), QColor( 0, 100, 0) },
{ ZColorListModel::tr("darkgrey"), QColor(169, 169, 169) },
{ ZColorListModel::tr("darkkhaki"), QColor(189, 183, 107) },
{ ZColorListModel::tr("darkmagenta"), QColor(139, 0, 139) },
{ ZColorListModel::tr("darkolivegreen"), QColor( 85, 107, 47) },
{ ZColorListModel::tr("darkorange"), QColor(255, 140, 0) },
{ ZColorListModel::tr("darkorchid"), QColor(153, 50, 204) },
{ ZColorListModel::tr("darkred"), QColor(139, 0, 0) },
{ ZColorListModel::tr("darksalmon"), QColor(233, 150, 122) },
{ ZColorListModel::tr("darkseagreen"), QColor(143, 188, 143) },
{ ZColorListModel::tr("darkslateblue"), QColor( 72, 61, 139) },
{ ZColorListModel::tr("darkslategray"), QColor( 47, 79, 79) },
{ ZColorListModel::tr("darkslategrey"), QColor( 47, 79, 79) },
{ ZColorListModel::tr("darkturquoise"), QColor( 0, 206, 209) },
{ ZColorListModel::tr("darkviolet"), QColor(148, 0, 211) },
{ ZColorListModel::tr("deeppink"), QColor(255, 20, 147) },
{ ZColorListModel::tr("deepskyblue"), QColor( 0, 191, 255) },
{ ZColorListModel::tr("dimgray"), QColor(105, 105, 105) },
{ ZColorListModel::tr("dimgrey"), QColor(105, 105, 105) },
{ ZColorListModel::tr("dodgerblue"), QColor( 30, 144, 255) },
{ ZColorListModel::tr("firebrick"), QColor(178, 34, 34) },
{ ZColorListModel::tr("floralwhite"), QColor(255, 250, 240) },
{ ZColorListModel::tr("forestgreen"), QColor( 34, 139, 34) },
{ ZColorListModel::tr("fuchsia"), QColor(255, 0, 255) },
{ ZColorListModel::tr("gainsboro"), QColor(220, 220, 220) },
{ ZColorListModel::tr("ghostwhite"), QColor(248, 248, 255) },
{ ZColorListModel::tr("gold"), QColor(255, 215, 0) },
{ ZColorListModel::tr("goldenrod"), QColor(218, 165, 32) },
{ ZColorListModel::tr("gray"), QColor(128, 128, 128) },
{ ZColorListModel::tr("grey"), QColor(128, 128, 128) },
{ ZColorListModel::tr("green"), QColor( 0, 128, 0) },
{ ZColorListModel::tr("greenyellow"), QColor(173, 255, 47) },
{ ZColorListModel::tr("honeydew"), QColor(240, 255, 240) },
{ ZColorListModel::tr("hotpink"), QColor(255, 105, 180) },
{ ZColorListModel::tr("indianred"), QColor(205, 92, 92) },
{ ZColorListModel::tr("indigo"), QColor( 75, 0, 130) },
{ ZColorListModel::tr("ivory"), QColor(255, 255, 240) },
{ ZColorListModel::tr("khaki"), QColor(240, 230, 140) },
{ ZColorListModel::tr("lavender"), QColor(230, 230, 250) },
{ ZColorListModel::tr("lavenderblush"), QColor(255, 240, 245) },
{ ZColorListModel::tr("lawngreen"), QColor(124, 252, 0) },
{ ZColorListModel::tr("lemonchiffon"), QColor(255, 250, 205) },
{ ZColorListModel::tr("lightblue"), QColor(173, 216, 230) },
{ ZColorListModel::tr("lightcoral"), QColor(240, 128, 128) },
{ ZColorListModel::tr("lightcyan"), QColor(224, 255, 255) },
{ ZColorListModel::tr("lightgoldenrodyellow"), QColor(250, 250, 210) },
{ ZColorListModel::tr("lightgray"), QColor(211, 211, 211) },
{ ZColorListModel::tr("lightgreen"), QColor(144, 238, 144) },
{ ZColorListModel::tr("lightgrey"), QColor(211, 211, 211) },
{ ZColorListModel::tr("lightpink"), QColor(255, 182, 193) },
{ ZColorListModel::tr("lightsalmon"), QColor(255, 160, 122) },
{ ZColorListModel::tr("lightseagreen"), QColor( 32, 178, 170) },
{ ZColorListModel::tr("lightskyblue"), QColor(135, 206, 250) },
{ ZColorListModel::tr("lightslategray"), QColor(119, 136, 153) },
{ ZColorListModel::tr("lightslategrey"), QColor(119, 136, 153) },
{ ZColorListModel::tr("lightsteelblue"), QColor(176, 196, 222) },
{ ZColorListModel::tr("lightyellow"), QColor(255, 255, 224) },
{ ZColorListModel::tr("lime"), QColor( 0, 255, 0) },
{ ZColorListModel::tr("limegreen"), QColor( 50, 205, 50) },
{ ZColorListModel::tr("linen"), QColor(250, 240, 230) },
{ ZColorListModel::tr("magenta"), QColor(255, 0, 255) },
{ ZColorListModel::tr("maroon"), QColor(128, 0, 0) },
{ ZColorListModel::tr("mediumaquamarine"), QColor(102, 205, 170) },
{ ZColorListModel::tr("mediumblue"), QColor( 0, 0, 205) },
{ ZColorListModel::tr("mediumorchid"), QColor(186, 85, 211) },
{ ZColorListModel::tr("mediumpurple"), QColor(147, 112, 219) },
{ ZColorListModel::tr("mediumseagreen"), QColor( 60, 179, 113) },
{ ZColorListModel::tr("mediumslateblue"), QColor(123, 104, 238) },
{ ZColorListModel::tr("mediumspringgreen"), QColor( 0, 250, 154) },
{ ZColorListModel::tr("mediumturquoise"), QColor( 72, 209, 204) },
{ ZColorListModel::tr("mediumvioletred"), QColor(199, 21, 133) },
{ ZColorListModel::tr("midnightblue"), QColor( 25, 25, 112) },
{ ZColorListModel::tr("mintcream"), QColor(245, 255, 250) },
{ ZColorListModel::tr("mistyrose"), QColor(255, 228, 225) },
{ ZColorListModel::tr("moccasin"), QColor(255, 228, 181) },
{ ZColorListModel::tr("navajowhite"), QColor(255, 222, 173) },
{ ZColorListModel::tr("navy"), QColor( 0, 0, 128) },
{ ZColorListModel::tr("oldlace"), QColor(253, 245, 230) },
{ ZColorListModel::tr("olive"), QColor(128, 128, 0) },
{ ZColorListModel::tr("olivedrab"), QColor(107, 142, 35) },
{ ZColorListModel::tr("orange"), QColor(255, 165, 0) },
{ ZColorListModel::tr("orangered"), QColor(255, 69, 0) },
{ ZColorListModel::tr("orchid"), QColor(218, 112, 214) },
{ ZColorListModel::tr("palegoldenrod"), QColor(238, 232, 170) },
{ ZColorListModel::tr("palegreen"), QColor(152, 251, 152) },
{ ZColorListModel::tr("paleturquoise"), QColor(175, 238, 238) },
{ ZColorListModel::tr("palevioletred"), QColor(219, 112, 147) },
{ ZColorListModel::tr("papayawhip"), QColor(255, 239, 213) },
{ ZColorListModel::tr("peachpuff"), QColor(255, 218, 185) },
{ ZColorListModel::tr("peru"), QColor(205, 133, 63) },
{ ZColorListModel::tr("pink"), QColor(255, 192, 203) },
{ ZColorListModel::tr("plum"), QColor(221, 160, 221) },
{ ZColorListModel::tr("powderblue"), QColor(176, 224, 230) },
{ ZColorListModel::tr("purple"), QColor(128, 0, 128) },
{ ZColorListModel::tr("red"), QColor(255, 0, 0) },
{ ZColorListModel::tr("rosybrown"), QColor(188, 143, 143) },
{ ZColorListModel::tr("royalblue"), QColor( 65, 105, 225) },
{ ZColorListModel::tr("saddlebrown"), QColor(139, 69, 19) },
{ ZColorListModel::tr("salmon"), QColor(250, 128, 114) },
{ ZColorListModel::tr("sandybrown"), QColor(244, 164, 96) },
{ ZColorListModel::tr("seagreen"), QColor( 46, 139, 87) },
{ ZColorListModel::tr("seashell"), QColor(255, 245, 238) },
{ ZColorListModel::tr("sienna"), QColor(160, 82, 45) },
{ ZColorListModel::tr("silver"), QColor(192, 192, 192) },
{ ZColorListModel::tr("skyblue"), QColor(135, 206, 235) },
{ ZColorListModel::tr("slateblue"), QColor(106, 90, 205) },
{ ZColorListModel::tr("slategray"), QColor(112, 128, 144) },
{ ZColorListModel::tr("slategrey"), QColor(112, 128, 144) },
{ ZColorListModel::tr("snow"), QColor(255, 250, 250) },
{ ZColorListModel::tr("springgreen"), QColor( 0, 255, 127) },
{ ZColorListModel::tr("steelblue"), QColor( 70, 130, 180) },
{ ZColorListModel::tr("tan"), QColor(210, 180, 140) },
{ ZColorListModel::tr("teal"), QColor( 0, 128, 128) },
{ ZColorListModel::tr("thistle"), QColor(216, 191, 216) },
{ ZColorListModel::tr("tomato"), QColor(255, 99, 71) },
{ ZColorListModel::tr("turquoise"), QColor( 64, 224, 208) },
{ ZColorListModel::tr("violet"), QColor(238, 130, 238) },
{ ZColorListModel::tr("wheat"), QColor(245, 222, 179) },
{ ZColorListModel::tr("white"), QColor(255, 255, 255) },
{ ZColorListModel::tr("whitesmoke"), QColor(245, 245, 245) },
{ ZColorListModel::tr("yellow"), QColor(255, 255, 0) },
{ ZColorListModel::tr("yellowgreen"), QColor(154, 205, 50) } };


ZColorListModel::ZColorListModel(QObject *parent) : base(parent), listcustom(true)
{
    owner = (ZColorComboBox*)parent;
}

ZColorListModel::~ZColorListModel()
{

}

QColor ZColorListModel::defaultColor() const
{
    return defcol;
}

void ZColorListModel::setDefaultColor(QColor color)
{
    if (defcol == color)
        return;

    bool changed = (defcol.isValid() != color.isValid());
    if (changed && !defcol.isValid())
        beginInsertRows(QModelIndex(), 0, 0);
    else if (changed && defcol.isValid())
        beginRemoveRows(QModelIndex(), 0, 0);
    defcol = color;
    if (changed && defcol.isValid())
        endInsertRows();
    else if (changed && !defcol.isValid())
        endRemoveRows();

    if (!changed && defcol.isValid())
        emit dataChanged(index(0, 0), index(0, 0));
}

bool ZColorListModel::listCustom() const
{
    return listcustom;
}

void ZColorListModel::setListCustom(bool allow)
{
    if (listcustom == allow)
        return;

    if (allow)
        beginInsertRows(QModelIndex(), defcol.isValid() ? 1 : 0, defcol.isValid() ? 1 : 0);
    else
        beginRemoveRows(QModelIndex(), defcol.isValid() ? 1 : 0, defcol.isValid() ? 1 : 0);
    listcustom = allow;
    if (!listcustom)
        custom = QColor();
    if (allow)
        endInsertRows();
    else
        endRemoveRows();
}

QColor ZColorListModel::customColor()
{
    if (listcustom)
        return custom;
    return QColor();
}

void ZColorListModel::setCustomColor(QColor color)
{
    if (!listcustom || (custom.isValid() == color.isValid() && custom.rgb() == color.rgb()))
        return;

    custom = color;
    emit dataChanged(index(defcol.isValid() ? 1 : 0, 0), index(defcol.isValid() ? 1 : 0, 0));
}

int ZColorListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return basecolors.size() + (defcol.isValid() ? 1 : 0) + (listcustom ? 1 : 0);
}

static const int ColorRole = Qt::UserRole;
static const int ColorTextRole = Qt::UserRole + 1;

QVariant ZColorListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (role != ColorRole && role != ColorTextRole && role != Qt::SizeHintRole))
        return QVariant();

    
    int row = index.row();
    if (role == ColorTextRole)
    {
        if (row == 0 && defcol.isValid())
            return tr("Default");
        if (listcustom && row == (defcol.isValid() ? 1 : 0))
            return tr("Custom...");

        return basecolors[row - (defcol.isValid() ? 1 : 0) - (listcustom ? 1 : 0)].first;
    }

    if (role == Qt::SizeHintRole)
        return owner->indexSizeHint(index);

    // ColorRole.
    if (row == 0 && defcol.isValid())
        return defcol;
    if (listcustom && row == (defcol.isValid() ? 1 : 0))
        return custom.isValid() ? custom : QVariant();
    return basecolors[row - (defcol.isValid() ? 1 : 0) - (listcustom ? 1 : 0)].second;
}


//-------------------------------------------------------------


ZColorListDelegate::ZColorListDelegate(ZColorComboBox *owner, QObject *parent) : base(parent), owner(owner)
{

}

ZColorListDelegate::~ZColorListDelegate()
{

}

void ZColorListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &op, const QModelIndex &index) const
{
    base::paint(painter, op, index);

    QStyleOptionComboBox opt;
    owner->initStyleOption(&opt);

    QColor cl;
    if (index.data(ColorRole).isValid())
        cl = index.data(ColorRole).value<QColor>();
    QString str = index.data(ColorTextRole).toString();

    painter->save();
    painter->setClipRect(op.rect);

    QStyle *s = owner->style();
    QRect r = op.rect;
    int h = std::ceil(QFontMetricsF(opt.fontMetrics).height()) - 2;

    QRect clrect = s->alignedRect(op.direction, Qt::AlignLeft | Qt::AlignVCenter, QSize(h, h), r);

    clrect.adjust(2, 0, 1, -1);
    clrect.adjust(0, -1, 1, 0);

    if (cl.isValid())
    {
        painter->setBrush(cl);
        painter->fillRect(clrect.adjusted(1, 1, 0, 0), cl);
    }
    else
    {
        painter->setPen(QPen(Qt::red, 2));
        painter->drawLine(clrect.left() + 1, clrect.bottom() + 1, clrect.right(), clrect.top() + 2);
    }

    painter->setPen(Qt::black);
    painter->drawRect(clrect);

    if (op.direction == Qt::RightToLeft)
        r.adjust(0, 0, -h - 6, 0);
    else
        r.adjust(h + 6, 0, 0, 0);

    if (!str.isEmpty())
    {
        if (op.state & QStyle::State_Selected)
            painter->setPen(op.palette.color(QPalette::HighlightedText));

        //QFont f(owner->font());
        //f.setFamily(owner->font().family());
        //f.setPointSize(owner->font().pointSize());

        //painter->setFont(f);
        s->drawItemText(painter, r.adjusted(1, 0, -1, 0), s->visualAlignment(op.direction, Qt::AlignLeft | Qt::AlignVCenter), op.palette, op.state & QStyle::State_Enabled, str);
    }

    painter->restore();
}

//QSize ZColorListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
//{
//    bool hascolor = index.data(ColorRole).isValid();
//    QString str = index.data(ColorTextRole).toString();
//
//    int h = option.fontMetrics.height();
//    return QSize(option.fontMetrics.boundingRect(str).width() + (hascolor ? h + 6 : 0),  h);
//}


//-------------------------------------------------------------

#define C11SIGNAL(classnam, ret, arg, name) ( ret ( classnam ::*)( arg ))(& classnam :: name)
#define C11SLOT(classnam, ret, arg, name)   ( ret ( classnam ::*)( arg ))(& classnam :: name)

ZColorComboBox::ZColorComboBox(QWidget *parent) : base(parent), firstshown(true), oldindex(0)
{
    ZColorListModel *m = new ZColorListModel(this);
    setModel(m);

    connect(m, &ZColorListModel::dataChanged, this, &ZColorComboBox::dataChanged);
    connect(m, &ZColorListModel::rowsInserted, this, &ZColorComboBox::dataChanged);
    connect(m, &ZColorListModel::rowsRemoved, this, &ZColorComboBox::dataChanged);

    itemDelegate()->deleteLater();
    setItemDelegate(new ZColorListDelegate(this));

    QFont f = font();
    f.setFamily(font().family());
    f.setPointSize(font().pointSize());
    view()->setFont(f);

    connect(this, C11SIGNAL(QComboBox, void, int, activated), this, C11SLOT(ZColorComboBox, void, int, baseActivated));
}

ZColorComboBox::~ZColorComboBox()
{

}

QColor ZColorComboBox::defaultColor() const
{
    return ((const ZColorListModel*)model())->defaultColor();
}

void ZColorComboBox::setDefaultColor(QColor color)
{
    ((ZColorListModel*)model())->setDefaultColor(color);
}

bool ZColorComboBox::listCustom() const
{
    return ((const ZColorListModel*)model())->listCustom();
}

void ZColorComboBox::setListCustom(bool allow)
{
    ((ZColorListModel*)model())->setListCustom(allow);
}

void ZColorComboBox::setCurrentColor(QColor color)
{
    bool hasdef = defaultColor().isValid();
    bool listcustom = listCustom();

    if (!color.isValid())
    {
        if (hasdef)
            setCurrentIndex(0);
        return;
    }

    for (int ix = 0, siz = basecolors.size(); ix != siz; ++ix)
    {
        if (color.rgb() == basecolors[ix].second.rgb())
        {
            setCurrentIndex(ix + (hasdef ? 1 : 0) + (listcustom ? 1 : 0));
            return;
        }
    }

    if (!listCustom())
        return;

    ((ZColorListModel*)model())->setCustomColor(color);
    setCurrentIndex(hasdef ? 1 : 0);
}

QColor ZColorComboBox::currentColor() const
{
    int row = currentIndex();
    if (row == -1 || (row == 0 && defaultColor().isValid()))
        return QColor();
    if (defaultColor().isValid())
        --row;
    if (row == 0 && listCustom())
        return ((ZColorListModel*)model())->customColor();
    if (listCustom())
        --row;

    return basecolors[row].second;
}

QSize ZColorComboBox::sizeHint() const
{
    if (!siz.isValid())
        computeSizeHint();
    return siz;
}

QSize ZColorComboBox::minimumSizeHint() const
{
    return sizeHint();
}

void ZColorComboBox::paintEvent(QPaintEvent *e)
{
    base::paintEvent(e);

    int cix = currentIndex();
    if (cix == -1)
        return;

    ZColorListModel *m = ((ZColorListModel*)model());

    QColor cl;
    if (m->data(m->index(cix, 0), ColorRole).isValid())
        cl = m->data(m->index(cix, 0), ColorRole).value<QColor>();

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QStylePainter painter(this);
    QStyle *s = style();

    QRect r = s->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    painter.setClipRect(r);

    int h = std::ceil(QFontMetricsF(opt.fontMetrics).height()) - 2;

    QRect clrect = s->alignedRect(opt.direction, Qt::AlignLeft | Qt::AlignVCenter, QSize(h, h), r);

    clrect.adjust(0, 0, -1, -1);
    clrect.adjust(0, -1, 1, 0);

    if (cl.isValid())
    {
        painter.setBrush(cl);
        painter.fillRect(clrect.adjusted(1, 1, 0, 0), cl);
    }
    else
    {
        painter.setPen(QPen(Qt::red, 2));
        painter.drawLine(clrect.left() + 1, clrect.bottom() + 1, clrect.right(), clrect.top() + 2);
    }

    painter.setPen(Qt::black);
    painter.drawRect(clrect);

    if (opt.direction == Qt::RightToLeft)
        r.adjust(0, 0, -h - 4, 0);
    else
        r.adjust(h + 4, 0, 0, 0);

    QString str = m->data(m->index(cix, 0), ColorTextRole).toString();

    if (!str.isEmpty())
    {
        //QFont f = painter.font();
        //f.setHintingPreference(QFont::PreferNoHinting);
        //f.setPixelSize(11);

        painter.setFont(font());
        s->drawItemText(&painter, r.adjusted(1, 0, -1, 0), s->visualAlignment(opt.direction, Qt::AlignLeft | Qt::AlignVCenter), opt.palette, opt.state & QStyle::State_Enabled, str);
    }
}

void ZColorComboBox::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::StyleChange:
#ifdef Q_OS_MAC
    case QEvent::MacSizeChange:
#endif
        siz = QSize();
        break;
    }

    base::changeEvent(e);
}

void ZColorComboBox::showEvent(QShowEvent *e)
{
    if (firstshown && sizeAdjustPolicy() == QComboBox::AdjustToContentsOnFirstShow)
    {
        siz = QSize();
        updateGeometry();
    }
    firstshown = false;

    base::showEvent(e);
}

void ZColorComboBox::dataChanged()
{
    if (sizeAdjustPolicy() == QComboBox::AdjustToContents)
    {
        siz = QSize();
        updateGeometry();
    }
}

void ZColorComboBox::baseActivated(int index)
{
    ZColorListModel *m = ((ZColorListModel*)model());

    bool colorchanged = index != oldindex;

    if (listCustom() && index == (defaultColor().isValid() ? 1 : 0))
    {
        QColor orig = m->customColor();
        QColor ret = QColorDialog::getColor(orig, parentWidget()->window(), tr("Select a color"), QColorDialog::DontUseNativeDialog);
        if (!ret.isValid())
        {
            if (!m->customColor().isValid() && oldindex != index)
            {
                setCurrentIndex(oldindex);
                return;
            }
        }
        else
        {
            m->setCustomColor(ret);

            if (ret != orig && index == oldindex)
                colorchanged = true;
        }
    }

    QString str = m->data(m->index(index, 0), ColorTextRole).toString();
    if (index != oldindex)
    {
        emit currentIndexChanged(index);
        emit currentIndexChanged(str);
    }
    oldindex = index;
    emit activated(index);
    emit activated(str);

    if (colorchanged)
    {
        if (defaultColor().isValid() && index == 0)
            emit currentColorChanged(QColor());
        else
            emit currentColorChanged(m->data(m->index(index, 0), ColorRole).value<QColor>());
    }
}

QSize ZColorComboBox::indexSizeHint(const QModelIndex &index) const
{
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    
    //bool hascolor = index.data(ColorRole).isValid();
    QString str = index.data(ColorTextRole).toString();

    int h = std::ceil(QFontMetricsF(opt.fontMetrics).height());
    return QSize(opt.fontMetrics.boundingRect(str).width() + /*(hascolor ? */h + 6/* : 0)*/, std::max(h, 14) + 2);

}

void ZColorComboBox::computeSizeHint() const
{
    ZColorListModel *m = ((ZColorListModel*)model());

    int cnt = m->rowCount();

    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QStyle *s = style();

    // Taken from Qt source for combo box size hint calculation.
    int h = std::ceil(QFontMetricsF(opt.fontMetrics).height());
    siz.setHeight(std::max(h, 14) + 2);

    h -= 2;

    if (cnt == 0)
        siz.setWidth(opt.fontMetrics.width(QLatin1Char('x')) * 7);
    else
    {
        for (int ix = 0; ix != cnt; ++ix)
        {
            //bool hascolor = m->data(m->index(ix, 0), ColorRole).isValid();
            QString str = m->data(m->index(ix, 0), ColorTextRole).toString();

            siz.setWidth(std::max<int>(siz.width(), opt.fontMetrics.boundingRect(str).width() + /*(hascolor ?*/ h + 6 /*: 0)*/));
        }
    }

    if (opt.rect.isValid())
        opt.rect.setWidth(siz.width());
    siz = s->sizeFromContents(QStyle::CT_ComboBox, &opt, siz, this);
    siz = siz.expandedTo(qApp->globalStrut());
}


//-------------------------------------------------------------

